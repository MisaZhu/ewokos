/*
 * scp - Secure Copy Protocol (server-side handler)
 *
 * This binary is invoked by sshd via exec when a remote SCP client
 * initiates a file transfer. It communicates over stdin/stdout using
 * the SCP protocol.
 *
 * Modes:
 *   scp -t [-r] [-d] <path>   — sink mode (receive files from client)
 *   scp -f [-r] <path>        — source mode (send files to client)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ewoksys/klog.h>
#include <ewoksys/keydef.h>
#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include "../ssh/ssh.h"

#define BUF_SIZE 4096

static int verbose = 0;

typedef struct {
    ssh_session_t *session;
    uint8_t buf[BUF_SIZE];
    size_t off;
    size_t len;
} scp_channel_reader_t;

typedef struct {
    char *user;
    char *host;
    char *path;
} scp_remote_spec_t;

typedef struct {
    unsigned long total_bytes;
    unsigned long done_bytes;
    struct timeval start_time;
    struct timeval last_draw_time;
    int enabled;
    int last_line_len;
    char display_name[256];
} scp_progress_t;

static int write_all(int fd, const void *buf, size_t n);
static void build_path(char *out, size_t out_sz, const char *dir, const char *name);

static void scp_progress_set_name(scp_progress_t *progress, const char *name) {
    if (!progress)
        return;
    if (!name)
        name = "";
    strncpy(progress->display_name, name, sizeof(progress->display_name) - 1);
    progress->display_name[sizeof(progress->display_name) - 1] = '\0';
}

static double scp_time_diff_sec(const struct timeval *a, const struct timeval *b) {
    double sec = (double)(a->tv_sec - b->tv_sec);
    double usec = (double)(a->tv_usec - b->tv_usec) / 1000000.0;
    return sec + usec;
}

static void scp_format_eta(double seconds, char *buf, size_t size) {
    unsigned long total;
    unsigned long mins;
    unsigned long secs;

    if (!buf || size == 0)
        return;

    if (seconds < 0)
        seconds = 0;

    total = (unsigned long)(seconds + 0.5);
    mins = total / 60;
    secs = total % 60;
    if (mins > 99)
        mins = 99;
    snprintf(buf, size, "%02lu:%02lu", mins, secs);
}

static void scp_progress_draw(scp_progress_t *progress, int force) {
    struct timeval now;
    double elapsed;
    double since_last;
    double rate_kb;
    double eta_sec;
    char eta_buf[16];
    char line[160];
    char out[320];
    int line_len;
    int out_len;
    unsigned int percent;
    unsigned long shown_kb;

    if (!progress || !progress->enabled)
        return;

    gettimeofday(&now, NULL);
    elapsed = scp_time_diff_sec(&now, &progress->start_time);
    since_last = scp_time_diff_sec(&now, &progress->last_draw_time);

    if (!force && since_last < 0.2)
        return;

    shown_kb = (progress->done_bytes + 1023ul) / 1024ul;
    if (progress->total_bytes == 0) {
        percent = 100;
    } else {
        percent = (unsigned int)((progress->done_bytes * 100ul) / progress->total_bytes);
        if (percent > 100)
            percent = 100;
    }

    if (elapsed <= 0.0)
        rate_kb = 0.0;
    else
        rate_kb = ((double)progress->done_bytes / 1024.0) / elapsed;

    if (progress->total_bytes > progress->done_bytes && rate_kb > 0.0) {
        eta_sec = ((double)(progress->total_bytes - progress->done_bytes) / 1024.0) / rate_kb;
    } else {
        eta_sec = 0.0;
    }

    scp_format_eta(eta_sec, eta_buf, sizeof(eta_buf));
    line_len = snprintf(line, sizeof(line), "%s   %3u%% %luKB  %.1fKB/s   %s",
                        progress->display_name,
                        percent, shown_kb, rate_kb, eta_buf);
    if (line_len > 0) {
        static const char prefix[] = "\033[2K\033[1G";
        int prefix_len = (int)(sizeof(prefix) - 1);

        out_len = 0;
        memcpy(out + out_len, prefix, (size_t)prefix_len);
        out_len += prefix_len;
        memcpy(out + out_len, line, (size_t)line_len);
        out_len += line_len;
        if (progress->last_line_len > line_len) {
            int pad = progress->last_line_len - line_len;
            if (out_len + pad >= (int)sizeof(out))
                pad = (int)sizeof(out) - out_len - 1;
            memset(out + out_len, ' ', (size_t)pad);
            out_len += pad;
        }
        if (force && out_len < (int)sizeof(out) - 1)
            out[out_len++] = '\n';
        (void)write(STDERR_FILENO, out, (size_t)out_len);
        progress->last_line_len = line_len;
    }

    progress->last_draw_time = now;
}

static void scp_progress_start(scp_progress_t *progress, const char *name, unsigned long total_bytes) {
    if (!progress)
        return;

    memset(progress, 0, sizeof(*progress));
    progress->total_bytes = total_bytes;
    progress->enabled = isatty(STDERR_FILENO);
    scp_progress_set_name(progress, name);
    gettimeofday(&progress->start_time, NULL);
    progress->last_draw_time = progress->start_time;
    progress->last_draw_time.tv_sec -= 1;
}

static void scp_progress_advance(scp_progress_t *progress, size_t delta_bytes) {
    if (!progress)
        return;

    progress->done_bytes += (unsigned long)delta_bytes;
    if (progress->done_bytes > progress->total_bytes)
        progress->done_bytes = progress->total_bytes;
    scp_progress_draw(progress, 0);
}

static void scp_progress_finish(scp_progress_t *progress) {
    if (!progress)
        return;

    progress->done_bytes = progress->total_bytes;
    scp_progress_draw(progress, 1);
}

static void scp_progress_abort(scp_progress_t *progress) {
    if (!progress || !progress->enabled)
        return;

    if (progress->last_line_len > 0) {
        char out[160];
        int out_len = 0;
        static const char prefix[] = "\033[2K\033[1G";
        int prefix_len = (int)(sizeof(prefix) - 1);

        memcpy(out + out_len, prefix, (size_t)prefix_len);
        out_len += prefix_len;
        out[out_len++] = '\n';
        (void)write(STDERR_FILENO, out, (size_t)out_len);
    } else {
        (void)write(STDERR_FILENO, "\n", 1);
    }
    progress->enabled = 0;
}

/* Send an error message to the SCP client */
static void scp_error(const char *msg) {
    char buf[512];
    int len = snprintf(buf, sizeof(buf), "\x02scp: %s\n", msg);
    write(STDOUT_FILENO, buf, len);
}

/* Send OK response */
static void scp_ok(void) {
    char c = '\0';
    write(STDOUT_FILENO, &c, 1);
}

static const char *scp_basename(const char *path) {
    const char *end;
    const char *name;

    if (!path || *path == '\0')
        return path;

    end = path + strlen(path);
    while (end > path && *(end - 1) == '/')
        end--;
    if (end == path)
        return path;

    name = end;
    while (name > path && *(name - 1) != '/')
        name--;
    if (name == end)
        return path;
    return name;
}

static int scp_dup_range(const char *start, size_t len, char **out) {
    char *s;

    if (!out)
        return -1;

    s = (char *)malloc(len + 1);
    if (!s)
        return -1;

    memcpy(s, start, len);
    s[len] = '\0';
    *out = s;
    return 0;
}

static void scp_free_remote_spec(scp_remote_spec_t *spec) {
    if (!spec)
        return;
    free(spec->user);
    free(spec->host);
    free(spec->path);
    memset(spec, 0, sizeof(*spec));
}

static int scp_parse_remote_spec(const char *spec, scp_remote_spec_t *out) {
    const char *colon;
    const char *at = NULL;
    const char *p;
    const char *host_start;

    if (!spec || !out)
        return 0;

    memset(out, 0, sizeof(*out));

    colon = strchr(spec, ':');
    if (!colon || colon == spec)
        return 0;

    for (p = spec; p < colon; p++) {
        if (*p == '/')
            return 0;
        if (*p == '@')
            at = p;
    }
    host_start = spec;
    if (at) {
        if (scp_dup_range(spec, (size_t)(at - spec), &out->user) < 0)
            goto fail;
        host_start = at + 1;
    }

    if (host_start == colon)
        goto fail;

    if (scp_dup_range(host_start, (size_t)(colon - host_start), &out->host) < 0)
        goto fail;

    out->path = strdup(colon + 1);
    if (!out->path)
        goto fail;

    return 1;

fail:
    scp_free_remote_spec(out);
    return -1;
}

static int scp_channel_write_all(ssh_session_t *session, const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;

    while (total < n) {
        size_t chunk = n - total;
        if (chunk > (size_t)(SSH_MAX_PACKET_SIZE - 64))
            chunk = (size_t)(SSH_MAX_PACKET_SIZE - 64);
        if (ssh_channel_send_data(session, p + total, chunk) < 0)
            return -1;
        total += chunk;
    }
    return 0;
}

static int scp_channel_fill(scp_channel_reader_t *reader) {
    int n;

    if (!reader || !reader->session)
        return -1;

    n = ssh_channel_receive_data(reader->session, reader->buf, sizeof(reader->buf));
    if (n <= 0)
        return n;

    reader->off = 0;
    reader->len = (size_t)n;
    return n;
}

static int scp_channel_read_exact(scp_channel_reader_t *reader, void *buf, size_t n) {
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;

    while (total < n) {
        size_t chunk;
        int ret;

        if (reader->off == reader->len) {
            ret = scp_channel_fill(reader);
            if (ret <= 0)
                return -1;
        }

        chunk = reader->len - reader->off;
        if (chunk > n - total)
            chunk = n - total;

        memcpy(p + total, reader->buf + reader->off, chunk);
        reader->off += chunk;
        total += chunk;
    }

    return 0;
}

static int scp_channel_read_byte(scp_channel_reader_t *reader, char *c) {
    return scp_channel_read_exact(reader, c, 1);
}

static int scp_channel_read_line(scp_channel_reader_t *reader, char *buf, int size) {
    int i = 0;

    while (i < size - 1) {
        char c;

        if (scp_channel_read_byte(reader, &c) < 0)
            return -1;
        if (c == '\n') {
            buf[i] = '\0';
            return i;
        }
        buf[i++] = c;
    }

    buf[i] = '\0';
    return i;
}

static int scp_channel_read_record(scp_channel_reader_t *reader, char *cmd, char *buf, int size) {
    int i = 0;
    char c;

    if (!reader || !cmd || !buf || size <= 1)
        return -1;

    if (scp_channel_read_byte(reader, &c) < 0)
        return -1;

    *cmd = c;

    while (i < size - 1) {
        if (scp_channel_read_byte(reader, &c) < 0)
            return -1;
        if (c == '\n') {
            buf[i] = '\0';
            return i;
        }
        buf[i++] = c;
    }

    buf[i] = '\0';
    return i;
}

static int scp_channel_send_status(ssh_session_t *session, char code, const char *msg) {
    char buf[512];
    int len;

    if (msg) {
        len = snprintf(buf, sizeof(buf), "%cscp: %s\n", code, msg);
        return scp_channel_write_all(session, buf, (size_t)len);
    }

    return scp_channel_write_all(session, &code, 1);
}

static int scp_channel_ok(ssh_session_t *session) {
    return scp_channel_send_status(session, '\0', NULL);
}

static int scp_channel_error(ssh_session_t *session, const char *msg) {
    return scp_channel_send_status(session, '\x02', msg);
}

static int scp_channel_wait_ok(scp_channel_reader_t *reader) {
    char c;

    if (scp_channel_read_byte(reader, &c) < 0) {
        fprintf(stderr, "scp: connection closed while waiting for protocol response\n");
        return -1;
    }
    if (c == 0)
        return 0;
    if (c == 1 || c == 2) {
        char buf[256];
        int len = scp_channel_read_line(reader, buf, sizeof(buf));
        if (len >= 0 && buf[0] != '\0')
            fprintf(stderr, "%s\n", buf);
    }
    return -1;
}

static int scp_read_password(char *password, size_t size) {
    size_t len = 0;

    if (!password || size == 0)
        return -1;

    for (;;) {
        unsigned char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            continue;

        if (c == '\r' || c == '\n' || c == KEY_ENTER) {
            password[len] = '\0';
            (void)write(STDOUT_FILENO, "\n", 1);
            return 0;
        }

        if (c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
            if (len > 0) {
                len--;
                (void)write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }

        if (c < KEY_SPACE)
            continue;

        if (len + 1 < size) {
            password[len++] = (char)c;
            (void)write(STDOUT_FILENO, "*", 1);
        }
    }
}

static int scp_ssh_request_service(ssh_session_t *session) {
    ssh_packet_t packet;

    memset(&packet, 0, sizeof(packet));
    packet.type = SSH_MSG_SERVICE_REQUEST;

    ssh_write_string(packet.payload, (const uint8_t *)"ssh-userauth", 12);
    packet.payload_len = 16;

    if (ssh_packet_send(session, &packet) < 0)
        return -1;

    if (ssh_packet_receive(session, &packet) < 0)
        return -1;

    if (packet.type == SSH_MSG_EXT_INFO) {
        if (ssh_packet_receive(session, &packet) < 0)
            return -1;
    }

    return (packet.type == SSH_MSG_SERVICE_ACCEPT) ? 0 : -1;
}

static int scp_ssh_connect(ssh_session_t **out_session, const char *host, const char *user, int port) {
    ssh_session_t *session;
    char password[128];

    if (!out_session || !host || !user)
        return -1;

    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
                        OPENSSL_INIT_ADD_ALL_CIPHERS |
                        OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL);

    session = ssh_session_new();
    if (!session) {
        fprintf(stderr, "Failed to create SSH session\n");
        return -1;
    }

    if (verbose)
        fprintf(stderr, "Connecting to %s@%s:%d...\n", user, host, port);

    if (ssh_connect(session, host, port) < 0) {
        fprintf(stderr, "Failed to connect: %s\n", ssh_get_error(session));
        ssh_session_free(session);
        return -1;
    }

    if (ssh_send_banner(session) < 0 ||
            ssh_receive_banner(session) < 0 ||
            ssh_send_kexinit(session) < 0) {
        fprintf(stderr, "SSH handshake failed: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_session_free(session);
        return -1;
    }

    usleep(50000);

    if (ssh_receive_kexinit(session) < 0 ||
            ssh_handle_kex(session) < 0 ||
            scp_ssh_request_service(session) < 0) {
        fprintf(stderr, "SSH setup failed: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_session_free(session);
        return -1;
    }

    if (ssh_userauth_list(session, user) < 0) {
        fprintf(stderr, "%s@%s password: ", user, host);
        fflush(stderr);
        if (scp_read_password(password, sizeof(password)) < 0 ||
                ssh_userauth_password(session, user, password) < 0) {
            fprintf(stderr, "Authentication failed: %s\n", ssh_get_error(session));
            ssh_disconnect(session);
            ssh_session_free(session);
            return -1;
        }
    }

    if (ssh_channel_open_session(session) < 0) {
        fprintf(stderr, "Failed to open SSH channel: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_session_free(session);
        return -1;
    }

    *out_session = session;
    return 0;
}

static int scp_send_file_channel(scp_channel_reader_t *reader, const char *path, const char *name) {
    struct stat st;
    char header[512];
    char buf[BUF_SIZE];
    int fd;
    int hlen;
    scp_progress_t progress;

    if (stat(path, &st) != 0) {
        scp_channel_error(reader->session, "cannot stat file");
        return -1;
    }

    hlen = snprintf(header, sizeof(header), "C%04o %lu %s\n",
                    (unsigned int)(st.st_mode & 0777),
                    (unsigned long)st.st_size,
                    name);
    if (scp_channel_write_all(reader->session, header, (size_t)hlen) < 0)
        return -1;

    if (scp_channel_wait_ok(reader) < 0)
        return -1;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        scp_channel_error(reader->session, "cannot open file");
        return -1;
    }

    scp_progress_start(&progress, path, (unsigned long)st.st_size);

    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0) {
            close(fd);
            scp_progress_abort(&progress);
            scp_channel_error(reader->session, "read error");
            return -1;
        }
        if (n == 0)
            break;
        if (scp_channel_write_all(reader->session, buf, (size_t)n) < 0) {
            close(fd);
            scp_progress_abort(&progress);
            return -1;
        }
        scp_progress_advance(&progress, (size_t)n);
    }

    close(fd);
    scp_progress_finish(&progress);

    if (scp_channel_ok(reader->session) < 0)
        return -1;

    return scp_channel_wait_ok(reader);
}

static int scp_send_dir_channel(scp_channel_reader_t *reader, const char *path, const char *name) {
    struct stat st;
    DIR *dirp;
    struct dirent *ent;
    char header[512];
    int hlen;

    if (stat(path, &st) != 0) {
        scp_channel_error(reader->session, "cannot stat directory");
        return -1;
    }

    hlen = snprintf(header, sizeof(header), "D%04o 0 %s\n",
                    (unsigned int)(st.st_mode & 0777), name);
    if (scp_channel_write_all(reader->session, header, (size_t)hlen) < 0)
        return -1;

    if (scp_channel_wait_ok(reader) < 0)
        return -1;

    dirp = opendir(path);
    if (!dirp) {
        scp_channel_error(reader->session, "cannot open directory");
        return -1;
    }

    while ((ent = readdir(dirp)) != NULL) {
        char child_path[512];

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        build_path(child_path, sizeof(child_path), path, ent->d_name);
        if (ent->d_type == DT_DIR) {
            if (scp_send_dir_channel(reader, child_path, ent->d_name) < 0) {
                closedir(dirp);
                return -1;
            }
        } else {
            if (scp_send_file_channel(reader, child_path, ent->d_name) < 0) {
                closedir(dirp);
                return -1;
            }
        }
    }

    closedir(dirp);

    if (scp_channel_write_all(reader->session, "E\n", 2) < 0)
        return -1;

    return scp_channel_wait_ok(reader);
}

static int scp_do_source_channel(scp_channel_reader_t *reader, const char *path, int recursive) {
    struct stat st;

    if (scp_channel_wait_ok(reader) < 0)
        return -1;

    if (stat(path, &st) != 0) {
        scp_channel_error(reader->session, "cannot stat path");
        return -1;
    }

    if ((st.st_mode & 0040000) != 0) {
        if (!recursive) {
            scp_channel_error(reader->session, "not a regular file");
            return -1;
        }
        return scp_send_dir_channel(reader, path, scp_basename(path));
    }

    return scp_send_file_channel(reader, path, scp_basename(path));
}

static int scp_do_sink_channel(scp_channel_reader_t *reader, const char *target, int recursive, int target_is_dir) {
    struct stat st;
    int is_dir = 0;
    int saw_entry = 0;
    char cur_dir[512];

    if (stat(target, &st) == 0) {
        if ((st.st_mode & 0040000) != 0)
            is_dir = 1;
    }

    if (target_is_dir && !is_dir) {
        scp_channel_error(reader->session, "target is not a directory");
        return -1;
    }

    strncpy(cur_dir, target, sizeof(cur_dir) - 1);
    cur_dir[sizeof(cur_dir) - 1] = '\0';

    if (scp_channel_ok(reader->session) < 0)
        return -1;

    while (1) {
        char line[2048];
        char cmd;
        int len = scp_channel_read_record(reader, &cmd, line, sizeof(line));

        if (len < 0) {
            if (!saw_entry) {
                fprintf(stderr, "scp: remote closed connection before sending file data\n");
                return -1;
            }
            break;
        }

        if (cmd == 1 || cmd == 2) {
            if (line[0] != '\0')
                fprintf(stderr, "%s\n", line);
            else
                fprintf(stderr, "scp: remote reported an unspecified error\n");
            return -1;
        }

        if (cmd == 0) {
            fprintf(stderr, "scp: unexpected protocol ack from remote source\n");
            return -1;
        }

        saw_entry = 1;

        if (cmd == 'T') {
            if (scp_channel_ok(reader->session) < 0)
                return -1;
            continue;
        }

        if (cmd == 'E') {
            char *slash = strrchr(cur_dir, '/');

            if (slash && slash != cur_dir)
                *slash = '\0';
            if (scp_channel_ok(reader->session) < 0)
                return -1;
            continue;
        }

        if (cmd == 'D') {
            unsigned int mode;
            char name[256];
            char *p = line;
            char new_dir[512];

            if (!recursive) {
                scp_channel_error(reader->session, "recursive mode not enabled");
                return -1;
            }

            mode = (unsigned int)strtoul(p, &p, 8);
            if (*p == ' ')
                p++;
            while (*p && *p != ' ')
                p++;
            if (*p == ' ')
                p++;

            strncpy(name, p, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';

            build_path(new_dir, sizeof(new_dir), cur_dir, name);
            if (mkdir(new_dir, mode & 0777) < 0 && errno != EEXIST) {
                scp_channel_error(reader->session, "cannot create directory");
                return -1;
            }

            strncpy(cur_dir, new_dir, sizeof(cur_dir) - 1);
            cur_dir[sizeof(cur_dir) - 1] = '\0';

            if (scp_channel_ok(reader->session) < 0)
                return -1;
            continue;
        }

        if (cmd == 'C') {
            unsigned int mode;
            unsigned long size;
            char name[256];
            char *p = line;
            char filepath[512];
            int fd;
            unsigned long remaining;
            int err = 0;
            scp_progress_t progress;

            mode = (unsigned int)strtoul(p, &p, 8);
            if (*p == ' ')
                p++;
            size = strtoul(p, &p, 10);
            if (*p == ' ')
                p++;

            strncpy(name, p, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';

            if (is_dir || recursive)
                build_path(filepath, sizeof(filepath), cur_dir, name);
            else {
                strncpy(filepath, target, sizeof(filepath) - 1);
                filepath[sizeof(filepath) - 1] = '\0';
            }

            if (scp_channel_ok(reader->session) < 0)
                return -1;

            fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, mode & 0777);
            if (fd < 0) {
                char drain[BUF_SIZE];
                remaining = size;
                while (remaining > 0) {
                    size_t chunk = remaining > sizeof(drain) ? sizeof(drain) : (size_t)remaining;
                    if (scp_channel_read_exact(reader, drain, chunk) < 0)
                        break;
                    remaining -= (unsigned long)chunk;
                }
                (void)scp_channel_wait_ok(reader);
                scp_channel_error(reader->session, "cannot create file");
                continue;
            }

            remaining = size;
            scp_progress_start(&progress, filepath, size);
            while (remaining > 0) {
                char buf[BUF_SIZE];
                size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : (size_t)remaining;

                if (scp_channel_read_exact(reader, buf, chunk) < 0) {
                    err = 1;
                    break;
                }
                if (write_all(fd, buf, chunk) < 0) {
                    err = 1;
                    break;
                }
                remaining -= (unsigned long)chunk;
                scp_progress_advance(&progress, chunk);
            }

            if (err) {
                char drain[BUF_SIZE];
                while (remaining > 0) {
                    size_t chunk = remaining > sizeof(drain) ? sizeof(drain) : (size_t)remaining;
                    if (scp_channel_read_exact(reader, drain, chunk) < 0)
                        break;
                    remaining -= (unsigned long)chunk;
                }
                scp_progress_abort(&progress);
            } else {
                scp_progress_finish(&progress);
            }

            fchmod(fd, mode & 0777);
            close(fd);

            if (scp_channel_wait_ok(reader) < 0) {
                scp_channel_error(reader->session, "transfer aborted");
                return -1;
            }

            if (err) {
                scp_channel_error(reader->session, "write error");
                return -1;
            }

            if (scp_channel_ok(reader->session) < 0)
                return -1;
            continue;
        }

        fprintf(stderr, "scp: unexpected protocol record '%c%s'\n",
                (cmd >= 32 && cmd < 127) ? cmd : '?', line);
        scp_channel_error(reader->session, "unknown command");
        return -1;
    }

    return 0;
}

static int scp_run_remote_to_local(const scp_remote_spec_t *src, const char *dst, int recursive, int preserve_times, int port) {
    ssh_session_t *session = NULL;
    scp_channel_reader_t reader;
    char command[1024];
    const char *user;
    int ret = -1;

    user = (src->user && src->user[0] != '\0') ? src->user : getenv("USER");
    if (!user || user[0] == '\0')
        user = "root";

    if (scp_ssh_connect(&session, src->host, user, port) < 0)
        return -1;

    snprintf(command, sizeof(command), "scp -f%s%s %s",
             recursive ? " -r" : "",
             preserve_times ? " -p" : "",
             src->path);

    if (ssh_channel_request_exec(session, command) < 0) {
        fprintf(stderr, "Failed to execute remote scp: %s\n", ssh_get_error(session));
        goto out;
    }

    memset(&reader, 0, sizeof(reader));
    reader.session = session;

    ret = scp_do_sink_channel(&reader, dst, recursive, 0);

out:
    if (session) {
        ssh_channel_close(session);
        ssh_disconnect(session);
        ssh_session_free(session);
    }
    return ret;
}

static int scp_run_local_to_remote(const char *src, const scp_remote_spec_t *dst, int recursive, int preserve_times, int port) {
    ssh_session_t *session = NULL;
    scp_channel_reader_t reader;
    char command[1024];
    const char *user;
    int ret = -1;

    user = (dst->user && dst->user[0] != '\0') ? dst->user : getenv("USER");
    if (!user || user[0] == '\0')
        user = "root";

    if (scp_ssh_connect(&session, dst->host, user, port) < 0)
        return -1;

    snprintf(command, sizeof(command), "scp -t%s%s %s",
             recursive ? " -r" : "",
             preserve_times ? " -p" : "",
             dst->path);

    if (ssh_channel_request_exec(session, command) < 0) {
        fprintf(stderr, "Failed to execute remote scp: %s\n", ssh_get_error(session));
        goto out;
    }

    memset(&reader, 0, sizeof(reader));
    reader.session = session;

    ret = scp_do_source_channel(&reader, src, recursive);

out:
    if (session) {
        ssh_channel_close(session);
        ssh_disconnect(session);
        ssh_session_free(session);
    }
    return ret;
}

static int scp_do_client_copy(const char *src, const char *dst, int recursive, int preserve_times, int port) {
    scp_remote_spec_t remote_src;
    scp_remote_spec_t remote_dst;
    int src_is_remote;
    int dst_is_remote;
    int ret = -1;

    memset(&remote_src, 0, sizeof(remote_src));
    memset(&remote_dst, 0, sizeof(remote_dst));

    src_is_remote = scp_parse_remote_spec(src, &remote_src);
    if (src_is_remote < 0) {
        fprintf(stderr, "Invalid remote source: %s\n", src);
        return -1;
    }

    dst_is_remote = scp_parse_remote_spec(dst, &remote_dst);
    if (dst_is_remote < 0) {
        fprintf(stderr, "Invalid remote target: %s\n", dst);
        scp_free_remote_spec(&remote_src);
        return -1;
    }

    if (src_is_remote && dst_is_remote) {
        fprintf(stderr, "Remote to remote copy is not supported\n");
        goto out;
    }

    if (!src_is_remote && !dst_is_remote) {
        fprintf(stderr, "At least one path must be remote\n");
        goto out;
    }

    if (src_is_remote)
        ret = scp_run_remote_to_local(&remote_src, dst, recursive, preserve_times, port);
    else
        ret = scp_run_local_to_remote(src, &remote_dst, recursive, preserve_times, port);

out:
    scp_free_remote_spec(&remote_src);
    scp_free_remote_spec(&remote_dst);
    return ret;
}

/* Wait for OK from client, returns 0 on success */
static int scp_wait_ok(void) {
    char c;
    int n = read(STDIN_FILENO, &c, 1);
    if (n != 1)
        return -1;
    if (c == 0)
        return 0;
    /* Error or warning from remote - drain message */
    if (c == 1 || c == 2) {
        char buf[256];
        int i = 0;
        while (i < (int)sizeof(buf) - 1) {
            if (read(STDIN_FILENO, &buf[i], 1) != 1)
                break;
            if (buf[i] == '\n')
                break;
            i++;
        }
        return -1;
    }
    return -1;
}

/* Read a line from stdin (up to newline), null-terminate, returns length or -1 */
static int read_line(char *buf, int size) {
    int i = 0;
    while (i < size - 1) {
        char c;
        int n = read(STDIN_FILENO, &c, 1);
        if (n != 1)
            return -1;
        if (c == '\n') {
            buf[i] = '\0';
            return i;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

/* Read exactly n bytes from stdin */
static int read_exact(int fd, void *buf, size_t n) {
    size_t total = 0;
    uint8_t *p = (uint8_t *)buf;
    while (total < n) {
        ssize_t r = read(fd, p + total, n - total);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return -1;
        total += (size_t)r;
    }
    return 0;
}

/* Write all bytes to fd */
static int write_all(int fd, const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;
    while (total < n) {
        ssize_t w = write(fd, p + total, n - total);
        if (w < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        total += (size_t)w;
    }
    return 0;
}

/* Build full path: dir/name */
static void build_path(char *out, size_t out_sz, const char *dir, const char *name) {
    size_t dlen = strlen(dir);
    if (dlen > 0 && dir[dlen - 1] == '/')
        snprintf(out, out_sz, "%s%s", dir, name);
    else
        snprintf(out, out_sz, "%s/%s", dir, name);
}

/*
 * SINK MODE: Receive files from client
 */
static int do_sink(const char *target, int recursive, int target_is_dir) {
    struct stat st;
    int is_dir = 0;
    char cur_dir[512];

    /* Check if target is an existing directory */
    if (stat(target, &st) == 0) {
        if ((st.st_mode & 0040000) != 0) /* S_ISDIR */
            is_dir = 1;
    }

    if (target_is_dir && !is_dir) {
        scp_error("target is not a directory");
        return -1;
    }

    strncpy(cur_dir, target, sizeof(cur_dir) - 1);
    cur_dir[sizeof(cur_dir) - 1] = '\0';

    /* Signal ready */
    scp_ok();

    while (1) {
        char line[2048];
        int len = read_line(line, sizeof(line));
        if (len < 0)
            break;
        if (len == 0)
            continue;

        char cmd = line[0];

        /* Timestamp commands (T) - acknowledge and skip */
        if (cmd == 'T') {
            scp_ok();
            continue;
        }

        /* End of directory */
        if (cmd == 'E') {
            /* Go up one directory level */
            char *slash = strrchr(cur_dir, '/');
            if (slash && slash != cur_dir) {
                *slash = '\0';
            }
            scp_ok();
            continue;
        }

        /* Directory entry: D<mode> 0 <name> */
        if (cmd == 'D') {
            if (!recursive) {
                scp_error("recursive mode not enabled");
                return -1;
            }
            /* Parse: D<mode> <ignored> <name> */
            unsigned int mode = 0;
            char name[256];
            char *p = line + 1;

            /* Parse mode (octal) */
            mode = (unsigned int)strtoul(p, &p, 8);
            if (*p == ' ') p++;
            /* Skip size field (always 0 for dirs) */
            while (*p && *p != ' ') p++;
            if (*p == ' ') p++;
            /* Rest is name */
            strncpy(name, p, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';

            /* Create directory */
            char new_dir[512];
            build_path(new_dir, sizeof(new_dir), cur_dir, name);
            mkdir(new_dir, mode & 0777);
            strncpy(cur_dir, new_dir, sizeof(cur_dir) - 1);
            cur_dir[sizeof(cur_dir) - 1] = '\0';
            scp_ok();
            continue;
        }

        /* File entry: C<mode> <size> <name> */
        if (cmd == 'C') {
            unsigned int mode = 0;
            unsigned long size = 0;
            char name[256];
            char *p = line + 1;

            /* Parse mode */
            mode = (unsigned int)strtoul(p, &p, 8);
            if (*p == ' ') p++;
            /* Parse size */
            size = strtoul(p, &p, 10);
            if (*p == ' ') p++;
            /* Rest is filename */
            strncpy(name, p, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';

            /* Determine output path */
            char filepath[512];
            if (is_dir || recursive) {
                build_path(filepath, sizeof(filepath), cur_dir, name);
            } else {
                strncpy(filepath, target, sizeof(filepath) - 1);
                filepath[sizeof(filepath) - 1] = '\0';
            }

            /* Acknowledge the file header */
            scp_ok();

            /* Open file for writing */
            int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, mode & 0777);
            if (fd < 0) {
                /* Drain the data anyway */
                unsigned long remaining = size;
                char drain[BUF_SIZE];
                while (remaining > 0) {
                    size_t chunk = remaining > sizeof(drain) ? sizeof(drain) : remaining;
                    if (read_exact(STDIN_FILENO, drain, chunk) < 0)
                        break;
                    remaining -= chunk;
                }
                /* Read trailing \0 */
                char c;
                read(STDIN_FILENO, &c, 1);
                scp_error("cannot create file");
                continue;
            }

            /* Receive file data */
            unsigned long remaining = size;
            char buf[BUF_SIZE];
            int err = 0;
            while (remaining > 0) {
                size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
                if (read_exact(STDIN_FILENO, buf, chunk) < 0) {
                    err = 1;
                    break;
                }
                if (write_all(fd, buf, chunk) < 0) {
                    err = 1;
                    break;
                }
                remaining -= chunk;
            }

            fchmod(fd, mode & 0777);
            close(fd);

            /* Read the trailing NUL byte from client */
            char c;
            if (!err)
                read(STDIN_FILENO, &c, 1);

            if (err) {
                scp_error("write error");
                return -1;
            }

            scp_ok();
            continue;
        }

        /* Unknown command */
        scp_error("unknown command");
        return -1;
    }
    return 0;
}

/*
 * SOURCE MODE: Send a single file to client
 */
static int send_file(const char *path, const char *name) {
    struct stat st;
    char header[512];
    int hlen;
    char buf[BUF_SIZE];

    if (stat(path, &st) != 0) {
        scp_error("cannot stat file");
        return -1;
    }

    /* Send file header: C<mode> <size> <name>\n */
    hlen = snprintf(header, sizeof(header), "C%04o %lu %s\n",
                    (unsigned int)(st.st_mode & 0777),
                    (unsigned long)st.st_size,
                    name);
    if (write_all(STDOUT_FILENO, header, hlen) < 0)
        return -1;

    /* Wait for client acknowledgment */
    if (scp_wait_ok() < 0)
        return -1;

    /* Send file content */
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        scp_error("cannot open file");
        return -1;
    }

    unsigned long remaining = (unsigned long)st.st_size;
    while (remaining > 0) {
        size_t chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        ssize_t n = read(fd, buf, chunk);
        if (n <= 0) {
            close(fd);
            scp_error("read error");
            return -1;
        }
        if (write_all(STDOUT_FILENO, buf, (size_t)n) < 0) {
            close(fd);
            return -1;
        }
        remaining -= (size_t)n;
    }
    close(fd);

    /* Send trailing NUL */
    scp_ok();

    /* Wait for acknowledgment */
    if (scp_wait_ok() < 0)
        return -1;

    return 0;
}

/*
 * SOURCE MODE: Send a directory recursively
 */
static int send_dir(const char *path, const char *name) {
    struct stat st;
    char header[512];
    int hlen;

    if (stat(path, &st) != 0) {
        scp_error("cannot stat directory");
        return -1;
    }

    /* Send directory header: D<mode> 0 <name>\n */
    hlen = snprintf(header, sizeof(header), "D%04o 0 %s\n",
                    (unsigned int)(st.st_mode & 0777), name);
    if (write_all(STDOUT_FILENO, header, hlen) < 0)
        return -1;

    if (scp_wait_ok() < 0)
        return -1;

    /* Iterate directory contents */
    DIR *dirp = opendir(path);
    if (dirp == NULL) {
        scp_error("cannot open directory");
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(dirp)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char child_path[512];
        build_path(child_path, sizeof(child_path), path, ent->d_name);

        if (ent->d_type == DT_DIR) {
            if (send_dir(child_path, ent->d_name) < 0) {
                closedir(dirp);
                return -1;
            }
        } else {
            if (send_file(child_path, ent->d_name) < 0) {
                closedir(dirp);
                return -1;
            }
        }
    }
    closedir(dirp);

    /* Send end of directory marker */
    if (write_all(STDOUT_FILENO, "E\n", 2) < 0)
        return -1;

    if (scp_wait_ok() < 0)
        return -1;

    return 0;
}

/*
 * SOURCE MODE: Send files to client
 */
static int do_source(const char *path, int recursive) {
    struct stat st;

    /* Wait for initial client ready signal */
    if (scp_wait_ok() < 0)
        return -1;

    if (stat(path, &st) != 0) {
        scp_error("cannot stat path");
        return -1;
    }

    if ((st.st_mode & 0040000) != 0) { /* Is directory */
        if (!recursive) {
            scp_error("not a regular file");
            return -1;
        }
        /* Extract basename for the directory */
        const char *name = strrchr(path, '/');
        name = name ? name + 1 : path;
        return send_dir(path, name);
    } else {
        /* Regular file */
        const char *name = strrchr(path, '/');
        name = name ? name + 1 : path;
        return send_file(path, name);
    }
}

static void usage(void) {
    fprintf(stderr, "Usage: scp [-P port] [-r] [-p] [-v] source target\n");
    fprintf(stderr, "       scp [-t|-f] [-r] [-d] [-v] <path>\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int mode_sink = 0;
    int mode_source = 0;
    int recursive = 0;
    int target_is_dir = 0;
    int preserve_times = 0;
    int port = SSH_DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "tfrTdvpP:")) != -1) {
        switch (opt) {
        case 't':
            mode_sink = 1;
            break;
        case 'f':
            mode_source = 1;
            break;
        case 'r':
            recursive = 1;
            break;
        case 'd':
            target_is_dir = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'T':
            /* Disable pseudo-terminal - ignored, we never use PTY */
            break;
        case 'p':
            /* Preserve mode/time - acknowledged for compat, handled via T lines */
            preserve_times = 1;
            break;
        case 'P':
            port = atoi(optarg);
            if (port <= 0 || port > 65535) {
                fprintf(stderr, "Invalid port: %s\n", optarg);
                return 1;
            }
            break;
        default:
            break;
        }
    }

    if (!mode_sink && !mode_source) {
        if (argc - optind != 2) {
            usage();
            return 1;
        }
        return scp_do_client_copy(argv[optind], argv[optind + 1], recursive, preserve_times, port) < 0 ? 1 : 0;
    }

    if (optind >= argc) {
        scp_error("missing path argument");
        return 1;
    }

    const char *path = argv[optind];

    if (mode_sink) {
        return do_sink(path, recursive, target_is_dir) < 0 ? 1 : 0;
    } else {
        return do_source(path, recursive) < 0 ? 1 : 0;
    }
}
