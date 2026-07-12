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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ewoksys/klog.h>

#define BUF_SIZE 4096

static int verbose = 0;

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
            int fd = open(filepath, O_WRONLY | O_CREAT);
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
    fprintf(stderr, "Usage: scp [-t|-f] [-r] [-d] [-v] <path>\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int mode_sink = 0;
    int mode_source = 0;
    int recursive = 0;
    int target_is_dir = 0;

    while ((opt = getopt(argc, argv, "tfrTdvp")) != -1) {
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
            break;
        default:
            break;
        }
    }

    if (!mode_sink && !mode_source) {
        usage();
        return 1;
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
