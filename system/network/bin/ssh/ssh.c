#include "ssh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/ecdsa.h>
#include <openssl/ed25519.h>

static char ssh_error_buffer[256];

/* Helper: Choose algorithm */
static int ssh_choose_algorithm(const char *client_list, const char *server_list,
                                 char *out, size_t out_len) {
    char client[512];
    char server[512];
    char *token, *saveptr;
    
    strncpy(client, client_list, sizeof(client)-1);
    strncpy(server, server_list, sizeof(server)-1);
    
    token = strtok_r(client, ",", &saveptr);
    while (token) {
        const char *sptr = server;
        while (*sptr) {
            /* Skip leading whitespace */
            while (*sptr == ',') sptr++;
            
            /* Compare */
            const char *t1 = token;
            const char *t2 = sptr;
            while (*t1 && *t2 && *t2 != ',' && *t1 == *t2) {
                t1++; t2++;
            }
            
            /* Check if match */
            if (*t1 == '\0' && (*t2 == '\0' || *t2 == ',')) {
                strncpy(out, token, out_len-1);
                out[out_len-1] = '\0';
                return 1;
            }
            
            /* Find next algorithm in server list */
            while (*sptr && *sptr != ',') sptr++;
            if (*sptr) sptr++;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    return 0;
}

/* Helper: Parse KEXINIT packet */
static int ssh_parse_kexinit(const uint8_t *payload, size_t payload_len,
                              char *kex, char *hostkey,
                              char *enc_c2s, char *enc_s2c,
                              char *mac_c2s, char *mac_s2c,
                              char *comp_c2s, char *comp_s2c) {
    const uint8_t *p = payload;
    uint32_t len;
    
    /* Skip cookie (16 bytes) */
    p += 16;
    
    /* KEX algorithms */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(kex, (char*)p, len);
    kex[len] = '\0';
    p += len;
    
    /* Host key algorithms */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(hostkey, (char*)p, len);
    hostkey[len] = '\0';
    p += len;
    
    /* Encryption c->s */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(enc_c2s, (char*)p, len);
    enc_c2s[len] = '\0';
    p += len;
    
    /* Encryption s->c */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(enc_s2c, (char*)p, len);
    enc_s2c[len] = '\0';
    p += len;
    
    /* MAC c->s */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(mac_c2s, (char*)p, len);
    mac_c2s[len] = '\0';
    p += len;
    
    /* MAC s->c */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(mac_s2c, (char*)p, len);
    mac_s2c[len] = '\0';
    p += len;
    
    /* Compression c->s */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(comp_c2s, (char*)p, len);
    comp_c2s[len] = '\0';
    p += len;
    
    /* Compression s->c */
    len = ssh_read_uint32(p);
    p += 4;
    strncpy(comp_s2c, (char*)p, len);
    comp_s2c[len] = '\0';
    
    return 0;
}

/* Helper: Check if using Curve25519 */
static int ssh_is_curve25519(const char *kex) {
    return (strcmp(kex, "curve25519-sha256@libssh.org") == 0 ||
            strcmp(kex, "curve25519-sha256") == 0 ||
            strncmp(kex, "curve25519-", 11) == 0);
}

/* DH Group 14 parameters (2048-bit) */
static const char dh_group14_p_hex[] = 
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"
    "8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"
    "302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"
    "A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE6"
    "49286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8"
    "FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D"
    "670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C"
    "180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"
    "3995497CEA956AE515D2261898FA051015728E5A8AACAA68FFFFFFFFFFFFFFFF";

/* Session Management */
ssh_session_t *ssh_session_new(void) {
    ssh_session_t *session = (ssh_session_t *)calloc(1, sizeof(ssh_session_t));
    if (session) {
        session->socket = -1;
        session->state = SSH_STATE_INIT;
        session->channel_local = 0;
        session->window_local = 32768;
        session->terminal_width = 80;
        session->terminal_height = 24;
        strncpy(session->client_version, SSH_VERSION_STRING, sizeof(session->client_version) - 1);
    }
    return session;
}

void ssh_session_free(ssh_session_t *session) {
    if (session) {
        if (session->socket >= 0) {
            close(session->socket);
        }
        free(session);
    }
}

int ssh_connect(ssh_session_t *session, const char *hostname, int port) {
    struct hostent *host;
    struct sockaddr_in addr;
    int sock;

    if (!session || !hostname) {
        ssh_set_error(session, "Invalid parameters");
        return -1;
    }

    if (port <= 0) port = SSH_DEFAULT_PORT;

    host = gethostbyname(hostname);
    if (!host) {
        ssh_set_error(session, "Failed to resolve hostname: %s", hostname);
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ssh_set_error(session, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ssh_set_error(session, "Failed to connect: %s", strerror(errno));
        close(sock);
        return -1;
    }

    session->socket = sock;
    session->state = SSH_STATE_INIT;

    return 0;
}

int ssh_disconnect(ssh_session_t *session) {
    if (!session) return -1;

    if (session->socket >= 0) {
        if (session->state >= SSH_STATE_KEXINIT_SENT) {
            ssh_packet_t packet;
            memset(&packet, 0, sizeof(packet));
            packet.type = SSH_MSG_DISCONNECT;
            
            uint8_t *p = packet.payload;
            ssh_write_uint32(p, SSH_DISCONNECT_BY_APPLICATION);
            p += 4;
            
            const char *reason = "User disconnected";
            ssh_write_uint32(p, strlen(reason));
            p += 4;
            memcpy(p, reason, strlen(reason));
            p += strlen(reason);
            
            ssh_write_string(p, (const uint8_t *)"", 0);
            p += 4;
            
            packet.payload_len = p - packet.payload;
            ssh_packet_send(session, &packet);
        }
        
        close(session->socket);
        session->socket = -1;
    }

    session->state = SSH_STATE_DISCONNECTED;
    return 0;
}

int ssh_is_connected(ssh_session_t *session) {
    if (!session) return 0;
    return (session->socket >= 0 && session->state != SSH_STATE_DISCONNECTED);
}

/* Protocol Handshake */
int ssh_send_banner(ssh_session_t *session) {
    char banner[256];
    int len;

    if (!session || session->socket < 0) return -1;

    len = snprintf(banner, sizeof(banner), "%s\r\n", session->client_version);

    if (write(session->socket, banner, len) != len) {
        ssh_set_error(session, "Failed to send banner: %s", strerror(errno));
        return -1;
    }

    session->state = SSH_STATE_BANNER_SENT;
    return 0;
}

int ssh_receive_banner(ssh_session_t *session) {
    char banner[256];
    int i = 0;
    char c;
    ssize_t n;

    if (!session || session->socket < 0) return -1;

    while (i < sizeof(banner) - 1) {
        n = read(session->socket, &c, 1);
        if (n < 0) {
#ifdef EINTR
            if (errno == EINTR) continue;
#endif
            ssh_set_error(session, "Failed to read banner: %s", strerror(errno));
            return -1;
        }
        if (n == 0) {
            ssh_set_error(session, "Connection closed while reading banner");
            return -1;
        }

        if (c == '\r') continue;
        if (c == '\n') break;

        banner[i++] = c;
    }
    banner[i] = '\0';

    if (strncmp(banner, "SSH-", 4) != 0) {
        ssh_set_error(session, "Invalid SSH banner: %s", banner);
        return -1;
    }

    strncpy(session->server_version, banner, sizeof(session->server_version) - 1);
    session->server_version[sizeof(session->server_version) - 1] = '\0';
    session->state = SSH_STATE_BANNER_RECEIVED;

    return 0;
}

int ssh_send_kexinit(ssh_session_t *session) {
    ssh_packet_t packet;
    uint8_t *p;
    uint8_t cookie[16];

    if (!session || session->socket < 0) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_KEXINIT;
    p = packet.payload;

    if (RAND_bytes(cookie, 16) != 1) {
        ssh_set_error(session, "Failed to generate cookie");
        return -1;
    }
    memcpy(p, cookie, 16);
    p += 16;

    const char *kex_algs = "curve25519-sha256,diffie-hellman-group14-sha256";
    ssh_write_uint32(p, strlen(kex_algs));
    p += 4;
    memcpy(p, kex_algs, strlen(kex_algs));
    p += strlen(kex_algs);

    const char *hostkey_algs = "ssh-rsa,ssh-dss,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,ssh-ed25519";
    ssh_write_uint32(p, strlen(hostkey_algs));
    p += 4;
    memcpy(p, hostkey_algs, strlen(hostkey_algs));
    p += strlen(hostkey_algs);

    const char *enc_algs = "aes256-ctr,aes192-ctr,aes128-ctr";
    ssh_write_uint32(p, strlen(enc_algs));
    p += 4;
    memcpy(p, enc_algs, strlen(enc_algs));
    p += strlen(enc_algs);

    ssh_write_uint32(p, strlen(enc_algs));
    p += 4;
    memcpy(p, enc_algs, strlen(enc_algs));
    p += strlen(enc_algs);

    const char *mac_algs = "hmac-sha2-256";
    ssh_write_uint32(p, strlen(mac_algs));
    p += 4;
    memcpy(p, mac_algs, strlen(mac_algs));
    p += strlen(mac_algs);

    ssh_write_uint32(p, strlen(mac_algs));
    p += 4;
    memcpy(p, mac_algs, strlen(mac_algs));
    p += strlen(mac_algs);

    const char *comp_algs = "none";
    ssh_write_uint32(p, strlen(comp_algs));
    p += 4;
    memcpy(p, comp_algs, strlen(comp_algs));
    p += strlen(comp_algs);

    ssh_write_uint32(p, strlen(comp_algs));
    p += 4;
    memcpy(p, comp_algs, strlen(comp_algs));
    p += strlen(comp_algs);

    ssh_write_uint32(p, 0);
    p += 4;

    ssh_write_uint32(p, 0);
    p += 4;

    *p++ = 0;

    ssh_write_uint32(p, 0);
    p += 4;

    packet.payload_len = p - packet.payload;

    memcpy(session->client_kexinit, &packet.type, 1);
    memcpy(session->client_kexinit + 1, packet.payload, packet.payload_len);
    session->client_kexinit_len = packet.payload_len + 1;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    session->state = SSH_STATE_KEXINIT_SENT;
    return 0;
}

int ssh_receive_kexinit(ssh_session_t *session) {
    ssh_packet_t packet;
    
    char client_kex[512], client_hostkey[512], client_enc[512], client_mac[512], client_comp[512];
    char server_kex[512], server_hostkey[512], server_enc[512], server_mac[512], server_comp[512];

    if (!session || session->socket < 0) return -1;

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_DISCONNECT) {
        uint32_t reason = ssh_read_uint32(packet.payload);
        uint32_t desc_len = ssh_read_uint32(packet.payload + 4);
        char *desc = (char *)(packet.payload + 8);
        ssh_set_error(session, "Server disconnected during KEXINIT, reason: %u, desc: %.*s",
                      reason, desc_len > 256 ? 256 : desc_len, desc);
        return -1;
    }

    if (packet.type != SSH_MSG_KEXINIT) {
        ssh_set_error(session, "Expected KEXINIT, got %d", packet.type);
        return -1;
    }

    /* Save server KEXINIT for exchange hash */
    memcpy(session->server_kexinit, &packet.type, 1);
    memcpy(session->server_kexinit + 1, packet.payload, packet.payload_len);
    session->server_kexinit_len = packet.payload_len + 1;

    /* Parse client and server KEXINIT packets */
    /* Parse client list (from known constant) */
    strncpy(client_kex, "curve25519-sha256,diffie-hellman-group14-sha256", sizeof(client_kex)-1);
    strncpy(client_hostkey, "ssh-rsa,ssh-dss,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,ssh-ed25519", sizeof(client_hostkey)-1);
    strncpy(client_enc, "aes256-ctr,aes192-ctr,aes128-ctr", sizeof(client_enc)-1);
    strncpy(client_mac, "hmac-sha2-256", sizeof(client_mac)-1);
    strncpy(client_comp, "none", sizeof(client_comp)-1);
    
    /* Parse server KEXINIT */
    ssh_parse_kexinit(packet.payload, packet.payload_len,
                       server_kex, server_hostkey,
                       server_enc, server_enc,
                       server_mac, server_mac,
                       server_comp, server_comp);

    /* Choose algorithms */
    if (!ssh_choose_algorithm(client_kex, server_kex, session->selected_kex, sizeof(session->selected_kex))) {
        ssh_set_error(session, "No common KEX algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_hostkey, server_hostkey, session->selected_hostkey, sizeof(session->selected_hostkey))) {
        ssh_set_error(session, "No common hostkey algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_enc, server_enc, session->selected_enc_c2s, sizeof(session->selected_enc_c2s))) {
        ssh_set_error(session, "No common encryption algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_enc, server_enc, session->selected_enc_s2c, sizeof(session->selected_enc_s2c))) {
        ssh_set_error(session, "No common encryption algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_mac, server_mac, session->selected_mac_c2s, sizeof(session->selected_mac_c2s))) {
        ssh_set_error(session, "No common MAC algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_mac, server_mac, session->selected_mac_s2c, sizeof(session->selected_mac_s2c))) {
        ssh_set_error(session, "No common MAC algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_comp, server_comp, session->selected_comp_c2s, sizeof(session->selected_comp_c2s))) {
        ssh_set_error(session, "No common compression algorithm");
        return -1;
    }
    if (!ssh_choose_algorithm(client_comp, server_comp, session->selected_comp_s2c, sizeof(session->selected_comp_s2c))) {
        ssh_set_error(session, "No common compression algorithm");
        return -1;
    }
    
    session->using_curve25519 = ssh_is_curve25519(session->selected_kex);
    
    printf("ssh: Using KEX: %s (curve25519=%d)\n", session->selected_kex, session->using_curve25519);
    printf("ssh: Hostkey: %s\n", session->selected_hostkey);
    printf("ssh: Encryption: %s/%s\n", session->selected_enc_c2s, session->selected_enc_s2c);

    session->state = SSH_STATE_KEXINIT_RECEIVED;
    return 0;
}

/* Compute SHA-256 hash */
static void sha256_hash(const uint8_t *data, size_t len, uint8_t *hash) {
    SHA256(data, len, hash);
}

/* Compute HMAC-SHA256 */
static void hmac_sha256(const uint8_t *key, size_t key_len,
                        const uint8_t *data, size_t data_len,
                        uint8_t *mac) {
    unsigned int mac_len = 32;
    HMAC(EVP_sha256(), key, key_len, data, data_len, mac, &mac_len);
}

/* Derive keys using KDF from RFC 4253 */
static int derive_keys(ssh_session_t *session, const uint8_t *shared_secret, size_t secret_len,
                       const uint8_t *exchange_hash, size_t hash_len) {
    uint8_t buf[4096];
    uint8_t *p = buf;
    uint8_t key[64];
    unsigned int key_len = 0;

    /* K1 = HASH(K || H || "A" || session_id) */
    memcpy(p, shared_secret, secret_len);
    p += secret_len;
    memcpy(p, exchange_hash, hash_len);
    p += hash_len;
    *p++ = 'A';
    memcpy(p, session->session_id, session->session_id_len);
    p += session->session_id_len;

    sha256_hash(buf, p - buf, session->int_key_client);

    /* K2 = HASH(K || H || "B" || session_id) */
    p = buf;
    memcpy(p, shared_secret, secret_len);
    p += secret_len;
    memcpy(p, exchange_hash, hash_len);
    p += hash_len;
    *p++ = 'B';
    memcpy(p, session->session_id, session->session_id_len);
    p += session->session_id_len;

    sha256_hash(buf, p - buf, session->int_key_server);

    /* K3 = HASH(K || H || "C" || session_id) */
    p = buf;
    memcpy(p, shared_secret, secret_len);
    p += secret_len;
    memcpy(p, exchange_hash, hash_len);
    p += hash_len;
    *p++ = 'C';
    memcpy(p, session->session_id, session->session_id_len);
    p += session->session_id_len;

    sha256_hash(buf, p - buf, session->enc_key_client);

    /* K4 = HASH(K || H || "D" || session_id) */
    p = buf;
    memcpy(p, shared_secret, secret_len);
    p += secret_len;
    memcpy(p, exchange_hash, hash_len);
    p += hash_len;
    *p++ = 'D';
    memcpy(p, session->session_id, session->session_id_len);
    p += session->session_id_len;

    sha256_hash(buf, p - buf, session->enc_key_server);

    return 0;
}

int ssh_handle_kex(ssh_session_t *session) {
    ssh_packet_t packet;
    uint8_t *p;
    uint8_t shared_secret[32];
    size_t secret_len = 0;
    uint8_t client_public[256];
    uint8_t server_public[256];
    uint32_t client_public_len = 0;
    uint32_t server_public_len = 0;
    uint32_t hostkey_len = 0;
    uint8_t *hostkey = NULL;

    if (!session) return -1;

    printf("ssh_handle_kex: Using KEX: %s\n", session->selected_kex);

    if (session->using_curve25519) {
        /* Curve25519 KEX */
        /* 1. Generate key pair */
        for (int i = 0; i < 32; i++) {
            session->curve25519_private[i] = rand() & 0xFF;
        }
        session->curve25519_private[0] &= 0xF8;
        session->curve25519_private[31] &= 0x7F;
        session->curve25519_private[31] |= 0x40;
        
        crypto_scalarmult_curve25519_base(session->curve25519_public, session->curve25519_private);
        printf("ssh_handle_kex: Curve25519 key pair generated\n");
        
        /* 2. Send ECDH_INIT */
        memset(&packet, 0, sizeof(packet));
        packet.type = SSH_MSG_KEX_ECDH_INIT;
        p = packet.payload;
        
        ssh_write_uint32(p, 32);
        p += 4;
        memcpy(p, session->curve25519_public, 32);
        p += 32;
        
        packet.payload_len = p - packet.payload;
        client_public_len = 32;
        memcpy(client_public, session->curve25519_public, 32);
        
        if (ssh_packet_send(session, &packet) < 0) {
            return -1;
        }
        
        session->state = SSH_STATE_KEXDH_SENT;
        
        printf("ssh_handle_kex: step1 - wait for KEX_ECDH_REPLY\n");
        
        /* 3. Receive ECDH_REPLY */
        memset(&packet, 0, sizeof(packet));
        if (ssh_packet_receive(session, &packet) < 0) {
            return -1;
        }
        
        if (packet.type == SSH_MSG_DISCONNECT) {
            uint32_t reason = ssh_read_uint32(packet.payload);
            uint32_t desc_len = ssh_read_uint32(packet.payload + 4);
            char *desc = (char *)(packet.payload + 8);
            ssh_set_error(session, "Server disconnected, reason: %u, desc: %.*s",
                          reason, desc_len > 256 ? 256 : desc_len, desc);
            return -1;
        }
        
        if (packet.type != SSH_MSG_KEX_ECDH_REPLY) {
            ssh_set_error(session, "Expected KEX_ECDH_REPLY, got %d", packet.type);
            return -1;
        }
        
        /* Parse KEX_ECDH_REPLY */
        p = packet.payload;
        
        hostkey_len = ssh_read_uint32(p);
        p += 4;
        hostkey = p;
        p += hostkey_len;
        
        server_public_len = ssh_read_uint32(p);
        p += 4;
        if (server_public_len != 32) {
            ssh_set_error(session, "Invalid Curve25519 key length: %u", server_public_len);
            return -1;
        }
        memcpy(server_public, p, 32);
        memcpy(session->curve25519_server_public, p, 32);
        p += 32;
        
        /* Skip signature */
        uint32_t sig_len = ssh_read_uint32(p);
        p += 4;
        
        /* Compute shared secret */
        crypto_scalarmult_curve25519(shared_secret, session->curve25519_private, session->curve25519_server_public);
        secret_len = 32;
        printf("ssh_handle_kex: Curve25519 shared secret computed\n");
    } else {
        /* Original DH (fallback) */
        DH *dh;
        BIGNUM *p_bn = NULL, *g_bn = NULL;
        uint8_t e[256];
        int e_len;
        
        dh = DH_new();
        if (!dh) {
            ssh_set_error(session, "Failed to create DH");
            return -1;
        }
        
        p_bn = BN_new();
        if (!p_bn) {
            ssh_set_error(session, "Failed to create p_bn");
            DH_free(dh);
            return -1;
        }
        
        if (!BN_hex2bn(&p_bn, dh_group14_p_hex)) {
            ssh_set_error(session, "Failed to parse DH p parameter");
            BN_free(p_bn);
            DH_free(dh);
            return -1;
        }
        
        g_bn = BN_new();
        if (!g_bn) {
            ssh_set_error(session, "Failed to create g_bn");
            BN_free(p_bn);
            DH_free(dh);
            return -1;
        }
        BN_set_word(g_bn, 2);
        
        if (!DH_set0_pqg(dh, p_bn, NULL, g_bn)) {
            ssh_set_error(session, "Failed to set DH parameters");
            BN_free(p_bn);
            BN_free(g_bn);
            DH_free(dh);
            return -1;
        }
        
        printf("ssh_handle_kex: - generate DH key\n");
        if (!DH_generate_key(dh)) {
            ssh_set_error(session, "Failed to generate DH key");
            DH_free(dh);
            return -1;
        }
        printf("ssh_handle_kex: - generated DH key\n");
        
        const BIGNUM *pub_key = NULL;
        DH_get0_key(dh, &pub_key, NULL);
        if (!pub_key) {
            ssh_set_error(session, "Failed to get DH public key");
            DH_free(dh);
            return -1;
        }
        
        e_len = BN_bn2bin(pub_key, e);
        if (e_len <= 0) {
            ssh_set_error(session, "Failed to convert DH public key");
            DH_free(dh);
            return -1;
        }
        
        /* Handle leading zero for sign */
        if (e[0] & 0x80) {
            client_public[0] = 0;
            memcpy(client_public + 1, e, e_len);
            client_public_len = e_len + 1;
        } else {
            memcpy(client_public, e, e_len);
            client_public_len = e_len;
        }
        
        /* Send KEXDH_INIT */
        memset(&packet, 0, sizeof(packet));
        packet.type = SSH_MSG_KEXDH_INIT;
        p = packet.payload;
        
        ssh_write_uint32(p, client_public_len);
        p += 4;
        memcpy(p, client_public, client_public_len);
        p += client_public_len;
        
        packet.payload_len = p - packet.payload;
        
        if (ssh_packet_send(session, &packet) < 0) {
            DH_free(dh);
            return -1;
        }
        
        session->state = SSH_STATE_KEXDH_SENT;
        
        printf("ssh_handle_kex: step1 - wait for KEXDH_REPLY\n");
        
        memset(&packet, 0, sizeof(packet));
        if (ssh_packet_receive(session, &packet) < 0) {
            DH_free(dh);
            return -1;
        }
        printf("ssh_handle_kex: step2 - KEXDH_REPLY received\n");
        
        if (packet.type == SSH_MSG_DISCONNECT) {
            uint32_t reason = ssh_read_uint32(packet.payload);
            uint32_t desc_len = ssh_read_uint32(packet.payload + 4);
            char *desc = (char *)(packet.payload + 8);
            ssh_set_error(session, "Server disconnected, reason: %u, desc: %.*s",
                          reason, desc_len > 256 ? 256 : desc_len, desc);
            DH_free(dh);
            return -1;
        }
        
        if (packet.type != SSH_MSG_KEXDH_REPLY) {
            ssh_set_error(session, "Expected KEXDH_REPLY, got %d", packet.type);
            DH_free(dh);
            return -1;
        }
        
        p = packet.payload;
        
        hostkey_len = ssh_read_uint32(p);
        p += 4;
        hostkey = p;
        p += hostkey_len;
        
        server_public_len = ssh_read_uint32(p);
        p += 4;
        memcpy(server_public, p, server_public_len);
        BIGNUM *f_bn = BN_bin2bn(p, server_public_len, NULL);
        p += server_public_len;
        
        /* Skip signature */
        uint32_t sig_len = ssh_read_uint32(p);
        p += 4;
        
        secret_len = DH_compute_key(shared_secret, f_bn, dh);
        BN_free(f_bn);
        
        if (secret_len <= 0) {
            ssh_set_error(session, "Failed to compute shared secret");
            DH_free(dh);
            return -1;
        }
        
        DH_free(dh);
    }
    
    /* Compute exchange hash */
    uint8_t exchange_hash[32];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    
    uint32_t len = strlen(session->client_version);
    ssh_write_uint32((uint8_t*)&len, len);
    SHA256_Update(&ctx, &len, 4);
    SHA256_Update(&ctx, session->client_version, len);
    
    len = strlen(session->server_version);
    SHA256_Update(&ctx, &len, 4);
    SHA256_Update(&ctx, session->server_version, len);
    
    len = session->client_kexinit_len;
    SHA256_Update(&ctx, &len, 4);
    SHA256_Update(&ctx, session->client_kexinit, session->client_kexinit_len);
    
    len = session->server_kexinit_len;
    SHA256_Update(&ctx, &len, 4);
    SHA256_Update(&ctx, session->server_kexinit, session->server_kexinit_len);
    
    SHA256_Update(&ctx, &hostkey_len, 4);
    SHA256_Update(&ctx, hostkey, hostkey_len);
    
    SHA256_Update(&ctx, &client_public_len, 4);
    SHA256_Update(&ctx, client_public, client_public_len);
    
    SHA256_Update(&ctx, &server_public_len, 4);
    SHA256_Update(&ctx, server_public, server_public_len);
    
    SHA256_Update(&ctx, &secret_len, 4);
    SHA256_Update(&ctx, shared_secret, secret_len);
    
    SHA256_Final(exchange_hash, &ctx);
    
    /* Save session ID (first exchange hash) */
    if (session->session_id_len == 0) {
        memcpy(session->session_id, exchange_hash, 32);
        session->session_id_len = 32;
    }
    printf("ssh_handle_kex: computed exchange hash\n");
    
    /* Derive keys */
    derive_keys(session, shared_secret, secret_len, exchange_hash, 32);
    
    /* Send NEWKEYS */
    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_NEWKEYS;
    packet.payload_len = 0;
    
    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }
    
    session->state = SSH_STATE_NEWKEYS_SENT;
    
    /* Receive NEWKEYS */
    memset(&packet, 0, sizeof(packet));
    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }
    
    if (packet.type == SSH_MSG_DISCONNECT) {
        uint32_t reason = ssh_read_uint32(packet.payload);
        uint32_t desc_len = ssh_read_uint32(packet.payload + 4);
        char *desc = (char *)(packet.payload + 8);
        ssh_set_error(session, "Server disconnected, reason: %u, desc: %.*s",
                      reason, desc_len > 256 ? 256 : desc_len, desc);
        return -1;
    }
    
    if (packet.type != SSH_MSG_NEWKEYS) {
        ssh_set_error(session, "Expected NEWKEYS, got %d", packet.type);
        return -1;
    }
    
    session->state = SSH_STATE_NEWKEYS_RECEIVED;
    session->encryption_enabled = 1;
    
    printf("ssh_handle_kex: KEX complete!\n");
    
    return 0;
}

/* Authentication */
int ssh_userauth_none(ssh_session_t *session, const char *username) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !username) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_USERAUTH_REQUEST;
    p = packet.payload;

    ssh_write_uint32(p, strlen(username));
    p += 4;
    memcpy(p, username, strlen(username));
    p += strlen(username);

    const char *service = "ssh-connection";
    ssh_write_uint32(p, strlen(service));
    p += 4;
    memcpy(p, service, strlen(service));
    p += strlen(service);

    ssh_write_uint32(p, strlen("none"));
    p += 4;
    memcpy(p, "none", 4);
    p += 4;

    packet.payload_len = p - packet.payload;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_USERAUTH_SUCCESS) {
        session->authenticated = 1;
        session->state = SSH_STATE_AUTH_SUCCESS;
        return 0;
    }

    return -1;
}

int ssh_userauth_password(ssh_session_t *session, const char *username, const char *password) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !username || !password) return -1;

    strncpy(session->username, username, sizeof(session->username) - 1);
    strncpy(session->password, password, sizeof(session->password) - 1);

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_USERAUTH_REQUEST;
    p = packet.payload;

    ssh_write_uint32(p, strlen(username));
    p += 4;
    memcpy(p, username, strlen(username));
    p += strlen(username);

    const char *service = "ssh-connection";
    ssh_write_uint32(p, strlen(service));
    p += 4;
    memcpy(p, service, strlen(service));
    p += strlen(service);

    ssh_write_uint32(p, strlen("password"));
    p += 4;
    memcpy(p, "password", 8);
    p += 8;

    *p++ = 0;

    ssh_write_uint32(p, strlen(password));
    p += 4;
    memcpy(p, password, strlen(password));
    p += strlen(password);

    packet.payload_len = p - packet.payload;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_USERAUTH_SUCCESS) {
        session->authenticated = 1;
        session->state = SSH_STATE_AUTH_SUCCESS;
        return 0;
    }

    return -1;
}

int ssh_userauth_list(ssh_session_t *session, const char *username) {
    return ssh_userauth_none(session, username);
}

/* Channel Management */
int ssh_channel_open_session(ssh_session_t *session) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !session->authenticated) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_OPEN;
    p = packet.payload;

    const char *type = "session";
    ssh_write_uint32(p, strlen(type));
    p += 4;
    memcpy(p, type, strlen(type));
    p += strlen(type);

    session->channel_local = 0;
    ssh_write_uint32(p, session->channel_local);
    p += 4;

    ssh_write_uint32(p, session->window_local);
    p += 4;

    ssh_write_uint32(p, SSH_MAX_PACKET_SIZE);
    p += 4;

    packet.payload_len = p - packet.payload;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_CHANNEL_OPEN_CONFIRMATION) {
        p = packet.payload;
        p += 4; /* recipient channel */
        session->channel_remote = ssh_read_uint32(p);
        p += 4;
        session->window_remote = ssh_read_uint32(p);
        p += 4;
        p += 4; /* max packet size */

        session->state = SSH_STATE_CHANNEL_OPEN;
        return 0;
    }

    return -1;
}

int ssh_channel_request_pty(ssh_session_t *session, const char *term) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !session->authenticated) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_REQUEST;
    p = packet.payload;

    ssh_write_uint32(p, session->channel_remote);
    p += 4;

    const char *type = "pty-req";
    ssh_write_uint32(p, strlen(type));
    p += 4;
    memcpy(p, type, strlen(type));
    p += strlen(type);

    *p++ = 1;

    if (!term) term = "xterm";
    ssh_write_uint32(p, strlen(term));
    p += 4;
    memcpy(p, term, strlen(term));
    p += strlen(term);

    ssh_write_uint32(p, session->terminal_width);
    p += 4;
    ssh_write_uint32(p, session->terminal_height);
    p += 4;

    ssh_write_uint32(p, 0);
    p += 4;
    ssh_write_uint32(p, 0);
    p += 4;

    ssh_write_uint32(p, 0);
    p += 4;

    packet.payload_len = p - packet.payload;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    return (packet.type == SSH_MSG_CHANNEL_SUCCESS) ? 0 : -1;
}

int ssh_channel_request_shell(ssh_session_t *session) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !session->authenticated) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_REQUEST;
    p = packet.payload;

    ssh_write_uint32(p, session->channel_remote);
    p += 4;

    const char *type = "shell";
    ssh_write_uint32(p, strlen(type));
    p += 4;
    memcpy(p, type, strlen(type));
    p += strlen(type);

    *p++ = 1;

    packet.payload_len = p - packet.payload;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_CHANNEL_SUCCESS) {
        session->state = SSH_STATE_CONNECTED;
        return 0;
    }

    return -1;
}

int ssh_channel_request_exec(ssh_session_t *session, const char *command) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !session->authenticated || !command) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_REQUEST;
    p = packet.payload;

    ssh_write_uint32(p, session->channel_remote);
    p += 4;

    const char *type = "exec";
    ssh_write_uint32(p, strlen(type));
    p += 4;
    memcpy(p, type, strlen(type));
    p += strlen(type);

    *p++ = 1;

    ssh_write_uint32(p, strlen(command));
    p += 4;
    memcpy(p, command, strlen(command));
    p += strlen(command);

    packet.payload_len = p - packet.payload;

    if (ssh_packet_send(session, &packet) < 0) {
        return -1;
    }

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_CHANNEL_SUCCESS) {
        session->state = SSH_STATE_CONNECTED;
        return 0;
    }

    return -1;
}

int ssh_channel_send_data(ssh_session_t *session, const void *data, size_t len) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !session->authenticated || !data) return -1;

    if (len > SSH_MAX_PACKET_SIZE - 64) {
        len = SSH_MAX_PACKET_SIZE - 64;
    }

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_DATA;
    p = packet.payload;

    ssh_write_uint32(p, session->channel_remote);
    p += 4;

    ssh_write_uint32(p, len);
    p += 4;
    memcpy(p, data, len);
    p += len;

    packet.payload_len = p - packet.payload;

    return ssh_packet_send(session, &packet);
}

int ssh_channel_receive_data(ssh_session_t *session, void *data, size_t len) {
    ssh_packet_t packet;

    if (!session || !session->authenticated || !data) return -1;

    if (ssh_packet_receive(session, &packet) < 0) {
        return -1;
    }

    if (packet.type == SSH_MSG_CHANNEL_DATA) {
        uint8_t *p = packet.payload;
        p += 4; /* channel */
        uint32_t data_len = ssh_read_uint32(p);
        p += 4;

        if (data_len > len) {
            data_len = len;
        }

        memcpy(data, p, data_len);
        return data_len;
    } else if (packet.type == SSH_MSG_CHANNEL_EOF) {
        return 0;
    } else if (packet.type == SSH_MSG_CHANNEL_CLOSE) {
        session->state = SSH_STATE_DISCONNECTED;
        return 0;
    }

    return -1;
}

int ssh_channel_close(ssh_session_t *session) {
    ssh_packet_t packet;
    uint8_t *p;

    if (!session || !session->authenticated) return -1;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_CLOSE;
    p = packet.payload;

    ssh_write_uint32(p, session->channel_remote);
    p += 4;

    packet.payload_len = p - packet.payload;

    ssh_packet_send(session, &packet);

    return 0;
}

/* Helper function to read exactly len bytes */
static int ssh_read_n(ssh_session_t *session, uint8_t *buf, size_t len) {
    size_t total = 0;
    ssize_t n;
    int saved_errno;

    while (total < len) {
        errno = 0;
        n = read(session->socket, buf + total, len - total);
        saved_errno = errno;

        if (n < 0) {
#ifdef EINTR
            if (saved_errno == EINTR) continue;
#endif
            if (saved_errno == 0) {
                ssh_set_error(session, "Read error: connection reset");
            } else {
                ssh_set_error(session, "Read error: %s", strerror(saved_errno));
            }
            return -1;
        }
        if (n == 0) {
            ssh_set_error(session, "Connection closed by peer");
            return -1;
        }
        total += n;
    }
    return 0;
}

/* Packet I/O */
int ssh_packet_send(ssh_session_t *session, ssh_packet_t *packet) {
    uint8_t buf[SSH_MAX_PACKET_SIZE + 64];
    uint32_t block_size = 8;
    uint32_t packet_len;
    uint32_t padding_len;
    size_t total_len;

    if (!session || !packet || session->socket < 0) return -1;

    packet_len = 1 + packet->payload_len;
    padding_len = block_size - (packet_len % block_size);
    if (padding_len < 4) {
        padding_len += block_size;
    }

    uint8_t *p = buf;

    ssh_write_uint32(p, 1 + packet->payload_len + padding_len);
    p += 4;

    *p++ = padding_len;
    *p++ = packet->type;

    memcpy(p, packet->payload, packet->payload_len);
    p += packet->payload_len;

    for (uint32_t i = 0; i < padding_len; i++) {
        *p++ = (uint8_t)(rand() & 0xFF);
    }

    total_len = p - buf;

    if (write(session->socket, buf, total_len) != (ssize_t)total_len) {
        ssh_set_error(session, "Failed to send packet: %s", strerror(errno));
        return -1;
    }

    session->packet_seq_client++;

    return 0;
}

int ssh_packet_receive(ssh_session_t *session, ssh_packet_t *packet) {
    uint8_t buf[4];
    uint32_t packet_len;
    uint8_t padding_len;
    uint8_t *packet_buf;

    if (!session || !packet || session->socket < 0) return -1;

    if (ssh_read_n(session, buf, 4) < 0) {
        return -1;
    }

    packet_len = ssh_read_uint32(buf);

    if (packet_len > SSH_MAX_PACKET_SIZE || packet_len < 5) {
        ssh_set_error(session, "Invalid packet length: %u", packet_len);
        return -1;
    }

    packet_buf = (uint8_t *)malloc(packet_len);
    if (!packet_buf) {
        ssh_set_error(session, "Failed to allocate packet buffer");
        return -1;
    }

    if (ssh_read_n(session, packet_buf, packet_len) < 0) {
        free(packet_buf);
        return -1;
    }

    padding_len = packet_buf[0];
    packet->type = packet_buf[1];
    packet->payload_len = packet_len - padding_len - 2;
    if (packet->payload_len > SSH_MAX_PACKET_SIZE) {
        free(packet_buf);
        ssh_set_error(session, "Invalid payload length");
        return -1;
    }
    memcpy(packet->payload, packet_buf + 2, packet->payload_len);

    free(packet_buf);

    session->packet_seq_server++;

    return 0;
}

int ssh_packet_read(ssh_session_t *session, uint8_t *buf, size_t len) {
    if (!session || !buf || session->socket < 0) return -1;

    ssize_t n = read(session->socket, buf, len);
    if (n < 0) {
        ssh_set_error(session, "Read error: %s", strerror(errno));
        return -1;
    }

    return n;
}

int ssh_packet_write(ssh_session_t *session, const uint8_t *buf, size_t len) {
    if (!session || !buf || session->socket < 0) return -1;

    ssize_t n = write(session->socket, buf, len);
    if (n < 0) {
        ssh_set_error(session, "Write error: %s", strerror(errno));
        return -1;
    }

    return n;
}

/* Utility Functions */
uint32_t ssh_read_uint32(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           (uint32_t)buf[3];
}

void ssh_write_uint32(uint8_t *buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

uint8_t *ssh_read_string(const uint8_t *buf, uint32_t *len) {
    *len = ssh_read_uint32(buf);
    return (uint8_t *)(buf + 4);
}

void ssh_write_string(uint8_t *buf, const uint8_t *str, uint32_t len) {
    ssh_write_uint32(buf, len);
    memcpy(buf + 4, str, len);
}

/* Error Handling */
const char *ssh_get_error(ssh_session_t *session) {
    (void)session;
    return ssh_error_buffer;
}

void ssh_set_error(ssh_session_t *session, const char *fmt, ...) {
    va_list ap;
    (void)session;

    va_start(ap, fmt);
    vsnprintf(ssh_error_buffer, sizeof(ssh_error_buffer), fmt, ap);
    va_end(ap);
}
