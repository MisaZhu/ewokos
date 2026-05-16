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

static volatile int _ended = 0;
static int sockfd = -1;

/* Handle a complete IAC command (2 bytes after IAC) */
static void handle_iac_cmd(uint8_t verb, uint8_t opt) {
    (void)verb;
    (void)opt;
}

/*
 * Process a single byte from the telnet stream.
 * Returns 0 if the byte should be passed to stdout, -1 otherwise.
 */
static int process_iac_byte(uint8_t c) {
    static enum { STATE_DATA, STATE_IAC, STATE_CMD } _iac_state = STATE_DATA;
    static uint8_t _iac_verb = 0;

    switch(_iac_state) {
        case STATE_DATA:
            if(c == 0xff) {
                _iac_state = STATE_IAC;
                return -1;
            }
            return 0;

        case STATE_IAC:
            if(c == 0xff) {
                _iac_state = STATE_DATA;
                return 0;
            }
            if(c == 0xfa) {
                _iac_state = STATE_CMD;
                _iac_verb = c;
                return -1;
            }
            if(c == 0xf0) {
                _iac_state = STATE_DATA;
                return -1;
            }
            _iac_verb = c;
            _iac_state = STATE_CMD;
            return -1;

        case STATE_CMD:
            if(_iac_verb == 0xfa) {
                if(c == 0xf0) {
                    _iac_state = STATE_DATA;
                    return -1;
                }
                if(c == 0xff) {
                    _iac_verb = 0xff;
                    return -1;
                }
                return -1;
            }
            handle_iac_cmd(_iac_verb, c);
            _iac_state = STATE_DATA;
            return -1;

        default:
            _iac_state = STATE_DATA;
            return -1;
    }
}

static int handle_socket_data(void) {
    uint8_t buffer[BUFFER_SIZE];
    int n;

    //klog("handle_socket_data\n");
    n = read(sockfd, buffer, BUFFER_SIZE);
    //klog("handle_socket_data read: %d\n", n);

    if (n > 0) {
        for(int i = 0; i < n; i++) {
            if(buffer[i] == '\r') {
                continue;
            }
            if(process_iac_byte(buffer[i]) == 0) {
                int sz = write(STDOUT_FILENO, &buffer[i], 1);
                if(sz <= 0) {
                    return -1;
                }
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

static int handle_stdin_data(void) {
    char buf[BUFFER_SIZE];
    int n;

    n = read(STDIN_FILENO, buf, BUFFER_SIZE - 1);

    if (n > 0) {
        for(int i = 0; i < n; i++) {
            if(buf[i] == '\n') {
                buf[i] = '\r';
            }
            if(special_char_check(buf[i])) {
                return -1;
            }
        }

        int sz = write(sockfd, buf, n);
        if(sz < 0) {
            printf("\nEOF received. Exiting...\n");
            return -1;
        }
    } else if (n == 0) {
        return -1;
    } else {
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
    printf("ok\n");

    //fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    //fcntl(sockfd, F_SETFL, O_NONBLOCK);

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
