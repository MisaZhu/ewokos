#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <ewoksys/core.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <ewoksys/proto.h>
#include <ewoksys/proc.h>
#include <ewoksys/session.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/wait.h>
#include <setenv.h>

#define SSH_DEFAULT_PORT        22
#define SSH_MAX_PACKET_SIZE     32768
#define SSH_SERVER_VERSION      "SSH-2.0-EwokOS_sshd"
#define SSH_HOST_KEY_PATH       "/etc/ssh_host_rsa_key.der"

#define SSH_MSG_DISCONNECT                      1
#define SSH_MSG_SERVICE_REQUEST                 5
#define SSH_MSG_SERVICE_ACCEPT                  6
#define SSH_MSG_KEXINIT                         20
#define SSH_MSG_NEWKEYS                         21
#define SSH_MSG_KEXDH_INIT                      30
#define SSH_MSG_KEXDH_REPLY                     31
#define SSH_MSG_USERAUTH_REQUEST                50
#define SSH_MSG_USERAUTH_FAILURE                51
#define SSH_MSG_USERAUTH_SUCCESS                52
#define SSH_MSG_GLOBAL_REQUEST                  80
#define SSH_MSG_REQUEST_FAILURE                 82
#define SSH_MSG_CHANNEL_OPEN                    90
#define SSH_MSG_CHANNEL_OPEN_CONFIRMATION       91
#define SSH_MSG_CHANNEL_OPEN_FAILURE            92
#define SSH_MSG_CHANNEL_WINDOW_ADJUST           93
#define SSH_MSG_CHANNEL_DATA                    94
#define SSH_MSG_CHANNEL_EXTENDED_DATA           95
#define SSH_MSG_CHANNEL_EOF                     96
#define SSH_MSG_CHANNEL_CLOSE                   97
#define SSH_MSG_CHANNEL_REQUEST                 98
#define SSH_MSG_CHANNEL_SUCCESS                 99
#define SSH_MSG_CHANNEL_FAILURE                 100

#define SSH_DISCONNECT_PROTOCOL_ERROR           2
#define SSH_DISCONNECT_KEY_EXCHANGE_FAILED      3
#define SSH_DISCONNECT_BY_APPLICATION           11

#define SSH_CHANNEL_OPEN_ADMINISTRATIVELY_PROHIBITED 1

#define SSH_STDERR_DATA                         1

#define CHILD_STDIN_READ   0
#define CHILD_STDIN_WRITE  1
#define CHILD_STDOUT_READ  0
#define CHILD_STDOUT_WRITE 1
#define CHILD_STDERR_READ  0
#define CHILD_STDERR_WRITE 1
#define CHILD_CTRL_READ    0
#define CHILD_CTRL_WRITE   1

#define SSHD_CHILD_START_MAGIC 0x53534844u

#define SSH_KEX_ALG_MLKEM768_X25519_SHA256 "mlkem768x25519-sha256"
#define SSH_KEX_ALG_DH_GROUP14_SHA256      "diffie-hellman-group14-sha256"

#define SSH_X25519_KEY_LEN         CURVE25519_PUB_KEY_SIZE
#define SSH_MLKEM768_PUB_KEY_LEN   WC_ML_KEM_768_PUBLIC_KEY_SIZE
#define SSH_MLKEM768_CT_LEN        WC_ML_KEM_768_CIPHER_TEXT_SIZE
#define SSH_HYBRID_SECRET_LEN      SHA256_DIGEST_LENGTH

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

typedef struct {
    uint8_t type;
    uint8_t payload[SSH_MAX_PACKET_SIZE];
    size_t payload_len;
} ssh_packet_t;

typedef struct sshd_session sshd_session_t;

typedef struct {
    sshd_session_t* session;
    int fd;
    uint32_t extended_type;
    volatile int start_ready;
} stream_thread_arg_t;

typedef struct {
    uint32_t magic;
    uint32_t is_shell;
    int32_t uid;
    int32_t gid;
    char user[SESSION_USER_MAX];
    char home[SESSION_HOME_MAX];
    char shell_cmd[SESSION_CMD_MAX];
    char exec_cmd[SESSION_CMD_MAX];
} sshd_child_start_msg_t;

struct sshd_session {
    int socket;
    char client_version[256];
    char server_version[256];

    uint8_t client_kexinit[SSH_MAX_PACKET_SIZE];
    size_t client_kexinit_len;
    uint8_t server_kexinit[SSH_MAX_PACKET_SIZE];
    size_t server_kexinit_len;

    uint8_t session_id[32];
    size_t session_id_len;
    char negotiated_kex_alg[64];
    char negotiated_hostkey_alg[32];

    uint8_t iv_c2s[16];
    uint8_t iv_s2c[16];
    uint8_t enc_key_c2s[32];
    uint8_t enc_key_s2c[32];
    uint8_t mac_key_c2s[32];
    uint8_t mac_key_s2c[32];
    uint32_t seq_c2s;
    uint32_t seq_s2c;
    int encryption_enabled;

    int authenticated;
    session_info_t user_info;

    uint32_t remote_channel;
    uint32_t local_channel;
    uint32_t remote_window;
    uint32_t remote_packet_max;
    uint32_t local_window;
    uint32_t local_packet_max;
    int channel_open;
    int child_started;
    int sent_close;
    int closing;

    int terminal_width;
    int terminal_height;
    int pty_enabled;

    int child_stdin[2];
    int child_stdout[2];
    int child_stderr[2];
    int child_control[2];
    pid_t child_pid;

    pthread_mutex_t send_lock;
};

static RSA* g_host_rsa = NULL;
static uint8_t g_hostkey_blob[512];
static size_t g_hostkey_blob_len = 0;
static char g_error[256];

static int sshd_clone_vfs_fds_to_pid(int pid) {
    proto_t in;
    int vfsd_pid;
    int res;

    if(pid <= 0)
        return -1;

    vfsd_pid = ipc_serv_get(IPC_SERV_VFS);
    if(vfsd_pid < 0)
        return -1;

    PF->init(&in)->addi(&in, proc_getpid(-1))->addi(&in, pid);
    res = ipc_call_wait(vfsd_pid, VFS_PROC_CLONE, &in);
    PF->clear(&in);
    return res;
}

static void sshd_release_vfs_fds_for_pid(int pid) {
    proto_t in;
    int vfsd_pid;

    if(pid <= 0)
        return;

    vfsd_pid = ipc_serv_get(IPC_SERV_VFS);
    if(vfsd_pid < 0)
        return;

    PF->init(&in)->addi(&in, pid);
    ipc_call_wait(vfsd_pid, VFS_PROC_EXIT, &in);
    PF->clear(&in);
}

static int write_ssh_string(uint8_t* buf, size_t cap, size_t* off,
        const uint8_t* data, uint32_t len);

#define sshd_dbg(...) ((void)0)
#define SSHD_CHILD_MARK(msg) ((void)0)

static void sshd_set_error(const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(g_error, sizeof(g_error), fmt, ap);
    va_end(ap);
}

/* #region debug-point openssl-error */
static void sshd_dbg_last_ssl_error(const char* stage) {
    unsigned long err;
    char err_buf[160];

    err = ERR_get_error();
    if(err == 0) {
        sshd_dbg("%s: no openssl error", stage);
        return;
    }
    ERR_error_string_n(err, err_buf, sizeof(err_buf));
    sshd_dbg("%s: openssl err=0x%lx %s", stage, err, err_buf);
}
/* #endregion debug-point openssl-error */

static uint32_t ssh_read_uint32(const uint8_t* buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           (uint32_t)buf[3];
}

static void ssh_write_uint32(uint8_t* buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

static void sha256_hash(const uint8_t* data, size_t len, uint8_t* out) {
    SHA256_CTX ctx;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(out, &ctx);
}

static void hmac_sha256(const uint8_t* key, size_t key_len,
        const uint8_t* data, size_t data_len, uint8_t* out) {
    unsigned int mac_len = 32;
    HMAC(EVP_sha256(), key, key_len, data, data_len, out, &mac_len);
}

static void ssh_ctr_increment(uint8_t* counter, size_t blocks) {
    while(blocks-- > 0) {
        for(int i = 15; i >= 0; i--) {
            counter[i]++;
            if(counter[i] != 0)
                break;
        }
    }
}

static int ssh_aes256_ctr_crypt(uint8_t* data, size_t len,
        const uint8_t* key, uint8_t* iv) {
    WOLFSSL_AES_KEY aes_key;
    uint8_t counter[16];
    uint8_t stream[16];
    size_t off = 0;

    if(wolfSSL_AES_set_encrypt_key(key, 256, &aes_key) != 0)
        return -1;

    memcpy(counter, iv, sizeof(counter));
    while(off < len) {
        size_t chunk = len - off;
        uint8_t zero_iv[16] = {0};
        if(chunk > sizeof(stream))
            chunk = sizeof(stream);
        wolfSSL_AES_cbc_encrypt(counter, stream, sizeof(stream), &aes_key, zero_iv, AES_ENCRYPT);
        for(size_t i = 0; i < chunk; i++)
            data[off + i] ^= stream[i];
        ssh_ctr_increment(counter, 1);
        off += chunk;
    }
    memcpy(iv, counter, sizeof(counter));
    return 0;
}

static int ssh_encode_mpint(const uint8_t* value, size_t value_len,
        uint8_t* out, size_t out_cap, size_t* out_len) {
    size_t offset = 0;
    size_t encoded_len = 0;

    while(offset < value_len && value[offset] == 0)
        offset++;

    if(offset == value_len) {
        encoded_len = 0;
    }
    else {
        encoded_len = value_len - offset;
        if(value[offset] & 0x80)
            encoded_len++;
    }

    if(4 + encoded_len > out_cap)
        return -1;

    ssh_write_uint32(out, encoded_len);
    if(encoded_len == 0) {
        *out_len = 4;
        return 0;
    }

    uint8_t* p = out + 4;
    if(value[offset] & 0x80)
        *p++ = 0;
    memcpy(p, value + offset, value_len - offset);
    *out_len = 4 + encoded_len;
    return 0;
}

static int ssh_encode_string(const uint8_t* value, size_t value_len,
        uint8_t* out, size_t out_cap, size_t* out_len) {
    size_t off = 0;

    if(write_ssh_string(out, out_cap, &off, value, (uint32_t)value_len) < 0)
        return -1;
    *out_len = off;
    return 0;
}

static void sha256_update_ssh_string(SHA256_CTX* ctx, const uint8_t* data, size_t len) {
    uint8_t len_buf[4];

    ssh_write_uint32(len_buf, (uint32_t)len);
    SHA256_Update(ctx, len_buf, sizeof(len_buf));
    if(len > 0)
        SHA256_Update(ctx, data, len);
}

static void ssh_reverse_copy(uint8_t* dst, const uint8_t* src, size_t len) {
    size_t i;

    for(i = 0; i < len; ++i)
        dst[i] = src[len - 1 - i];
}

static int ssh_curve25519_shared_secret_be(curve25519_key* private_key,
        const uint8_t* peer_public, uint8_t* out) {
    curve25519_key peer_key;
    byte shared_secret_le[SSH_X25519_KEY_LEN];
    word32 shared_secret_len = sizeof(shared_secret_le);
    int ret;

    memset(&peer_key, 0, sizeof(peer_key));
    memset(shared_secret_le, 0, sizeof(shared_secret_le));
    wc_curve25519_init(&peer_key);

    ret = wc_curve25519_import_public_ex(peer_public, SSH_X25519_KEY_LEN,
            &peer_key, EC25519_LITTLE_ENDIAN);
    if(ret == 0) {
        ret = wc_curve25519_shared_secret(private_key, &peer_key,
                shared_secret_le, &shared_secret_len);
    }
    if(ret == 0 && shared_secret_len != SSH_X25519_KEY_LEN)
        ret = -1;
    if(ret == 0)
        ssh_reverse_copy(out, shared_secret_le, SSH_X25519_KEY_LEN);

    wc_curve25519_free(&peer_key);
    memset(shared_secret_le, 0, sizeof(shared_secret_le));
    return ret;
}

static void derive_one_key(uint8_t* out, size_t out_len,
        const uint8_t* shared_secret_encoded, size_t shared_secret_encoded_len,
        const uint8_t* exchange_hash, size_t hash_len, uint8_t id,
        const uint8_t* session_id, size_t session_id_len) {
    uint8_t buf[4096];
    uint8_t digest[32];
    uint8_t* p = buf;
    size_t copied = 0;

    memcpy(p, shared_secret_encoded, shared_secret_encoded_len);
    p += shared_secret_encoded_len;
    memcpy(p, exchange_hash, hash_len);
    p += hash_len;
    *p++ = id;
    memcpy(p, session_id, session_id_len);
    p += session_id_len;
    sha256_hash(buf, p - buf, digest);

    while(copied < out_len) {
        size_t chunk = out_len - copied;
        if(chunk > sizeof(digest))
            chunk = sizeof(digest);
        memcpy(out + copied, digest, chunk);
        copied += chunk;
    }
}

static void derive_keys(sshd_session_t* s, const uint8_t* shared_secret_mpint,
        size_t shared_secret_mpint_len, const uint8_t* exchange_hash, size_t hash_len) {
    derive_one_key(s->iv_c2s, sizeof(s->iv_c2s), shared_secret_mpint, shared_secret_mpint_len,
            exchange_hash, hash_len, 'A', s->session_id, s->session_id_len);
    derive_one_key(s->iv_s2c, sizeof(s->iv_s2c), shared_secret_mpint, shared_secret_mpint_len,
            exchange_hash, hash_len, 'B', s->session_id, s->session_id_len);
    derive_one_key(s->enc_key_c2s, sizeof(s->enc_key_c2s), shared_secret_mpint, shared_secret_mpint_len,
            exchange_hash, hash_len, 'C', s->session_id, s->session_id_len);
    derive_one_key(s->enc_key_s2c, sizeof(s->enc_key_s2c), shared_secret_mpint, shared_secret_mpint_len,
            exchange_hash, hash_len, 'D', s->session_id, s->session_id_len);
    derive_one_key(s->mac_key_c2s, sizeof(s->mac_key_c2s), shared_secret_mpint, shared_secret_mpint_len,
            exchange_hash, hash_len, 'E', s->session_id, s->session_id_len);
    derive_one_key(s->mac_key_s2c, sizeof(s->mac_key_s2c), shared_secret_mpint, shared_secret_mpint_len,
            exchange_hash, hash_len, 'F', s->session_id, s->session_id_len);
}

static int ssh_read_n(sshd_session_t* s, void* buf, size_t len) {
    uint8_t* p = (uint8_t*)buf;
    size_t total = 0;

    while(total < len) {
        ssize_t n = read(s->socket, p + total, len - total);
        if(n < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                proc_usleep(1000);
                continue;
            }
            sshd_set_error("read failed: %s", strerror(errno));
            return -1;
        }
        if(n == 0) {
            sshd_set_error("peer closed");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

static int ssh_write_n(sshd_session_t* s, const void* buf, size_t len) {
    const uint8_t* p = (const uint8_t*)buf;
    size_t total = 0;

    while(total < len) {
        ssize_t n = write(s->socket, p + total, len - total);
        if(n < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                proc_usleep(1000);
                continue;
            }
            sshd_set_error("write failed: %s", strerror(errno));
            return -1;
        }
        if(n == 0) {
            sshd_set_error("short write");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

static int write_fd_all(int fd, const void* buf, size_t len) {
    const uint8_t* p = (const uint8_t*)buf;
    size_t total = 0;

    while(total < len) {
        ssize_t n = write(fd, p + total, len - total);
        if(n < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                proc_usleep(1000);
                continue;
            }
            return -1;
        }
        if(n == 0)
            return -1;
        total += (size_t)n;
    }
    return 0;
}

static int read_fd_all(int fd, void* buf, size_t len) {
    uint8_t* p = (uint8_t*)buf;
    size_t total = 0;

    while(total < len) {
        ssize_t n = read(fd, p + total, len - total);
        if(n < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                proc_usleep(1000);
                continue;
            }
            return -1;
        }
        if(n == 0)
            return -1;
        total += (size_t)n;
    }
    return 0;
}

static int exec_program_direct(const char* name) {
    char fpath[64];
    int sz = 0;
    uint8_t* buf;

    if(name == NULL || name[0] == 0)
        return -1;

    memset(fpath, 0, sizeof(fpath));
    for(size_t i = 0; i < sizeof(fpath) - 1; i++) {
        if(name[i] == '\0' || name[i] == ' ' || name[i] == '\t' || name[i] == '\n')
            break;
        fpath[i] = name[i];
    }

    SSHD_CHILD_MARK("exec-read-begin");
    buf = vfs_readfile(fpath, &sz);
    if(buf == NULL) {
        SSHD_CHILD_MARK("exec-read-fail");
        return -1;
    }
    SSHD_CHILD_MARK("exec-read-ok");

    SSHD_CHILD_MARK("exec-sys-begin");
    proc_exec_elf(name, (const char*)buf, sz);
    SSHD_CHILD_MARK("exec-sys-return");
    free(buf);
    return 0;
}

static void copy_cstr(char* dst, size_t dst_size, const char* src) {
    size_t len;

    if(dst == NULL || dst_size == 0)
        return;
    if(src == NULL) {
        dst[0] = 0;
        return;
    }
    len = strlen(src);
    if(len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = 0;
}

static int build_hostkey_blob_from_rsa(RSA* rsa) {
    const BIGNUM *rsa_e, *rsa_n;
    uint8_t e_bin[8];
    uint8_t n_bin[256];
    size_t e_mpint_len = 0;
    size_t n_mpint_len = 0;
    uint8_t e_mpint[16];
    uint8_t n_mpint[264];
    size_t off = 0;
    int e_len;
    int n_len;

    if(rsa == NULL)
        return -1;

    RSA_get0_key(rsa, &rsa_n, &rsa_e, NULL);
    if(rsa_n == NULL || rsa_e == NULL)
        return -1;

    e_len = BN_bn2bin(rsa_e, e_bin);
    n_len = BN_bn2bin(rsa_n, n_bin);
    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: exponent_len=%d modulus_len=%d", e_len, n_len);
    /* #endregion debug-point init-host-key */
    if(e_len <= 0 || n_len <= 0)
        return -1;

    if(ssh_encode_mpint(e_bin, (size_t)e_len, e_mpint, sizeof(e_mpint), &e_mpint_len) < 0)
        return -1;
    if(ssh_encode_mpint(n_bin, (size_t)n_len, n_mpint, sizeof(n_mpint), &n_mpint_len) < 0)
        return -1;
    if(write_ssh_string(g_hostkey_blob, sizeof(g_hostkey_blob), &off, (const uint8_t*)"ssh-rsa", 7) < 0 ||
            write_ssh_string(g_hostkey_blob, sizeof(g_hostkey_blob), &off,
                e_mpint + 4, (uint32_t)(e_mpint_len - 4)) < 0 ||
            write_ssh_string(g_hostkey_blob, sizeof(g_hostkey_blob), &off,
                n_mpint + 4, (uint32_t)(n_mpint_len - 4)) < 0) {
        return -1;
    }
    g_hostkey_blob_len = off;
    return 0;
}

static int load_host_key_from_file(const char* path) {
    struct stat st;
    int fd = -1;
    uint8_t der[2048];
    ssize_t n;

    if(path == NULL || stat(path, &st) != 0)
        return -1;
    if(st.st_size <= 0 || st.st_size > (int)sizeof(der))
        return -1;

    fd = open(path, O_RDONLY);
    if(fd < 0)
        return -1;
    n = read(fd, der, sizeof(der));
    close(fd);
    if(n <= 0)
        return -1;

    g_host_rsa = RSA_new();
    if(g_host_rsa == NULL)
        return -1;
    if(wolfSSL_RSA_LoadDer(g_host_rsa, der, (int)n) != 1) {
        RSA_free(g_host_rsa);
        g_host_rsa = NULL;
        return -1;
    }
    if(build_hostkey_blob_from_rsa(g_host_rsa) < 0) {
        RSA_free(g_host_rsa);
        g_host_rsa = NULL;
        return -1;
    }
    return 0;
}

static int save_host_key_to_file(const char* path, const uint8_t* der, int der_len) {
    int fd;

    if(path == NULL || der == NULL || der_len <= 0)
        return -1;
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if(fd < 0)
        return -1;
    if(write_fd_all(fd, der, (size_t)der_len) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int ssh_packet_send_locked(sshd_session_t* s, const ssh_packet_t* packet) {
    uint8_t buf[SSH_MAX_PACKET_SIZE + 96];
    uint8_t mac[32];
    uint8_t mac_input[4 + SSH_MAX_PACKET_SIZE + 64];
    uint32_t block_size = s->encryption_enabled ? 16 : 8;
    uint32_t base_len = 1 + 1 + packet->payload_len;
    uint32_t padding_len = block_size - ((4 + base_len) % block_size);
    uint32_t packet_len;
    uint32_t seq;
    size_t total_len;
    size_t wire_len;
    uint8_t* p;

    if(padding_len < 4)
        padding_len += block_size;
    packet_len = base_len + padding_len;
    total_len = 4 + packet_len;

    if(total_len > sizeof(buf)) {
        sshd_set_error("packet too large");
        return -1;
    }

    p = buf;
    ssh_write_uint32(p, packet_len);
    p += 4;
    *p++ = (uint8_t)padding_len;
    *p++ = packet->type;
    memcpy(p, packet->payload, packet->payload_len);
    p += packet->payload_len;
    for(uint32_t i = 0; i < padding_len; i++)
        *p++ = (uint8_t)(rand() & 0xff);

    wire_len = total_len;
    if(s->encryption_enabled) {
        seq = s->seq_s2c;
        ssh_write_uint32(mac_input, seq);
        memcpy(mac_input + 4, buf, total_len);
        hmac_sha256(s->mac_key_s2c, sizeof(s->mac_key_s2c), mac_input, 4 + total_len, mac);
        if(ssh_aes256_ctr_crypt(buf, total_len, s->enc_key_s2c, s->iv_s2c) < 0) {
            sshd_set_error("encrypt failed");
            return -1;
        }
        memcpy(buf + total_len, mac, sizeof(mac));
        wire_len += sizeof(mac);
    }

    if(ssh_write_n(s, buf, wire_len) < 0)
        return -1;
    s->seq_s2c++;
    return 0;
}

static int ssh_packet_send(sshd_session_t* s, const ssh_packet_t* packet) {
    int ret;

    pthread_mutex_lock(&s->send_lock);
    ret = ssh_packet_send_locked(s, packet);
    pthread_mutex_unlock(&s->send_lock);
    return ret;
}

static int ssh_packet_receive(sshd_session_t* s, ssh_packet_t* packet) {
    uint8_t first_block[16];
    uint8_t mac[32];
    uint8_t calc_mac[32];
    uint8_t mac_input[4 + SSH_MAX_PACKET_SIZE + 64];
    uint8_t* packet_buf;
    uint32_t block_size = s->encryption_enabled ? 16 : 8;
    uint32_t packet_len;
    uint32_t total_len;
    uint32_t seq = s->seq_c2s;
    uint8_t padding_len;

    if(!s->encryption_enabled) {
        uint8_t len_buf[4];
        if(ssh_read_n(s, len_buf, 4) < 0)
            return -1;
        packet_len = ssh_read_uint32(len_buf);
        if(packet_len > SSH_MAX_PACKET_SIZE || packet_len < 5) {
            sshd_set_error("invalid packet length: %u", packet_len);
            return -1;
        }
        total_len = packet_len + 4;
        packet_buf = (uint8_t*)malloc(total_len);
        if(packet_buf == NULL)
            return -1;
        memcpy(packet_buf, len_buf, 4);
        if(ssh_read_n(s, packet_buf + 4, packet_len) < 0) {
            free(packet_buf);
            return -1;
        }
    }
    else {
        uint8_t iv[16];

        if(ssh_read_n(s, first_block, block_size) < 0)
            return -1;
        memcpy(iv, s->iv_c2s, sizeof(iv));
        if(ssh_aes256_ctr_crypt(first_block, block_size, s->enc_key_c2s, iv) < 0) {
            sshd_set_error("decrypt header failed");
            return -1;
        }
        packet_len = ssh_read_uint32(first_block);
        if(packet_len > SSH_MAX_PACKET_SIZE || packet_len < 5) {
            sshd_set_error("invalid encrypted packet length: %u", packet_len);
            return -1;
        }
        total_len = packet_len + 4;
        packet_buf = (uint8_t*)malloc(total_len);
        if(packet_buf == NULL)
            return -1;
        memcpy(packet_buf, first_block, block_size);
        if(ssh_read_n(s, packet_buf + block_size, total_len - block_size) < 0) {
            free(packet_buf);
            return -1;
        }
        if(ssh_read_n(s, mac, sizeof(mac)) < 0) {
            free(packet_buf);
            return -1;
        }
        if(ssh_aes256_ctr_crypt(packet_buf + block_size, total_len - block_size, s->enc_key_c2s, iv) < 0) {
            free(packet_buf);
            sshd_set_error("decrypt body failed");
            return -1;
        }
        memcpy(s->iv_c2s, iv, sizeof(iv));
        ssh_write_uint32(mac_input, seq);
        memcpy(mac_input + 4, packet_buf, total_len);
        hmac_sha256(s->mac_key_c2s, sizeof(s->mac_key_c2s), mac_input, 4 + total_len, calc_mac);
        if(memcmp(mac, calc_mac, sizeof(mac)) != 0) {
            free(packet_buf);
            sshd_set_error("packet mac mismatch");
            return -1;
        }
    }

    padding_len = packet_buf[4];
    packet->type = packet_buf[5];
    packet->payload_len = packet_len - padding_len - 2;
    if(packet->payload_len > sizeof(packet->payload)) {
        free(packet_buf);
        sshd_set_error("invalid payload length");
        return -1;
    }
    memcpy(packet->payload, packet_buf + 6, packet->payload_len);
    free(packet_buf);
    s->seq_c2s++;
    return 0;
}

static int read_ssh_string(const uint8_t* buf, size_t buf_len, size_t* off,
        const uint8_t** out, uint32_t* out_len) {
    uint32_t len;

    if(*off + 4 > buf_len)
        return -1;
    len = ssh_read_uint32(buf + *off);
    *off += 4;
    if(*off + len > buf_len)
        return -1;
    *out = buf + *off;
    *out_len = len;
    *off += len;
    return 0;
}

static int write_ssh_string(uint8_t* buf, size_t cap, size_t* off,
        const uint8_t* data, uint32_t len) {
    if(*off + 4 + len > cap)
        return -1;
    ssh_write_uint32(buf + *off, len);
    *off += 4;
    if(len > 0) {
        memcpy(buf + *off, data, len);
        *off += len;
    }
    return 0;
}

static int choose_client_algo(const uint8_t* list, uint32_t list_len, const char* desired) {
    const char* p = (const char*)list;
    size_t desired_len = strlen(desired);
    size_t i = 0;
    size_t start = 0;

    while(i <= list_len) {
        if(i == list_len || p[i] == ',') {
            size_t token_len = i - start;
            if(token_len == desired_len && memcmp(p + start, desired, desired_len) == 0)
                return 0;
            start = i + 1;
        }
        i++;
    }
    return -1;
}

static int choose_first_supported_algo(const uint8_t* list, uint32_t list_len,
        const char* const* desired_list, size_t desired_count, char* chosen, size_t chosen_cap) {
    const char* p = (const char*)list;
    size_t i = 0;
    size_t start = 0;

    if(chosen == NULL || chosen_cap == 0)
        return -1;
    chosen[0] = 0;

    /* RFC 4253: pick the first algorithm from the client's list that we support. */
    while(i <= list_len) {
        if(i == list_len || p[i] == ',') {
            size_t token_len = i - start;

            for(size_t j = 0; j < desired_count; ++j) {
                size_t desired_len = strlen(desired_list[j]);

                if(token_len == desired_len &&
                        memcmp(p + start, desired_list[j], token_len) == 0) {
                    snprintf(chosen, chosen_cap, "%s", desired_list[j]);
                    return 0;
                }
            }
            start = i + 1;
        }
        i++;
    }
    return -1;
}

static int parse_client_kexinit(sshd_session_t* s,
        const uint8_t* payload, size_t payload_len) {
    static const char* kex_pref[] = {
        SSH_KEX_ALG_MLKEM768_X25519_SHA256,
        SSH_KEX_ALG_DH_GROUP14_SHA256
    };
    static const char* hostkey_pref[] = {
        "rsa-sha2-256",
        "ssh-rsa"
    };
    const uint8_t* kex = NULL;
    const uint8_t* hostkey = NULL;
    const uint8_t* enc_c2s = NULL;
    const uint8_t* enc_s2c = NULL;
    const uint8_t* mac_c2s = NULL;
    const uint8_t* mac_s2c = NULL;
    const uint8_t* comp_c2s = NULL;
    const uint8_t* comp_s2c = NULL;
    uint32_t kex_len = 0, hostkey_len = 0;
    uint32_t enc_c2s_len = 0, enc_s2c_len = 0;
    uint32_t mac_c2s_len = 0, mac_s2c_len = 0;
    uint32_t comp_c2s_len = 0, comp_s2c_len = 0;
    size_t off = 16;

    if(payload_len < 16)
        return -1;

    if(read_ssh_string(payload, payload_len, &off, &kex, &kex_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &hostkey, &hostkey_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &enc_c2s, &enc_c2s_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &enc_s2c, &enc_s2c_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &mac_c2s, &mac_c2s_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &mac_s2c, &mac_s2c_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &comp_c2s, &comp_c2s_len) < 0 ||
            read_ssh_string(payload, payload_len, &off, &comp_s2c, &comp_s2c_len) < 0) {
        return -1;
    }

    if(choose_first_supported_algo(kex, kex_len,
                kex_pref, sizeof(kex_pref) / sizeof(kex_pref[0]),
                s->negotiated_kex_alg, sizeof(s->negotiated_kex_alg)) < 0) {
        sshd_set_error("unsupported client kex algorithms");
        return -1;
    }
    if(choose_first_supported_algo(hostkey, hostkey_len,
                hostkey_pref, sizeof(hostkey_pref) / sizeof(hostkey_pref[0]),
                s->negotiated_hostkey_alg, sizeof(s->negotiated_hostkey_alg)) < 0) {
        sshd_set_error("unsupported client hostkey algorithms");
        return -1;
    }
    if(choose_client_algo(enc_c2s, enc_c2s_len, "aes256-ctr") < 0 ||
            choose_client_algo(enc_s2c, enc_s2c_len, "aes256-ctr") < 0) {
        sshd_set_error("unsupported client cipher algorithms");
        return -1;
    }
    if(choose_client_algo(mac_c2s, mac_c2s_len, "hmac-sha2-256") < 0 ||
            choose_client_algo(mac_s2c, mac_s2c_len, "hmac-sha2-256") < 0) {
        sshd_set_error("unsupported client mac algorithms");
        return -1;
    }
    if(choose_client_algo(comp_c2s, comp_c2s_len, "none") < 0 ||
            choose_client_algo(comp_s2c, comp_s2c_len, "none") < 0) {
        sshd_set_error("unsupported client compression algorithms");
        return -1;
    }
    /* #region debug-point kex-negotiation */
    sshd_dbg("parse_client_kexinit: kex_alg=%s hostkey_alg=%s",
            s->negotiated_kex_alg, s->negotiated_hostkey_alg);
    /* #endregion debug-point kex-negotiation */
    return 0;
}

static int send_disconnect(sshd_session_t* s, uint32_t reason, const char* desc) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_DISCONNECT;
    ssh_write_uint32(packet.payload + off, reason);
    off += 4;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)desc, (uint32_t)strlen(desc)) < 0)
        return -1;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off, (const uint8_t*)"", 0) < 0)
        return -1;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_banner(sshd_session_t* s) {
    char banner[256];
    int len = snprintf(banner, sizeof(banner), "%s\r\n", s->server_version);
    return (len > 0 && ssh_write_n(s, banner, (size_t)len) == 0) ? 0 : -1;
}

static int receive_banner(sshd_session_t* s) {
    char c;
    size_t i = 0;

    while(i < sizeof(s->client_version) - 1) {
        ssize_t n = read(s->socket, &c, 1);
        if(n < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                proc_usleep(1000);
                continue;
            }
            return -1;
        }
        if(n == 0)
            return -1;
        if(c == '\r')
            continue;
        if(c == '\n')
            break;
        s->client_version[i++] = c;
    }
    s->client_version[i] = 0;
    return strncmp(s->client_version, "SSH-", 4) == 0 ? 0 : -1;
}

static int send_kexinit(sshd_session_t* s) {
    ssh_packet_t packet;
    size_t off = 0;
    uint8_t cookie[16];

    /* #region debug-point kex-flow */
    sshd_dbg("send_kexinit: begin");
    /* #endregion debug-point kex-flow */
    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_KEXINIT;
    if(RAND_bytes(cookie, sizeof(cookie)) != 1) {
        /* #region debug-point kex-flow */
        sshd_dbg("send_kexinit: RAND_bytes failed");
        /* #endregion debug-point kex-flow */
        return -1;
    }
    memcpy(packet.payload + off, cookie, sizeof(cookie));
    off += sizeof(cookie);

    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)SSH_KEX_ALG_MLKEM768_X25519_SHA256 ","
                SSH_KEX_ALG_DH_GROUP14_SHA256,
                sizeof(SSH_KEX_ALG_MLKEM768_X25519_SHA256 ","
                    SSH_KEX_ALG_DH_GROUP14_SHA256) - 1) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"rsa-sha2-256,ssh-rsa", 20) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"aes256-ctr", 10) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"aes256-ctr", 10) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"hmac-sha2-256", 14) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"hmac-sha2-256", 14) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"none", 4) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"none", 4) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off, (const uint8_t*)"", 0) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off, (const uint8_t*)"", 0) < 0) {
        return -1;
    }

    if(off + 5 > sizeof(packet.payload))
        return -1;
    packet.payload[off++] = 0;
    ssh_write_uint32(packet.payload + off, 0);
    off += 4;
    packet.payload_len = off;

    s->server_kexinit[0] = packet.type;
    memcpy(s->server_kexinit + 1, packet.payload, packet.payload_len);
    s->server_kexinit_len = packet.payload_len + 1;
    if(ssh_packet_send(s, &packet) < 0) {
        /* #region debug-point kex-flow */
        sshd_dbg("send_kexinit: ssh_packet_send failed");
        /* #endregion debug-point kex-flow */
        return -1;
    }
    /* #region debug-point kex-flow */
    sshd_dbg("send_kexinit: done, payload_len=%u", (unsigned int)packet.payload_len);
    /* #endregion debug-point kex-flow */
    return 0;
}

static int receive_kexinit(sshd_session_t* s) {
    ssh_packet_t packet;

    if(ssh_packet_receive(s, &packet) < 0)
        return -1;
    if(packet.type != SSH_MSG_KEXINIT) {
        sshd_set_error("expected KEXINIT, got %u", packet.type);
        return -1;
    }
    s->client_kexinit[0] = packet.type;
    memcpy(s->client_kexinit + 1, packet.payload, packet.payload_len);
    s->client_kexinit_len = packet.payload_len + 1;
    return parse_client_kexinit(s, packet.payload, packet.payload_len);
}

static int build_signature_blob(const uint8_t* exchange_hash, size_t exchange_hash_len,
        const char* hostkey_alg, uint8_t* out, size_t out_cap, size_t* out_len) {
    uint8_t digest[64];
    uint8_t sig[256];
    unsigned int sig_len = 0;
    size_t off = 0;
    const uint8_t* sig_name = (const uint8_t*)"ssh-rsa";
    uint32_t sig_name_len = 7;
    unsigned int digest_len = 0;
    int sign_nid = NID_sha1;

    if(g_host_rsa == NULL)
        return -1;
    if(hostkey_alg != NULL && strcmp(hostkey_alg, "rsa-sha2-256") == 0) {
        sig_name = (const uint8_t*)"rsa-sha2-256";
        sig_name_len = 12;
        sign_nid = NID_sha256;
        SHA256(exchange_hash, exchange_hash_len, digest);
        digest_len = SHA256_DIGEST_LENGTH;
    }
    else {
        SHA1(exchange_hash, exchange_hash_len, digest);
        digest_len = SHA_DIGEST_LENGTH;
    }
    if(RSA_sign(sign_nid, digest, digest_len, sig, &sig_len, g_host_rsa) != 1)
        return -1;

    if(write_ssh_string(out, out_cap, &off, sig_name, sig_name_len) < 0 ||
            write_ssh_string(out, out_cap, &off, sig, sig_len) < 0)
        return -1;

    *out_len = off;
    return 0;
}

static int do_key_exchange_group14_sha256(sshd_session_t* s) {
    ssh_packet_t packet;
    DH* dh = NULL;
    BIGNUM *p_bn = NULL, *g_bn = NULL, *e_bn = NULL;
    const BIGNUM* f_pub = NULL;
    uint8_t f_raw[256];
    uint8_t f_mpint[264];
    uint8_t k_raw[256];
    uint8_t k_mpint[264];
    uint8_t exchange_hash[32];
    uint8_t signature_blob[384];
    size_t f_mpint_len = 0;
    size_t k_mpint_len = 0;
    size_t signature_blob_len = 0;
    int f_raw_len;
    int k_raw_len;
    size_t off = 0;
    const uint8_t* e_ptr;
    uint32_t e_len;

    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: waiting KEXDH_INIT");
    /* #endregion debug-point kex-flow */
    if(ssh_packet_receive(s, &packet) < 0) {
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: ssh_packet_receive failed");
        /* #endregion debug-point kex-flow */
        return -1;
    }
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: received packet type=%u", packet.type);
    /* #endregion debug-point kex-flow */
    if(packet.type != SSH_MSG_KEXDH_INIT) {
        sshd_set_error("expected KEXDH_INIT, got %u", packet.type);
        return -1;
    }

    if(read_ssh_string(packet.payload, packet.payload_len, &off, &e_ptr, &e_len) < 0)
        return -1;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: client dh pubkey len=%u", e_len);
    /* #endregion debug-point kex-flow */
    if(e_len == 0 || (e_len > 1 && e_ptr[0] == 0 && e_ptr[1] == 0)) {
        sshd_set_error("invalid client dh pubkey mpint");
        goto fail;
    }
    if(e_len > 1 && e_ptr[0] == 0) {
        e_ptr++;
        e_len--;
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: normalized client dh pubkey len=%u", e_len);
        /* #endregion debug-point kex-flow */
    }

    dh = DH_new();
    p_bn = BN_new();
    g_bn = BN_new();
    if(dh == NULL || p_bn == NULL || g_bn == NULL)
        goto fail;
    if(!BN_hex2bn(&p_bn, dh_group14_p_hex))
        goto fail;
    if(!BN_set_word(g_bn, 2))
        goto fail;
    if(!DH_set0_pqg(dh, p_bn, NULL, g_bn))
        goto fail;
    p_bn = NULL;
    g_bn = NULL;
    if(!DH_generate_key(dh))
        goto fail;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: DH_generate_key ok");
    /* #endregion debug-point kex-flow */

    e_bn = BN_bin2bn(e_ptr, e_len, NULL);
    if(e_bn == NULL)
        goto fail;

    DH_get0_key(dh, &f_pub, NULL);
    if(f_pub == NULL)
        goto fail;
    f_raw_len = BN_bn2bin(f_pub, f_raw);
    if(f_raw_len <= 0)
        goto fail;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: server dh pubkey raw_len=%d", f_raw_len);
    /* #endregion debug-point kex-flow */
    if(ssh_encode_mpint(f_raw, (size_t)f_raw_len, f_mpint, sizeof(f_mpint), &f_mpint_len) < 0)
        goto fail;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: server dh pubkey mpint_len=%u", (unsigned int)f_mpint_len);
    /* #endregion debug-point kex-flow */

    k_raw_len = DH_compute_key(k_raw, e_bn, dh);
    if(k_raw_len <= 0)
        goto fail;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: DH_compute_key ok, secret_len=%d", k_raw_len);
    /* #endregion debug-point kex-flow */
    if(ssh_encode_mpint(k_raw, (size_t)k_raw_len, k_mpint, sizeof(k_mpint), &k_mpint_len) < 0)
        goto fail;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: shared secret mpint_len=%u", (unsigned int)k_mpint_len);
    /* #endregion debug-point kex-flow */

    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: exchange hash begin");
        /* #endregion debug-point kex-flow */

        ssh_write_uint32(exchange_hash, (uint32_t)strlen(s->client_version));
        SHA256_Update(&ctx, exchange_hash, 4);
        SHA256_Update(&ctx, s->client_version, strlen(s->client_version));

        ssh_write_uint32(exchange_hash, (uint32_t)strlen(s->server_version));
        SHA256_Update(&ctx, exchange_hash, 4);
        SHA256_Update(&ctx, s->server_version, strlen(s->server_version));

        ssh_write_uint32(exchange_hash, (uint32_t)s->client_kexinit_len);
        SHA256_Update(&ctx, exchange_hash, 4);
        SHA256_Update(&ctx, s->client_kexinit, s->client_kexinit_len);

        ssh_write_uint32(exchange_hash, (uint32_t)s->server_kexinit_len);
        SHA256_Update(&ctx, exchange_hash, 4);
        SHA256_Update(&ctx, s->server_kexinit, s->server_kexinit_len);

        ssh_write_uint32(exchange_hash, (uint32_t)g_hostkey_blob_len);
        SHA256_Update(&ctx, exchange_hash, 4);
        SHA256_Update(&ctx, g_hostkey_blob, g_hostkey_blob_len);

        SHA256_Update(&ctx, packet.payload, packet.payload_len);
        SHA256_Update(&ctx, f_mpint, f_mpint_len);
        SHA256_Update(&ctx, k_mpint, k_mpint_len);
        SHA256_Final(exchange_hash, &ctx);
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: exchange hash done");
        /* #endregion debug-point kex-flow */
    }

    if(s->session_id_len == 0) {
        memcpy(s->session_id, exchange_hash, sizeof(exchange_hash));
        s->session_id_len = sizeof(exchange_hash);
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: session id initialized, len=%u", (unsigned int)s->session_id_len);
        /* #endregion debug-point kex-flow */
    }
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: derive_keys begin");
    /* #endregion debug-point kex-flow */
    derive_keys(s, k_mpint, k_mpint_len, exchange_hash, sizeof(exchange_hash));
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: derive_keys done");
    /* #endregion debug-point kex-flow */

    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: build_signature_blob begin, alg=%s", s->negotiated_hostkey_alg[0] ? s->negotiated_hostkey_alg : "<none>");
    /* #endregion debug-point kex-flow */
    if(build_signature_blob(exchange_hash, sizeof(exchange_hash), s->negotiated_hostkey_alg,
                signature_blob, sizeof(signature_blob), &signature_blob_len) < 0) {
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: build_signature_blob failed");
        /* #endregion debug-point kex-flow */
        goto fail;
    }
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: signature blob len=%u", (unsigned int)signature_blob_len);
    /* #endregion debug-point kex-flow */

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_KEXDH_REPLY;
    off = 0;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                g_hostkey_blob, (uint32_t)g_hostkey_blob_len) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                f_mpint + 4, (uint32_t)(f_mpint_len - 4)) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                signature_blob, (uint32_t)signature_blob_len) < 0) {
        goto fail;
    }
    packet.payload_len = off;
    if(ssh_packet_send(s, &packet) < 0) {
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: send KEXDH_REPLY failed");
        /* #endregion debug-point kex-flow */
        goto fail;
    }
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: sent KEXDH_REPLY");
    /* #endregion debug-point kex-flow */

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_NEWKEYS;
    if(ssh_packet_send(s, &packet) < 0) {
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: send NEWKEYS failed");
        /* #endregion debug-point kex-flow */
        goto fail;
    }
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: sent NEWKEYS");
    /* #endregion debug-point kex-flow */
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: waiting client NEWKEYS");
    /* #endregion debug-point kex-flow */
    if(ssh_packet_receive(s, &packet) < 0) {
        /* #region debug-point kex-flow */
        sshd_dbg("do_key_exchange: receive client NEWKEYS failed");
        /* #endregion debug-point kex-flow */
        goto fail;
    }
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: received post-kex packet type=%u", packet.type);
    /* #endregion debug-point kex-flow */
    if(packet.type != SSH_MSG_NEWKEYS) {
        sshd_set_error("expected NEWKEYS, got %u", packet.type);
        goto fail;
    }

    s->encryption_enabled = 1;
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: complete");
    /* #endregion debug-point kex-flow */
    BN_free(e_bn);
    DH_free(dh);
    return 0;

fail:
    /* #region debug-point kex-flow */
    sshd_dbg("do_key_exchange: fail, error=%s", g_error[0] ? g_error : "<none>");
    /* #endregion debug-point kex-flow */
    BN_free(e_bn);
    BN_free(p_bn);
    BN_free(g_bn);
    DH_free(dh);
    return -1;
}

static int do_key_exchange_mlkem768x25519_sha256(sshd_session_t* s) {
    ssh_packet_t packet;
    WC_RNG rng;
    curve25519_key server_key;
    MlKemKey client_mlkem;
    const uint8_t* c_init = NULL;
    uint32_t c_init_len = 0;
    const uint8_t* c_mlkem_pub;
    const uint8_t* c_x25519_pub;
    uint8_t server_pub[SSH_X25519_KEY_LEN];
    word32 server_pub_len = sizeof(server_pub);
    uint8_t s_reply[SSH_MLKEM768_CT_LEN + SSH_X25519_KEY_LEN];
    uint8_t pq_shared[WC_ML_KEM_SS_SZ];
    uint8_t cl_shared[SSH_X25519_KEY_LEN];
    uint8_t hybrid_input[WC_ML_KEM_SS_SZ + SSH_X25519_KEY_LEN];
    uint8_t k_raw[SSH_HYBRID_SECRET_LEN];
    uint8_t k_string[4 + SSH_HYBRID_SECRET_LEN];
    uint8_t exchange_hash[32];
    uint8_t signature_blob[384];
    size_t k_string_len = 0;
    size_t signature_blob_len = 0;
    int rng_inited = 0;
    int server_key_inited = 0;
    int client_mlkem_inited = 0;
    size_t off = 0;

    memset(&rng, 0, sizeof(rng));
    memset(&server_key, 0, sizeof(server_key));
    memset(&client_mlkem, 0, sizeof(client_mlkem));
    memset(server_pub, 0, sizeof(server_pub));
    memset(s_reply, 0, sizeof(s_reply));
    memset(pq_shared, 0, sizeof(pq_shared));
    memset(cl_shared, 0, sizeof(cl_shared));
    memset(hybrid_input, 0, sizeof(hybrid_input));
    memset(k_raw, 0, sizeof(k_raw));
    memset(k_string, 0, sizeof(k_string));

    if(ssh_packet_receive(s, &packet) < 0)
        return -1;
    if(packet.type != SSH_MSG_KEXDH_INIT) {
        sshd_set_error("expected KEX_HYBRID_INIT, got %u", packet.type);
        return -1;
    }
    if(read_ssh_string(packet.payload, packet.payload_len, &off, &c_init, &c_init_len) < 0)
        return -1;
    if(c_init_len != SSH_MLKEM768_PUB_KEY_LEN + SSH_X25519_KEY_LEN) {
        sshd_set_error("invalid hybrid client key length: %u", c_init_len);
        goto fail;
    }

    c_mlkem_pub = c_init;
    c_x25519_pub = c_init + SSH_MLKEM768_PUB_KEY_LEN;

    if(wc_InitRng(&rng) != 0) {
        sshd_set_error("wc_InitRng failed");
        goto fail;
    }
    rng_inited = 1;

    wc_curve25519_init(&server_key);
    server_key_inited = 1;
    if(wc_curve25519_make_key(&rng, SSH_X25519_KEY_LEN, &server_key) != 0) {
        sshd_set_error("wc_curve25519_make_key failed");
        goto fail;
    }
    if(wc_curve25519_export_public_ex(&server_key, server_pub, &server_pub_len,
                EC25519_LITTLE_ENDIAN) != 0 ||
            server_pub_len != SSH_X25519_KEY_LEN) {
        sshd_set_error("wc_curve25519_export_public_ex failed");
        goto fail;
    }

    if(wc_MlKemKey_Init(&client_mlkem, WC_ML_KEM_768, NULL, INVALID_DEVID) != 0) {
        sshd_set_error("wc_MlKemKey_Init failed");
        goto fail;
    }
    client_mlkem_inited = 1;
    if(wc_MlKemKey_DecodePublicKey(&client_mlkem, c_mlkem_pub,
                SSH_MLKEM768_PUB_KEY_LEN) != 0) {
        sshd_set_error("wc_MlKemKey_DecodePublicKey failed");
        goto fail;
    }
    if(wc_MlKemKey_Encapsulate(&client_mlkem, s_reply, pq_shared, &rng) != 0) {
        sshd_set_error("wc_MlKemKey_Encapsulate failed");
        goto fail;
    }
    memcpy(s_reply + SSH_MLKEM768_CT_LEN, server_pub, SSH_X25519_KEY_LEN);

    if(ssh_curve25519_shared_secret_be(&server_key, c_x25519_pub, cl_shared) != 0) {
        sshd_set_error("curve25519 shared secret failed");
        goto fail;
    }

    memcpy(hybrid_input, pq_shared, WC_ML_KEM_SS_SZ);
    memcpy(hybrid_input + WC_ML_KEM_SS_SZ, cl_shared, SSH_X25519_KEY_LEN);
    sha256_hash(hybrid_input, sizeof(hybrid_input), k_raw);
    if(ssh_encode_string(k_raw, sizeof(k_raw), k_string, sizeof(k_string), &k_string_len) < 0) {
        sshd_set_error("encode hybrid shared secret failed");
        goto fail;
    }

    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        sha256_update_ssh_string(&ctx, (const uint8_t*)s->client_version, strlen(s->client_version));
        sha256_update_ssh_string(&ctx, (const uint8_t*)s->server_version, strlen(s->server_version));
        sha256_update_ssh_string(&ctx, s->client_kexinit, s->client_kexinit_len);
        sha256_update_ssh_string(&ctx, s->server_kexinit, s->server_kexinit_len);
        sha256_update_ssh_string(&ctx, g_hostkey_blob, g_hostkey_blob_len);
        sha256_update_ssh_string(&ctx, c_init, c_init_len);
        sha256_update_ssh_string(&ctx, s_reply, sizeof(s_reply));
        sha256_update_ssh_string(&ctx, k_raw, sizeof(k_raw));
        SHA256_Final(exchange_hash, &ctx);
    }

    if(s->session_id_len == 0) {
        memcpy(s->session_id, exchange_hash, sizeof(exchange_hash));
        s->session_id_len = sizeof(exchange_hash);
    }
    derive_keys(s, k_string, k_string_len, exchange_hash, sizeof(exchange_hash));

    if(build_signature_blob(exchange_hash, sizeof(exchange_hash), s->negotiated_hostkey_alg,
                signature_blob, sizeof(signature_blob), &signature_blob_len) < 0) {
        sshd_set_error("build hybrid signature failed");
        goto fail;
    }

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_KEXDH_REPLY;
    off = 0;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                g_hostkey_blob, (uint32_t)g_hostkey_blob_len) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                s_reply, (uint32_t)sizeof(s_reply)) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                signature_blob, (uint32_t)signature_blob_len) < 0) {
        sshd_set_error("build hybrid KEX reply failed");
        goto fail;
    }
    packet.payload_len = off;
    if(ssh_packet_send(s, &packet) < 0)
        goto fail;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_NEWKEYS;
    if(ssh_packet_send(s, &packet) < 0)
        goto fail;
    if(ssh_packet_receive(s, &packet) < 0)
        goto fail;
    if(packet.type != SSH_MSG_NEWKEYS) {
        sshd_set_error("expected NEWKEYS, got %u", packet.type);
        goto fail;
    }

    s->encryption_enabled = 1;
    if(client_mlkem_inited)
        wc_MlKemKey_Free(&client_mlkem);
    if(server_key_inited)
        wc_curve25519_free(&server_key);
    if(rng_inited)
        wc_FreeRng(&rng);
    memset(pq_shared, 0, sizeof(pq_shared));
    memset(cl_shared, 0, sizeof(cl_shared));
    memset(hybrid_input, 0, sizeof(hybrid_input));
    memset(k_raw, 0, sizeof(k_raw));
    memset(k_string, 0, sizeof(k_string));
    return 0;

fail:
    if(client_mlkem_inited)
        wc_MlKemKey_Free(&client_mlkem);
    if(server_key_inited)
        wc_curve25519_free(&server_key);
    if(rng_inited)
        wc_FreeRng(&rng);
    memset(pq_shared, 0, sizeof(pq_shared));
    memset(cl_shared, 0, sizeof(cl_shared));
    memset(hybrid_input, 0, sizeof(hybrid_input));
    memset(k_raw, 0, sizeof(k_raw));
    memset(k_string, 0, sizeof(k_string));
    return -1;
}

static int do_key_exchange(sshd_session_t* s) {
    if(strcmp(s->negotiated_kex_alg, SSH_KEX_ALG_MLKEM768_X25519_SHA256) == 0)
        return do_key_exchange_mlkem768x25519_sha256(s);
    if(strcmp(s->negotiated_kex_alg, SSH_KEX_ALG_DH_GROUP14_SHA256) == 0)
        return do_key_exchange_group14_sha256(s);

    sshd_set_error("unsupported negotiated kex algorithm: %s", s->negotiated_kex_alg);
    return -1;
}

static int send_service_accept(sshd_session_t* s, const char* name) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_SERVICE_ACCEPT;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)name, (uint32_t)strlen(name)) < 0)
        return -1;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_userauth_failure(sshd_session_t* s, const char* methods) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_USERAUTH_FAILURE;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)methods, (uint32_t)strlen(methods)) < 0)
        return -1;
    packet.payload[off++] = 0;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int handle_service_and_auth(sshd_session_t* s) {
    ssh_packet_t packet;
    const uint8_t *user, *service, *method, *password;
    uint32_t user_len, service_len, method_len, password_len;
    size_t off;

    /* #region debug-point auth-flow */
    sshd_dbg("handle_service_and_auth: waiting service request");
    /* #endregion debug-point auth-flow */
    if(ssh_packet_receive(s, &packet) < 0)
        return -1;
    /* #region debug-point auth-flow */
    sshd_dbg("handle_service_and_auth: received packet type=%u", packet.type);
    /* #endregion debug-point auth-flow */
    if(packet.type != SSH_MSG_SERVICE_REQUEST)
        return -1;

    off = 0;
    if(read_ssh_string(packet.payload, packet.payload_len, &off, &service, &service_len) < 0)
        return -1;
    if(service_len != strlen("ssh-userauth") ||
            memcmp(service, "ssh-userauth", service_len) != 0)
        return -1;
    /* #region debug-point auth-flow */
    sshd_dbg("handle_service_and_auth: service=%.*s", (int)service_len, service);
    /* #endregion debug-point auth-flow */
    if(send_service_accept(s, "ssh-userauth") < 0)
        return -1;
    /* #region debug-point auth-flow */
    sshd_dbg("handle_service_and_auth: sent service accept");
    /* #endregion debug-point auth-flow */

    while(!s->authenticated) {
        int auth_res = -1;
        char username[SESSION_USER_MAX];
        char passwd[SESSION_PSWD_MAX];

        if(ssh_packet_receive(s, &packet) < 0)
            return -1;
        /* #region debug-point auth-flow */
        sshd_dbg("handle_service_and_auth: auth packet type=%u", packet.type);
        /* #endregion debug-point auth-flow */
        if(packet.type != SSH_MSG_USERAUTH_REQUEST)
            return -1;

        off = 0;
        if(read_ssh_string(packet.payload, packet.payload_len, &off, &user, &user_len) < 0 ||
                read_ssh_string(packet.payload, packet.payload_len, &off, &service, &service_len) < 0 ||
                read_ssh_string(packet.payload, packet.payload_len, &off, &method, &method_len) < 0) {
            return -1;
        }

        if(service_len != strlen("ssh-connection") ||
                memcmp(service, "ssh-connection", service_len) != 0) {
            if(send_userauth_failure(s, "password") < 0)
                return -1;
            continue;
        }

        if(user_len == 0 || user_len >= sizeof(username)) {
            if(send_userauth_failure(s, "password") < 0)
                return -1;
            continue;
        }
        memcpy(username, user, user_len);
        username[user_len] = 0;
        /* #region debug-point auth-flow */
        sshd_dbg("handle_service_and_auth: user=%s service=%.*s method=%.*s",
                username, (int)service_len, service, (int)method_len, method);
        /* #endregion debug-point auth-flow */

        if(method_len == strlen("none") && memcmp(method, "none", method_len) == 0) {
            if(send_userauth_failure(s, "password") < 0)
                return -1;
            continue;
        }

        if(!(method_len == strlen("password") && memcmp(method, "password", method_len) == 0)) {
            if(send_userauth_failure(s, "password") < 0)
                return -1;
            continue;
        }

        if(off >= packet.payload_len)
            return -1;
        off++;
        if(read_ssh_string(packet.payload, packet.payload_len, &off, &password, &password_len) < 0)
            return -1;
        if(password_len >= sizeof(passwd))
            return -1;
        memcpy(passwd, password, password_len);
        passwd[password_len] = 0;

        auth_res = session_check(username, passwd, &s->user_info);
        /* #region debug-point auth-flow */
        sshd_dbg("handle_service_and_auth: session_check ret=%d user=%s uid=%d gid=%d home=%s cmd=%s",
                auth_res, username, s->user_info.uid, s->user_info.gid,
                s->user_info.home, s->user_info.cmd);
        /* #endregion debug-point auth-flow */
        if(auth_res == 0 && s->user_info.cmd[0] != 0) {
            memset(&packet, 0, sizeof(packet));
            packet.type = SSH_MSG_USERAUTH_SUCCESS;
            packet.payload_len = 0;
            if(ssh_packet_send(s, &packet) < 0)
                return -1;
            s->authenticated = 1;
            /* #region debug-point auth-flow */
            sshd_dbg("handle_service_and_auth: auth success");
            /* #endregion debug-point auth-flow */
            return 0;
        }

        /* #region debug-point auth-flow */
        sshd_dbg("handle_service_and_auth: auth failure");
        /* #endregion debug-point auth-flow */
        if(send_userauth_failure(s, "password") < 0)
            return -1;
    }
    return 0;
}

static int send_channel_confirm(sshd_session_t* s) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_OPEN_CONFIRMATION;
    ssh_write_uint32(packet.payload + off, s->remote_channel);
    off += 4;
    ssh_write_uint32(packet.payload + off, s->local_channel);
    off += 4;
    ssh_write_uint32(packet.payload + off, s->local_window);
    off += 4;
    ssh_write_uint32(packet.payload + off, s->local_packet_max);
    off += 4;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_channel_open_failure(sshd_session_t* s, uint32_t reason, const char* desc) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_OPEN_FAILURE;
    ssh_write_uint32(packet.payload + off, s->remote_channel);
    off += 4;
    ssh_write_uint32(packet.payload + off, reason);
    off += 4;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)desc, (uint32_t)strlen(desc)) < 0 ||
            write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"", 0) < 0) {
        return -1;
    }
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_channel_status(sshd_session_t* s, uint8_t type) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = type;
    ssh_write_uint32(packet.payload + off, s->remote_channel);
    off += 4;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_request_failure(sshd_session_t* s) {
    ssh_packet_t packet;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_REQUEST_FAILURE;
    packet.payload_len = 0;
    return ssh_packet_send(s, &packet);
}

static size_t ssh_console_copy_crlf(uint8_t* dst, size_t dst_cap,
        const uint8_t* src, size_t src_len) {
    size_t i;
    size_t out = 0;

    if(dst == NULL || src == NULL || dst_cap == 0)
        return 0;

    for(i = 0; i < src_len && out < dst_cap; i++) {
        if(src[i] == '\n' && (i == 0 || src[i - 1] != '\r')) {
            if(out + 2 > dst_cap)
                break;
            dst[out++] = '\r';
        }
        dst[out++] = src[i];
    }
    return out;
}

static int send_channel_data_packet(sshd_session_t* s, uint32_t extended_type,
        const uint8_t* data, size_t len) {
    ssh_packet_t packet;
    uint8_t text_buf[2048];
    const uint8_t* payload = data;
    size_t payload_len = len;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = (extended_type == 0) ? SSH_MSG_CHANNEL_DATA : SSH_MSG_CHANNEL_EXTENDED_DATA;
    ssh_write_uint32(packet.payload + off, s->remote_channel);
    off += 4;
    if(extended_type != 0) {
        ssh_write_uint32(packet.payload + off, extended_type);
        off += 4;
    }
    if(s->pty_enabled) {
        payload_len = ssh_console_copy_crlf(text_buf, sizeof(text_buf), data, len);
        payload = text_buf;
    }
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                payload, (uint32_t)payload_len) < 0)
        return -1;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_channel_request_exit_status(sshd_session_t* s, uint32_t status) {
    ssh_packet_t packet;
    size_t off = 0;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_REQUEST;
    ssh_write_uint32(packet.payload + off, s->remote_channel);
    off += 4;
    if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                (const uint8_t*)"exit-status", 11) < 0)
        return -1;
    packet.payload[off++] = 0;
    ssh_write_uint32(packet.payload + off, status);
    off += 4;
    packet.payload_len = off;
    return ssh_packet_send(s, &packet);
}

static int send_channel_eof_and_close(sshd_session_t* s) {
    if(s->sent_close)
        return 0;
    if(send_channel_status(s, SSH_MSG_CHANNEL_EOF) < 0)
        return -1;
    if(send_channel_status(s, SSH_MSG_CHANNEL_CLOSE) < 0)
        return -1;
    s->sent_close = 1;
    return 0;
}

static void* stream_to_channel_thread(void* arg) {
    stream_thread_arg_t* ctx = (stream_thread_arg_t*)arg;
    uint8_t buf[1024];

    while(!ctx->start_ready)
        proc_usleep(1000);

    sshd_dbg("stream_to_channel_thread: start fd=%d ext=%u",
            ctx->fd, ctx->extended_type);

    while(!ctx->session->closing) {
        ssize_t n = read(ctx->fd, buf, sizeof(buf));
        if(n < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                proc_usleep(1000);
                continue;
            }
            sshd_dbg("stream_to_channel_thread: fd=%d read failed errno=%d ext=%u",
                    ctx->fd, errno, ctx->extended_type);
            break;
        }
        if(n == 0)
            break;
        sshd_dbg("stream_to_channel_thread: fd=%d read=%d ext=%u",
                ctx->fd, (int)n, ctx->extended_type);
        if(send_channel_data_packet(ctx->session, ctx->extended_type, buf, (size_t)n) < 0) {
            sshd_dbg("stream_to_channel_thread: send failed ext=%u len=%d",
                    ctx->extended_type, (int)n);
            break;
        }
    }

    close(ctx->fd);
    sshd_release_vfs_fds_for_pid((int)pthread_self());
    free(ctx);
    return NULL;
}

static void* wait_child_thread(void* arg) {
    sshd_session_t* s = (sshd_session_t*)arg;
    sshd_dbg("wait_child_thread: start pid=%d", s->child_pid);
    int status = waitpid(s->child_pid);
    uint32_t exit_status = (status < 0) ? 255u : (uint32_t)status;

    /* #region debug-point session-flow */
    sshd_dbg("wait_child_thread: pid=%d status=%d exit_status=%u", s->child_pid, status, exit_status);
    /* #endregion debug-point session-flow */
    s->closing = 1;
    if(s->child_stdin[CHILD_STDIN_WRITE] >= 0) {
        close(s->child_stdin[CHILD_STDIN_WRITE]);
        s->child_stdin[CHILD_STDIN_WRITE] = -1;
    }

    send_channel_request_exit_status(s, exit_status);
    send_channel_eof_and_close(s);
    close(s->socket);
    sshd_release_vfs_fds_for_pid((int)pthread_self());
    return NULL;
}

static void close_child_bundle(int child_stdin[2], int child_stdout[2],
        int child_stderr[2], int child_control[2]) {
    if(child_stdin[CHILD_STDIN_READ] >= 0) {
        close(child_stdin[CHILD_STDIN_READ]);
        child_stdin[CHILD_STDIN_READ] = -1;
    }
    if(child_stdin[CHILD_STDIN_WRITE] >= 0) {
        close(child_stdin[CHILD_STDIN_WRITE]);
        child_stdin[CHILD_STDIN_WRITE] = -1;
    }
    if(child_stdout[CHILD_STDOUT_READ] >= 0) {
        close(child_stdout[CHILD_STDOUT_READ]);
        child_stdout[CHILD_STDOUT_READ] = -1;
    }
    if(child_stdout[CHILD_STDOUT_WRITE] >= 0) {
        close(child_stdout[CHILD_STDOUT_WRITE]);
        child_stdout[CHILD_STDOUT_WRITE] = -1;
    }
    if(child_stderr[CHILD_STDERR_READ] >= 0) {
        close(child_stderr[CHILD_STDERR_READ]);
        child_stderr[CHILD_STDERR_READ] = -1;
    }
    if(child_stderr[CHILD_STDERR_WRITE] >= 0) {
        close(child_stderr[CHILD_STDERR_WRITE]);
        child_stderr[CHILD_STDERR_WRITE] = -1;
    }
    if(child_control[CHILD_CTRL_READ] >= 0) {
        close(child_control[CHILD_CTRL_READ]);
        child_control[CHILD_CTRL_READ] = -1;
    }
    if(child_control[CHILD_CTRL_WRITE] >= 0) {
        close(child_control[CHILD_CTRL_WRITE]);
        child_control[CHILD_CTRL_WRITE] = -1;
    }
}

static void run_session_helper(int sock, int server_sock, int child_stdin[2],
        int child_stdout[2], int child_stderr[2], int child_control[2]) {
    sshd_child_start_msg_t msg;
    const char* cmd;

    SSHD_CHILD_MARK("helper-start");
    if(sock >= 0)
        close(sock);
    if(server_sock >= 0)
        close(server_sock);

    close(child_control[CHILD_CTRL_WRITE]);
    child_control[CHILD_CTRL_WRITE] = -1;
    close(child_stdin[CHILD_STDIN_WRITE]);
    child_stdin[CHILD_STDIN_WRITE] = -1;
    close(child_stdout[CHILD_STDOUT_READ]);
    child_stdout[CHILD_STDOUT_READ] = -1;
    close(child_stderr[CHILD_STDERR_READ]);
    child_stderr[CHILD_STDERR_READ] = -1;

    SSHD_CHILD_MARK("wait-msg");
    if(read_fd_all(child_control[CHILD_CTRL_READ], &msg, sizeof(msg)) < 0)
        exit(1);
    if(msg.magic != SSHD_CHILD_START_MAGIC)
        exit(1);
    SSHD_CHILD_MARK("msg-ok");

    if(dup2(child_stdin[CHILD_STDIN_READ], 0) < 0 ||
            dup2(child_stdout[CHILD_STDOUT_WRITE], 1) < 0 ||
            dup2(child_stderr[CHILD_STDERR_WRITE], 2) < 0 ||
            dup2(child_stdin[CHILD_STDIN_READ], VFS_BACKUP_FD0) < 0 ||
            dup2(child_stdout[CHILD_STDOUT_WRITE], VFS_BACKUP_FD1) < 0) {
        exit(2);
    }
    SSHD_CHILD_MARK("dup-ok");

    close(child_control[CHILD_CTRL_READ]);
    child_control[CHILD_CTRL_READ] = -1;
    close(child_stdin[CHILD_STDIN_READ]);
    child_stdin[CHILD_STDIN_READ] = -1;
    close(child_stdout[CHILD_STDOUT_WRITE]);
    child_stdout[CHILD_STDOUT_WRITE] = -1;
    close(child_stderr[CHILD_STDERR_WRITE]);
    child_stderr[CHILD_STDERR_WRITE] = -1;

    if(setenv("CONSOLE_ID", "ssh") != 0 ||
            setenv("HOME", msg.home) != 0 ||
            setenv("USER", msg.user) != 0) {
        exit(3);
    }
    SSHD_CHILD_MARK("env-ok");
    if(core_set_env("CONSOLE_ID", "ssh") != 0 ||
            core_set_env("HOME", msg.home) != 0 ||
            core_set_env("USER", msg.user) != 0) {
        exit(5);
    }
    SSHD_CHILD_MARK("core-env-ok");
    if(setgid(msg.gid) != 0 || setuid(msg.uid) != 0)
        exit(4);
    SSHD_CHILD_MARK("ids-ok");

    cmd = (msg.is_shell != 0) ? msg.shell_cmd : msg.exec_cmd;
    if(cmd[0] != 0) {
        SSHD_CHILD_MARK("exec-begin");
        if(exec_program_direct(cmd) < 0)
            SSHD_CHILD_MARK("exec-fail");
    }
    exit(-1);
}

static void close_child_fds(sshd_session_t* s) {
    close_child_bundle(s->child_stdin, s->child_stdout, s->child_stderr, s->child_control);
}

static int create_child_session_bundle(int server_sock, int client_sock,
        int child_stdin[2], int child_stdout[2], int child_stderr[2],
        int child_control[2], pid_t* child_pid) {
    pid_t pid;

    child_stdin[0] = child_stdin[1] = -1;
    child_stdout[0] = child_stdout[1] = -1;
    child_stderr[0] = child_stderr[1] = -1;
    child_control[0] = child_control[1] = -1;
    *child_pid = -1;

    if(pipe(child_stdin) != 0 || pipe(child_stdout) != 0 ||
            pipe(child_stderr) != 0 || pipe(child_control) != 0) {
        close_child_bundle(child_stdin, child_stdout, child_stderr, child_control);
        return -1;
    }

    pid = fork();
    if(pid < 0) {
        close_child_bundle(child_stdin, child_stdout, child_stderr, child_control);
        return -1;
    }

    if(pid == 0) {
        run_session_helper(client_sock, server_sock, child_stdin, child_stdout,
                child_stderr, child_control);
        exit(1);
    }

    *child_pid = pid;
    return 0;
}

static void session_attach_child_bundle(sshd_session_t* s, pid_t child_pid,
        const int child_stdin[2], const int child_stdout[2],
        const int child_stderr[2], const int child_control[2]) {
    memcpy(s->child_stdin, child_stdin, sizeof(s->child_stdin));
    memcpy(s->child_stdout, child_stdout, sizeof(s->child_stdout));
    memcpy(s->child_stderr, child_stderr, sizeof(s->child_stderr));
    memcpy(s->child_control, child_control, sizeof(s->child_control));
    s->child_pid = child_pid;

    if(s->child_stdin[CHILD_STDIN_READ] >= 0) {
        close(s->child_stdin[CHILD_STDIN_READ]);
        s->child_stdin[CHILD_STDIN_READ] = -1;
    }
    if(s->child_stdout[CHILD_STDOUT_WRITE] >= 0) {
        close(s->child_stdout[CHILD_STDOUT_WRITE]);
        s->child_stdout[CHILD_STDOUT_WRITE] = -1;
    }
    if(s->child_stderr[CHILD_STDERR_WRITE] >= 0) {
        close(s->child_stderr[CHILD_STDERR_WRITE]);
        s->child_stderr[CHILD_STDERR_WRITE] = -1;
    }
    if(s->child_control[CHILD_CTRL_READ] >= 0) {
        close(s->child_control[CHILD_CTRL_READ]);
        s->child_control[CHILD_CTRL_READ] = -1;
    }
}

static int spawn_child_session(sshd_session_t* s, const char* command, int is_shell) {
    pthread_t tid;
    stream_thread_arg_t* stdout_arg = NULL;
    stream_thread_arg_t* stderr_arg = NULL;
    sshd_child_start_msg_t msg;
    int rc;

    /* #region debug-point session-flow */
    sshd_dbg("spawn_child_session: activate helper is_shell=%d command=%s user_cmd=%s uid=%d gid=%d",
            is_shell, command ? command : "<null>", s->user_info.cmd,
            s->user_info.uid, s->user_info.gid);
    /* #endregion debug-point session-flow */

    if(s->child_pid <= 0 || s->child_control[CHILD_CTRL_WRITE] < 0)
        return -1;

    memset(&msg, 0, sizeof(msg));
    msg.magic = SSHD_CHILD_START_MAGIC;
    msg.is_shell = is_shell ? 1u : 0u;
    msg.uid = s->user_info.uid;
    msg.gid = s->user_info.gid;
    copy_cstr(msg.user, sizeof(msg.user), s->user_info.user);
    copy_cstr(msg.home, sizeof(msg.home), s->user_info.home);
    copy_cstr(msg.shell_cmd, sizeof(msg.shell_cmd), s->user_info.cmd);
    if(command != NULL)
        copy_cstr(msg.exec_cmd, sizeof(msg.exec_cmd), command);

    if(write_fd_all(s->child_control[CHILD_CTRL_WRITE], &msg, sizeof(msg)) < 0)
        return -1;
    close(s->child_control[CHILD_CTRL_WRITE]);
    s->child_control[CHILD_CTRL_WRITE] = -1;

    s->child_started = 1;
    sshd_dbg("spawn_child_session: fds stdin_w=%d stdout_r=%d stderr_r=%d child_pid=%d",
            s->child_stdin[CHILD_STDIN_WRITE], s->child_stdout[CHILD_STDOUT_READ],
            s->child_stderr[CHILD_STDERR_READ], s->child_pid);

    stdout_arg = (stream_thread_arg_t*)calloc(1, sizeof(stream_thread_arg_t));
    stderr_arg = (stream_thread_arg_t*)calloc(1, sizeof(stream_thread_arg_t));
    if(stdout_arg == NULL || stderr_arg == NULL)
        return -1;

    stdout_arg->session = s;
    stdout_arg->fd = s->child_stdout[CHILD_STDOUT_READ];
    stdout_arg->extended_type = 0;
    stderr_arg->session = s;
    stderr_arg->fd = s->child_stderr[CHILD_STDERR_READ];
    stderr_arg->extended_type = SSH_STDERR_DATA;

    rc = pthread_create(&tid, NULL, stream_to_channel_thread, stdout_arg);
    sshd_dbg("spawn_child_session: stdout thread rc=%d tid=%d", rc, tid);
    if(rc != 0)
        return -1;
    if(sshd_clone_vfs_fds_to_pid((int)tid) != 0) {
        sshd_dbg("spawn_child_session: stdout thread clone fds failed tid=%d", tid);
        return -1;
    }
    stdout_arg->start_ready = 1;
    pthread_detach(tid);
    rc = pthread_create(&tid, NULL, stream_to_channel_thread, stderr_arg);
    sshd_dbg("spawn_child_session: stderr thread rc=%d tid=%d", rc, tid);
    if(rc != 0)
        return -1;
    if(sshd_clone_vfs_fds_to_pid((int)tid) != 0) {
        sshd_dbg("spawn_child_session: stderr thread clone fds failed tid=%d", tid);
        return -1;
    }
    stderr_arg->start_ready = 1;
    pthread_detach(tid);
    rc = pthread_create(&tid, NULL, wait_child_thread, s);
    sshd_dbg("spawn_child_session: wait thread rc=%d tid=%d", rc, tid);
    if(rc != 0)
        return -1;
    if(sshd_clone_vfs_fds_to_pid((int)tid) != 0) {
        sshd_dbg("spawn_child_session: wait thread clone fds failed tid=%d", tid);
        return -1;
    }
    pthread_detach(tid);

    return 0;
}

static int handle_channel_open(sshd_session_t* s, const ssh_packet_t* packet) {
    const uint8_t* channel_type;
    uint32_t type_len;
    size_t off = 0;

    if(read_ssh_string(packet->payload, packet->payload_len, &off, &channel_type, &type_len) < 0)
        return -1;
    /* #region debug-point session-flow */
    sshd_dbg("handle_channel_open: type=%.*s", (int)type_len, channel_type);
    /* #endregion debug-point session-flow */
    if(type_len != strlen("session") || memcmp(channel_type, "session", type_len) != 0) {
        s->remote_channel = ssh_read_uint32(packet->payload + off);
        return send_channel_open_failure(s, SSH_CHANNEL_OPEN_ADMINISTRATIVELY_PROHIBITED,
                "only session channel is supported");
    }
    s->remote_channel = ssh_read_uint32(packet->payload + off);
    off += 4;
    s->remote_window = ssh_read_uint32(packet->payload + off);
    off += 4;
    s->remote_packet_max = ssh_read_uint32(packet->payload + off);
    /* #region debug-point session-flow */
    sshd_dbg("handle_channel_open: remote_channel=%u remote_window=%u remote_packet_max=%u",
            s->remote_channel, s->remote_window, s->remote_packet_max);
    /* #endregion debug-point session-flow */

    s->local_channel = 0;
    s->local_window = SSH_MAX_PACKET_SIZE;
    s->local_packet_max = 16384;
    s->channel_open = 1;
    return send_channel_confirm(s);
}

static int handle_channel_request(sshd_session_t* s, const ssh_packet_t* packet) {
    const uint8_t* req_type;
    uint32_t req_len;
    uint32_t channel_id;
    size_t off = 0;
    int want_reply;

    if(packet->payload_len < 5)
        return -1;
    channel_id = ssh_read_uint32(packet->payload + off);
    off += 4;
    if(channel_id != s->local_channel)
        return -1;
    if(read_ssh_string(packet->payload, packet->payload_len, &off, &req_type, &req_len) < 0)
        return -1;
    if(off >= packet->payload_len)
        return -1;
    want_reply = packet->payload[off++];
    /* #region debug-point session-flow */
    sshd_dbg("handle_channel_request: channel_id=%u req=%.*s want_reply=%d",
            channel_id, (int)req_len, req_type, want_reply);
    /* #endregion debug-point session-flow */

    if(req_len == strlen("pty-req") && memcmp(req_type, "pty-req", req_len) == 0) {
        const uint8_t* term;
        uint32_t term_len;
        if(read_ssh_string(packet->payload, packet->payload_len, &off, &term, &term_len) < 0)
            return -1;
        (void)term;
        (void)term_len;
        if(off + 16 > packet->payload_len)
            return -1;
        s->terminal_width = (int)ssh_read_uint32(packet->payload + off);
        off += 4;
        s->terminal_height = (int)ssh_read_uint32(packet->payload + off);
        s->pty_enabled = 1;
        if(want_reply)
            return send_channel_status(s, SSH_MSG_CHANNEL_SUCCESS);
        return 0;
    }

    if(req_len == strlen("shell") && memcmp(req_type, "shell", req_len) == 0) {
        if(!s->child_started && spawn_child_session(s, NULL, 1) < 0)
            return -1;
        if(want_reply)
            return send_channel_status(s, SSH_MSG_CHANNEL_SUCCESS);
        return 0;
    }

    if(req_len == strlen("exec") && memcmp(req_type, "exec", req_len) == 0) {
        const uint8_t* cmd;
        uint32_t cmd_len;
        char command[256];

        if(read_ssh_string(packet->payload, packet->payload_len, &off, &cmd, &cmd_len) < 0)
            return -1;
        if(cmd_len == 0 || cmd_len >= sizeof(command))
            return -1;
        memcpy(command, cmd, cmd_len);
        command[cmd_len] = 0;
        if(!s->child_started && spawn_child_session(s, command, 0) < 0)
            return -1;
        if(want_reply)
            return send_channel_status(s, SSH_MSG_CHANNEL_SUCCESS);
        return 0;
    }

    if(req_len == strlen("window-change") && memcmp(req_type, "window-change", req_len) == 0) {
        if(off + 16 <= packet->payload_len) {
            s->terminal_width = (int)ssh_read_uint32(packet->payload + off);
            off += 4;
            s->terminal_height = (int)ssh_read_uint32(packet->payload + off);
        }
        if(want_reply)
            return send_channel_status(s, SSH_MSG_CHANNEL_SUCCESS);
        return 0;
    }

    if(want_reply)
        return send_channel_status(s, SSH_MSG_CHANNEL_FAILURE);
    return 0;
}

static int handle_channel_data(sshd_session_t* s, const ssh_packet_t* packet) {
    const uint8_t* data;
    uint32_t data_len;
    uint32_t channel_id;
    size_t off = 0;

    if(!s->child_started)
        return -1;
    channel_id = ssh_read_uint32(packet->payload + off);
    off += 4;
    if(channel_id != s->local_channel)
        return -1;
    if(read_ssh_string(packet->payload, packet->payload_len, &off, &data, &data_len) < 0)
        return -1;
    if(data_len == 0)
        return 0;
    sshd_dbg("handle_channel_data: len=%u", (unsigned int)data_len);
    if(write_fd_all(s->child_stdin[CHILD_STDIN_WRITE], data, data_len) < 0) {
        sshd_dbg("handle_channel_data: write child stdin failed errno=%d", errno);
        return -1;
    }
    return 0;
}

static int handle_session_packets(sshd_session_t* s) {
    ssh_packet_t packet;

    while(!s->sent_close) {
        if(ssh_packet_receive(s, &packet) < 0)
            return -1;
        /* #region debug-point session-flow */
        sshd_dbg("handle_session_packets: packet type=%u", packet.type);
        /* #endregion debug-point session-flow */

        switch(packet.type) {
        case SSH_MSG_GLOBAL_REQUEST:
            if(send_request_failure(s) < 0)
                return -1;
            break;
        case SSH_MSG_CHANNEL_OPEN:
            if(handle_channel_open(s, &packet) < 0)
                return -1;
            break;
        case SSH_MSG_CHANNEL_REQUEST:
            if(handle_channel_request(s, &packet) < 0)
                return -1;
            break;
        case SSH_MSG_CHANNEL_DATA:
            if(handle_channel_data(s, &packet) < 0)
                return -1;
            break;
        case SSH_MSG_CHANNEL_WINDOW_ADJUST:
            if(packet.payload_len >= 8)
                s->remote_window += ssh_read_uint32(packet.payload + 4);
            break;
        case SSH_MSG_CHANNEL_EOF:
            if(s->child_stdin[CHILD_STDIN_WRITE] >= 0) {
                close(s->child_stdin[CHILD_STDIN_WRITE]);
                s->child_stdin[CHILD_STDIN_WRITE] = -1;
            }
            break;
        case SSH_MSG_CHANNEL_CLOSE:
            send_channel_eof_and_close(s);
            return 0;
        default:
            break;
        }
    }
    return 0;
}

static void session_init(sshd_session_t* s, int sock) {
    memset(s, 0, sizeof(*s));
    s->socket = sock;
    strncpy(s->server_version, SSH_SERVER_VERSION, sizeof(s->server_version) - 1);
    s->terminal_width = 80;
    s->terminal_height = 24;
    s->remote_packet_max = 16384;
    s->local_packet_max = 16384;
    s->child_stdin[0] = s->child_stdin[1] = -1;
    s->child_stdout[0] = s->child_stdout[1] = -1;
    s->child_stderr[0] = s->child_stderr[1] = -1;
    s->child_control[0] = s->child_control[1] = -1;
    pthread_mutex_init(&s->send_lock, NULL);
}

static void session_destroy(sshd_session_t* s) {
    close_child_fds(s);
    if(s->child_pid > 0 && !s->child_started)
        waitpid(s->child_pid);
    if(s->socket >= 0)
        close(s->socket);
}

static int open_server_socket(int port) {
    struct sockaddr_in addr;

    /* #region debug-point open-server-socket */
    sshd_dbg("open_server_socket: begin, port=%d", port);
    /* #endregion debug-point open-server-socket */
    while(true) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(sock < 0) {
            /* #region debug-point open-server-socket */
            sshd_dbg("open_server_socket: socket failed, errno=%d", errno);
            /* #endregion debug-point open-server-socket */
            proc_usleep(200000);
            continue;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_un.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        /* #region debug-point open-server-socket */
        sshd_dbg("open_server_socket: socket ok, fd=%d", sock);
        /* #endregion debug-point open-server-socket */
        if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0 && listen(sock, 5) == 0) {
            /* #region debug-point open-server-socket */
            sshd_dbg("open_server_socket: listen ok, fd=%d", sock);
            /* #endregion debug-point open-server-socket */
            return sock;
        }
        /* #region debug-point open-server-socket */
        sshd_dbg("open_server_socket: bind/listen failed, errno=%d", errno);
        /* #endregion debug-point open-server-socket */
        close(sock);
        proc_usleep(200000);
    }
}

static int init_host_key(void) {
    WC_RNG rng;
    RsaKey wc_rsa;
    uint8_t der[2048];
    int ret;

    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: begin");
    /* #endregion debug-point init-host-key */
    /* #region debug-point openssl-error */
    ERR_clear_error();
    /* #endregion debug-point openssl-error */
    if(load_host_key_from_file(SSH_HOST_KEY_PATH) == 0) {
        /* #region debug-point init-host-key */
        sshd_dbg("init_host_key: loaded persisted key from %s", SSH_HOST_KEY_PATH);
        sshd_dbg("init_host_key: success, hostkey_blob_len=%u", (unsigned int)g_hostkey_blob_len);
        /* #endregion debug-point init-host-key */
        return 0;
    }

    g_host_rsa = RSA_new();
    if(g_host_rsa == NULL) {
        /* #region debug-point init-host-key */
        sshd_dbg("init_host_key: RSA_new failed");
        /* #endregion debug-point init-host-key */
        /* #region debug-point openssl-error */
        sshd_dbg_last_ssl_error("init_host_key: alloc");
        /* #endregion debug-point openssl-error */
        return -1;
    }
    memset(&rng, 0, sizeof(rng));
    memset(&wc_rsa, 0, sizeof(wc_rsa));
    ret = wc_InitRng(&rng);
    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: wc_InitRng ret=%d", ret);
    /* #endregion debug-point init-host-key */
    if(ret != 0)
        return -1;
    ret = wc_InitRsaKey(&wc_rsa, NULL);
    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: wc_InitRsaKey ret=%d", ret);
    /* #endregion debug-point init-host-key */
    if(ret != 0) {
        wc_FreeRng(&rng);
        return -1;
    }
    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: generating RSA key");
    /* #endregion debug-point init-host-key */
    ret = wc_MakeRsaKey(&wc_rsa, 2048, RSA_F4, &rng);
    if(ret != 0) {
        /* #region debug-point init-host-key */
        sshd_dbg("init_host_key: wc_MakeRsaKey failed, ret=%d", ret);
        /* #endregion debug-point init-host-key */
        wc_FreeRsaKey(&wc_rsa);
        wc_FreeRng(&rng);
        return -1;
    }
    ret = wc_RsaKeyToDer(&wc_rsa, der, sizeof(der));
    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: wc_RsaKeyToDer ret=%d", ret);
    /* #endregion debug-point init-host-key */
    if(ret <= 0) {
        wc_FreeRsaKey(&wc_rsa);
        wc_FreeRng(&rng);
        return -1;
    }
    if(wolfSSL_RSA_LoadDer(g_host_rsa, der, ret) != 1) {
        /* #region debug-point init-host-key */
        sshd_dbg("init_host_key: wolfSSL_RSA_LoadDer failed");
        /* #endregion debug-point init-host-key */
        /* #region debug-point openssl-error */
        sshd_dbg_last_ssl_error("init_host_key: wolfSSL_RSA_LoadDer");
        /* #endregion debug-point openssl-error */
        wc_FreeRsaKey(&wc_rsa);
        wc_FreeRng(&rng);
        return -1;
    }
    if(save_host_key_to_file(SSH_HOST_KEY_PATH, der, ret) == 0) {
        /* #region debug-point init-host-key */
        sshd_dbg("init_host_key: persisted key to %s", SSH_HOST_KEY_PATH);
        /* #endregion debug-point init-host-key */
    }
    else {
        /* #region debug-point init-host-key */
        sshd_dbg("init_host_key: persist key failed, path=%s errno=%d", SSH_HOST_KEY_PATH, errno);
        /* #endregion debug-point init-host-key */
    }
    wc_FreeRsaKey(&wc_rsa);
    wc_FreeRng(&rng);
    if(build_hostkey_blob_from_rsa(g_host_rsa) < 0)
        return -1;
    /* #region debug-point init-host-key */
    sshd_dbg("init_host_key: success, hostkey_blob_len=%u", (unsigned int)g_hostkey_blob_len);
    /* #endregion debug-point init-host-key */
    return 0;
}

static int serve_client(int sock, pid_t helper_pid, const int child_stdin[2],
        const int child_stdout[2], const int child_stderr[2], const int child_control[2]) {
    sshd_session_t* session;
    int ret = -1;

    session = (sshd_session_t*)calloc(1, sizeof(*session));
    if(session == NULL) {
        close(sock);
        return -1;
    }

    session_init(session, sock);
    session_attach_child_bundle(session, helper_pid, child_stdin, child_stdout,
            child_stderr, child_control);
    /* #region debug-point session-flow */
    sshd_dbg("serve_client: attached helper pid=%d", helper_pid);
    /* #endregion debug-point session-flow */

    /* #region debug-point kex-flow */
    sshd_dbg("serve_client: start handshake");
    /* #endregion debug-point kex-flow */
    if(send_banner(session) < 0 || receive_banner(session) < 0)
        goto out;
    /* #region debug-point kex-flow */
    sshd_dbg("serve_client: banner exchange ok");
    /* #endregion debug-point kex-flow */
    if(receive_kexinit(session) < 0)
        goto out;
    /* #region debug-point kex-flow */
    sshd_dbg("serve_client: receive_kexinit ok");
    /* #endregion debug-point kex-flow */
    if(send_kexinit(session) < 0)
        goto out;
    /* #region debug-point kex-flow */
    sshd_dbg("serve_client: send_kexinit ok");
    /* #endregion debug-point kex-flow */
    if(do_key_exchange(session) < 0)
        goto out;
    /* #region debug-point kex-flow */
    sshd_dbg("serve_client: key exchange ok");
    /* #endregion debug-point kex-flow */
    if(handle_service_and_auth(session) < 0)
        goto out;
    if(handle_session_packets(session) < 0)
        goto out;

    ret = 0;

out:
    /* #region debug-point kex-flow */
    sshd_dbg("serve_client: out, ret=%d, error=%s", ret, g_error[0] ? g_error : "<none>");
    /* #endregion debug-point kex-flow */
    if(ret < 0 && session->socket >= 0)
        send_disconnect(session, SSH_DISCONNECT_BY_APPLICATION, g_error[0] ? g_error : "sshd error");
    session_destroy(session);
    return ret;
}

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : SSH_DEFAULT_PORT;
    int server_sock;

    /* #region debug-point main-startup */
    sshd_dbg("main: start, argc=%d, port=%d", argc, port);
    /* #endregion debug-point main-startup */
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
            OPENSSL_INIT_ADD_ALL_CIPHERS |
            OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
    srand(42);
    /* #region debug-point main-startup */
    sshd_dbg("main: openssl init done");
    /* #endregion debug-point main-startup */

    if(init_host_key() < 0) {
        /* #region debug-point main-startup */
        sshd_dbg("main: init_host_key failed");
        /* #endregion debug-point main-startup */
        return -1;
    }
    /* #region debug-point main-startup */
    sshd_dbg("main: init_host_key ok");
    /* #endregion debug-point main-startup */

    server_sock = open_server_socket(port);
    /* #region debug-point main-startup */
    sshd_dbg("main: server socket ready, fd=%d", server_sock);
    /* #endregion debug-point main-startup */
    while(true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        int child_stdin[2] = {-1, -1};
        int child_stdout[2] = {-1, -1};
        int child_stderr[2] = {-1, -1};
        int child_control[2] = {-1, -1};
        pid_t helper_pid = -1;
        if(client_sock < 0) {
            /* #region debug-point main-startup */
            sshd_dbg("main: accept failed, errno=%d", errno);
            /* #endregion debug-point main-startup */
            proc_usleep(10000);
            continue;
        }
        /* #region debug-point main-startup */
        sshd_dbg("main: accept ok, client_fd=%d", client_sock);
        /* #endregion debug-point main-startup */

        if(create_child_session_bundle(server_sock, client_sock, child_stdin,
                child_stdout, child_stderr, child_control, &helper_pid) < 0) {
            sshd_dbg("main: create_child_session_bundle failed");
            close(client_sock);
            continue;
        }
        /* #region debug-point main-startup */
        sshd_dbg("main: helper fork ok, pid=%d", helper_pid);
        /* #endregion debug-point main-startup */

        int pid = fork();
        if(pid < 0) {
            /* #region debug-point main-startup */
            sshd_dbg("main: fork failed");
            /* #endregion debug-point main-startup */
            close_child_bundle(child_stdin, child_stdout, child_stderr, child_control);
            close(client_sock);
            waitpid(helper_pid);
            continue;
        }
        if(pid == 0) {
            /* #region debug-point main-startup */
            sshd_dbg("main: child start, client_fd=%d", client_sock);
            /* #endregion debug-point main-startup */
            close(server_sock);
            serve_client(client_sock, helper_pid, child_stdin, child_stdout,
                    child_stderr, child_control);
            exit(0);
        }
        /* #region debug-point main-startup */
        sshd_dbg("main: fork ok, pid=%d", pid);
        /* #endregion debug-point main-startup */
        close_child_bundle(child_stdin, child_stdout, child_stderr, child_control);
        close(client_sock);
    }

    return 0;
}
