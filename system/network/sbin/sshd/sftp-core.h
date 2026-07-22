#ifndef SFTP_CORE_H
#define SFTP_CORE_H

#include <stddef.h>
#include <stdint.h>

typedef int (*sftp_emit_fn)(void *user, const uint8_t *data, size_t len);

typedef struct {
    sftp_emit_fn emit;
    void *user;
} sftp_server_io_t;

typedef struct sftp_server sftp_server_t;

sftp_server_t *sftp_server_create(const sftp_server_io_t *io);
void sftp_server_destroy(sftp_server_t *srv);
int sftp_server_feed(sftp_server_t *srv, const uint8_t *data, size_t len);

#endif
