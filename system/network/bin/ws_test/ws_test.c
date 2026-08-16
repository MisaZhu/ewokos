/**
 * ws_test - WebSocket echo test client for EwokOS
 *
 * Connects to wss://ws.ifelse.io (echo service), sends test messages,
 * verifies echo responses.
 *
 * Usage: ws_test [host] [port]
 *   Default: ws.ifelse.io:443 (WSS)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libwebsockets/libwebsockets.h>

#define DEFAULT_HOST "ws.ifelse.io"
#define DEFAULT_PORT 443
#define MAX_MSG_SIZE 1024
#define TEST_MSG_COUNT 3

static const char *test_messages[TEST_MSG_COUNT] = {
    "Hello from EwokOS!",
    "libwebsockets port test - binary safe \x01\x02\x03",
    "Final message. Goodbye!"
};

static int done = 0;
static int msg_index = 0;
static int success_count = 0;
static int greeting_consumed = 0;

static int callback_echo_client(struct lws *wsi,
        enum lws_callback_reasons reason,
        void *user, void *in, size_t len)
{
    unsigned char buf[LWS_PRE + MAX_MSG_SIZE];
    int n;

    (void)user;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("[ws] connected! waiting for server greeting...\n");
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        /* consume server greeting first */
        if (!greeting_consumed) {
            printf("[ws] server greeting: \"%.*s\"\n",
                (int)len, (char *)in);
            greeting_consumed = 1;
            /* now send first test message */
            printf("[ws] sending %d test messages...\n",
                TEST_MSG_COUNT);
            n = (int)strlen(test_messages[0]);
            memcpy(&buf[LWS_PRE], test_messages[0], (size_t)n);
            lws_write(wsi, &buf[LWS_PRE], (size_t)n,
                LWS_WRITE_TEXT);
            printf("[ws] sent #%d: \"%s\"\n", msg_index + 1,
                test_messages[msg_index]);
            break;
        }

        /* verify echo matches what we sent */
        const char *expected = test_messages[msg_index];
        int exp_len = (int)strlen(expected);

        if ((int)len == exp_len &&
            memcmp(in, expected, len) == 0) {
            printf("[ws] echo #%d OK (%d bytes)\n",
                msg_index + 1, (int)len);
            success_count++;
        } else {
            printf("[ws] echo #%d MISMATCH! expected %d bytes, got %d\n",
                msg_index + 1, exp_len, (int)len);
            printf("  expected: \"%.*s\"\n", exp_len, expected);
            printf("  received: \"%.*s\"\n", (int)len, (char *)in);
        }

        msg_index++;

        if (msg_index < TEST_MSG_COUNT) {
            /* send next message */
            n = (int)strlen(test_messages[msg_index]);
            memcpy(&buf[LWS_PRE], test_messages[msg_index], (size_t)n);
            lws_write(wsi, &buf[LWS_PRE], (size_t)n, LWS_WRITE_TEXT);
            printf("[ws] sent #%d: \"%s\"\n", msg_index + 1,
                test_messages[msg_index]);
        } else {
            /* all done */
            printf("[ws] closing connection\n");
            lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
            done = 1;
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        printf("[ws] CONNECTION ERROR: %s\n",
            in ? (char *)in : "unknown");
        done = 1;
        break;

    case LWS_CALLBACK_CLOSED:
        printf("[ws] connection closed\n");
        done = 1;
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "",  /* default protocol (echo) */
        callback_echo_client,
        0,
        MAX_MSG_SIZE,
        0, NULL
    },
    { NULL, NULL, 0, 0, 0, NULL }
};

int main(int argc, char **argv)
{
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;
    struct lws *wsi;
    int timeout_count = 0;

    if (argc > 1)
        host = argv[1];
    if (argc > 2)
        port = atoi(argv[2]);

    printf("=== EwokOS WebSocket Echo Test (WSS) ===\n");
    printf("target: wss://%s:%d/\n\n", host, port);

    memset(&info, 0, sizeof(info));
    info.port = -1; /* client only */
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        printf("FATAL: lws_create_context failed\n");
        return 1;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = host;
    ccinfo.port = port;
    ccinfo.ssl_connection = 1; /* WSS */
    ccinfo.path = "/";
    ccinfo.host = host;
    ccinfo.protocol = "";

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        printf("FATAL: connect to %s:%d failed\n", host, port);
        lws_context_destroy(context);
        return 1;
    }

    printf("[wss] connecting to %s:%d ...\n", host, port);

    while (!done) {
        lws_service(context, 100);
        timeout_count++;
        /* 30 second timeout */
        if (timeout_count > 300) {
            printf("[ws] TIMEOUT after 30s\n");
            break;
        }
    }

    lws_context_destroy(context);

    printf("\n=== Result: %d/%d echo tests passed ===\n",
        success_count, TEST_MSG_COUNT);

    return (success_count == TEST_MSG_COUNT) ? 0 : 1;
}
