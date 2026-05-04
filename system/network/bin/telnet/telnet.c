#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/fcntl.h>

#define BUFFER_SIZE 1024

bool _ended = false;
int sockfd = -1;

/* Handle a complete IAC command (2 bytes after IAC) */
static void handle_iac_cmd(uint8_t verb, uint8_t opt) {
    /*
     * Telnet verbs:
     *   251 (0xfb) = WILL
     *   252 (0xfc) = WONT
     *   253 (0xfd) = DO
     *   254 (0xfe) = DONT
     *   250 (0xfa) = SB (subnegotiation begin)
     *   240 (0xf0) = SE (subnegotiation end)
     *   255 (0xff) = IAC (data byte 0xff)
     *
     * We silently ignore most options. For a full telnet client,
     * we would negotiate terminal type, window size, echo, etc.
     */
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
                /* IAC IAC = literal 0xff byte */
                _iac_state = STATE_DATA;
                return 0;
            }
            if(c == 0xfa) {
                /* SB - subnegotiation begin */
                _iac_state = STATE_CMD;
                _iac_verb = c;
                return -1;
            }
            if(c == 0xf0) {
                /* SE - subnegotiation end (should not happen without SB) */
                _iac_state = STATE_DATA;
                return -1;
            }
            /* DO/DONT/WILL/WONT - expect one option byte */
            _iac_verb = c;
            _iac_state = STATE_CMD;
            return -1;

        case STATE_CMD:
            if(_iac_verb == 0xfa) {
                /* In subnegotiation, collect bytes until SE */
                if(c == 0xf0) {
                    /* End of subnegotiation */
                    _iac_state = STATE_DATA;
                    return -1;
                }
                if(c == 0xff) {
                    /* IAC inside SB - next byte should be SE or IAC */
                    _iac_verb = 0xff;
                    return -1;
                }
                return -1;
            }
            /* DO/DONT/WILL/WONT - c is the option byte */
            handle_iac_cmd(_iac_verb, c);
            _iac_state = STATE_DATA;
            return -1;

        default:
            _iac_state = STATE_DATA;
            return -1;
    }
}

static void *receive_thread(void *arg) {
    uint8_t buffer[BUFFER_SIZE];
    int n;
        
    while (!_ended) {
        n = read(sockfd, buffer, BUFFER_SIZE);
        if (n > 0) {
            for(int i = 0; i < n; i++) {
                if(buffer[i] == '\r') {
                    /* Skip CR - telnet uses CR LF for newline */
                    continue;
                }
                if(process_iac_byte(buffer[i]) == 0) {
                    /* Not a telnet command byte, write to stdout */
                    int sz = write(STDOUT_FILENO, &buffer[i], 1);
                    if(sz <= 0) {
                        _ended = true;
                        break;
                    }
                }
            }
        } else if (n == 0) {
            printf("\nConnection closed by remote host (read returned 0, EOF).\n");
            break;
        } else {
            if(errno != EAGAIN) {
                printf("\nConnection closed by remote host (read returned %d, errno=%d: %s).\n",
                       n, errno, strerror(errno));
            }
            break;
        }
    }
    _ended = true;
    return NULL;
}

bool sepcial_char_check(uint8_t c) {
    // Special character handling
    if (c == 0x1D) { // Ctrl+]
        printf("\nCtrl+] Exiting connection...\n");
        _ended = true;
        return true;
    } else if (c == 0x04) { // Ctrl+D
        printf("\nCtrl+D Exiting connection...\n");
        _ended = true;
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t tid;
    int n;
    struct hostent *he;
    char buf[BUFFER_SIZE];
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <host> [port]\n", argv[0]);
        exit(1);
    }
    
    setbuf(stdout, NULL);
    
    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (argc > 2)
        server_addr.sin_port = htons(atoi(argv[2]));
    else
        server_addr.sin_port = htons(23);

    // Try to resolve by hostname
    he = gethostbyname(argv[1]);
    if (he == NULL) {
        // Try to resolve IP address directly
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
            fprintf(stderr, "Invalid address or hostname: %s\n", argv[1]);
            return -1;
        }
    } else {
        // Use resolved IP address
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    /*struct timeval timeout;
    timeout.tv_sec = 3; // 3 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }
    */
    
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
    if (he != NULL) {
        printf("Connecting to %s (%s):%s ... ", argv[1], addr_str, argv[2]);
    } else {
        printf("Connecting to %s:%s ... ", argv[1], argv[2]);
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("failed\n");
        close(sockfd);
        return -1;
    }
    printf("ok\n");
    
    if (pthread_create(&tid, NULL, receive_thread, NULL) != 0) {
        close(sockfd);
        return -1;
    }

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    
    while (!_ended) {
        n = read(STDIN_FILENO, buf, BUFFER_SIZE - 1);
        if (n > 0) {
            buf[n] = '\0';
            for(int i = 0; i < n; i++) {
                if(buf[i] == '\n') {
                    buf[i] = '\r';
                }
                if(sepcial_char_check(buf[i]))
                    break;
            }
            if(_ended)
                break;

            // Send normal characters
            int sz =  write(sockfd, buf, n);
            if(sz < 0) {
                printf("\nEOF received. Exiting...\n");
                _ended = true;
                break;
            }
        } 
        else if(errno != EAGAIN && errno != 0){
            _ended = true;
            break;
        }
        usleep(10000);
    }
    // Wait for receive thread to finish
    pthread_join(tid, NULL);
    // Close connection
    close(sockfd);

    while (!_ended) {
        usleep(10000);
    }

    return 0;
}
