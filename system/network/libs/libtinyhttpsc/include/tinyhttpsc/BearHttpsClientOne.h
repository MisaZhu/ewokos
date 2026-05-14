/* MIT License

Copyright (c) 2025 OUI

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#ifndef BEARHTTPS_CLIENT_ONE_H
#define BEARHTTPS_CLIENT_ONE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Forward declarations */
typedef struct BearHttpsRequest BearHttpsRequest;
typedef struct BearHttpsResponse BearHttpsResponse;
typedef struct BearHttpsClientDnsProvider BearHttpsClientDnsProvider;

/* BearHttpsRequest functions */
BearHttpsRequest * newBearHttpsRequest_with_url_ownership_config(char *url, short url_ownership_mode);
BearHttpsRequest * newBearHttpsRequest(const char *url);
BearHttpsRequest * newBearHttpsRequest_fmt(const char *url, ...);
void BearHttpsRequest_set_url_with_ownership_config(BearHttpsRequest *self, char *url, short url_ownership_mode);
void BearHttpsRequest_set_url(BearHttpsRequest *self, const char *url);
void BearHttpsRequest_set_timeout(BearHttpsRequest *self, int timeout_ms);
void BearHttpsRequest_add_header_with_ownership_config(BearHttpsRequest *self, char *key, short key_ownership_mode, char *value, short value_owner);
void BearHttpsRequest_add_header(BearHttpsRequest *self, const char *key, const char *value);
void BearHttpsRequest_add_header_fmt(BearHttpsRequest *self, const char *key, const char *format, ...);
void BearHttpsRequest_set_method(BearHttpsRequest *self, const char *method);
void BearHttpsRequest_set_max_redirections(BearHttpsRequest *self, int max_redirections);
void BearHttpsRequest_set_dns_providers(BearHttpsRequest *self, BearHttpsClientDnsProvider *dns_providers, int total_dns_proviers);
void BearHttpsRequest_set_chunk_header_read_props(BearHttpsRequest *self, int chunk_size, int max_chunk_size);
void BearHttpsRequest_set_trusted_anchors(BearHttpsRequest *self, void *trust_anchors, size_t trusted_anchors_size);
void BearHttpsRequest_represent(BearHttpsRequest *self);
void BearHttpsRequest_free(BearHttpsRequest *self);
void BearHttpsRequest_send_any_with_ownership_control(BearHttpsRequest *self, unsigned char *content, long size, short ownership_mode);
void BearHttpsRequest_send_any(BearHttpsRequest *self, unsigned char *content, long size);
void BearHttpsRequest_send_body_str_with_ownership_control(BearHttpsRequest *self, char *content, short ownership_mode);
void BearHttpsRequest_send_body_str(BearHttpsRequest *self, char *content);
void BearHttpsRequest_send_file_with_ownership_control(BearHttpsRequest *self, char *path, short ownership_mode, const char *content_type);
void BearHttpsRequest_send_file(BearHttpsRequest *self, const char *path, const char *content_type);
void BearHttpsRequest_send_file_auto_detect_content_type(BearHttpsRequest *self, const char *path);
void BearHttpsRequest_send_cJSON_with_ownership_control(BearHttpsRequest *self, void *json, short ownership_mode);
void BearHttpsRequest_send_cJSON(BearHttpsRequest *self, void *json);
void * BearHttpsRequest_create_cJSONPayloadObject(BearHttpsRequest *self);
void * BearHttpsRequest_create_cJSONPayloadArray(BearHttpsRequest *self);
BearHttpsResponse * BearHttpsRequest_fetch(BearHttpsRequest *self);

/* BearHttpsResponse functions */
int BearHttpsResponse_read_body_chunck(BearHttpsResponse *self, unsigned char *buffer, long size);
const unsigned char * BearHttpsResponse_read_body(BearHttpsResponse *self);
const char * BearHttpsResponse_read_body_str(BearHttpsResponse *self);
int BearHttpsResponse_get_status_code(BearHttpsResponse *self);
int BearHttpsResponse_get_body_size(BearHttpsResponse *self);
int BearHttpsResponse_get_headers_size(BearHttpsResponse *self);
const char * BearHttpsResponse_get_header_value_by_index(BearHttpsResponse *self, int index);
const char * BearHttpsResponse_get_header_key_by_index(BearHttpsResponse *self, int index);
const char * BearHttpsResponse_get_header_value_by_key(BearHttpsResponse *self, const char *key);
const char * BearHttpsResponse_get_header_value_by_sanitized_key(BearHttpsResponse *self, const char *key);
void BearHttpsResponse_set_max_body_size(BearHttpsResponse *self, long size);
void BearHttpsResponse_set_body_read_props(BearHttpsResponse *self, int chunk_size, double realloc_factor);
bool BearHttpsResponse_error(BearHttpsResponse *self);
const char * BearHttpsResponse_get_error_msg(BearHttpsResponse *self);
int BearHttpsResponse_get_error_code(BearHttpsResponse *self);
void BearHttpsResponse_free(BearHttpsResponse *self);
void * BearHttpsResponse_read_body_json(BearHttpsResponse *self);

/* BearSSL functions */
int br_ssl_engine_last_error(void *eng);

/* Ewok compatibility functions */
int ewok_https_validation_time(uint32_t *days, uint32_t *seconds);
void ewok_https_entropy_fill(void *buf, size_t len);
int ewok_https_open_compat(const char *path, int flags, ...);
ssize_t ewok_https_read_compat(int fd, void *buf, size_t len);
int ewok_https_close_compat(int fd);
int ewok_https_fcntl_compat(int fd, int cmd, ...);
int ewok_https_select_compat(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);
int ewok_https_getsockopt_compat(int fd, int level, int optname, void *optval, void *optlen);
int ewok_https_getentropy_compat(void *buf, size_t len);
const char* ewok_gai_strerror_compat(int ecode);
void ewok_freeaddrinfo_compat(struct addrinfo *res);
uint64_t ewok_https_entropy_word(void);
int ewok_https_month_from_abbrev(const char *mon);
int64_t ewok_https_days_from_civil(int year, unsigned month, unsigned day);

#endif /* BEARHTTPS_CLIENT_ONE_H */
