#ifndef TINYHTTPSC_H
#define TINYHTTPSC_H

#include <tinyhttpsc/BearHttpsClientOne.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef BearHttpsRequest TinyHttpsRequest;
typedef BearHttpsResponse TinyHttpsResponse;

/**
 * @brief Callback function type for stream reading
 * @param data Pointer to received data chunk
 * @param size Size of the data chunk in bytes
 * @param user_data User-provided context pointer
 * @return 0 to continue reading, non-zero to abort
 */
typedef int (*StreamReadCallback)(const char* data, int size, void* user_data);

/**
 * @brief Create a new HTTPS request
 * @param url The target URL (e.g., "https://example.com/api")
 * @return Pointer to the request object, or NULL on failure
 */
TinyHttpsRequest* NewHttpsRequest(const char* url);

/**
 * @brief Free an HTTPS request object and release resources
 * @param request The request object to free
 */
void HttpsRequestFree(TinyHttpsRequest* request);

/**
 * @brief Free an HTTPS response object and release resources
 * @param response The response object to free
 */
void HttpsResponseFree(TinyHttpsResponse* response);

/**
 * @brief Set timeout for the HTTPS request
 * @param request The request object
 * @param timeout_ms Timeout value in milliseconds
 */
void HttpsRequestSetTimeout(TinyHttpsRequest* request, int timeout_ms);

/**
 * @brief Set maximum number of redirections to follow
 * @param request The request object
 * @param max_redirections Maximum number of redirections (0 to disable)
 */
void HttpsRequestSetMaxRedirections(TinyHttpsRequest* request, int max_redirections);

/**
 * @brief Add a header to the HTTPS request
 * @param request The request object
 * @param key Header name (e.g., "Content-Type")
 * @param value Header value (e.g., "application/json")
 */
void HttpsRequestAddHeader(TinyHttpsRequest* request, const char* key, const char* value);


/**
 * @brief Send binary data as request body
 * @param self The request object
 * @param content Pointer to binary data
 * @param size Size of the data in bytes
 */
void HttpsRequestSendAny(TinyHttpsRequest *self, unsigned char *content, long size);

/**
 * @brief Send string as request body
 * @param self The request object
 * @param content Null-terminated string to send
 */
void HttpsRequestSendBodyStr(TinyHttpsRequest *self, char *content);

/**
 * @brief Set HTTP method for the request
 * @param self The request object
 * @param method HTTP method (e.g., "GET", "POST", "PUT", "DELETE")
 */
void HttpsRequestSetMethod(TinyHttpsRequest *self, const char *method);

/**
 * @brief Execute the HTTPS request and get the response
 * @param request The request object
 * @return Pointer to the response object, or NULL on failure
 */
TinyHttpsResponse* HttpsRequestFetch(TinyHttpsRequest* request);

/**
 * @brief Check if the response has an error
 * @param response The response object
 * @return true if error occurred, false otherwise
 */
bool HttpsResponseError(TinyHttpsResponse* response);

/**
 * @brief Get error message from the response
 * @param response The response object
 * @return Error message string, or NULL if no error
 */
const char* HttpsResponseGetErrorMsg(TinyHttpsResponse* response);

/**
 * @brief Get error code from the response
 * @param response The response object
 * @return Error code (0 if no error)
 */
int HttpsResponseGetErrorCode(TinyHttpsResponse* response);

/**
 * @brief Get HTTP status code from the response
 * @param response The response object
 * @return HTTP status code (e.g., 200, 404, 500)
 */
int HttpsResponseGetStatusCode(TinyHttpsResponse* response);

/**
 * @brief Get the size of the response body
 * @param response The response object
 * @return Body size in bytes
 */
int HttpsResponseGetBodySize(TinyHttpsResponse* response);

/**
 * @brief Get header value by key
 * @param response The response object
 * @param key Header name to look up
 * @return Header value string, or NULL if not found
 */
const char* HttpsResponseGetHeaderValueByKey(TinyHttpsResponse* response, const char* key);

/**
 * @brief Read the entire response body as binary data
 * @param response The response object
 * @param size Output parameter to receive the body size
 * @return Pointer to body data, or NULL on error
 */
const char* HttpsResponseReadBody(TinyHttpsResponse* response, int* size);

/**
 * @brief Read the entire response body as null-terminated string
 * @param response The response object
 * @param size Output parameter to receive the body size
 * @return Pointer to body string, or NULL on error
 */
const char* HttpsResponseReadBodyStr(TinyHttpsResponse* response, int* size);

/**
 * @brief Read a chunk of the response body (for streaming/SSE support)
 * @param response The response object
 * @param buffer Buffer to store the read data
 * @param size Maximum number of bytes to read
 * @return Number of bytes read, 0 if end of stream, -1 on error
 */
int HttpsResponseReadBodyChunk(TinyHttpsResponse* response, unsigned char* buffer, int size);

/**
 * @brief Read response body in streaming mode with callback
 * @param response The response object
 * @param callback Function called for each received chunk
 * @param user_data User context pointer passed to callback
 * @return 0 on success, non-zero on error
 */
int HttpsResponseReadBodyStream(TinyHttpsResponse* response, StreamReadCallback callback, void* user_data);

/**
 * @brief Check if response has more data to read (for streaming)
 * @param response The response object
 * @return true if more data available, false otherwise
 */
bool HttpsResponseHasMoreData(TinyHttpsResponse* response);

#ifdef __cplusplus
}
#endif

#endif
