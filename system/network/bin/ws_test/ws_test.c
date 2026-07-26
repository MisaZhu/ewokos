/**
 * ws_test - WebSocket test tool for EwokOS libwebsockets port
 *
 * Usage:
 *   ws_test -s [port]          Run WebSocket echo server (default port 9001)
 *   ws_test -c <host> [port]   Connect as client, send test messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libwebsockets/libwebsockets.h>

#define DEFAULT_PORT 9001
#define MAX_MSG_SIZE 1024

/* ---------- echo server protocol ---------- */

struct per_session_data {
	int msg_count;
};

static int callback_echo(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	struct per_session_data *pss = (struct per_session_data *)user;
	unsigned char buf[LWS_PRE + MAX_MSG_SIZE];

	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED:
		printf("[server] client connected\n");
		pss->msg_count = 0;
		break;

	case LWS_CALLBACK_RECEIVE:
		pss->msg_count++;
		printf("[server] recv #%d (%d bytes): %.*s\n",
			pss->msg_count, (int)len, (int)len, (char *)in);

		/* echo back */
		if (len < MAX_MSG_SIZE) {
			memcpy(&buf[LWS_PRE], in, len);
			lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
		}
		break;

	case LWS_CALLBACK_CLOSED:
		printf("[server] client disconnected (total msgs: %d)\n",
			pss->msg_count);
		break;

	default:
		break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
	{
		"ws-echo",
		callback_echo,
		sizeof(struct per_session_data),
		MAX_MSG_SIZE,
		0, NULL
	},
	{ NULL, NULL, 0, 0, 0, NULL } /* terminator */
};

/* ---------- server mode ---------- */

static int run_server(int port)
{
	struct lws_context_creation_info info;
	struct lws_context *context;

	memset(&info, 0, sizeof(info));
	info.port = port;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	printf("[server] starting WebSocket echo server on port %d\n", port);

	context = lws_create_context(&info);
	if (!context) {
		printf("[server] ERROR: failed to create context\n");
		return 1;
	}

	printf("[server] listening... (Ctrl+C to stop)\n");

	while (1) {
		lws_service(context, 50);
	}

	lws_context_destroy(context);
	return 0;
}

/* ---------- client mode ---------- */

static int client_done = 0;
static int client_msg_count = 0;

static int callback_client(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	unsigned char buf[LWS_PRE + MAX_MSG_SIZE];
	int n;

	(void)user;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		printf("[client] connected to server\n");
		/* send first test message */
		n = snprintf((char *)&buf[LWS_PRE], MAX_MSG_SIZE,
			"Hello from EwokOS! msg#1");
		lws_write(wsi, &buf[LWS_PRE], (size_t)n, LWS_WRITE_TEXT);
		client_msg_count = 1;
		printf("[client] sent: %s\n", &buf[LWS_PRE]);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		printf("[client] echo received (%d bytes): %.*s\n",
			(int)len, (int)len, (char *)in);

		/* send more messages up to 5 */
		if (client_msg_count < 5) {
			client_msg_count++;
			n = snprintf((char *)&buf[LWS_PRE], MAX_MSG_SIZE,
				"Hello from EwokOS! msg#%d", client_msg_count);
			lws_write(wsi, &buf[LWS_PRE], (size_t)n, LWS_WRITE_TEXT);
			printf("[client] sent: %s\n", &buf[LWS_PRE]);
		} else {
			printf("[client] all messages echoed successfully!\n");
			lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
			client_done = 1;
		}
		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		printf("[client] connection error: %s\n",
			in ? (char *)in : "unknown");
		client_done = 1;
		break;

	case LWS_CALLBACK_CLOSED:
		printf("[client] connection closed\n");
		client_done = 1;
		break;

	default:
		break;
	}

	return 0;
}

static struct lws_protocols client_protocols[] = {
	{
		"ws-echo",
		callback_client,
		0,
		MAX_MSG_SIZE,
		0, NULL
	},
	{ NULL, NULL, 0, 0, 0, NULL }
};

static int run_client(const char *host, int port)
{
	struct lws_context_creation_info info;
	struct lws_client_connect_info ccinfo;
	struct lws_context *context;
	struct lws *wsi;

	memset(&info, 0, sizeof(info));
	info.port = -1; /* client only, no listen */
	info.protocols = client_protocols;
	info.gid = -1;
	info.uid = -1;

	printf("[client] connecting to ws://%s:%d/\n", host, port);

	context = lws_create_context(&info);
	if (!context) {
		printf("[client] ERROR: failed to create context\n");
		return 1;
	}

	memset(&ccinfo, 0, sizeof(ccinfo));
	ccinfo.context = context;
	ccinfo.address = host;
	ccinfo.port = port;
	ccinfo.path = "/";
	ccinfo.host = host;
	ccinfo.protocol = "ws-echo";

	wsi = lws_client_connect_via_info(&ccinfo);
	if (!wsi) {
		printf("[client] ERROR: failed to connect\n");
		lws_context_destroy(context);
		return 1;
	}

	/* service loop until done */
	while (!client_done) {
		lws_service(context, 50);
	}

	lws_context_destroy(context);
	printf("[client] test complete\n");
	return 0;
}

/* ---------- main ---------- */

static void usage(const char *prog)
{
	printf("WebSocket test tool (libwebsockets for EwokOS)\n");
	printf("Usage:\n");
	printf("  %s -s [port]          Run echo server (default port %d)\n",
		prog, DEFAULT_PORT);
	printf("  %s -c <host> [port]   Connect as client and run echo test\n",
		prog);
	printf("\nExamples:\n");
	printf("  %s -s 9001            Start server on port 9001\n", prog);
	printf("  %s -c 192.168.1.10    Connect to server at 192.168.1.10:9001\n",
		prog);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-s") == 0) {
		int port = DEFAULT_PORT;
		if (argc > 2)
			port = atoi(argv[2]);
		return run_server(port);
	}

	if (strcmp(argv[1], "-c") == 0) {
		if (argc < 3) {
			printf("error: -c requires <host>\n");
			usage(argv[0]);
			return 1;
		}
		const char *host = argv[2];
		int port = DEFAULT_PORT;
		if (argc > 3)
			port = atoi(argv[3]);
		return run_client(host, port);
	}

	usage(argv[0]);
	return 1;
}
