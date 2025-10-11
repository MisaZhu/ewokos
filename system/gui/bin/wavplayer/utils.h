
#ifndef ROKID_LOG_H_
#define ROKID_LOG_H_

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

extern void x_log(int logLevel, const char* tag, const char *fmt, ...);

enum log_level {
  VERBOSE_L = 0,
  ERROR_L,
  DEBUG_L,
  WARNING_L,
};

/*#define LOGV(FMT, ...) x_log(VERBOSE_L, TAG, FMT, ##__VA_ARGS__)
#define LOGD(FMT, ...) x_log(DEBUG_L, TAG, FMT, ##__VA_ARGS__)
#define LOGE(FMT, ...) x_log(ERROR_L, TAG, FMT, ##__VA_ARGS__)
#define LOGW(FMT, ...) x_log(WARNING_L, TAG, FMT, ##__VA_ARGS__)
*/

#define LOGV(FMT, ...) slog(FMT, ##__VA_ARGS__)
#define LOGD(FMT, ...) slog(FMT, ##__VA_ARGS__)
#define LOGE(FMT, ...) slog(FMT, ##__VA_ARGS__)
#define LOGW(FMT, ...) slog(FMT, ##__VA_ARGS__)

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

int parse_wav_header(FILE *file, struct chunk_fmt *wavFormat);

#endif