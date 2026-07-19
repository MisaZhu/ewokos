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
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <poll.h>
#include <ewoksys/proc.h>
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
#include <signal.h>
#include "sftp-core.h"

#define SSH_DEFAULT_PORT        22
#define SSH_CHANNEL_DATA_MAX    32768
#define SSH_CHANNEL_MSG_OVERHEAD 12
#define SSH_PACKET_PAYLOAD_SIZE (SSH_CHANNEL_DATA_MAX + SSH_CHANNEL_MSG_OVERHEAD)
#define SSH_PACKET_PADDING_MAX  32
#define SSH_PACKET_LENGTH_MAX   (2 + SSH_PACKET_PAYLOAD_SIZE + SSH_PACKET_PADDING_MAX)
#define SSH_PACKET_WIRE_MAX     (4 + SSH_PACKET_LENGTH_MAX)
#define SSH_PACKET_MAC_INPUT_MAX (4 + SSH_PACKET_WIRE_MAX)
#define SSH_LOCAL_WINDOW_SIZE   (8*1024*1024)
#define SSH_LOCAL_WINDOW_LOW    (6*1024*1024)
#define SSH_WINDOW_ADJUST_MIN   (128*1024)
#define SSH_CHILD_STDIN_STEP    4096
#define SSH_RELAY_READ_SIZE     16384
#define SSH_SOCKET_WRITE_STEP   1024
#define SSH_PENDING_INPUT_WAIT_US 1000
#define SSH_CHILD_STDIN_POLL_MS 5
#define SSH_CHILD_READY_TIMEOUT_MS 2000
#define SSH_SFTP_PENDING_HIGH_WATER (8*1024*1024)
#define SSH_SERVER_VERSION      "SSH-2.0-EwokOS_sshd"
#define SSH_HOST_KEY_PATH       "/etc/ssh_host_rsa_key.der"
#define SSHD_MAX_TRACKED_WORKERS 128

#ifdef SSHD_DEBUG
#define SSHD_DBG(fmt, ...) klog("sshd: " fmt, ##__VA_ARGS__)
#else
#define SSHD_DBG(fmt, ...) do { if(0) klog("sshd: " fmt, ##__VA_ARGS__); } while(0)
#endif
#define SSHD_STALL_LOG(fmt, ...) do { } while(0)

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
    uint8_t payload[SSH_PACKET_PAYLOAD_SIZE];
    size_t payload_len;
} ssh_packet_t;

typedef struct sshd_session sshd_session_t;

static void session_close_socket(sshd_session_t* s);

typedef struct {
    sshd_session_t* session;
} session_io_thread_arg_t;

typedef struct {
    sshd_session_t* s;
} internal_sftp_arg_t;

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

    uint8_t client_kexinit[SSH_PACKET_PAYLOAD_SIZE];
    size_t client_kexinit_len;
    uint8_t server_kexinit[SSH_PACKET_PAYLOAD_SIZE];
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
    int peer_eof;
    int peer_close;

    int terminal_width;
    int terminal_height;
    int pty_enabled;

    int child_stdin[2];
    int child_stdout[2];
    int child_control[2];
    int close_notify[2];
    pid_t child_pid;
    int child_io_thread_started;
    int child_is_sftp;
    int internal_sftp_active;
    int internal_sftp_done;
    int child_eof_seen;
    int pipe_hup;  /* POLLHUP seen on child_stdout pipe (write-end closed) */

    pthread_t internal_sftp_tid;
    int internal_sftp_started;
    internal_sftp_arg_t internal_sftp_arg;

    pthread_mutex_t state_lock;
    pthread_mutex_t send_lock;
    pthread_cond_t remote_window_cv;
    pthread_mutex_t sftp_lock;
    pthread_cond_t sftp_cv;

    /* Non-blocking stdin buffering (inbound thread must never block on child stdin) */
    uint8_t* pending_in;
    size_t pending_in_len;
    size_t pending_in_off;
    size_t pending_in_cap;

    /* Deferred WINDOW_ADJUST (inbound thread must never block on socket write) */
    uint32_t win_adjust_owe;

    /* In-process SFTP input queue */
    uint8_t* sftp_in;
    size_t sftp_in_len;
    size_t sftp_in_off;
    size_t sftp_in_cap;
    int sftp_in_eof;
};

static RSA* g_host_rsa = NULL;
static uint8_t g_hostkey_blob[512];
static size_t g_hostkey_blob_len = 0;
static char g_error[256];
static pid_t g_tracked_workers[SSHD_MAX_TRACKED_WORKERS];

static int write_ssh_string(uint8_t* buf, size_t cap, size_t* off,
        const uint8_t* data, uint32_t len);

static int child_needs_ready_handshake(const char* command, int is_shell) {
    return (!is_shell && command != NULL && strcmp(command, "sftp-server") == 0);
}

static int wait_child_ready(int fd) {
    struct pollfd pfd;
    uint8_t marker = 0;
    int rc;
    ssize_t n;

    if(fd < 0)
        return 0;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
    rc = poll(&pfd, 1, SSH_CHILD_READY_TIMEOUT_MS);
    if(rc <= 0) {
        errno = (rc == 0) ? ETIMEDOUT : errno;
        return -1;
    }
    if((pfd.revents & POLLNVAL) != 0) {
        errno = EBADF;
        return -1;
    }
    if((pfd.revents & POLLERR) != 0) {
        errno = EIO;
        return -1;
    }
    if((pfd.revents & POLLIN) == 0) {
        errno = EPIPE;
        return -1;
    }
    do {
        errno = 0;
        n = read(fd, &marker, 1);
    } while(n < 0 && errno == EINTR);
    if(n != 1) {
        if(n == 0)
            errno = EPIPE;
        else if(errno == 0)
            errno = EIO;
        return -1;
    }
    if(marker != 'R') {
        errno = EIO;
        return -1;
    }
    return 0;
}

static size_t pending_input_bytes(const sshd_session_t* s) {
    if(s == NULL || s->pending_in_len <= s->pending_in_off)
        return 0;
    return s->pending_in_len - s->pending_in_off;
}

static size_t sftp_pending_excess(size_t pending_bytes) {
    if(pending_bytes <= SSH_SFTP_PENDING_HIGH_WATER)
        return 0;
    return pending_bytes - SSH_SFTP_PENDING_HIGH_WATER;
}

static uint32_t sftp_credit_on_enqueue(size_t pending_before, size_t incoming) {
    size_t excess_before = sftp_pending_excess(pending_before);
    size_t excess_after = sftp_pending_excess(pending_before + incoming);
    size_t consume = excess_after - excess_before;

    if(consume > incoming)
        consume = incoming;
    return (uint32_t)(incoming - consume);
}

static uint32_t sftp_credit_on_drain(size_t pending_before, size_t drained) {
    size_t excess_before = sftp_pending_excess(pending_before);
    size_t pending_after = (pending_before > drained) ? (pending_before - drained) : 0;
    size_t excess_after = sftp_pending_excess(pending_after);

    return (uint32_t)(excess_before - excess_after);
}


static void sshd_set_error(const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(g_error, sizeof(g_error), fmt, ap);
    va_end(ap);
}

static uint32_t sshd_remote_window_load(sshd_session_t* s) {
    uint32_t value;
    pthread_mutex_lock(&s->state_lock);
    value = s->remote_window;
    pthread_mutex_unlock(&s->state_lock);
    return value;
}

static uint32_t sshd_remote_window_add(sshd_session_t* s, uint32_t delta) {
    uint32_t value;
    pthread_mutex_lock(&s->state_lock);
    s->remote_window += delta;
    value = s->remote_window;
    pthread_cond_broadcast(&s->remote_window_cv);
    pthread_mutex_unlock(&s->state_lock);
    return value;
}

static uint32_t sshd_remote_window_sub(sshd_session_t* s, uint32_t delta) {
    uint32_t value;
    pthread_mutex_lock(&s->state_lock);
    s->remote_window -= delta;
    value = s->remote_window;
    pthread_mutex_unlock(&s->state_lock);
    return value;
}

static int sshd_wait_remote_window(sshd_session_t* s) {
    pthread_mutex_lock(&s->state_lock);
    while(s->remote_window == 0 && !s->closing) {
        pthread_cond_wait(&s->remote_window_cv, &s->state_lock);
    }
    pthread_mutex_unlock(&s->state_lock);
    if(s->closing) {
        errno = EINTR;
        return -1;
    }
    return 0;
}

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

static int wait_fd_ready(int fd, uint16_t events) {
    struct pollfd pfd;

    if(fd < 0) {
        errno = EBADF;
        return -1;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = (short)(events | POLLHUP | POLLERR | POLLNVAL);
    while(true) {
        int rc = poll(&pfd, 1, -1);
        if(rc > 0) {
            if((pfd.revents & POLLNVAL) != 0) {
                errno = EBADF;
                return -1;
            }
            if((pfd.revents & POLLERR) != 0) {
                errno = EIO;
                return -1;
            }
            if((pfd.revents & events) != 0)
                return 0;
            if((pfd.revents & POLLHUP) != 0) {
                errno = EPIPE;
                return -1;
            }
            continue;
        }
        if(rc == 0)
            continue;
        if(errno != EINTR)
            return -1;
    }
}

static void close_fd_if_valid(int* fd) {
    if(fd != NULL && *fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static void session_signal_close(sshd_session_t* s) {
    if(s == NULL)
        return;
    close_fd_if_valid(&s->close_notify[1]);
}

static int wait_session_fd_ready(sshd_session_t* s, int fd, uint16_t events) {
    struct pollfd pfd;

    if(fd < 0) {
        errno = EBADF;
        return -1;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = (short)(events | POLLHUP | POLLERR | POLLNVAL);

    while(true) {
        int rc;
        if(s != NULL && s->closing) {
            errno = EINTR;
            return -1;
        }
        /*
         * Use a bounded wait instead of an infinite block. The net stack can
         * still occasionally miss a RW wake edge under backpressure; timing
         * out here lets sshd re-sample readiness without falling back to the
         * old usleep spin loops.
         */
        rc = poll(&pfd, 1, 1000);
        if(rc > 0) {
            if((pfd.revents & POLLNVAL) != 0) {
                errno = EBADF;
                return -1;
            }
            if((pfd.revents & POLLERR) != 0) {
                errno = EIO;
                return -1;
            }
            if((pfd.revents & events) != 0)
                return 0;
            if((pfd.revents & POLLHUP) != 0) {
                errno = EPIPE;
                return -1;
            }
            continue;
        }
        if(rc == 0)
            continue;
        if(errno != EINTR)
            return -1;
    }
}

static int set_fd_nonblock(int fd) {
    int flags;

    if(fd < 0)
        return -1;
    flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        return -1;
    if((flags & O_NONBLOCK) != 0)
        return 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int ssh_read_n(sshd_session_t* s, void* buf, size_t len) {
    uint8_t* p = (uint8_t*)buf;
    size_t total = 0;

    while(total < len) {
        int saved_errno;
        errno = 0;
        ssize_t n = read(s->socket, p + total, len - total);
        saved_errno = errno;
        if(n < 0) {
            if(saved_errno == 0)
                saved_errno = EAGAIN;
            if(saved_errno == EINTR) {
                continue;
            }
            if(saved_errno == EAGAIN || saved_errno == ETIMEDOUT) {
                if(s->closing || s->sent_close) {
                    sshd_set_error("session closing");
                    return -1;
                }
                if(wait_session_fd_ready(s, s->socket, POLLIN) != 0) {
                    if(s->closing || s->sent_close)
                        sshd_set_error("session closing");
                    else
                        sshd_set_error("read wait failed: %s", strerror(errno));
                    return -1;
                }
                continue;
            }
            sshd_set_error("read failed: %s", strerror(saved_errno));
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

    g_error[0] = 0;

    while(total < len) {
        int saved_errno;
        size_t step = len - total;
        if(step > SSH_SOCKET_WRITE_STEP)
            step = SSH_SOCKET_WRITE_STEP;
        errno = 0;
        /*
         * EwokOS socket writes become unreliable when we hand large SSH packet
         * buffers to a single write() call; runtime logs consistently showed
         * progress only when the stack accepted ~1 KiB chunks. Keep each write
         * small even on blocking sockets so large SFTP replies are streamed out
         * incrementally instead of stalling mid-packet.
         */
        ssize_t n = write(s->socket, p + total, step);
        saved_errno = errno;
        if(n < 0) {
            if(saved_errno == 0)
                saved_errno = EAGAIN;
            if(saved_errno == EINTR) {
                continue;
            }
            if(saved_errno == EAGAIN) {
                if(wait_session_fd_ready(s, s->socket, POLLOUT) != 0) {
                    if(s->closing || s->sent_close)
                        sshd_set_error("session closing");
                    else
                        sshd_set_error("write wait failed: %s", strerror(errno));
                    return -1;
                }
                continue;
            }
            sshd_set_error("write failed: %s", strerror(saved_errno));
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
        int saved_errno;
        errno = 0;
        ssize_t n = write(fd, p + total, len - total);
        saved_errno = errno;
        if(n < 0) {
            if(saved_errno == 0)
                saved_errno = EAGAIN;
            if(saved_errno == EINTR) {
                continue;
            }
            if(saved_errno == EAGAIN) {
                if(wait_fd_ready(fd, POLLOUT) != 0)
                    return -1;
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

static __attribute__((unused)) int read_fd_all(int fd, void* buf, size_t len) {
    uint8_t* p = (uint8_t*)buf;
    size_t total = 0;

    while(total < len) {
        int saved_errno;
        errno = 0;
        ssize_t n = read(fd, p + total, len - total);
        saved_errno = errno;
        if(n < 0) {
            if(saved_errno == 0)
                saved_errno = EAGAIN;
            if(saved_errno == EINTR) {
                continue;
            }
            if(saved_errno == EAGAIN) {
                if(wait_fd_ready(fd, POLLIN) != 0)
                    return -1;
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
    static const char* search_dirs[] = {"/bin/", "/sbin/"};
    char fpath[64];
    char exec_cmd[128];
    const char* suffix = NULL;
    int sz = 0;
    uint8_t* buf;

    if(name == NULL || name[0] == 0)
        return -1;

    memset(fpath, 0, sizeof(fpath));
    for(size_t i = 0; i < sizeof(fpath) - 1; i++) {
        if(name[i] == '\0' || name[i] == ' ' || name[i] == '\t' || name[i] == '\n') {
            suffix = name + i;
            break;
        }
        fpath[i] = name[i];
    }
    if(suffix == NULL)
        suffix = name + strlen(name);

    buf = vfs_readfile(fpath, &sz);
    if(buf == NULL && fpath[0] != '/') {
        for(size_t i = 0; i < sizeof(search_dirs) / sizeof(search_dirs[0]); i++) {
            char resolved[sizeof(fpath)];

            snprintf(resolved, sizeof(resolved), "%s%s", search_dirs[i], fpath);
            buf = vfs_readfile(resolved, &sz);
            if(buf != NULL) {
                strncpy(fpath, resolved, sizeof(fpath) - 1);
                fpath[sizeof(fpath) - 1] = 0;
                break;
            }
        }
    }
    if(buf == NULL)
        return -1;

    snprintf(exec_cmd, sizeof(exec_cmd), "%s%s", fpath, suffix);
    proc_exec_elf(exec_cmd, (const char*)buf, sz);
    free(buf);
    return 0;
}

static __attribute__((unused)) void copy_cstr(char* dst, size_t dst_size, const char* src) {
    size_t len;

    if(dst == NULL || dst_size == 0)
        return;
    if(src == NULL) {
        dst[0] = 0;
        return;
    }
    len = 0;
    while(len + 1 < dst_size && src[len] != 0)
        len++;
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
    uint8_t mac[32];
    uint8_t* buf = NULL;
    uint8_t* mac_input = NULL;
    uint32_t block_size = s->encryption_enabled ? 16 : 8;
    uint32_t base_len = 1 + 1 + packet->payload_len;
    uint32_t padding_len = block_size - ((4 + base_len) % block_size);
    uint32_t packet_len;
    uint32_t seq;
    size_t total_len;
    size_t wire_len;
    uint8_t* p;
    int ret = -1;

    /*
     * buf and mac_input are ~32KB each. Keeping them on the stack pushed this
     * frame to ~64KB; combined with the caller's ssh_packet_t (~32KB) the send
     * path overran the pthread stack. Since thread stacks are packed with no
     * guard page, that silently smashed an adjacent thread's saved registers
     * and later caused a data abort on a corrupted pointer. Allocate on the
     * heap instead (send path is already serialized by send_lock).
     */
    buf = (uint8_t*)malloc(SSH_PACKET_WIRE_MAX + sizeof(mac));
    mac_input = (uint8_t*)malloc(SSH_PACKET_MAC_INPUT_MAX);
    if(buf == NULL || mac_input == NULL) {
        sshd_set_error("packet send out of memory");
        goto out;
    }

    if(padding_len < 4)
        padding_len += block_size;
    packet_len = base_len + padding_len;
    total_len = 4 + packet_len;

    if(total_len > (size_t)SSH_PACKET_WIRE_MAX) {
        sshd_set_error("packet too large");
        goto out;
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
            goto out;
        }
        memcpy(buf + total_len, mac, sizeof(mac));
        wire_len += sizeof(mac);
    }

    if(ssh_write_n(s, buf, wire_len) < 0)
        goto out;
    s->seq_s2c++;
    ret = 0;
out:
    free(buf);
    free(mac_input);
    return ret;
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
    uint8_t* mac_input = NULL;
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
        if(packet_len > SSH_PACKET_LENGTH_MAX || packet_len < 5) {
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
        if(packet_len > SSH_PACKET_LENGTH_MAX || packet_len < 5) {
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
        mac_input = (uint8_t*)malloc(SSH_PACKET_MAC_INPUT_MAX);
        if(mac_input == NULL) {
            free(packet_buf);
            sshd_set_error("packet receive out of memory");
            return -1;
        }
        ssh_write_uint32(mac_input, seq);
        memcpy(mac_input + 4, packet_buf, total_len);
        hmac_sha256(s->mac_key_c2s, sizeof(s->mac_key_c2s), mac_input, 4 + total_len, calc_mac);
        if(memcmp(mac, calc_mac, sizeof(mac)) != 0) {
            free(mac_input);
            free(packet_buf);
            sshd_set_error("packet mac mismatch");
            return -1;
        }
        free(mac_input);
    }

    padding_len = packet_buf[4];
    if(packet_len < (uint32_t)(padding_len + 2)) {
        free(packet_buf);
        sshd_set_error("invalid padding length");
        return -1;
    }
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
            if(errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT) {
                if(s->closing || s->sent_close)
                    return -1;
                if(wait_session_fd_ready(s, s->socket, POLLIN) != 0)
                    return -1;
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

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_KEXINIT;
    if(RAND_bytes(cookie, sizeof(cookie)) != 1) {
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
        return -1;
    }
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

    if(ssh_packet_receive(s, &packet) < 0) {
        return -1;
    }
    if(packet.type != SSH_MSG_KEXDH_INIT) {
        sshd_set_error("expected KEXDH_INIT, got %u", packet.type);
        return -1;
    }

    if(read_ssh_string(packet.payload, packet.payload_len, &off, &e_ptr, &e_len) < 0)
        return -1;
    if(e_len == 0 || (e_len > 1 && e_ptr[0] == 0 && e_ptr[1] == 0)) {
        sshd_set_error("invalid client dh pubkey mpint");
        goto fail;
    }
    if(e_len > 1 && e_ptr[0] == 0) {
        e_ptr++;
        e_len--;
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

    e_bn = BN_bin2bn(e_ptr, e_len, NULL);
    if(e_bn == NULL)
        goto fail;

    DH_get0_key(dh, &f_pub, NULL);
    if(f_pub == NULL)
        goto fail;
    f_raw_len = BN_bn2bin(f_pub, f_raw);
    if(f_raw_len <= 0)
        goto fail;
    if(ssh_encode_mpint(f_raw, (size_t)f_raw_len, f_mpint, sizeof(f_mpint), &f_mpint_len) < 0)
        goto fail;

    k_raw_len = DH_compute_key(k_raw, e_bn, dh);
    if(k_raw_len <= 0)
        goto fail;
    if(ssh_encode_mpint(k_raw, (size_t)k_raw_len, k_mpint, sizeof(k_mpint), &k_mpint_len) < 0)
        goto fail;

    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

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
    }

    if(s->session_id_len == 0) {
        memcpy(s->session_id, exchange_hash, sizeof(exchange_hash));
        s->session_id_len = sizeof(exchange_hash);
    }
    derive_keys(s, k_mpint, k_mpint_len, exchange_hash, sizeof(exchange_hash));

    if(build_signature_blob(exchange_hash, sizeof(exchange_hash), s->negotiated_hostkey_alg,
                signature_blob, sizeof(signature_blob), &signature_blob_len) < 0) {
        goto fail;
    }

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
        goto fail;
    }

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_NEWKEYS;
    if(ssh_packet_send(s, &packet) < 0) {
        goto fail;
    }
    if(ssh_packet_receive(s, &packet) < 0) {
        goto fail;
    }
    if(packet.type != SSH_MSG_NEWKEYS) {
        sshd_set_error("expected NEWKEYS, got %u", packet.type);
        goto fail;
    }

    s->encryption_enabled = 1;
    BN_free(e_bn);
    DH_free(dh);
    return 0;

fail:
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

    if(ssh_packet_receive(s, &packet) < 0)
        return -1;
    if(packet.type != SSH_MSG_SERVICE_REQUEST)
        return -1;

    off = 0;
    if(read_ssh_string(packet.payload, packet.payload_len, &off, &service, &service_len) < 0)
        return -1;
    if(service_len != strlen("ssh-userauth") ||
            memcmp(service, "ssh-userauth", service_len) != 0)
        return -1;
    if(send_service_accept(s, "ssh-userauth") < 0)
        return -1;

    while(!s->authenticated) {
        int auth_res = -1;
        char username[SESSION_USER_MAX];
        char passwd[SESSION_PSWD_MAX];

        if(ssh_packet_receive(s, &packet) < 0)
            return -1;
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
        if(auth_res == 0 && s->user_info.cmd[0] != 0) {
            memset(&packet, 0, sizeof(packet));
            packet.type = SSH_MSG_USERAUTH_SUCCESS;
            packet.payload_len = 0;
            if(ssh_packet_send(s, &packet) < 0)
                return -1;
            s->authenticated = 1;
            return 0;
        }

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
    if(ssh_packet_send(s, &packet) < 0) {
        return -1;
    }
    return 0;
}

static int send_request_failure(sshd_session_t* s) {
    ssh_packet_t packet;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_REQUEST_FAILURE;
    packet.payload_len = 0;
    return ssh_packet_send(s, &packet);
}

static int try_send_window_adjust(sshd_session_t* s);

static void clear_fd_write_poll_event(int fd) {
    fsinfo_t info;

    if(fd < 0)
        return;
    if(vfs_get_by_fd(fd, &info) != 0 || info.node == 0)
        return;
    vfs_clear_poll_events(info.node, VFS_EVT_WR);
}

static void log_fd_write_failure(sshd_session_t* s, int fd, int saved_errno) {
    fsinfo_t info;
    int info_rc;
    int flags;

    info_rc = vfs_get_by_fd(fd, &info);
    flags = vfs_get_flags(fd);
    SSHD_DBG("child_stdin fail pid=%d fd=%d err=%d info_rc=%d node=%u type=%u flags=%x\n",
            (int)s->child_pid, fd, saved_errno, info_rc,
            info_rc == 0 ? info.node : 0,
            info_rc == 0 ? info.type : 0,
            flags);
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
    uint8_t text_buf[2048];
    const uint8_t* payload = data;
    size_t payload_len = len;
    size_t sent = 0;
    unsigned int chunk_index = 0;

    if(s->pty_enabled) {
        payload_len = ssh_console_copy_crlf(text_buf, sizeof(text_buf), data, len);
        payload = text_buf;
    }

    // #region debug-point D:sshd-child-stdout
    if(s->child_is_sftp && extended_type == 0 && payload_len > 0 && payload_len <= 128) {
        klog("sftp-klog:sshd-child-stdout pid=%d len=%u first=%u\n",
                (int)s->child_pid,
                (unsigned int)payload_len,
                (unsigned int)payload[0]);
    }
    // #endregion

    while(sent < payload_len && !s->closing) {
        ssh_packet_t packet;
        size_t off = 0;
        size_t chunk = payload_len - sent;
        uint32_t win;
        uint32_t packet_max;

        win = sshd_remote_window_load(s);
        packet_max = s->remote_packet_max;
        if(packet_max == 0 || packet_max > SSH_CHANNEL_DATA_MAX)
            packet_max = SSH_CHANNEL_DATA_MAX;

        if(win == 0) {
            SSHD_DBG("channel_out wait_remote pid=%d ext=%u sent=%u remain=%u\n",
                    (int)s->child_pid, extended_type,
                    (unsigned int)sent,
                    (unsigned int)(payload_len - sent));
            if(sshd_wait_remote_window(s) != 0)
                break;
            continue;
        }

        if(chunk > win)
            chunk = win;
        if(chunk > packet_max)
            chunk = packet_max;
        if(chunk > sizeof(packet.payload) - (extended_type != 0 ? 12u : 8u))
            chunk = sizeof(packet.payload) - (extended_type != 0 ? 12u : 8u);

        /*
         * Writers still serialize packet encryption/sequence under send_lock,
         * but remote_window itself is updated atomically so a WINDOW_ADJUST
         * packet can advance it even while this sender is blocked in write().
         */
        pthread_mutex_lock(&s->send_lock);
        win = sshd_remote_window_load(s);
        if(win == 0) {
            pthread_mutex_unlock(&s->send_lock);
            if(sshd_wait_remote_window(s) != 0)
                break;
            continue;
        }

        if(chunk > win)
            chunk = win;
        if(chunk > packet_max)
            chunk = packet_max;
        if(chunk > sizeof(packet.payload) - (extended_type != 0 ? 12u : 8u))
            chunk = sizeof(packet.payload) - (extended_type != 0 ? 12u : 8u);

        memset(&packet, 0, sizeof(packet));
        packet.type = (extended_type == 0) ? SSH_MSG_CHANNEL_DATA : SSH_MSG_CHANNEL_EXTENDED_DATA;
        ssh_write_uint32(packet.payload + off, s->remote_channel);
        off += 4;
        if(extended_type != 0) {
            ssh_write_uint32(packet.payload + off, extended_type);
            off += 4;
        }
        if(write_ssh_string(packet.payload, sizeof(packet.payload), &off,
                    payload + sent, (uint32_t)chunk) < 0) {
            pthread_mutex_unlock(&s->send_lock);
            return -1;
        }
        packet.payload_len = off;
        if(ssh_packet_send_locked(s, &packet) < 0) {
            pthread_mutex_unlock(&s->send_lock);
            return -1;
        }
        sshd_remote_window_sub(s, (uint32_t)chunk);
        SSHD_DBG("channel_out sent pid=%d ext=%u chunk=%u win_left=%u\n",
                (int)s->child_pid, extended_type, (unsigned int)chunk,
                (unsigned int)sshd_remote_window_load(s));
        pthread_mutex_unlock(&s->send_lock);
        /* Replenish upload credit immediately after sending if owed */
        try_send_window_adjust(s);
        sent += chunk;
        chunk_index++;
    }

    if(sent == payload_len)
        return 0;
    sshd_set_error("session closing");
    return -1;
}

static __attribute__((unused)) int send_channel_request_exit_status(sshd_session_t* s, uint32_t status) {
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
    if(send_channel_status(s, SSH_MSG_CHANNEL_EOF) < 0) {
        return -1;
    }
    if(send_channel_status(s, SSH_MSG_CHANNEL_CLOSE) < 0) {
        return -1;
    }
    s->sent_close = 1;
    return 0;
}

static size_t internal_sftp_queue_bytes_locked(const sshd_session_t* s) {
    if(s == NULL || s->sftp_in_len <= s->sftp_in_off)
        return 0;
    return s->sftp_in_len - s->sftp_in_off;
}

static int internal_sftp_queue_append(sshd_session_t* s, const uint8_t* data,
        size_t len, size_t* pending_before) {
    size_t before;
    size_t remain;
    uint8_t* new_buf;
    size_t new_cap;

    if(s == NULL || (data == NULL && len > 0))
        return -1;
    pthread_mutex_lock(&s->sftp_lock);
    before = internal_sftp_queue_bytes_locked(s);
    if(pending_before != NULL)
        *pending_before = before;
    if(s->sftp_in_off > 0 && s->sftp_in_len > s->sftp_in_off) {
        remain = s->sftp_in_len - s->sftp_in_off;
        memmove(s->sftp_in, s->sftp_in + s->sftp_in_off, remain);
        s->sftp_in_len = remain;
        s->sftp_in_off = 0;
    } else if(s->sftp_in_off >= s->sftp_in_len) {
        s->sftp_in_off = 0;
        s->sftp_in_len = 0;
    }
    if(s->sftp_in_len + len > s->sftp_in_cap) {
        new_cap = s->sftp_in_cap ? s->sftp_in_cap : 4096;
        while(new_cap < s->sftp_in_len + len)
            new_cap *= 2;
        new_buf = (uint8_t*)realloc(s->sftp_in, new_cap);
        if(new_buf == NULL) {
            pthread_mutex_unlock(&s->sftp_lock);
            return -1;
        }
        s->sftp_in = new_buf;
        s->sftp_in_cap = new_cap;
    }
    if(len > 0) {
        memcpy(s->sftp_in + s->sftp_in_len, data, len);
        s->sftp_in_len += len;
    }
    pthread_cond_signal(&s->sftp_cv);
    pthread_mutex_unlock(&s->sftp_lock);
    return 0;
}

static int internal_sftp_emit(void* user, const uint8_t* data, size_t len) {
    sshd_session_t* s = (sshd_session_t*)user;
    return send_channel_data_packet(s, 0, data, len);
}

static void* internal_sftp_thread(void* arg) {
    internal_sftp_arg_t* a = (internal_sftp_arg_t*)arg;
    sshd_session_t* s = a->s;
    sftp_server_io_t io;
    sftp_server_t* srv;
    uint8_t buf[16384];
    int rc = 0;
    int need_close = 0;

    memset(&io, 0, sizeof(io));
    io.emit = internal_sftp_emit;
    io.user = s;
    srv = sftp_server_create(&io);
    if(srv == NULL)
        rc = -1;

    while(rc == 0 && !s->closing) {
        size_t pending_before;
        size_t chunk;
        uint32_t grant = 0;

        pthread_mutex_lock(&s->sftp_lock);
        while(!s->closing && internal_sftp_queue_bytes_locked(s) == 0 &&
                !s->sftp_in_eof) {
            pthread_cond_wait(&s->sftp_cv, &s->sftp_lock);
        }
        pending_before = internal_sftp_queue_bytes_locked(s);
        if(s->closing || (pending_before == 0 && s->sftp_in_eof)) {
            pthread_mutex_unlock(&s->sftp_lock);
            break;
        }
        chunk = pending_before;
        if(chunk > sizeof(buf))
            chunk = sizeof(buf);
        memcpy(buf, s->sftp_in + s->sftp_in_off, chunk);
        s->sftp_in_off += chunk;
        if(s->sftp_in_off >= s->sftp_in_len) {
            s->sftp_in_off = 0;
            s->sftp_in_len = 0;
        }
        pthread_mutex_unlock(&s->sftp_lock);

        grant = sftp_credit_on_drain(pending_before, chunk);
        if(grant > 0) {
            pthread_mutex_lock(&s->state_lock);
            s->win_adjust_owe += grant;
            pthread_mutex_unlock(&s->state_lock);
            if(try_send_window_adjust(s) < 0) {
                rc = -1;
                break;
            }
        }
        if(sftp_server_feed(srv, buf, chunk) < 0) {
            rc = -1;
            break;
        }
    }

    sftp_server_destroy(srv);
    pthread_mutex_lock(&s->state_lock);
    s->internal_sftp_done = 1;
    need_close = !s->sent_close;
    pthread_mutex_unlock(&s->state_lock);
    if(need_close) {
        (void)send_channel_request_exit_status(s, rc == 0 ? 0 : 1);
        (void)send_channel_eof_and_close(s);
    }
    pthread_mutex_lock(&s->state_lock);
    if(!s->closing) {
        s->closing = 1;
        pthread_cond_broadcast(&s->remote_window_cv);
    }
    pthread_mutex_unlock(&s->state_lock);
    session_close_socket(s);
    return NULL;
}

static int start_internal_sftp(sshd_session_t* s) {
    if(s->internal_sftp_active)
        return 0;
    s->internal_sftp_arg.s = s;
    s->internal_sftp_done = 0;
    s->sftp_in_eof = 0;
    s->internal_sftp_started = (pthread_create(&s->internal_sftp_tid, NULL,
                internal_sftp_thread, &s->internal_sftp_arg) == 0);
    if(!s->internal_sftp_started)
        return -1;
    s->internal_sftp_active = 1;
    return 0;
}

static void session_close_socket(sshd_session_t* s) {
    int fd = -1;

    pthread_mutex_lock(&s->state_lock);
    if(s->socket >= 0) {
        fd = s->socket;
        s->socket = -1;
    }
    pthread_mutex_unlock(&s->state_lock);
    if(fd >= 0)
        close(fd);
}

/*
 * Drain child stdout into the SSH channel. Non-blocking reads, capped by
 * remote window to avoid blocking on cond_wait in the single-threaded relay.
 * On EOF: send exit-status + channel eof/close, flag closing, close socket.
 */
static void drain_child_output(sshd_session_t* s) {
    if(s->internal_sftp_active || s->child_eof_seen)
        return;
    int fd = s->child_stdout[CHILD_STDOUT_READ];
    if(fd < 0)
        return;

    uint8_t buf[SSH_RELAY_READ_SIZE];
    while(!s->closing) {
        uint32_t win = sshd_remote_window_load(s);
        if(win == 0)
            break;
        /* Cap read size by window to avoid mid-send stall on window exhaustion */
        size_t max_read = s->pty_enabled ? (win / 2) : win;
        if(max_read == 0)
            break;
        if(max_read > sizeof(buf))
            max_read = sizeof(buf);
        /* PTY CRLF expansion can double payload; cap at 1024 to avoid truncation */
        if(s->pty_enabled && max_read > 1024)
            max_read = 1024;

        ssize_t n = read(fd, buf, max_read);
        if(n > 0) {
            if(send_channel_data_packet(s, 0, buf, (size_t)n) < 0)
                break;
            continue;
        }
        /*
         * n <= 0: EwokOS pipe semantics are inverted vs POSIX — a real EOF
         * (write-end fully closed) returns -1, while a non-blocking empty
         * read returns 0 (and may even be flipped to -1 by the libc wrapper
         * when a stale errno is set). Neither the return value nor errno can
         * reliably distinguish EOF from "no data yet".
         *
         * Primary EOF signal: pipe_hup (POLLHUP seen in wait_session_events)
         * means the write-end is closed — authoritative EOF regardless of
         * child liveness.
         *
         * Fallback: waitpid(WNOHANG) detects a zombie child (exited but not
         * yet reaped).  proc_get_uuid() is NOT reliable here because the
         * kernel only clears uuid in proc_funeral after the parent reaps.
         */
        if(!s->pipe_hup) {
            if(s->child_pid > 1) {
                pid_t wrc = waitpid(s->child_pid, NULL, WNOHANG);
                if(wrc == 0)
                    break;  /* child still running: temporarily no data */
                s->child_pid = -1;
            }
        }
        /* EOF: shell exited */
        s->child_eof_seen = 1;
        close_fd_if_valid(&s->child_stdout[CHILD_STDOUT_READ]);
        (void)send_channel_request_exit_status(s, 0);
        (void)send_channel_eof_and_close(s);
        pthread_mutex_lock(&s->state_lock);
        s->closing = 1;
        pthread_cond_broadcast(&s->remote_window_cv);
        pthread_mutex_unlock(&s->state_lock);
        session_close_socket(s);
        break;
    }
}

/*
 * Wait for socket and/or pipe readiness. Returns bitmask:
 *   bit 0: socket ready (POLLIN)
 *   bit 1: pipe ready (POLLIN)
 *   0: timeout
 *   <0: error
 * Excludes pipe from poll set when window exhausted or pipe invalid/EOF.
 */
#define SESSION_EVT_SOCKET  0x01
#define SESSION_EVT_PIPE    0x02
static int wait_session_events(sshd_session_t* s, int timeout_ms) {
    struct pollfd pfds[2];
    nfds_t n = 0;
    int sock = s->socket;
    int pfd = s->child_stdout[CHILD_STDOUT_READ];

    if(sock < 0) {
        errno = EBADF;
        return -1;
    }
    pfds[n].fd = sock;
    pfds[n].events = POLLIN | POLLHUP | POLLERR;
    n++;

    bool watch_pipe = !s->internal_sftp_active && !s->child_eof_seen &&
                      pfd >= 0 && sshd_remote_window_load(s) > 0;
    if(watch_pipe) {
        pfds[n].fd = pfd;
        pfds[n].events = POLLIN | POLLHUP | POLLERR;
        n++;
    }

    int rc = poll(pfds, n, timeout_ms);
    if(rc < 0)
        return (errno == EINTR) ? 0 : -1;
    if(rc == 0)
        return 0;  /* timeout */

    int result = 0;
    if(pfds[0].revents & (POLLIN | POLLHUP | POLLERR))
        result |= SESSION_EVT_SOCKET;
    if(n > 1) {
        if(pfds[1].revents & POLLHUP)
            s->pipe_hup = 1;
        /* Only signal pipe-ready when there is actual data (POLLIN).
         * Bare POLLHUP (write-end closed, buffer empty) must NOT
         * generate SESSION_EVT_PIPE, otherwise the relay loop spins
         * on the empty pipe and starves the socket of WINDOW_ADJUST
         * reads, deadlocking the channel.  pipe_hup is consumed by
         * drain_child_output for EOF detection. */
        if(pfds[1].revents & (POLLIN | POLLERR))
            result |= SESSION_EVT_PIPE;
    }
    return result;
}


static void close_child_bundle(int child_stdin[2], int child_stdout[2],
        int child_control[2]) {
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
    if(child_control[CHILD_CTRL_READ] >= 0) {
        close(child_control[CHILD_CTRL_READ]);
        child_control[CHILD_CTRL_READ] = -1;
    }
    if(child_control[CHILD_CTRL_WRITE] >= 0) {
        close(child_control[CHILD_CTRL_WRITE]);
        child_control[CHILD_CTRL_WRITE] = -1;
    }
}

static void become_login_process(sshd_session_t* s, const char* cmd) {
    int use_ready = child_needs_ready_handshake(cmd, 0);
    /*
     * Runs in the connection-handler process (e.g. pid 101). It replaces this
     * process image with the login shell/command so that the shell keeps the
     * same pid as the connection handler, instead of forking an extra process.
     * The SSH crypto relay is carried by the forked relay child instead.
     */
    close_fd_if_valid(&s->child_stdin[CHILD_STDIN_WRITE]);
    close_fd_if_valid(&s->child_stdout[CHILD_STDOUT_READ]);
    close_fd_if_valid(&s->close_notify[0]);
    close_fd_if_valid(&s->close_notify[1]);
    if(s->socket >= 0) {
        close(s->socket);
        s->socket = -1;
    }

    if(dup2(s->child_stdin[CHILD_STDIN_READ], 0) < 0 ||
            dup2(s->child_stdout[CHILD_STDOUT_WRITE], 1) < 0 ||
            dup2(s->child_stdout[CHILD_STDOUT_WRITE], 2) < 0 ||
            dup2(s->child_stdin[CHILD_STDIN_READ], VFS_BACKUP_FD0) < 0) {
        exit(2);
    }
    if(use_ready) {
        if(dup2(s->child_control[CHILD_CTRL_WRITE], VFS_BACKUP_FD1) < 0)
            exit(2);
    } else {
        close(VFS_BACKUP_FD1);
    }

    close_fd_if_valid(&s->child_stdin[CHILD_STDIN_READ]);
    close_fd_if_valid(&s->child_stdout[CHILD_STDOUT_WRITE]);
    close_fd_if_valid(&s->child_control[CHILD_CTRL_READ]);
    close_fd_if_valid(&s->child_control[CHILD_CTRL_WRITE]);

    if(setenv("CONSOLE_ID", "ssh") != 0 ||
            setenv("HOME", s->user_info.home) != 0 ||
            setenv("USER", s->user_info.user) != 0) {
        exit(3);
    }
    if(core_set_env("CONSOLE_ID", "ssh") != 0 ||
            core_set_env("HOME", s->user_info.home) != 0 ||
            core_set_env("USER", s->user_info.user) != 0) {
        exit(5);
    }
    if(setgid(s->user_info.gid) != 0 || setuid(s->user_info.uid) != 0)
        exit(4);

    if(cmd != NULL && cmd[0] != 0) {
        (void)exec_program_direct(cmd);
    }
    exit(-1);
}

static void close_child_fds(sshd_session_t* s) {
    close_child_bundle(s->child_stdin, s->child_stdout, s->child_control);
}

static int spawn_child_session(sshd_session_t* s, const char* command, int is_shell) {
    pid_t pid;
    int use_ready = child_needs_ready_handshake(command, is_shell);

    s->child_stdin[0] = s->child_stdin[1] = -1;
    s->child_stdout[0] = s->child_stdout[1] = -1;
    s->child_control[0] = s->child_control[1] = -1;

    if(pipe(s->child_stdin) != 0 || pipe(s->child_stdout) != 0 ||
            (use_ready && pipe(s->child_control) != 0)) {
        close_child_bundle(s->child_stdin, s->child_stdout, s->child_control);
        return -1;
    }

    /*
     * fork() runs on the connection handler's main thread, so it is safe. Keep
     * the parent as the SSH relay so the listener can continue to track and
     * reap the per-connection worker normally; run the login shell / sftp-server
     * in the child process connected through the pipe pair below.
     */
    pid = fork();
    if(pid < 0) {
        close_child_bundle(s->child_stdin, s->child_stdout, s->child_control);
        return -1;
    }

    if(pid == 0) {
        become_login_process(s, is_shell ? s->user_info.cmd : command);
        exit(-1); /* not reached */
    }

    s->child_pid = pid;
    s->child_is_sftp = use_ready;
    /* Relay child: keep the socket end, drop the shell-side pipe ends. */
    close_fd_if_valid(&s->child_stdin[CHILD_STDIN_READ]);
    close_fd_if_valid(&s->child_stdout[CHILD_STDOUT_WRITE]);
    close_fd_if_valid(&s->child_control[CHILD_CTRL_WRITE]);
    if(set_fd_nonblock(s->child_stdin[CHILD_STDIN_WRITE]) < 0) {
        close_child_fds(s);
        return -1;
    }
    if(set_fd_nonblock(s->child_stdout[CHILD_STDOUT_READ]) < 0) {
        close_child_fds(s);
        return -1;
    }
    if(use_ready) {
        if(wait_child_ready(s->child_control[CHILD_CTRL_READ]) < 0) {
            close_child_fds(s);
            if(pid > 1) {
                kill(pid, SIGKILL);
                ewok_waitpid(pid);
            }
            return -1;
        }
        close_fd_if_valid(&s->child_control[CHILD_CTRL_READ]);
    }

    /*
     * The login shell / sftp-server is our child process. The main thread runs
     * the single-task relay loop: drain child stdout (non-blocking, window-
     * capped) and dispatch inbound packets, parking on {socket, pipe} via
     * multi-fd poll. This eliminates the output_relay_thread, reducing ps from
     * 3 lines (shell + sshd + THRD) to 2 (shell + sshd).
     */
    s->child_started = 1;
    s->child_eof_seen = 0;
    return 0;
}

static int handle_channel_open(sshd_session_t* s, const ssh_packet_t* packet) {
    const uint8_t* channel_type;
    uint32_t type_len;
    size_t off = 0;

    if(read_ssh_string(packet->payload, packet->payload_len, &off, &channel_type, &type_len) < 0)
        return -1;
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

    s->local_channel = 0;
    s->local_window = SSH_LOCAL_WINDOW_SIZE;
    s->local_packet_max = SSH_CHANNEL_DATA_MAX;
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

    if(req_len == strlen("subsystem") && memcmp(req_type, "subsystem", req_len) == 0) {
        const uint8_t* sub;
        uint32_t sub_len;

        if(read_ssh_string(packet->payload, packet->payload_len, &off, &sub, &sub_len) < 0)
            return -1;
        if(sub_len == 0 || sub_len >= 64)
            goto subsystem_fail;

        if(sub_len == 4 && memcmp(sub, "sftp", 4) == 0) {
            if(!s->internal_sftp_active && start_internal_sftp(s) < 0)
                goto subsystem_fail;
        } else {
            goto subsystem_fail;
        }
        if(want_reply)
            return send_channel_status(s, SSH_MSG_CHANNEL_SUCCESS);
        return 0;

subsystem_fail:
        if(want_reply)
            return send_channel_status(s, SSH_MSG_CHANNEL_FAILURE);
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

/*
 * flush_pending_input: non-blocking drain of the pending stdin buffer into
 * child_stdin. Returns 0 if fully flushed, 1 if data remains.
 */
static int flush_pending_input(sshd_session_t* s) {
    int drained = 0;
    size_t pending_before = pending_input_bytes(s);

    while(s->pending_in_off < s->pending_in_len) {
        size_t chunk = s->pending_in_len - s->pending_in_off;
        int saved_errno;
        if(chunk > SSH_CHILD_STDIN_STEP)
            chunk = SSH_CHILD_STDIN_STEP;
        errno = 0;
        ssize_t n = write(s->child_stdin[CHILD_STDIN_WRITE],
                s->pending_in + s->pending_in_off,
                chunk);
        saved_errno = errno;
        if(n > 0) {
            s->pending_in_off += (size_t)n;
            drained += (int)n;
            continue;
        }
        if(n < 0 && saved_errno == 0)
            saved_errno = EAGAIN;
        if(n < 0 && saved_errno == EINTR)
            continue;
        if(n < 0 && saved_errno == EAGAIN)
            clear_fd_write_poll_event(s->child_stdin[CHILD_STDIN_WRITE]);
        else if(n < 0)
            log_fd_write_failure(s, s->child_stdin[CHILD_STDIN_WRITE], saved_errno);
        if(n < 0) {
            pthread_mutex_lock(&s->state_lock);
            SSHD_STALL_LOG("flush-stop err=%d chunk=%u pending=%u owe=%u",
                    saved_errno,
                    (unsigned int)chunk,
                    (unsigned int)(s->pending_in_len - s->pending_in_off),
                    (unsigned int)s->win_adjust_owe);
            pthread_mutex_unlock(&s->state_lock);
            // #region debug-point A:sshd-flush-stop
            if(s->child_is_sftp) {
                klog("sftp-klog:sshd-flush-stop pid=%d err=%d chunk=%u pending_before=%u pending_after=%u owe=%u\n",
                        (int)s->child_pid,
                        saved_errno,
                        (unsigned int)chunk,
                        (unsigned int)pending_before,
                        (unsigned int)pending_input_bytes(s),
                        (unsigned int)s->win_adjust_owe);
            }
            // #endregion
        }
        /* EAGAIN or error — stop for now */
        SSHD_DBG("flush_pending stop pid=%d n=%d err=%d off=%u len=%u chunk=%u\n",
                (int)s->child_pid, (int)n, saved_errno,
                (unsigned int)s->pending_in_off,
                (unsigned int)s->pending_in_len,
                (unsigned int)chunk);
        errno = saved_errno;
        break;
    }
    if(drained > 0) {
        uint32_t grant = s->child_is_sftp ?
                sftp_credit_on_drain(pending_before, (size_t)drained) :
                (uint32_t)drained;
        // #region debug-point B:sshd-flush-drained
        if(s->child_is_sftp && (pending_before >= 32768 || drained >= 32768 || grant >= 32768)) {
            klog("sftp-klog:sshd-flush-drained pid=%d drained=%u grant=%u pending_before=%u pending_after=%u\n",
                    (int)s->child_pid,
                    (unsigned int)drained,
                    (unsigned int)grant,
                    (unsigned int)pending_before,
                    (unsigned int)pending_input_bytes(s));
        }
        // #endregion
        pthread_mutex_lock(&s->state_lock);
        s->win_adjust_owe += grant;
        if(drained >= 4096 || grant >= 4096 || s->win_adjust_owe >= SSH_WINDOW_ADJUST_MIN) {
            SSHD_STALL_LOG("flush-drained drained=%u off=%u len=%u owe=%u",
                    (unsigned int)drained,
                    (unsigned int)s->pending_in_off,
                    (unsigned int)s->pending_in_len,
                    (unsigned int)s->win_adjust_owe);
        }
        pthread_mutex_unlock(&s->state_lock);
        if(try_send_window_adjust(s) < 0)
            return -1;
    }
    if(s->pending_in_off >= s->pending_in_len) {
        s->pending_in_off = 0;
        s->pending_in_len = 0;
        return 0;
    }
    return 1;
}

static int wait_child_stdin_writable(sshd_session_t* s) {
    struct pollfd pfds[3];
    nfds_t nfds = 0;

    if(s->child_stdin[CHILD_STDIN_WRITE] < 0) {
        errno = EPIPE;
        return -1;
    }

    while(!s->closing) {
        /* Send WINDOW_ADJUST if owed, prevent peer window stall during backpressure */
        try_send_window_adjust(s);

        nfds = 0;
        memset(pfds, 0, sizeof(pfds));
        pfds[nfds].fd = s->child_stdin[CHILD_STDIN_WRITE];
        pfds[nfds].events = POLLOUT | POLLHUP | POLLERR | POLLNVAL;
        nfds++;
        pfds[nfds].fd = s->close_notify[CHILD_CTRL_READ];
        pfds[nfds].events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
        nfds++;
        if(s->socket >= 0) {
            pfds[nfds].fd = s->socket;
            pfds[nfds].events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
            nfds++;
        }

        if(poll(pfds, nfds, SSH_CHILD_STDIN_POLL_MS) < 0) {
            if(errno == EINTR)
                continue;
            return -1;
        }
        if((pfds[1].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) != 0) {
            errno = EINTR;
            return -1;
        }
        if((pfds[0].revents & POLLNVAL) != 0) {
            errno = EBADF;
            return -1;
        }
        if((pfds[0].revents & POLLERR) != 0) {
            errno = EIO;
            return -1;
        }
        if((pfds[0].revents & POLLHUP) != 0) {
            errno = EPIPE;
            return -1;
        }
        if((pfds[0].revents & POLLOUT) != 0) {
            SSHD_DBG("wait_child_stdin writable pid=%d pending=%u/%u\n",
                    (int)s->child_pid,
                    (unsigned int)s->pending_in_off,
                    (unsigned int)s->pending_in_len);
            return 0;
        }
        if(nfds > 2 && (pfds[2].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) != 0) {
            SSHD_DBG("wait_child_stdin socket_event pid=%d revents=%x pending=%u/%u\n",
                    (int)s->child_pid, pfds[2].revents,
                    (unsigned int)s->pending_in_off,
                    (unsigned int)s->pending_in_len);
            return 1;
        }
    }

    errno = EINTR;
    return -1;
}

static void maybe_close_child_stdin(sshd_session_t* s) {
    if(s->internal_sftp_active) {
        pthread_mutex_lock(&s->sftp_lock);
        if((s->peer_eof || s->peer_close) && internal_sftp_queue_bytes_locked(s) == 0) {
            s->sftp_in_eof = 1;
            pthread_cond_signal(&s->sftp_cv);
        }
        pthread_mutex_unlock(&s->sftp_lock);
        return;
    }
    if((s->peer_eof || s->peer_close) &&
            s->pending_in_len == s->pending_in_off &&
            s->child_stdin[CHILD_STDIN_WRITE] >= 0) {
        close(s->child_stdin[CHILD_STDIN_WRITE]);
        s->child_stdin[CHILD_STDIN_WRITE] = -1;
    }
}

/*
 * queue_input: append data to the pending stdin buffer. Grows as needed.
 */
static int queue_input(sshd_session_t* s, const uint8_t* data, size_t len) {
    size_t avail;

    /* Compact buffer if needed */
    if(s->pending_in_off > 0 && s->pending_in_len > 0) {
        size_t remain = s->pending_in_len - s->pending_in_off;
        memmove(s->pending_in, s->pending_in + s->pending_in_off, remain);
        s->pending_in_len = remain;
        s->pending_in_off = 0;
    }

    avail = s->pending_in_cap - s->pending_in_len;
    if(avail < len) {
        size_t new_cap = s->pending_in_cap + len + 4096;
        uint8_t* new_buf = (uint8_t*)realloc(s->pending_in, new_cap);
        if(new_buf == NULL)
            return -1;
        s->pending_in = new_buf;
        s->pending_in_cap = new_cap;
    }
    memcpy(s->pending_in + s->pending_in_len, data, len);
    s->pending_in_len += len;
    return 0;
}

/*
 * try_send_window_adjust: send deferred WINDOW_ADJUST to the client.
 *
 * The inbound path must never block behind the output thread while it is
 * streaming channel data, otherwise we can stop reading new client packets
 * right when more upload credit needs to be granted. If send_lock is busy we
 * simply leave win_adjust_owe queued for a later attempt.
 */
static int try_send_window_adjust(sshd_session_t* s) {
    ssh_packet_t packet;
    size_t off = 0;
    uint32_t delta;
    uint32_t local_window;

    pthread_mutex_lock(&s->state_lock);
    delta = s->win_adjust_owe;
    local_window = s->local_window;
    pthread_mutex_unlock(&s->state_lock);
    if(delta == 0)
        return 0;

    /*
     * Coalesce upload credit instead of sending tiny WINDOW_ADJUST packets for
     * every pipe drain fragment (for example 4032 + 128 around the shm-pipe
     * boundary). Under heavy SFTP upload this control traffic competes with the
     * output relay thread for send_lock and can starve 21-byte SSH_FXP_STATUS
     * replies long enough for the client to time out.
     *
     * Keep the advertised receive window large enough that the client can keep
     * streaming past 32KB/64KB boundaries without visibly pausing, but still
     * batch replenishment into reasonably sized WINDOW_ADJUST packets while the
     * window remains healthy.
     */
    if(local_window > SSH_LOCAL_WINDOW_LOW &&
            delta < SSH_WINDOW_ADJUST_MIN) {
        if(delta >= 16384) {
            SSHD_STALL_LOG("adjust-held delta=%u local=%u low=%u min=%u",
                    (unsigned int)delta,
                    (unsigned int)local_window,
                    (unsigned int)SSH_LOCAL_WINDOW_LOW,
                    (unsigned int)SSH_WINDOW_ADJUST_MIN);
        }
        return 0;
    }

    if(pthread_mutex_trylock(&s->send_lock) != 0)
    {
        if(delta >= 16384) {
            SSHD_STALL_LOG("adjust-sendlock-busy delta=%u local=%u remote=%u",
                    (unsigned int)delta,
                    (unsigned int)local_window,
                    (unsigned int)sshd_remote_window_load(s));
        }
        return 0;
    }

    pthread_mutex_lock(&s->state_lock);
    delta = s->win_adjust_owe;
    local_window = s->local_window;
    pthread_mutex_unlock(&s->state_lock);
    if(delta == 0) {
        pthread_mutex_unlock(&s->send_lock);
        return 0;
    }
    if(local_window > SSH_LOCAL_WINDOW_LOW &&
            delta < SSH_WINDOW_ADJUST_MIN) {
        pthread_mutex_unlock(&s->send_lock);
        if(delta >= 16384) {
            SSHD_STALL_LOG("adjust-held-locked delta=%u local=%u low=%u min=%u",
                    (unsigned int)delta,
                    (unsigned int)local_window,
                    (unsigned int)SSH_LOCAL_WINDOW_LOW,
                    (unsigned int)SSH_WINDOW_ADJUST_MIN);
        }
        return 0;
    }
    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_CHANNEL_WINDOW_ADJUST;
    ssh_write_uint32(packet.payload + off, s->remote_channel);
    off += 4;
    ssh_write_uint32(packet.payload + off, delta);
    off += 4;
    packet.payload_len = off;

    if(ssh_packet_send_locked(s, &packet) < 0) {
        pthread_mutex_unlock(&s->send_lock);
        return -1;
    }
    // #region debug-point C:sshd-window-adjust
    if(s->child_is_sftp && delta >= 32768) {
        klog("sftp-klog:sshd-adjust-send pid=%d delta=%u local_before=%u remote=%u\n",
                (int)s->child_pid,
                (unsigned int)delta,
                (unsigned int)local_window,
                (unsigned int)sshd_remote_window_load(s));
    }
    // #endregion
    pthread_mutex_lock(&s->state_lock);
    s->local_window += delta;
    s->win_adjust_owe -= delta;
    SSHD_STALL_LOG("adjust-sent delta=%u local=%u owe=%u remote=%u",
            (unsigned int)delta,
            (unsigned int)s->local_window,
            (unsigned int)s->win_adjust_owe,
            (unsigned int)s->remote_window);
    SSHD_DBG("window_adjust sent pid=%d delta=%u local=%u owe=%u\n",
            (int)s->child_pid, delta,
            (unsigned int)s->local_window,
            (unsigned int)s->win_adjust_owe);
    pthread_mutex_unlock(&s->state_lock);
    pthread_mutex_unlock(&s->send_lock);
    return 0;
}

static int handle_channel_data(sshd_session_t* s, const ssh_packet_t* packet) {
    const uint8_t* data;
    uint32_t data_len;
    uint32_t channel_id;
    size_t off = 0;
    size_t pending_before;
    uint32_t enqueue_credit = 0;

    if(!s->child_started && !s->internal_sftp_active)
        return -1;
    channel_id = ssh_read_uint32(packet->payload + off);
    off += 4;
    if(channel_id != s->local_channel)
        return -1;
    if(read_ssh_string(packet->payload, packet->payload_len, &off, &data, &data_len) < 0)
        return -1;
    if(data_len == 0)
        return 0;

    if(s->internal_sftp_active) {
        pending_before = 0;
        if(internal_sftp_queue_append(s, data, data_len, &pending_before) < 0)
            return -1;
        enqueue_credit = sftp_credit_on_enqueue(pending_before, data_len);
    } else {
        pending_before = pending_input_bytes(s);
        if(queue_input(s, data, data_len) < 0)
            return -1;
        if(s->child_is_sftp)
            enqueue_credit = sftp_credit_on_enqueue(pending_before, data_len);
        if(flush_pending_input(s) < 0)
            return -1;
    }

    /*
     * local_window now tracks bytes accepted from the SSH socket but not yet
     * returned to the peer. Upload credit is replenished only after
     * flush_pending_input() actually drains data into child stdin, so sshd does
     * not over-advertise capacity while the child process is backpressured.
     */
    pthread_mutex_lock(&s->state_lock);
    if(s->local_window >= data_len)
        s->local_window -= data_len;
    else
        s->local_window = 0;
    s->win_adjust_owe += enqueue_credit;
    if(data_len >= 16384 || s->local_window <= 65536 ||
            s->pending_in_len > s->pending_in_off) {
        SSHD_STALL_LOG("channel-data len=%u local=%u pending=%u owe=%u",
                (unsigned int)data_len,
                (unsigned int)s->local_window,
                (unsigned int)(s->pending_in_len - s->pending_in_off),
                (unsigned int)s->win_adjust_owe);
    }
    pthread_mutex_unlock(&s->state_lock);
    if(enqueue_credit > 0 && try_send_window_adjust(s) < 0)
        return -1;

    if(s->pending_in_len > s->pending_in_off || data_len >= SSH_LOCAL_WINDOW_LOW) {
        SSHD_DBG("channel_data pid=%d data=%u pending=%u/%u\n",
                (int)s->child_pid, data_len,
                (unsigned int)s->pending_in_off,
                (unsigned int)s->pending_in_len);
    }
    return 0;
}

static int dispatch_session_packet(sshd_session_t* s, const ssh_packet_t* packet) {
    switch(packet->type) {
    case SSH_MSG_GLOBAL_REQUEST:
        return send_request_failure(s);
    case SSH_MSG_CHANNEL_OPEN:
        return handle_channel_open(s, packet);
    case SSH_MSG_CHANNEL_REQUEST:
        return handle_channel_request(s, packet);
    case SSH_MSG_CHANNEL_DATA:
        return handle_channel_data(s, packet);
    case SSH_MSG_CHANNEL_WINDOW_ADJUST:
        if(packet->payload_len >= 8) {
            uint32_t delta = ssh_read_uint32(packet->payload + 4);
            sshd_remote_window_add(s, delta);
        }
        return 0;
    case SSH_MSG_CHANNEL_EOF:
        s->peer_eof = 1;
        maybe_close_child_stdin(s);
        return 0;
    case SSH_MSG_CHANNEL_CLOSE:
        s->peer_close = 1;
        maybe_close_child_stdin(s);
        return 0;
    default:
        break;
    }
    return 0;
}

/*
 * Single-task relay loop. The main thread parks on {socket, pipe} via multi-fd
 * poll (or {socket} only when window exhausted or pipe invalid/EOF), drains
 * child stdout into the SSH channel, and dispatches inbound packets. This
 * eliminates the output_relay_thread, reducing ps from 3 lines (shell + sshd +
 * THRD) to 2 (shell + sshd), matching the telnetd merge.
 *
 * Teardown is hang-free:
 *  - Shell exits: drain_child_output sees EOF, sends exit-status + channel
 *    eof/close, flags closing, closes socket → loop exits.
 *  - Client closes: loop exits; we flag closing, close pipes, kill child.
 */
static int handle_session_packets(sshd_session_t* s) {
    ssh_packet_t packet;
    int rc = 0;

    while(!s->sent_close && !s->closing) {
        /* Send deferred WINDOW_ADJUST if owed */
        try_send_window_adjust(s);

        /*
         * If part of a client packet is still buffered for the child process,
         * keep pumping it into stdin before blocking for more SSH traffic.
         * Otherwise uploads can deadlock when the client pauses waiting for an
         * SFTP reply while the tail of the WRITE request is still stranded in
         * pending_in.
         */
        if(!s->internal_sftp_active && s->pending_in_len > s->pending_in_off) {
            int flush_rc = flush_pending_input(s);
            int wait_rc;
            if(flush_rc != 0) {
                wait_rc = wait_child_stdin_writable(s);
                if(wait_rc < 0) {
                    rc = -1;
                    break;
                }
                if(wait_rc == 0) {
                    /* Child stdin is writable again, loop back to flush it */
                    continue;
                }
                /* wait_rc == 1: Socket has data but child stdin is still blocked.
                 * We MUST fall through to ssh_packet_receive() to read the socket,
                 * otherwise we'll livelock and miss WINDOW_ADJUST packets! */
            }
        }
        maybe_close_child_stdin(s);

        /* Drain shell output (non-blocking, window-capped) */
        drain_child_output(s);
        if(s->closing)
            break;

        /* Wait for socket and/or pipe readiness */
        int ev = wait_session_events(s, 1000);
        if(ev < 0) {
            rc = -1;
            break;
        }
        if(ev == 0)
            continue;  /* timeout, re-loop */

        /*
         * Process socket even when the pipe also had data.  The old
         * "if(PIPE) continue" skipped the socket entirely, starving
         * WINDOW_ADJUST reads whenever the pipe kept producing events
         * (e.g. POLLHUP after pipeline exit), deadlocking the channel.
         * Pipe data is drained at the top of the next iteration.
         */
        if(ev & SESSION_EVT_SOCKET) {
            if(ssh_packet_receive(s, &packet) < 0) {
                rc = -1;
                break;
            }
            if(dispatch_session_packet(s, &packet) < 0) {
                rc = -1;
                break;
            }
        }
    }

    pthread_mutex_lock(&s->state_lock);
    s->closing = 1;
    pthread_cond_broadcast(&s->remote_window_cv);
    pthread_mutex_unlock(&s->state_lock);
    pthread_mutex_lock(&s->sftp_lock);
    s->sftp_in_eof = 1;
    pthread_cond_broadcast(&s->sftp_cv);
    pthread_mutex_unlock(&s->sftp_lock);
    session_signal_close(s);
    session_close_socket(s);
    close_fd_if_valid(&s->child_stdin[CHILD_STDIN_WRITE]);
    close_fd_if_valid(&s->child_stdout[CHILD_STDOUT_READ]);
    if(s->internal_sftp_started)
        pthread_join(s->internal_sftp_tid, NULL);
    /*
     * Kill the login process (sftp-server / shell) which is our parent.
     * In EwokOS closing the stdin pipe write-end does not reliably unblock
     * a reader, so the child process may still be stuck in read(). Sending
     * SIGKILL ensures it terminates and does not linger.
     */
    if(s->child_pid > 1) {
        kill(s->child_pid, SIGKILL);
        ewok_waitpid(s->child_pid);
    }
    s->child_pid = -1;
    return rc;
}

static int session_init(sshd_session_t* s, int sock) {
    memset(s, 0, sizeof(*s));
    s->socket = sock;
    strncpy(s->server_version, SSH_SERVER_VERSION, sizeof(s->server_version) - 1);
    s->terminal_width = 80;
    s->terminal_height = 24;
    s->remote_packet_max = 16384;
    s->local_packet_max = SSH_CHANNEL_DATA_MAX;
    s->child_stdin[0] = s->child_stdin[1] = -1;
    s->child_stdout[0] = s->child_stdout[1] = -1;
    s->child_control[0] = s->child_control[1] = -1;
    s->close_notify[0] = s->close_notify[1] = -1;
    s->child_is_sftp = 0;
    if(pipe(s->close_notify) != 0)
        return -1;
    /*
     * Keep the session socket non-blocking so the output relay never sleeps
     * forever inside a blocking write() while holding send_lock. The send path
     * already retries EAGAIN via wait_session_fd_ready(), which is safer than
     * trusting the kernel/socket stack to always wake a blocking writer.
     */
    if(set_fd_nonblock(s->socket) != 0)
        return -1;
    pthread_mutex_init(&s->state_lock, NULL);
    pthread_mutex_init(&s->send_lock, NULL);
    pthread_mutex_init(&s->sftp_lock, NULL);
    pthread_cond_init(&s->remote_window_cv, NULL);
    pthread_cond_init(&s->sftp_cv, NULL);
    return 0;
}

static void session_destroy(sshd_session_t* s) {
    pthread_mutex_lock(&s->state_lock);
    s->closing = 1;
    pthread_cond_broadcast(&s->remote_window_cv);
    pthread_mutex_unlock(&s->state_lock);
    pthread_mutex_lock(&s->sftp_lock);
    s->sftp_in_eof = 1;
    pthread_cond_broadcast(&s->sftp_cv);
    pthread_mutex_unlock(&s->sftp_lock);
    session_signal_close(s);
    session_close_socket(s);
    close_child_fds(s);
    if(s->internal_sftp_started)
        pthread_join(s->internal_sftp_tid, NULL);
    if(s->child_pid > 1) {
        kill(s->child_pid, SIGKILL);
        ewok_waitpid(s->child_pid);
        s->child_pid = -1;
    }
    close_fd_if_valid(&s->close_notify[0]);
    if(s->socket >= 0) {
        close(s->socket);
    }
    if(s->pending_in != NULL) {
        free(s->pending_in);
        s->pending_in = NULL;
    }
    if(s->sftp_in != NULL) {
        free(s->sftp_in);
        s->sftp_in = NULL;
    }
    pthread_cond_destroy(&s->sftp_cv);
    pthread_cond_destroy(&s->remote_window_cv);
    pthread_mutex_destroy(&s->sftp_lock);
    pthread_mutex_destroy(&s->send_lock);
    pthread_mutex_destroy(&s->state_lock);
}

static int open_server_socket(int port) {
    struct sockaddr_in addr;

    while(true) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(sock < 0) {
            proc_usleep(200000);
            continue;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_un.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0 && listen(sock, 5) == 0) {
            return sock;
        }
        close(sock);
        proc_usleep(200000);
    }
}

static void track_worker_pid(pid_t pid) {
    for(size_t i = 0; i < SSHD_MAX_TRACKED_WORKERS; ++i) {
        if(g_tracked_workers[i] == 0) {
            g_tracked_workers[i] = pid;
            return;
        }
    }
}

static void reap_finished_workers(void) {
    int status;

    for(size_t i = 0; i < SSHD_MAX_TRACKED_WORKERS; ++i) {
        pid_t pid = g_tracked_workers[i];
        pid_t rc;

        if(pid <= 0)
            continue;
        rc = waitpid(pid, &status, WNOHANG);
        SSHD_DBG("reap worker pid=%d rc=%d err=%d\n", (int)pid, (int)rc, errno);
        if(rc == pid) {
            g_tracked_workers[i] = 0;
            continue;
        }
        if(rc < 0 && errno == ECHILD) {
            g_tracked_workers[i] = 0;
        }
    }
}

static int init_host_key(void) {
    WC_RNG rng;
    RsaKey wc_rsa;
    uint8_t der[2048];
    int ret;

    ERR_clear_error();
    if(load_host_key_from_file(SSH_HOST_KEY_PATH) == 0) {
        return 0;
    }

    g_host_rsa = RSA_new();
    if(g_host_rsa == NULL) {
        return -1;
    }
    memset(&rng, 0, sizeof(rng));
    memset(&wc_rsa, 0, sizeof(wc_rsa));
    ret = wc_InitRng(&rng);
    if(ret != 0)
        return -1;
    ret = wc_InitRsaKey(&wc_rsa, NULL);
    if(ret != 0) {
        wc_FreeRng(&rng);
        return -1;
    }
    ret = wc_MakeRsaKey(&wc_rsa, 2048, RSA_F4, &rng);
    if(ret != 0) {
        wc_FreeRsaKey(&wc_rsa);
        wc_FreeRng(&rng);
        return -1;
    }
    ret = wc_RsaKeyToDer(&wc_rsa, der, sizeof(der));
    if(ret <= 0) {
        wc_FreeRsaKey(&wc_rsa);
        wc_FreeRng(&rng);
        return -1;
    }
    if(wolfSSL_RSA_LoadDer(g_host_rsa, der, ret) != 1) {
        wc_FreeRsaKey(&wc_rsa);
        wc_FreeRng(&rng);
        return -1;
    }
    if(save_host_key_to_file(SSH_HOST_KEY_PATH, der, ret) == 0) {
    }
    else {
    }
    wc_FreeRsaKey(&wc_rsa);
    wc_FreeRng(&rng);
    if(build_hostkey_blob_from_rsa(g_host_rsa) < 0)
        return -1;
    return 0;
}

static int serve_client(int sock) {
    sshd_session_t* session;
    int ret = -1;

    SSHD_DBG("serve_client start sock=%d\n", sock);
    session = (sshd_session_t*)calloc(1, sizeof(*session));
    if(session == NULL) {
        close(sock);
        return -1;
    }

    if(session_init(session, sock) < 0) {
        close(sock);
        free(session);
        return -1;
    }

    if(send_banner(session) < 0) {
        SSHD_DBG("serve_client send_banner fail sock=%d err=%s\n", sock,
                g_error[0] ? g_error : "unknown");
        goto out;
    }
    SSHD_DBG("serve_client banner_sent sock=%d\n", sock);
    if(receive_banner(session) < 0) {
        SSHD_DBG("serve_client recv_banner fail sock=%d err=%s\n", sock,
                g_error[0] ? g_error : "unknown");
        goto out;
    }
    SSHD_DBG("serve_client banner_recv sock=%d client=%s\n", sock, session->client_version);
    if(receive_kexinit(session) < 0)
        goto out;
    if(send_kexinit(session) < 0)
        goto out;
    if(do_key_exchange(session) < 0)
        goto out;
    if(handle_service_and_auth(session) < 0)
        goto out;
    if(handle_session_packets(session) < 0)
        goto out;

    ret = 0;

out:
    if(ret < 0 && session->socket >= 0)
        send_disconnect(session, SSH_DISCONNECT_BY_APPLICATION, g_error[0] ? g_error : "sshd error");
    SSHD_DBG("serve_client done sock=%d ret=%d err=%s\n", sock, ret,
            g_error[0] ? g_error : "none");
    session_destroy(session);
    return ret;
}

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : SSH_DEFAULT_PORT;
    int server_sock;

    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
            OPENSSL_INIT_ADD_ALL_CIPHERS |
            OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);
    srand(42);

    if(init_host_key() < 0) {
        return -1;
    }

    server_sock = open_server_socket(port);
    while(true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        reap_finished_workers();
        SSHD_DBG("listener before accept\n");
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if(client_sock < 0) {
            proc_usleep(10000);
            continue;
        }
        SSHD_DBG("listener accepted sock=%d\n", client_sock);

        int pid = fork();
        if(pid < 0) {
            close(client_sock);
            continue;
        }
        if(pid == 0) {
            close(server_sock);
            serve_client(client_sock);
            exit(0);
        }
        SSHD_DBG("listener forked pid=%d sock=%d\n", pid, client_sock);
        track_worker_pid(pid);
        close(client_sock);
    }

    return 0;
}
