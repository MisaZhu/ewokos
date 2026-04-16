#include <stddef.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/proc.h>

#include "tinyhttpsc/tinyhttpsc.h"

static int parse_timeout_ms(const char *value) {
	int timeout = atoi(value);
	return timeout > 0 ? timeout : 5000;
}

#define EWOK_HTTPS_PREVIEW_LIMIT -1

static void print_preview(const char *body, int body_size) {
	int limit = EWOK_HTTPS_PREVIEW_LIMIT;
	if(limit < 0 || limit > body_size)
		limit = body_size;

	for (int i = 0; i < limit; ++i) {
		unsigned char ch = (unsigned char)body[i];
		if (ch == '\n' || ch == '\r' || ch == '\t' || (ch >= 32 && ch <= 126)) {
			putchar((int)ch);
		}
		else {
			putchar('.');
		}
	}
	if (limit > 0 && body[limit - 1] != '\n') {
		putchar('\n');
	}
}

static void print_usage(const char *prog) {
	printf("usage: %s <url> [timeout_ms]\n", prog);
	printf("example: %s https://dns.google/resolve?name=example.com&type=A 5000\n", prog);
	printf("example: %s http://httpbin.org/get 5000\n", prog);
}

int main(int argc, char **argv) {
	const char *url;
	const char *body;
	const char *content_type;
	TinyHttpsRequest *request;
	TinyHttpsResponse *response;
	int timeout_ms = 5000;
	int status;
	int body_size;

	if (argc < 2) {
		print_usage(argv[0]);
		return 1;
	}

	url = argv[1];
	if (argc > 2) {
		timeout_ms = parse_timeout_ms(argv[3]);
	}

	if (strncmp(url, "https://", 8) != 0 && strncmp(url, "http://", 7) != 0) {
		printf("error: URL must start with http:// or https://\n");
		return 1;
	}

	request = NewHttpsRequest(url);
	if (request == NULL) {
		printf("error: cannot allocate request\n");
		return 1;
	}

	HttpsRequestSetTimeout(request, timeout_ms);
	HttpsRequestSetMaxRedirections(request, 2);
	HttpsRequestAddHeader(request, "User-Agent", "ewokos-https-test/1");

	//printf("requesting %s (timeout=%dms)\n", url, timeout_ms);
	response = HttpsRequestFetch(request);
	if (response == NULL) {
		printf("error: fetch returned null response\n");
		HttpsRequestFree(request);
		return 1;
	}

	if (HttpsResponseError(response)) {
		int ssl_error = 0;
		//if (response->is_https) {
		//	ssl_error = br_ssl_engine_last_error(&response->ssl_client.eng);
		//}
		printf("error: %s (code=%d, ssl=%d)\n",
			HttpsResponseGetErrorMsg(response),
			HttpsResponseGetErrorCode(response),
			ssl_error);
		HttpsResponseFree(response);
		HttpsRequestFree(request);
		return 2;
	}

	status = HttpsResponseGetStatusCode(response);
	content_type = HttpsResponseGetHeaderValueByKey(response, "content-type");
	body = HttpsResponseReadBodyStr(response, &body_size);

	//printf("status: %d\n", status);
	if (status >= 300 && status < 400) {
		const char *location = HttpsResponseGetHeaderValueByKey(response, "location");
		if (location != NULL) {
			printf("location: %s\n", location);
		}
	}
	//printf("body_size: %d\n", body_size);
	if (content_type != NULL) {
		printf("content_type: %s\n", content_type);
	}

	if (body != NULL && body_size > 0) {
		print_preview(body, body_size);
	}

	HttpsResponseFree(response);
	HttpsRequestFree(request);
	return 0;
}
