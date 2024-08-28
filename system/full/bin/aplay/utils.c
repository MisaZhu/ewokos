
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <ewoksys/klog.h>

#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

typedef struct {
	char* p;
	uint32_t index;
	uint32_t size;
} outc_arg_t;

static void outc_sn(char c, void* p) {
	outc_arg_t* arg = (outc_arg_t*)p;
	if(arg->index >= arg->size)
		return;
	arg->p[arg->index] = c;
	arg->index++;
}

extern void x_log(int logLevel, const char* tag, const char *fmt, ...)
{
    if (logLevel <= VERBOSE_L) {
        return;
    };

    if (tag == NULL) {
        fprintf(stderr, "Error! TAG Not defined!\n");
        return;
    }

    enum {
        MAX_LOG_BUF_SIZE = 256,
    };
    char logBuf[MAX_LOG_BUF_SIZE] = {0};
    int len = 0;

    len = snprintf(logBuf + len, MAX_LOG_BUF_SIZE - len, "[%s] ", tag);
    switch (logLevel)
    {
    case ERROR_L:
        len += snprintf(logBuf + len, MAX_LOG_BUF_SIZE - len, "%s", "[ERR] ");
        break;
    case DEBUG_L:
        len += snprintf(logBuf + len, MAX_LOG_BUF_SIZE - len, "%s", "[DBG] ");
        break;
    case WARNING_L:
        len += snprintf(logBuf + len, MAX_LOG_BUF_SIZE - len, "%s", "[WAR] ");
        break;
    default:
        len += snprintf(logBuf + len, MAX_LOG_BUF_SIZE - len, "%s", "[NUL] ");
        break;
    }

	outc_arg_t arg;
	arg.p = logBuf + len;
	arg.index = 0;
	arg.size = MAX_LOG_BUF_SIZE - len;

	va_list ap;
	va_start(ap, fmt);
	//vprintf(outc_sn, &arg, fmt, ap); //TODO
	arg.p[arg.index] = 0;
	va_end(ap);
	len += arg.index;

	fprintf(stderr, "%s\n", logBuf);
}

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

static int is_little_ending(void)
{
    short s = 0x0110;
    char *p = (char *)&s;

    if (p[0] == 0x10) {
        return 1;
    } else {
        return 0;
    }
}

static uint32_t bswap_32(uint32_t x)
{
    x = ((x << 8) &0xFF00FF00) | ((x >> 8) &0x00FF00FF);
    return (x >> 16) | (x << 16);
}

static uint32_t le32toh(uint32_t val)
{
    if (is_little_ending() != 0) {
        return val;
    } else {
        return bswap_32(val);
    }
}

/*
    Return value:
        <0 parse wav header fail
        =0 success
    Note:
        This function will filter wav header from fread,
        so user read audio content after call it.
*/
int parse_wav_header(FILE *file, struct chunk_fmt *wavFormat)
{
    struct riff_wave_header riff_wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;
    int more_chunks = 1;

    fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
    if ((riff_wave_header.riff_id != ID_RIFF) ||
        (riff_wave_header.wave_id != ID_WAVE)) {
        fprintf(stderr, "%s() Error! not a riff/wave file\n", __func__);
        return -1;
    }

    do {
        fread(&chunk_header, sizeof(chunk_header), 1, file);

        switch (chunk_header.id) {
        case ID_FMT:
            fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
            /* If the format header is larger, skip the rest */
            if (chunk_header.sz > sizeof(chunk_fmt))
                fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            chunk_header.sz = le32toh(chunk_header.sz);
            break;
        default:
            /* Unknown chunk, skip bytes */
            fseek(file, chunk_header.sz, SEEK_CUR);
        }
    } while (more_chunks);

    memcpy(wavFormat, &chunk_fmt, sizeof(chunk_fmt));
    return 0;
}