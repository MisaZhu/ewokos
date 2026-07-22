#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <poll.h>

#define BUFFER_SIZE 1024
#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251
#define TELNET_SB   250
#define TELNET_SE   240

#define TELNET_SB_IS   0
#define TELNET_SB_SEND 1

#define TELNET_OPT_BINARY 0
#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_STATUS 5
#define TELNET_OPT_TERMINAL_TYPE 24
#define TELNET_OPT_NAWS 31
#define TELNET_OPT_LINEMODE 34
#define TELNET_OPT_ENVIRON 36
#define TELNET_OPT_NEW_ENVIRON 39

#define TELNET_ENV_VAR 0
#define TELNET_ENV_VALUE 1

static volatile int _ended = 0;
static int sockfd = -1;
static int _is_telnet_console = 0;
static uint8_t telnet_pending_buf[512];
static int telnet_pending_len = 0;
static int telnet_caps_offered = 0;
static uint8_t stdout_pending_buf[BUFFER_SIZE * 64];
static int stdout_pending_len = 0;

static int stdout_flush_pending(void);

static void detect_telnet_console(void) {
	const char *cid = getenv("CONSOLE_ID");
	_is_telnet_console = (cid != NULL && strcmp(cid, "telnet") == 0);
}

/* State machine for filtering IAC from stdin (telnet console input) */
static int _stdin_iac_state = 0;  /* 0=data, 1=IAC, 2=option, 3=SB, 4=SB_IAC */
static int _stdin_prev_cr = 0;    /* previous byte was CR */

/* Filter telnet IAC sequences from stdin buffer in-place.
   Also normalizes CRLF to LF and bare CR to LF.
   Returns number of clean data bytes remaining. */
static ssize_t filter_stdin_iac(uint8_t *buf, ssize_t len) {
	ssize_t out = 0;

	for(ssize_t i = 0; i < len; i++) {
		uint8_t c = buf[i];

		switch(_stdin_iac_state) {
		case 0: /* DATA */
			if(_stdin_prev_cr) {
				_stdin_prev_cr = 0;
				if(c == '\n') {
					/* CRLF -> LF */
					buf[out++] = '\n';
					continue;
				} else if(c == '\0') {
					/* CR NUL -> LF */
					buf[out++] = '\n';
					continue;
				} else {
					/* bare CR followed by other char -> LF + that char */
					buf[out++] = '\n';
				}
			}
			if(c == TELNET_IAC) {
				_stdin_iac_state = 1;
			} else if(c == '\r') {
				_stdin_prev_cr = 1;
			} else {
				buf[out++] = c;
			}
			break;

		case 1: /* IAC */
			if(c == TELNET_IAC) {
				/* escaped IAC -> literal 0xFF */
				buf[out++] = 0xFF;
				_stdin_iac_state = 0;
			} else if(c == TELNET_SB) {
				_stdin_iac_state = 3;
			} else if(c == TELNET_WILL || c == TELNET_WONT ||
			          c == TELNET_DO || c == TELNET_DONT) {
				_stdin_iac_state = 2;
			} else {
				/* single-byte command (e.g. SE, NOP, etc.) */
				_stdin_iac_state = 0;
			}
			break;

		case 2: /* OPTION (after WILL/WONT/DO/DONT) */
			/* consume the option byte, back to data */
			_stdin_iac_state = 0;
			break;

		case 3: /* SB data */
			if(c == TELNET_IAC) {
				_stdin_iac_state = 4;
			}
			/* else consume sub-negotiation byte */
			break;

		case 4: /* SB_IAC */
			if(c == TELNET_SE) {
				_stdin_iac_state = 0;
			} else {
				_stdin_iac_state = 3;
			}
			break;

		default:
			_stdin_iac_state = 0;
			break;
		}
	}

	/* Handle trailing CR at end of buffer */
	if(_stdin_prev_cr && _stdin_iac_state == 0) {
		/* Keep prev_cr set so next call can resolve it */
	}

	return out;
}

static int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int wait_socket_event(short events, int timeout_ms) {
    struct pollfd pfd;

    pfd.fd = sockfd;
    pfd.events = events;
    pfd.revents = 0;

    while(1) {
        int ret = poll(&pfd, 1, timeout_ms);
        if(ret > 0) {
            if(pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                errno = EIO;
                return -1;
            }
            if(pfd.revents & events) {
                return 0;
            }
            continue;
        }
        if(ret == 0) {
            errno = EAGAIN;
            return -1;
        }
        if(errno == EINTR) {
            continue;
        }
        return -1;
    }
}

static int stdout_write_all(const uint8_t *buf, int len) {
    int sent = 0;

    while(sent < len) {
        int sz = write(STDOUT_FILENO, buf + sent, len - sent);
        if(sz > 0) {
            sent += sz;
            continue;
        }

        if(sz < 0 && errno == EAGAIN) {
            usleep(1000);
            continue;
        }
        return -1;
    }
    return 0;
}

static int telnet_write_all(const uint8_t *buf, int len) {
    int sent = 0;

    while(sent < len) {
        int sz = write(sockfd, buf + sent, len - sent);
        if(sz > 0) {
            sent += sz;
            continue;
        }

        if(sz < 0 && errno == EAGAIN) {
            if(wait_socket_event(POLLOUT, -1) == 0) {
                continue;
            }
        }

        if(sz < 0 && errno == EINTR) {
            continue;
        }

        if(sz == 0) {
            errno = EIO;
            return -1;
        }
        return -1;
    }
    return 0;
}

static int telnet_queue_bytes(const uint8_t *buf, int len) {
    if(len <= 0) {
        return 0;
    }

    if(telnet_pending_len + len > (int)sizeof(telnet_pending_buf)) {
        if(telnet_write_all(telnet_pending_buf, telnet_pending_len) < 0) {
            return -1;
        }
        telnet_pending_len = 0;
    }

    if(len > (int)sizeof(telnet_pending_buf)) {
        return telnet_write_all(buf, len);
    }

    memcpy(telnet_pending_buf + telnet_pending_len, buf, len);
    telnet_pending_len += len;
    return 0;
}

static int telnet_flush_pending(void) {
    if(telnet_pending_len <= 0) {
        return 0;
    }
    if(telnet_write_all(telnet_pending_buf, telnet_pending_len) < 0) {
        return -1;
    }
    telnet_pending_len = 0;
    return 0;
}

static int stdout_queue_bytes(const uint8_t *buf, int len) {
    if(len <= 0) {
        return 0;
    }

    if(stdout_pending_len + len > (int)sizeof(stdout_pending_buf)) {
        if(stdout_flush_pending() < 0) {
            return -1;
        }
    }

    /*
     * Never truncate stdout data silently. Dropping bytes corrupts terminal
     * escape/control sequences and makes the session look hung while socket
     * reads are still progressing.
     */
    if(len > (int)sizeof(stdout_pending_buf)) {
        if(stdout_pending_len > 0) {
            if(stdout_flush_pending() < 0) {
                return -1;
            }
        }
        return stdout_write_all(buf, len) == 0 ? len : -1;
    }

    if(stdout_pending_len + len > (int)sizeof(stdout_pending_buf)) {
        if(stdout_flush_pending() < 0) {
            return -1;
        }
        if(stdout_pending_len + len > (int)sizeof(stdout_pending_buf)) {
            return -1;
        }
    }

    memcpy(stdout_pending_buf + stdout_pending_len, buf, len);
    stdout_pending_len += len;
    return len;
}

static int stdout_flush_pending(void) {
    while(stdout_pending_len > 0) {
        int sz = write(STDOUT_FILENO, stdout_pending_buf, stdout_pending_len);
        if(sz > 0) {
            if(sz < stdout_pending_len) {
                memmove(stdout_pending_buf, stdout_pending_buf + sz, stdout_pending_len - sz);
            }
            stdout_pending_len -= sz;
            continue;
        }

        if(sz < 0 && errno == EAGAIN) {
            usleep(1000);
            continue;
        }
        return -1;
    }
    return 0;
}

static int telnet_send_cmd(uint8_t verb, uint8_t opt) {
    uint8_t cmd[3];

    cmd[0] = TELNET_IAC;
    cmd[1] = verb;
    cmd[2] = opt;
    return telnet_queue_bytes(cmd, sizeof(cmd));
}

static int telnet_send_terminal_type(void) {
    static const uint8_t resp[] = {
        TELNET_IAC, TELNET_SB, TELNET_OPT_TERMINAL_TYPE, TELNET_SB_IS,
        'v', 't', '1', '0', '0',
        TELNET_IAC, TELNET_SE
    };
    return telnet_queue_bytes(resp, sizeof(resp));
}

static int telnet_send_naws(uint16_t cols, uint16_t rows) {
    uint8_t resp[9];

    resp[0] = TELNET_IAC;
    resp[1] = TELNET_SB;
    resp[2] = TELNET_OPT_NAWS;
    resp[3] = (uint8_t)((cols >> 8) & 0xff);
    resp[4] = (uint8_t)(cols & 0xff);
    resp[5] = (uint8_t)((rows >> 8) & 0xff);
    resp[6] = (uint8_t)(rows & 0xff);
    resp[7] = TELNET_IAC;
    resp[8] = TELNET_SE;
    return telnet_queue_bytes(resp, sizeof(resp));
}

static int telnet_send_environ(uint8_t opt) {
    static const uint8_t resp[] = {
        TELNET_IAC, TELNET_SB, 0, TELNET_SB_IS,
        TELNET_ENV_VAR, 'T', 'E', 'R', 'M',
        TELNET_ENV_VALUE, 'v', 't', '1', '0', '0',
        TELNET_ENV_VAR, 'C', 'O', 'L', 'U', 'M', 'N', 'S',
        TELNET_ENV_VALUE, '8', '0',
        TELNET_ENV_VAR, 'L', 'I', 'N', 'E', 'S',
        TELNET_ENV_VALUE, '2', '5',
        TELNET_IAC, TELNET_SE
    };
    uint8_t buf[sizeof(resp)];

    memcpy(buf, resp, sizeof(buf));
    buf[2] = opt;
    return telnet_queue_bytes(buf, sizeof(buf));
}

static void telnet_send_initial_negotiation(void) {
    /*
     * Stay passive on connect. Some peers stall if we eagerly advertise a
     * full batch of options before they start their own negotiation.
     */
}

static void telnet_offer_capabilities(void) {
    if(telnet_caps_offered) {
        return;
    }

    telnet_caps_offered = 1;
    telnet_send_cmd(TELNET_WILL, TELNET_OPT_TERMINAL_TYPE);
    telnet_send_cmd(TELNET_WILL, TELNET_OPT_NAWS);
    telnet_send_cmd(TELNET_WILL, TELNET_OPT_ENVIRON);
    telnet_send_cmd(TELNET_WILL, TELNET_OPT_NEW_ENVIRON);
}

static int telnet_support_remote_option(uint8_t opt) {
    return opt == TELNET_OPT_ECHO ||
           opt == TELNET_OPT_SUPPRESS_GA ||
           opt == TELNET_OPT_BINARY;
}

static int telnet_support_local_option(uint8_t opt) {
    return opt == TELNET_OPT_SUPPRESS_GA ||
           opt == TELNET_OPT_TERMINAL_TYPE ||
           opt == TELNET_OPT_NAWS ||
           opt == TELNET_OPT_ENVIRON ||
           opt == TELNET_OPT_NEW_ENVIRON ||
           opt == TELNET_OPT_BINARY;
}

static void handle_iac_cmd(uint8_t verb, uint8_t opt) {
    uint8_t reply = 0;

    if(verb == TELNET_WILL || verb == TELNET_DO) {
        telnet_offer_capabilities();
    }

    switch(verb) {
        case TELNET_WILL:
            reply = telnet_support_remote_option(opt) ? TELNET_DO : TELNET_DONT;
            break;
        case TELNET_DO:
            reply = telnet_support_local_option(opt) ? TELNET_WILL : TELNET_WONT;
            break;
        case TELNET_WONT:
        case TELNET_DONT:
        default:
            reply = 0;
            break;
    }

    if(reply != 0) {
        telnet_send_cmd(reply, opt);
        if(verb == TELNET_DO && reply == TELNET_WILL && opt == TELNET_OPT_NAWS) {
            telnet_send_naws(80, 25);
        } else if(verb == TELNET_DO && reply == TELNET_WILL &&
                  (opt == TELNET_OPT_ENVIRON || opt == TELNET_OPT_NEW_ENVIRON)) {
            telnet_send_environ(opt);
        }
    }
}

/*
 * Process a single byte from the telnet stream.
 * Returns 0 if the byte should be passed to stdout, -1 otherwise.
 */
static int process_iac_byte(uint8_t c) {
    static enum { STATE_DATA, STATE_IAC, STATE_CMD, STATE_SB, STATE_SB_DATA, STATE_SB_IAC } _iac_state = STATE_DATA;
    static uint8_t _iac_verb = 0;
    static uint8_t _sb_opt = 0;
    static uint8_t _sb_buf[64];
    static int _sb_len = 0;

    switch(_iac_state) {
        case STATE_DATA:
            if(c == TELNET_IAC) {
                _iac_state = STATE_IAC;
                return -1;
            }
            return 0;

        case STATE_IAC:
            if(c == TELNET_IAC) {
                _iac_state = STATE_DATA;
                return 0;
            }
            if(c == TELNET_SB) {
                _iac_state = STATE_SB;
                return -1;
            }
            if(c == TELNET_SE) {
                _iac_state = STATE_DATA;
                return -1;
            }
            _iac_verb = c;
            _iac_state = STATE_CMD;
            return -1;

        case STATE_CMD:
            handle_iac_cmd(_iac_verb, c);
            _iac_state = STATE_DATA;
            return -1;

        case STATE_SB:
            _sb_opt = c;
            _sb_len = 0;
            _iac_state = STATE_SB_DATA;
            return -1;

        case STATE_SB_DATA:
            if(c == TELNET_IAC) {
                _iac_state = STATE_SB_IAC;
                return -1;
            }
            if(_sb_len < (int)sizeof(_sb_buf)) {
                _sb_buf[_sb_len++] = c;
            }
            return -1;

        case STATE_SB_IAC:
            if(c == TELNET_SE) {
                if(_sb_opt == TELNET_OPT_TERMINAL_TYPE &&
                   _sb_len > 0 &&
                   _sb_buf[0] == TELNET_SB_SEND) {
                    telnet_send_terminal_type();
                } else if((_sb_opt == TELNET_OPT_ENVIRON || _sb_opt == TELNET_OPT_NEW_ENVIRON) &&
                          _sb_len > 0 &&
                          _sb_buf[0] == TELNET_SB_SEND) {
                    telnet_send_environ(_sb_opt);
                }
                _iac_state = STATE_DATA;
                return -1;
            }
            if(c == TELNET_IAC && _sb_len < (int)sizeof(_sb_buf)) {
                _sb_buf[_sb_len++] = c;
            }
            _iac_state = STATE_SB_DATA;
            return -1;

        default:
            _iac_state = STATE_DATA;
            return -1;
    }
}

static int handle_socket_data(void) {
    uint8_t buffer[BUFFER_SIZE];
    uint8_t output[BUFFER_SIZE];
    int n;
    n = read(sockfd, buffer, BUFFER_SIZE);

    if (n > 0) {
        int out_len = 0;
        telnet_pending_len = 0;
        for(int i = 0; i < n; i++) {
            if(buffer[i] == '\r') {
                continue;
            }
            if(process_iac_byte(buffer[i]) == 0) {
                output[out_len++] = buffer[i];
                if(out_len == BUFFER_SIZE) {
                    if(stdout_queue_bytes(output, out_len) < 0) {
                        return -1;
                    }
                    if(stdout_flush_pending() < 0) {
                        return -1;
                    }
                    out_len = 0;
                }
            }
        }
        if(telnet_flush_pending() < 0) {
            return -1;
        }
        if(out_len > 0) {
            if(stdout_queue_bytes(output, out_len) < 0) {
                return -1;
            }
            if(stdout_flush_pending() < 0) {
                return -1;
            }
        }
    } else if (n == 0) {
        printf("\nConnection closed by remote host (read returned 0, EOF).\n");
        return -1;
    } else {
        if(errno != EAGAIN) {
            printf("\nConnection closed by remote host (read returned %d, errno=%d: %s).\n",
                   n, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

static int special_char_check(uint8_t c) {
    if (c == 0x1D) {
        printf("\nCtrl+] Exiting connection...\n");
        return 1;
    } else if (c == 0x04) {
        printf("\nCtrl+D Exiting connection...\n");
        return 1;
    }
    return 0;
}

static int append_stdin_bytes(char *dst, int dst_size, const char *src, int src_len) {
    int out = 0;

    for(int i = 0; i < src_len; i++) {
        uint8_t c = (uint8_t)src[i];

        if(special_char_check(c)) {
            return -1;
        }

        if(c == '\n') {
            if(out + 2 > dst_size) {
                break;
            }
            dst[out++] = '\r';
            dst[out++] = '\n';
            continue;
        }

        if(c == TELNET_IAC) {
            if(out + 2 > dst_size) {
                break;
            }
            dst[out++] = TELNET_IAC;
            dst[out++] = TELNET_IAC;
            continue;
        }

        if(out + 1 > dst_size) {
            break;
        }
        dst[out++] = (char)c;
    }

    return out;
}

static int handle_stdin_data(void) {
    char buf[BUFFER_SIZE];
    char send_buf[BUFFER_SIZE * 2];
    int n;
    int send_len;

    n = read(STDIN_FILENO, buf, BUFFER_SIZE - 1);

    if (n > 0) {
        if(_is_telnet_console) {
            n = (int)filter_stdin_iac((uint8_t *)buf, (ssize_t)n);
            if(n <= 0) {
                return 0;
            }
        }
        struct pollfd pfd;

        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        pfd.revents = 0;

        while(n < (BUFFER_SIZE - 1)) {
            int pret = poll(&pfd, 1, 5);
            if(pret <= 0 || !(pfd.revents & POLLIN)) {
                break;
            }

            int more = read(STDIN_FILENO, buf + n, (BUFFER_SIZE - 1) - n);
            if(more <= 0) {
                break;
            }
            n += more;
        }

        send_len = append_stdin_bytes(send_buf, sizeof(send_buf), buf, n);
        if(send_len < 0) {
            return -1;
        }

        if(telnet_write_all((const uint8_t *)send_buf, send_len) < 0) {
            printf("\nEOF received. Exiting...\n");
            return -1;
        }
    } else if (n == 0) {
        /*
         * On EwokOS console-like stdin, poll may occasionally report POLLIN
         * while read() still returns 0. Treat it as "no input for now"
         * instead of EOF, otherwise one of multiple telnet sessions can
         * spuriously exit or appear stuck.
         */
        usleep(1000);
        return 0;
    } else {
        if(errno == 0) {
            /*
             * Some console backends can also surface a transient readable event
             * followed by read() == -1 without setting errno. Treat it the
             * same as "no input yet" instead of tearing the session down.
             */
            usleep(1000);
            return 0;
        }
        if(errno != EAGAIN) {
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    struct hostent *he;
    struct pollfd fds[2];
    int port;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <host> [port]\n", argv[0]);
        exit(1);
    }

    setbuf(stdout, NULL);
    detect_telnet_console();

    port = (argc > 2) ? atoi(argv[2]) : 23;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    he = gethostbyname(argv[1]);
    if (he == NULL) {
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
            fprintf(stderr, "Invalid address or hostname: %s\n", argv[1]);
            return -1;
        }
    } else {
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
    if (argc > 2) {
        printf("Connecting to %s (%s):%s ... ", argv[1], addr_str, argv[2]);
    } else {
        printf("Connecting to %s (%s):23 ... ", argv[1], addr_str);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("failed\n");
        close(sockfd);
        return -1;
    }
    if(set_nonblock(sockfd) < 0) {
        printf("failed\n");
        close(sockfd);
        return -1;
    }
    printf("ok\n");

    telnet_send_initial_negotiation();

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    while (!_ended) {
        int ret = poll(fds, 2, -1);

        if (ret < 0) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            if (handle_stdin_data() < 0) {
                break;
            }
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        }


        if (fds[1].revents & POLLIN) {
            if (handle_socket_data() < 0) {
                break;
            }
        }

        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            printf("\nConnection closed.\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}
