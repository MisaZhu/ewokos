#include "tinyhttpsc/tinyhttpsc.h"

TinyHttpsRequest* NewHttpsRequest(const char* url) {
	return newBearHttpsRequest(url);
}
void HttpsRequestFree(TinyHttpsRequest* request) {
	BearHttpsRequest_free(request);
}
void HttpsResponseFree(TinyHttpsResponse* response) {
	BearHttpsResponse_free(response);
}

void HttpsRequestSetMaxRedirections(TinyHttpsRequest* request, int max_redirections) {
	BearHttpsRequest_set_max_redirections(request, max_redirections);
}
 
void HttpsRequestSetTimeout(TinyHttpsRequest* request, int timeout_ms) {
	BearHttpsRequest_set_timeout(request, timeout_ms);
}

void HttpsRequestAddHeader(TinyHttpsRequest* request, const char* key, const char* value) {
	BearHttpsRequest_add_header(request, key, value);
}
TinyHttpsResponse* HttpsRequestFetch(TinyHttpsRequest* request) {
	return BearHttpsRequest_fetch(request);
}
bool HttpsResponseError(TinyHttpsResponse* response) {
	return BearHttpsResponse_error(response);
}
const char* HttpsResponseGetErrorMsg(TinyHttpsResponse* response) {
	return BearHttpsResponse_get_error_msg(response);
}
int HttpsResponseGetErrorCode(TinyHttpsResponse* response) {
	return BearHttpsResponse_get_error_code(response);
}
int HttpsResponseGetStatusCode(TinyHttpsResponse* response) {
	return BearHttpsResponse_get_status_code(response);
}
int HttpsResponseGetBodySize(TinyHttpsResponse* response) {
	return BearHttpsResponse_get_body_size(response);
}
const char* HttpsResponseGetHeaderValueByKey(TinyHttpsResponse* response, const char* key) {
	return BearHttpsResponse_get_header_value_by_sanitized_key(response, key);
}

const char* HttpsResponseReadBody(TinyHttpsResponse* response, int* size) {
	const char* ret = (const char*)BearHttpsResponse_read_body(response);
	if(size != NULL)
		*size = BearHttpsResponse_get_body_size(response);
	return ret;
}

const char* HttpsResponseReadBodyStr(TinyHttpsResponse* response, int* size) {
	const char* ret = BearHttpsResponse_read_body_str(response);
	if(size != NULL)
		*size = BearHttpsResponse_get_body_size(response);
	return ret;
}

void HttpsRequestSendAny(TinyHttpsRequest *self, unsigned char *content, long size) {
	BearHttpsRequest_send_any(self, content, size);
}

void HttpsRequestSendBodyStr(TinyHttpsRequest *self, char *content) {
	BearHttpsRequest_send_body_str(self, content);
}

void HttpsRequestSetMethod(TinyHttpsRequest *self, const char *method) {
	BearHttpsRequest_set_method(self, method);
}

int HttpsResponseReadBodyChunk(TinyHttpsResponse* response, unsigned char* buffer, int size) {
	if (response == NULL || buffer == NULL || size <= 0) {
		return -1;
	}
	return BearHttpsResponse_read_body_chunck(response, buffer, size);
}

int HttpsResponseReadBodyStream(TinyHttpsResponse* response, StreamReadCallback callback, void* user_data) {
	if (response == NULL || callback == NULL) {
		return -1;
	}
	
	// Check if there's an error
	if (BearHttpsResponse_error(response)) {
		return -1;
	}
	
	unsigned char buffer[4096];
	int total_read = 0;
	
	while (1) {
		int bytes_read = BearHttpsResponse_read_body_chunck(response, buffer, sizeof(buffer));
		
		if (bytes_read < 0) {
			// Error occurred
			return -1;
		}
		
		if (bytes_read == 0) {
			// End of stream
			break;
		}
		
		total_read += bytes_read;
		
		// Call the callback with the received data
		int result = callback((const char*)buffer, bytes_read, user_data);
		if (result != 0) {
			// Callback requested to abort
			return result;
		}
	}
	
	return 0;
}

bool HttpsResponseHasMoreData(TinyHttpsResponse* response) {
	if (response == NULL) {
		return false;
	}
	
	if (BearHttpsResponse_error(response)) {
		return false;
	}
	
	// Get current read size and total content length
	int body_size = BearHttpsResponse_get_body_size(response);
	// This is a simplified check - in practice, you might need to track
	// the actual bytes read vs expected
	return body_size >= 0;
}
