#include "nfs.h"
#include <string.h>
#include <stdlib.h>

void xdr_init(xdr_t* x, uint8_t* buf, uint32_t cap) {
    x->buf = buf;
    x->len = 0;
    x->pos = 0;
    x->cap = cap;
    x->err = 0;
}

int xdr_ok(const xdr_t* x) {
    return x->err == 0;
}

static int xdr_need(xdr_t* x, uint32_t n) {
    if(x->err || x->pos + n > x->len) {
        x->err = 1;
        return 0;
    }
    return 1;
}

static int xdr_room(xdr_t* x, uint32_t n) {
    if(x->err || x->len + n > x->cap) {
        x->err = 1;
        return 0;
    }
    return 1;
}

void xdr_put_u32(xdr_t* x, uint32_t v) {
    if(!xdr_room(x, 4)) return;
    x->buf[x->len++] = (uint8_t)(v >> 24);
    x->buf[x->len++] = (uint8_t)(v >> 16);
    x->buf[x->len++] = (uint8_t)(v >> 8);
    x->buf[x->len++] = (uint8_t)v;
}

void xdr_put_u64(xdr_t* x, uint64_t v) {
    xdr_put_u32(x, (uint32_t)(v >> 32));
    xdr_put_u32(x, (uint32_t)v);
}

static void xdr_put_pad(xdr_t* x, uint32_t n) {
    while(n & 3) {
        if(!xdr_room(x, 1)) return;
        x->buf[x->len++] = 0;
        n++;
    }
}

void xdr_put_opaque(xdr_t* x, const void* data, uint32_t len) {
    xdr_put_u32(x, len);
    if(!xdr_room(x, len)) return;
    if(len) memcpy(x->buf + x->len, data, len);
    x->len += len;
    xdr_put_pad(x, len);
}

void xdr_put_string(xdr_t* x, const char* s) {
    uint32_t len = s ? (uint32_t)strlen(s) : 0;
    xdr_put_opaque(x, s, len);
}

uint32_t xdr_get_u32(xdr_t* x) {
    if(!xdr_need(x, 4)) return 0;
    uint32_t v = ((uint32_t)x->buf[x->pos] << 24) |
                 ((uint32_t)x->buf[x->pos + 1] << 16) |
                 ((uint32_t)x->buf[x->pos + 2] << 8) |
                 (uint32_t)x->buf[x->pos + 3];
    x->pos += 4;
    return v;
}

uint64_t xdr_get_u64(xdr_t* x) {
    uint64_t hi = xdr_get_u32(x);
    uint64_t lo = xdr_get_u32(x);
    return (hi << 32) | lo;
}

uint32_t xdr_get_opaque(xdr_t* x, void* out, uint32_t max) {
    uint32_t len = xdr_get_u32(x);
    if(!xdr_need(x, len)) return 0;
    uint32_t copy = len < max ? len : max;
    if(out && copy) memcpy(out, x->buf + x->pos, copy);
    x->pos += len;
    uint32_t pad = (4 - (len & 3)) & 3;
    if(!xdr_need(x, pad)) return 0;
    x->pos += pad;
    return len;
}

char* xdr_get_string(xdr_t* x) {
    uint32_t len = xdr_get_u32(x);
    if(!xdr_need(x, len)) return NULL;
    char* s = (char*)malloc(len + 1);
    if(!s) { x->err = 1; return NULL; }
    if(len) memcpy(s, x->buf + x->pos, len);
    s[len] = 0;
    x->pos += len;
    uint32_t pad = (4 - (len & 3)) & 3;
    if(!xdr_need(x, pad)) { free(s); return NULL; }
    x->pos += pad;
    return s;
}

void xdr_skip(xdr_t* x, uint32_t n) {
    if(!xdr_need(x, n)) return;
    x->pos += n;
}

void xdr_get_nfstime3(xdr_t* x, nfstime3* t) {
    t->sec = xdr_get_u32(x);
    t->usec = xdr_get_u32(x);
}

void xdr_put_fh3(xdr_t* x, const nfs_fh3* fh) {
    xdr_put_opaque(x, fh->data, fh->len);
}

int xdr_get_fh3(xdr_t* x, nfs_fh3* fh) {
    fh->len = xdr_get_opaque(x, fh->data, NFS_FH3_MAX);
    if(!xdr_ok(x) || fh->len > NFS_FH3_MAX) {
        x->err = 1;
        return -1;
    }
    return 0;
}

void xdr_get_fattr3(xdr_t* x, fattr3* a) {
    memset(a, 0, sizeof(*a));
    a->type = xdr_get_u32(x);
    a->mode = xdr_get_u32(x);
    a->nlink = xdr_get_u32(x);
    a->uid = xdr_get_u32(x);
    a->gid = xdr_get_u32(x);
    a->size = xdr_get_u64(x);
    a->used = xdr_get_u64(x);
    a->rdev1 = xdr_get_u32(x);
    a->rdev2 = xdr_get_u32(x);
    a->fsid = xdr_get_u64(x);
    a->fileid = xdr_get_u64(x);
    xdr_get_nfstime3(x, &a->atime);
    xdr_get_nfstime3(x, &a->mtime);
    xdr_get_nfstime3(x, &a->ctime);
}

int xdr_get_post_attr(xdr_t* x, fattr3* a) {
    uint32_t follows = xdr_get_u32(x);
    if(follows) {
        xdr_get_fattr3(x, a);
        return xdr_ok(x) ? 1 : -1;
    }
    memset(a, 0, sizeof(*a));
    return 0;
}