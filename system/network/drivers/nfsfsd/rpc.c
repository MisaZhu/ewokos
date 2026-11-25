#include "nfs.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef SO_RCVBUF
#define SO_RCVBUF 0x1002
#endif
#ifndef SO_SNDBUF
#define SO_SNDBUF 0x1001
#endif
#ifndef SOCK_STREAM
#define SOCK_STREAM 1
#endif
#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

static void fill_addr(struct sockaddr_in* a, uint32_t ip_net, uint16_t port_host) {
    memset(a, 0, sizeof(*a));
    a->sin_len = sizeof(*a);
    a->sin_family = AF_INET;
    a->sin_port = htons(port_host);
    a->sin_addr.s_un.s_addr = ip_net;
}

static void set_bufsize(int sock) {
    int bufsize = 4 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
}

int rpc_init(rpc_client_t* c, uint32_t server_ip, int timeout_ms, int retries) {
    memset(c, 0, sizeof(*c));
    c->server_ip = server_ip;
    c->timeout_ms = timeout_ms > 0 ? timeout_ms : 3000;
    c->retries = retries > 0 ? retries : 4;
    c->xid = (uint32_t)getpid() << 16;
    c->tcp_sock = -1;
    c->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(c->sock < 0)
        return -1;
    struct timeval tv;
    tv.tv_sec = c->timeout_ms / 1000;
    tv.tv_usec = (c->timeout_ms % 1000) * 1000;
    setsockopt(c->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    set_bufsize(c->sock);
    return 0;
}

void rpc_close(rpc_client_t* c) {
    if(c->sock >= 0) { close(c->sock); c->sock = -1; }
    if(c->tcp_sock >= 0) { close(c->tcp_sock); c->tcp_sock = -1; }
}

int rpc_tcp_connect(rpc_client_t* c, uint16_t port) {
    if(c->tcp_sock >= 0)
        return 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s < 0)
        return -1;
    struct timeval tv;
    tv.tv_sec = c->timeout_ms / 1000;
    tv.tv_usec = (c->timeout_ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    set_bufsize(s);
    struct sockaddr_in dst;
    fill_addr(&dst, c->server_ip, port);
    if(connect(s, (struct sockaddr*)&dst, sizeof(dst)) != 0) {
        close(s);
        return -1;
    }
    c->tcp_sock = s;
    c->use_tcp = 1;
    return 0;
}

/* 构建 RPC CALL 头 + 参数 */
static void build_call(xdr_t* x, uint32_t xid, uint32_t prog, uint32_t vers,
                       uint32_t proc, uint32_t uid, uint32_t gid,
                       const uint8_t* args, uint32_t args_len) {
    xdr_put_u32(x, xid);
    xdr_put_u32(x, RPC_CALL);
    xdr_put_u32(x, 2);
    xdr_put_u32(x, prog);
    xdr_put_u32(x, vers);
    xdr_put_u32(x, proc);
    uint8_t cbuf[128];
    xdr_t cred;
    xdr_init(&cred, cbuf, sizeof(cbuf));
    xdr_put_u32(&cred, 0);
    xdr_put_string(&cred, "ewokos");
    xdr_put_u32(&cred, uid);
    xdr_put_u32(&cred, gid);
    xdr_put_u32(&cred, 0);
    xdr_put_u32(x, AUTH_UNIX);
    xdr_put_opaque(x, cbuf, cred.len);
    xdr_put_u32(x, AUTH_NULL);
    xdr_put_u32(x, 0);
    if(args_len && args) {
        memcpy(x->buf + x->len, args, args_len);
        x->len += args_len;
    }
}

/* 解析 RPC 回复头,拷贝 body 到 reply;返回 body 长度,<0 失败/不匹配 */
static int parse_reply(const uint8_t* rbuf, uint32_t n, uint32_t xid,
                       uint8_t* reply, uint32_t reply_cap) {
    xdr_t r;
    xdr_init(&r, (uint8_t*)rbuf, n);
    r.len = n;
    uint32_t rxid = xdr_get_u32(&r);
    uint32_t mtype = xdr_get_u32(&r);
    if(rxid != xid || mtype != RPC_REPLY)
        return -1;
    uint32_t reply_stat = xdr_get_u32(&r);
    if(reply_stat != RPC_MSG_ACCEPTED)
        return -2;
    xdr_get_u32(&r);                    /* verf flavor */
    uint32_t verf_len = xdr_get_u32(&r);
    xdr_skip(&r, verf_len + ((4 - (verf_len & 3)) & 3));
    uint32_t accept_stat = xdr_get_u32(&r);
    if(!xdr_ok(&r))
        return -1;
    if(accept_stat != RPC_SUCCESS)
        return -2;
    uint32_t body_len = r.len - r.pos;
    if(body_len > reply_cap)
        body_len = reply_cap;
    memcpy(reply, rbuf + r.pos, body_len);
    return (int)body_len;
}

static int tcp_send_all(int fd, const void* buf, uint32_t n) {
    const uint8_t* p = (const uint8_t*)buf;
    uint32_t sent = 0;
    while(sent < n) {
        int32_t k = send(fd, p + sent, n - sent, 0);
        if(k <= 0)
            return -1;
        sent += (uint32_t)k;
    }
    return 0;
}

static int tcp_recv_all(int fd, void* buf, uint32_t n) {
    uint8_t* p = (uint8_t*)buf;
    uint32_t got = 0;
    while(got < n) {
        int32_t k = recv(fd, p + got, n - got, 0);
        if(k <= 0)
            return -1;
        got += (uint32_t)k;
    }
    return 0;
}

/* TCP 传输:RPC record marking(RFC 1057 §10),流式无 datagram 尺寸限制 */
static int rpc_call_tcp(rpc_client_t* c, const uint8_t* pkt, uint32_t pkt_len,
                        uint32_t xid, uint8_t* reply, uint32_t reply_cap) {
    uint8_t mark[4];
    uint32_t rec = pkt_len | 0x80000000u;   /* last fragment */
    mark[0] = (uint8_t)(rec >> 24); mark[1] = (uint8_t)(rec >> 16);
    mark[2] = (uint8_t)(rec >> 8);  mark[3] = (uint8_t)rec;
    if(tcp_send_all(c->tcp_sock, mark, 4) != 0)
        return -1;
    if(tcp_send_all(c->tcp_sock, pkt, pkt_len) != 0)
        return -1;
    /* 接收回复(可能多 fragment) */
    uint32_t rbuf_cap = reply_cap + 1024;
    uint8_t* rbuf = (uint8_t*)malloc(rbuf_cap);
    if(!rbuf)
        return -1;
    uint32_t total = 0;
    int result = -1;
    for(;;) {
        if(tcp_recv_all(c->tcp_sock, mark, 4) != 0)
            break;
        uint32_t hdr = ((uint32_t)mark[0] << 24) | ((uint32_t)mark[1] << 16) |
                       ((uint32_t)mark[2] << 8) | mark[3];
        uint32_t frag_len = hdr & 0x7FFFFFFFu;
        int last = (hdr & 0x80000000u) ? 1 : 0;
        if(total + frag_len > rbuf_cap)
            break;
        if(tcp_recv_all(c->tcp_sock, rbuf + total, frag_len) != 0)
            break;
        total += frag_len;
        if(last) {
            result = parse_reply(rbuf, total, xid, reply, reply_cap);
            break;
        }
    }
    free(rbuf);
    return result;
}

int rpc_call(rpc_client_t* c, uint16_t port, uint32_t prog, uint32_t vers,
             uint32_t proc, const uint8_t* args, uint32_t args_len,
             uint8_t* reply, uint32_t reply_cap) {
    uint32_t pkt_cap = args_len + 256;
    uint8_t* pkt = (uint8_t*)malloc(pkt_cap);
    if(!pkt)
        return -1;
    xdr_t x;
    xdr_init(&x, pkt, pkt_cap);
    uint32_t xid = ++c->xid;
    build_call(&x, xid, prog, vers, proc, c->uid, c->gid, args, args_len);
    if(!xdr_ok(&x)) {
        free(pkt);
        return -1;
    }

    int result;
    if(c->use_tcp && c->tcp_sock >= 0) {
        result = rpc_call_tcp(c, pkt, x.len, xid, reply, reply_cap);
        if(result == -1) {              /* 连接断 → 重连一次 */
            close(c->tcp_sock);
            c->tcp_sock = -1;
            if(rpc_tcp_connect(c, port) == 0)
                result = rpc_call_tcp(c, pkt, x.len, xid, reply, reply_cap);
        }
        free(pkt);
        return result;
    }

    uint32_t rbuf_cap = reply_cap + 1024;
    uint8_t* rbuf = (uint8_t*)malloc(rbuf_cap);
    if(!rbuf) {
        free(pkt);
        return -1;
    }
    struct sockaddr_in dst;
    fill_addr(&dst, c->server_ip, port);
    result = -1;
    for(int attempt = 0; attempt < c->retries; attempt++) {
        int32_t sent = sendto(c->sock, pkt, x.len, 0,
                              (struct sockaddr*)&dst, sizeof(dst));
        if(sent < 0)
            continue;
        struct sockaddr_in from;
        uint32_t from_len = sizeof(from);
        int32_t n = recvfrom(c->sock, rbuf, rbuf_cap, 0,
                             (struct sockaddr*)&from, &from_len);
        if(n < 8)
            continue;
        int pr = parse_reply(rbuf, (uint32_t)n, xid, reply, reply_cap);
        if(pr == -1)
            continue;                   /* 不匹配 → 重传 */
        result = pr;
        break;
    }
    free(pkt);
    free(rbuf);
    return result;
}

uint16_t rpc_getport(rpc_client_t* c, uint32_t prog, uint32_t vers, uint32_t proto) {
    uint8_t args[16];
    xdr_t a;
    xdr_init(&a, args, sizeof(args));
    xdr_put_u32(&a, prog);
    xdr_put_u32(&a, vers);
    xdr_put_u32(&a, proto);
    xdr_put_u32(&a, 0);
    uint8_t reply[8];
    int save_tcp = c->use_tcp;          /* portmap 走 UDP */
    c->use_tcp = 0;
    int n = rpc_call(c, 111, PMAP_PROGRAM, PMAP_V2, PMAP_GETPORT,
                     args, a.len, reply, sizeof(reply));
    c->use_tcp = save_tcp;
    if(n < 4)
        return 0;
    xdr_t r;
    xdr_init(&r, reply, (uint32_t)n);
    r.len = (uint32_t)n;
    uint32_t port = xdr_get_u32(&r);
    return (uint16_t)port;
}