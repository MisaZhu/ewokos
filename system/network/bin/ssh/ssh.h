#ifndef SSH_H
#define SSH_H

#include <stdint.h>
#include <stddef.h>

/* SSH Protocol Constants */
#define SSH_DEFAULT_PORT        22
#define SSH_MAX_PACKET_SIZE     32768
#define SSH_VERSION_STRING      "SSH-2.0-OpenSSH_8.0"

/* SSH Message Numbers */
#define SSH_MSG_DISCONNECT                      1
#define SSH_MSG_IGNORE                          2
#define SSH_MSG_UNIMPLEMENTED                   3
#define SSH_MSG_DEBUG                           4
#define SSH_MSG_SERVICE_REQUEST                 5
#define SSH_MSG_SERVICE_ACCEPT                  6
#define SSH_MSG_KEXINIT                         20
#define SSH_MSG_NEWKEYS                         21
#define SSH_MSG_KEXDH_INIT                      30
#define SSH_MSG_KEXDH_REPLY                     31
#define SSH_MSG_USERAUTH_REQUEST                50
#define SSH_MSG_USERAUTH_FAILURE                51
#define SSH_MSG_USERAUTH_SUCCESS                52
#define SSH_MSG_USERAUTH_BANNER                 53
#define SSH_MSG_GLOBAL_REQUEST                  80
#define SSH_MSG_REQUEST_SUCCESS                 81
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

/* SSH Disconnect Reasons */
#define SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT      1
#define SSH_DISCONNECT_PROTOCOL_ERROR                   2
#define SSH_DISCONNECT_KEY_EXCHANGE_FAILED              3
#define SSH_DISCONNECT_RESERVED                         4
#define SSH_DISCONNECT_MAC_ERROR                        5
#define SSH_DISCONNECT_COMPRESSION_ERROR                6
#define SSH_DISCONNECT_SERVICE_NOT_AVAILABLE            7
#define SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED   8
#define SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE          9
#define SSH_DISCONNECT_CONNECTION_LOST                  10
#define SSH_DISCONNECT_BY_APPLICATION                   11
#define SSH_DISCONNECT_TOO_MANY_CONNECTIONS             12
#define SSH_DISCONNECT_AUTH_CANCELLED_BY_USER           13
#define SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE   14
#define SSH_DISCONNECT_ILLEGAL_USER_NAME                15

/* SSH Channel Types */
#define SSH_CHANNEL_SESSION         "session"
#define SSH_CHANNEL_DIRECT_TCPIP    "direct-tcpip"
#define SSH_CHANNEL_FORWARDED_TCPIP "forwarded-tcpip"
#define SSH_CHANNEL_X11             "x11"

/* SSH Authentication Methods */
#define SSH_AUTH_NONE               "none"
#define SSH_AUTH_PASSWORD           "password"
#define SSH_AUTH_PUBLICKEY          "publickey"
#define SSH_AUTH_HOSTBASED          "hostbased"
#define SSH_AUTH_KEYBOARD_INTERACTIVE "keyboard-interactive"

/* SSH States */
typedef enum {
    SSH_STATE_INIT,
    SSH_STATE_BANNER_SENT,
    SSH_STATE_BANNER_RECEIVED,
    SSH_STATE_KEXINIT_SENT,
    SSH_STATE_KEXINIT_RECEIVED,
    SSH_STATE_KEXDH_SENT,
    SSH_STATE_NEWKEYS_SENT,
    SSH_STATE_NEWKEYS_RECEIVED,
    SSH_STATE_SERVICE_REQUEST_SENT,
    SSH_STATE_SERVICE_ACCEPT_RECEIVED,
    SSH_STATE_AUTH_REQUEST_SENT,
    SSH_STATE_AUTH_SUCCESS,
    SSH_STATE_CHANNEL_OPEN,
    SSH_STATE_CONNECTED,
    SSH_STATE_DISCONNECTED
} ssh_state_t;

/* SSH Session Structure */
typedef struct ssh_session {
    int socket;
    ssh_state_t state;
    
    /* Version strings */
    char client_version[256];
    char server_version[256];
    
    /* KEX data */
    uint8_t client_kexinit[1024];
    uint8_t server_kexinit[1024];
    size_t client_kexinit_len;
    size_t server_kexinit_len;
    
    /* Session ID */
    uint8_t session_id[32];
    size_t session_id_len;
    
    /* Keys */
    uint8_t enc_key_client[32];
    uint8_t enc_key_server[32];
    uint8_t int_key_client[32];
    uint8_t int_key_server[32];
    
    /* Authentication */
    char username[64];
    char password[64];
    int auth_attempts;
    
    /* Channel data */
    uint32_t channel_local;
    uint32_t channel_remote;
    uint32_t window_local;
    uint32_t window_remote;
    uint32_t packet_seq_client;
    uint32_t packet_seq_server;
    
    /* Terminal settings */
    int terminal_width;
    int terminal_height;
    
    /* Flags */
    int compression_enabled;
    int encryption_enabled;
    int authenticated;
    
} ssh_session_t;

/* SSH Packet Structure */
typedef struct ssh_packet {
    uint32_t length;
    uint8_t padding_length;
    uint8_t type;
    uint8_t payload[SSH_MAX_PACKET_SIZE];
    size_t payload_len;
} ssh_packet_t;

/* Function Prototypes */

/* Session Management */
ssh_session_t *ssh_session_new(void);
void ssh_session_free(ssh_session_t *session);
int ssh_connect(ssh_session_t *session, const char *hostname, int port);
int ssh_disconnect(ssh_session_t *session);
int ssh_is_connected(ssh_session_t *session);

/* Protocol Handshake */
int ssh_send_banner(ssh_session_t *session);
int ssh_receive_banner(ssh_session_t *session);
int ssh_send_kexinit(ssh_session_t *session);
int ssh_receive_kexinit(ssh_session_t *session);
int ssh_handle_kex(ssh_session_t *session);

/* Authentication */
int ssh_userauth_password(ssh_session_t *session, const char *username, const char *password);
int ssh_userauth_none(ssh_session_t *session, const char *username);
int ssh_userauth_list(ssh_session_t *session, const char *username);

/* Channel Management */
int ssh_channel_open_session(ssh_session_t *session);
int ssh_channel_request_pty(ssh_session_t *session, const char *term);
int ssh_channel_request_shell(ssh_session_t *session);
int ssh_channel_request_exec(ssh_session_t *session, const char *command);
int ssh_channel_send_data(ssh_session_t *session, const void *data, size_t len);
int ssh_channel_receive_data(ssh_session_t *session, void *data, size_t len);
int ssh_channel_close(ssh_session_t *session);

/* Packet I/O */
int ssh_packet_send(ssh_session_t *session, ssh_packet_t *packet);
int ssh_packet_receive(ssh_session_t *session, ssh_packet_t *packet);
int ssh_packet_read(ssh_session_t *session, uint8_t *buf, size_t len);
int ssh_packet_write(ssh_session_t *session, const uint8_t *buf, size_t len);

/* Utility Functions */
uint32_t ssh_read_uint32(const uint8_t *buf);
void ssh_write_uint32(uint8_t *buf, uint32_t val);
uint8_t *ssh_read_string(const uint8_t *buf, uint32_t *len);
void ssh_write_string(uint8_t *buf, const uint8_t *str, uint32_t len);
int ssh_read_mpint(const uint8_t *buf, uint8_t *out, size_t *out_len);
void ssh_write_mpint(uint8_t *buf, const uint8_t *val, size_t len);

/* Crypto Functions */
int ssh_generate_key_pair(uint8_t *public_key, size_t *public_len,
                           uint8_t *private_key, size_t *private_len);
int ssh_compute_shared_secret(const uint8_t *client_public, size_t client_len,
                               const uint8_t *server_public, size_t server_len,
                               uint8_t *shared_secret, size_t *secret_len);
int ssh_derive_keys(ssh_session_t *session, const uint8_t *shared_secret, size_t secret_len);
int ssh_encrypt_packet(ssh_session_t *session, uint8_t *packet, size_t *len);
int ssh_decrypt_packet(ssh_session_t *session, uint8_t *packet, size_t *len);

/* Hash Functions */
int ssh_hash_sha256(const uint8_t *data, size_t len, uint8_t *hash);
int ssh_hash_sha1(const uint8_t *data, size_t len, uint8_t *hash);
int ssh_hmac_sha256(const uint8_t *key, size_t key_len,
                     const uint8_t *data, size_t data_len,
                     uint8_t *mac);

/* Error Handling */
const char *ssh_get_error(ssh_session_t *session);
void ssh_set_error(ssh_session_t *session, const char *fmt, ...);

#endif /* SSH_H */
