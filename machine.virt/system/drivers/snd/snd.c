#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/virt/virtio.h>
#include <arch/virt/virtio_snd.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proto.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>

#define UNUSED(v) ((void)(v))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CTRL_PCM_DEV_HW 0xF0
#define CTRL_PCM_DEV_HW_FREE 0xF1
#define CTRL_PCM_DEV_PRPARE 0xF2
#define CTRL_PCM_BUF_AVAIL 0xF3
#define CTRL_PCM_DEV_HW_IN 0xF4
#define CTRL_PCM_DEV_HW_FREE_IN 0xF5
#define CTRL_PCM_DEV_PRPARE_IN 0xF6
#define CTRL_PCM_BUF_AVAIL_IN 0xF7
#define CTRL_PCM_DEV_START_IN 0xFB
#define CTRL_SND_SET_VOLUME 0xF8
#define CTRL_SND_GET_VOLUME 0xF9
#define CTRL_SND_GET_STATS 0xFA

#define SND_WRITE_TIMEOUT_MS 2000

struct pcm_config
{
    int bit_depth;
    int rate;
    int channels;
    int period_size;
    int period_count;
    int start_threshold;
    int stop_threshold;
};

typedef struct
{
    virtio_dev_t dev;
    struct virtio_snd_config config;
    struct virtio_snd_pcm_info stream_info;
    struct virtio_snd_pcm_info input_stream_info;
    struct pcm_config pcm_cfg;
    struct pcm_config input_pcm_cfg;
    uint32_t period_bytes;
    uint32_t buffer_bytes;
    uint32_t input_period_bytes;
    uint32_t input_buffer_bytes;
    int stream_id;
    int input_stream_id;
    int open_count;
    bool configured;
    bool prepared;
    bool started;
    bool input_configured;
    bool input_prepared;
    bool input_started;
    int occupied_pid;
    int volume_percent;
    uint32_t xrun_count;
} snd_dev_t;

static snd_dev_t _snd = {.stream_id = -1, .input_stream_id = -1, .volume_percent = 100};

static inline int snd_owner_pid(int pid)
{
    return proc_getpid_or_raw(pid);
}

static int snd_rate_to_virtio(int rate)
{
    switch (rate)
    {
    case 8000:
        return VIRTIO_SND_PCM_RATE_8000;
    case 16000:
        return VIRTIO_SND_PCM_RATE_16000;
    case 32000:
        return VIRTIO_SND_PCM_RATE_32000;
    case 44100:
        return VIRTIO_SND_PCM_RATE_44100;
    case 48000:
        return VIRTIO_SND_PCM_RATE_48000;
    case 96000:
        return VIRTIO_SND_PCM_RATE_96000;
    default:
        return -1;
    }
}

static int snd_format_to_virtio(int bit_depth)
{
    switch (bit_depth)
    {
    case 8:
        return VIRTIO_SND_PCM_FMT_S8;
    case -8:
        return VIRTIO_SND_PCM_FMT_U8;
    case 16:
        return VIRTIO_SND_PCM_FMT_S16;
    case 24:
        return VIRTIO_SND_PCM_FMT_S24_3;
    case 32:
        return VIRTIO_SND_PCM_FMT_S32;
    default:
        return -1;
    }
}

static int snd_select_output_stream(const struct pcm_config *cfg, int fmt_id, int rate_id,
                                    struct virtio_snd_pcm_info *chosen)
{
    uint64_t fmt_mask = 1ULL << fmt_id;
    uint64_t rate_mask = 1ULL << rate_id;

    for (uint32_t i = 0; i < _snd.config.streams; i++)
    {
        struct virtio_snd_pcm_info info;
        memset(&info, 0, sizeof(info));
        if (virtio_snd_query_pcm_info(_snd.dev, i, &info) != 0)
        {
            continue;
        }
        if (info.direction != VIRTIO_SND_D_OUTPUT)
        {
            continue;
        }
        if (cfg->channels < info.channels_min || cfg->channels > info.channels_max)
        {
            continue;
        }
        if ((info.formats & fmt_mask) == 0 || (info.rates & rate_mask) == 0)
        {
            continue;
        }

        memcpy(chosen, &info, sizeof(*chosen));
        return (int)i;
    }

    return -1;
}

static int snd_select_input_stream(const struct pcm_config *cfg, int fmt_id, int rate_id,
                   struct virtio_snd_pcm_info *chosen)
{
    uint64_t fmt_mask = 1ULL << fmt_id;
    uint64_t rate_mask = 1ULL << rate_id;

    for (uint32_t i = 0; i < _snd.config.streams; i++)
    {
        struct virtio_snd_pcm_info info;
        memset(&info, 0, sizeof(info));
        if (virtio_snd_query_pcm_info(_snd.dev, i, &info) != 0)
        {
            continue;
        }
        if (info.direction != VIRTIO_SND_D_INPUT)
        {
            continue;
        }
        if (cfg->channels < info.channels_min || cfg->channels > info.channels_max)
        {
            continue;
        }
        if ((info.formats & fmt_mask) == 0 || (info.rates & rate_mask) == 0)
        {
            continue;
        }

        memcpy(chosen, &info, sizeof(*chosen));
        return (int)i;
    }

    return -1;
}

static int snd_stop_and_release(bool reset_config)
{
    if (_snd.configured)
    {
        if (_snd.started)
        {
            (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_STOP, (uint32_t)_snd.stream_id);
        }
        (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_RELEASE, (uint32_t)_snd.stream_id);
    }

    if (_snd.input_configured)
    {
        if (_snd.input_started)
        {
            virtio_snd_rx_stop(_snd.dev);
            (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_STOP,
                         (uint32_t)_snd.input_stream_id);
        }
        (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_RELEASE,
                     (uint32_t)_snd.input_stream_id);
    }

    virtio_snd_tx_reset(_snd.dev);
    virtio_snd_rx_reset(_snd.dev);
    virtio_snd_clear_error(_snd.dev);
    _snd.prepared = false;
    _snd.started = false;
    _snd.input_prepared = false;
    _snd.input_started = false;
    if (reset_config)
    {
        _snd.configured = false;
        _snd.input_configured = false;
        _snd.stream_id = -1;
        _snd.input_stream_id = -1;
        memset(&_snd.pcm_cfg, 0, sizeof(_snd.pcm_cfg));
        memset(&_snd.input_pcm_cfg, 0, sizeof(_snd.input_pcm_cfg));
        memset(&_snd.stream_info, 0, sizeof(_snd.stream_info));
        memset(&_snd.input_stream_info, 0, sizeof(_snd.input_stream_info));
        _snd.period_bytes = 0;
        _snd.buffer_bytes = 0;
        _snd.input_period_bytes = 0;
        _snd.input_buffer_bytes = 0;
    }
    return 0;
}

static int snd_hw_params(const struct pcm_config *cfg)
{
    int fmt_id = snd_format_to_virtio(cfg->bit_depth);
    int rate_id = snd_rate_to_virtio(cfg->rate);
    struct virtio_snd_pcm_info info;

    if (fmt_id < 0 || rate_id < 0 || cfg->channels <= 0 ||
        cfg->period_size <= 0 || cfg->period_count <= 0)
    {
        return -1;
    }

    uint32_t frame_size = (uint32_t)(cfg->channels * cfg->bit_depth / 8);
    uint32_t period_bytes = (uint32_t)cfg->period_size * frame_size;
    uint32_t buffer_bytes = period_bytes * (uint32_t)cfg->period_count;
    if (frame_size == 0 || period_bytes == 0 || buffer_bytes == 0)
    {
        return -1;
    }

    int stream_id = snd_select_output_stream(cfg, fmt_id, rate_id, &info);
    if (stream_id < 0)
    {
        klog("virtio-snd: no output stream for %dch/%dbit/%dHz\n",
             cfg->channels, cfg->bit_depth, cfg->rate);
        return -1;
    }

    snd_stop_and_release(true);

    if (virtio_snd_tx_init(_snd.dev,
                           MIN((uint32_t)cfg->period_count, (uint32_t)VIRTIO_SND_TX_SLOT_MAX),
                           period_bytes) != 0)
    {
        return -1;
    }

    if (virtio_snd_pcm_set_params(_snd.dev, (uint32_t)stream_id, buffer_bytes, period_bytes, 0,
                                  (uint8_t)cfg->channels, (uint8_t)fmt_id, (uint8_t)rate_id) != 0)
    {
        virtio_snd_tx_reset(_snd.dev);
        return -1;
    }

    memcpy(&_snd.stream_info, &info, sizeof(info));
    memcpy(&_snd.pcm_cfg, cfg, sizeof(*cfg));
    _snd.period_bytes = period_bytes;
    _snd.buffer_bytes = buffer_bytes;
    _snd.stream_id = stream_id;
    _snd.configured = true;
    _snd.prepared = false;
    _snd.started = false;
    virtio_snd_clear_error(_snd.dev);
    return 0;
}

static int snd_hw_params_input(const struct pcm_config *cfg)
{
    int fmt_id = snd_format_to_virtio(cfg->bit_depth);
    int rate_id = snd_rate_to_virtio(cfg->rate);
    struct virtio_snd_pcm_info info;

    if (fmt_id < 0 || rate_id < 0 || cfg->channels <= 0 ||
        cfg->period_size <= 0 || cfg->period_count <= 0)
    {
        return -1;
    }

    uint32_t frame_size = (uint32_t)(cfg->channels * cfg->bit_depth / 8);
    uint32_t period_bytes = (uint32_t)cfg->period_size * frame_size;
    uint32_t buffer_bytes = period_bytes * (uint32_t)cfg->period_count;
    if (frame_size == 0 || period_bytes == 0 || buffer_bytes == 0)
    {
        return -1;
    }

    int stream_id = snd_select_input_stream(cfg, fmt_id, rate_id, &info);
    if (stream_id < 0)
    {
        klog("virtio-snd: no input stream for %dch/%dbit/%dHz\n",
             cfg->channels, cfg->bit_depth, cfg->rate);
        return -1;
    }

    /* Release any previous input stream */
    if (_snd.input_configured)
    {
        if (_snd.input_started)
        {
            virtio_snd_rx_stop(_snd.dev);
            (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_STOP,
                         (uint32_t)_snd.input_stream_id);
        }
        (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_RELEASE,
                     (uint32_t)_snd.input_stream_id);
        virtio_snd_rx_reset(_snd.dev);
    }

    if (virtio_snd_rx_init(_snd.dev,
                   MIN((uint32_t)cfg->period_count, (uint32_t)VIRTIO_SND_RX_SLOT_MAX),
                   period_bytes) != 0)
    {
        return -1;
    }

    if (virtio_snd_pcm_set_params(_snd.dev, (uint32_t)stream_id, buffer_bytes, period_bytes, 0,
                      (uint8_t)cfg->channels, (uint8_t)fmt_id, (uint8_t)rate_id) != 0)
    {
        virtio_snd_rx_reset(_snd.dev);
        return -1;
    }

    memcpy(&_snd.input_stream_info, &info, sizeof(info));
    memcpy(&_snd.input_pcm_cfg, cfg, sizeof(*cfg));
    _snd.input_period_bytes = period_bytes;
    _snd.input_buffer_bytes = buffer_bytes;
    _snd.input_stream_id = stream_id;
    _snd.input_configured = true;
    _snd.input_prepared = false;
    _snd.input_started = false;
    return 0;
}

static int snd_prepare_stream(void)
{
    if (!_snd.configured)
    {
        klog("virtio-snd: snd_prepare_stream: stream not configured\n");
        return -1;
    }
    if (_snd.prepared)
    {
        return 0;
    }

    virtio_snd_tx_reset(_snd.dev);
    virtio_snd_clear_error(_snd.dev);
    if (virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_PREPARE, (uint32_t)_snd.stream_id) != 0)
    {
        klog("virtio-snd: snd_prepare_stream: failed to prepare stream\n");
        return -1;
    }

    _snd.prepared = true;
    return 0;
}

static int snd_start_stream(void)
{
    if (!_snd.prepared)
    {
        return -1;
    }
    if (_snd.started)
    {
        return 0;
    }

    if (virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_START, (uint32_t)_snd.stream_id) != 0)
    {
        return -1;
    }
    _snd.started = true;
    return 0;
}

static int snd_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t *info, int oflag, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)oflag;
    (void)p;

    /*if (_snd.open_count > 0)
    {
        return -1;
    }
    */
    snd_stop_and_release(true);
    _snd.open_count = 1;
    _snd.occupied_pid = snd_owner_pid(from_pid);
    return 0;
}

static int snd_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t *info, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)node;
    (void)info;
    (void)p;

    if (_snd.occupied_pid != snd_owner_pid(from_pid)) {
        return -1;
    }

    _snd.open_count = 0;
    snd_stop_and_release(true);
    return 0;
}

/*
 * Apply software volume gain to PCM samples in-place.
 * Uses fixed-point arithmetic: gain = volume_percent / 100.
 * Skips processing when volume is 100% (zero-cost passthrough).
 */
static void snd_apply_volume(void *buf, uint32_t size, int bit_depth, int volume_percent)
{
    if (volume_percent >= 100 || volume_percent < 0)
    {
        return;
    }

    uint8_t *samples = (uint8_t *)buf;
    uint32_t sample_count;
    int bytes_per_sample;

    switch (bit_depth)
    {
    case 8:
    case -8:
        bytes_per_sample = 1;
        break;
    case 16:
        bytes_per_sample = 2;
        break;
    case 24:
        bytes_per_sample = 3;
        break;
    case 32:
        bytes_per_sample = 4;
        break;
    default:
        return;
    }

    sample_count = size / (uint32_t)bytes_per_sample;

    for (uint32_t i = 0; i < sample_count; i++)
    {
        int32_t val;
        uint8_t *s = samples + i * bytes_per_sample;

        switch (bit_depth)
        {
        case 8:
            val = (int32_t)((int8_t)*s);
            val = val * volume_percent / 100;
            if (val > 127) val = 127;
            if (val < -128) val = -128;
            *s = (uint8_t)(int8_t)val;
            break;
        case -8:
            val = (int32_t)*s - 128;
            val = val * volume_percent / 100;
            if (val > 127) val = 127;
            if (val < -128) val = -128;
            *s = (uint8_t)(val + 128);
            break;
        case 16:
            val = (int32_t)((int16_t)((uint16_t)s[0] | ((uint16_t)s[1] << 8)));
            val = val * volume_percent / 100;
            if (val > 32767) val = 32767;
            if (val < -32768) val = -32768;
            s[0] = (uint8_t)(val & 0xFF);
            s[1] = (uint8_t)((val >> 8) & 0xFF);
            break;
        case 32:
            val = (int32_t)((uint32_t)s[0] | ((uint32_t)s[1] << 8) |
                   ((uint32_t)s[2] << 16) | ((uint32_t)s[3] << 24));
            {
                int64_t val64 = (int64_t)val * volume_percent / 100;
                if (val64 > 2147483647LL) val64 = 2147483647LL;
                if (val64 < -2147483648LL) val64 = -2147483648LL;
                val = (int32_t)val64;
            }
            s[0] = (uint8_t)(val & 0xFF);
            s[1] = (uint8_t)((val >> 8) & 0xFF);
            s[2] = (uint8_t)((val >> 16) & 0xFF);
            s[3] = (uint8_t)((val >> 24) & 0xFF);
            break;
        }
    }
}

static int snd_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t *info,
                     const void *buf, int size, int offset, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;

    if (!_snd.configured || size <= 0 || _snd.occupied_pid != snd_owner_pid(from_pid))
    {
        return -1;
    }

    if (snd_prepare_stream() != 0 || snd_start_stream() != 0)
    {
        return -1;
    }

    /* Apply software volume gain if needed */
    const uint8_t *tx_buf = (const uint8_t *)buf;
    uint32_t tx_size = (uint32_t)size;
    int ret;

    if (_snd.volume_percent < 100 && _snd.volume_percent >= 0 &&
        tx_size <= 4096)
    {
        uint8_t vol_buf[4096];
        memcpy(vol_buf, tx_buf, tx_size);
        snd_apply_volume(vol_buf, tx_size, _snd.pcm_cfg.bit_depth,
                 _snd.volume_percent);
        ret = virtio_snd_tx_write(_snd.dev, (uint32_t)_snd.stream_id,
                      vol_buf, tx_size,
                      SND_WRITE_TIMEOUT_MS);
    }
    else
    {
        ret = virtio_snd_tx_write(_snd.dev, (uint32_t)_snd.stream_id,
                      tx_buf, tx_size,
                      SND_WRITE_TIMEOUT_MS);
    }

    if (ret < 0)
    {
        _snd.prepared = false;
        _snd.started = false;
    }
    return ret;
}

static int snd_prepare_input_stream(void)
{
    if (!_snd.input_configured)
    {
        return -1;
    }
    if (_snd.input_prepared)
    {
        return 0;
    }

    virtio_snd_rx_reset(_snd.dev);
    virtio_snd_clear_error(_snd.dev);
    if (virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_PREPARE,
                   (uint32_t)_snd.input_stream_id) != 0)
    {
        return -1;
    }

    _snd.input_prepared = true;
    return 0;
}

static int snd_start_input_stream(void)
{
    if (!_snd.input_prepared)
    {
        return -1;
    }
    if (_snd.input_started)
    {
        return 0;
    }

    if (virtio_snd_rx_start(_snd.dev, (uint32_t)_snd.input_stream_id) != 0)
    {
        return -1;
    }

    if (virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_START,
                   (uint32_t)_snd.input_stream_id) != 0)
    {
        virtio_snd_rx_stop(_snd.dev);
        return -1;
    }

    _snd.input_started = true;
    return 0;
}

static int snd_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t *info,
            void *buf, int size, int offset, void *p)
{
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;

    if (!_snd.input_configured || size <= 0 ||
        _snd.occupied_pid != snd_owner_pid(from_pid))
    {
        return -1;
    }

    if (snd_prepare_input_stream() != 0 || snd_start_input_stream() != 0)
    {
        return -1;
    }

    int ret = virtio_snd_rx_read(_snd.dev, buf, (uint32_t)size);
    if (ret == 0 && _snd.input_started)
    {
        return VFS_ERR_RETRY;
    }
    if (ret < 0)
    {
        _snd.input_prepared = false;
        _snd.input_started = false;
    }
    return ret;
}

static int snd_dcntl(vdevice_t* dev, int from_pid, int cmd, proto_t *in, proto_t *ret, void *p)
{
    (void)dev;
    (void)from_pid;
    (void)p;

    int result = 0;
    struct pcm_config cfg;

    if (_snd.occupied_pid != snd_owner_pid(from_pid)) {
        return -1;
    }

    switch (cmd)
    {
    case CTRL_PCM_DEV_HW:
        memset(&cfg, 0, sizeof(cfg));
        proto_read_to(in, &cfg, sizeof(cfg));
        result = snd_hw_params(&cfg);
        break;
    case CTRL_PCM_DEV_HW_FREE:
        result = snd_stop_and_release(true);
        break;
    case CTRL_PCM_DEV_PRPARE:
        result = snd_prepare_stream();
        break;
    case CTRL_PCM_BUF_AVAIL:
        result = virtio_snd_poll(_snd.dev);
        if (result == 0 && _snd.configured)
        {
            result = virtio_snd_tx_avail_bytes(_snd.dev);
        }
        else if (result == 0)
        {
            result = -1;
        }
        break;
    case CTRL_PCM_DEV_HW_IN:
        memset(&cfg, 0, sizeof(cfg));
        proto_read_to(in, &cfg, sizeof(cfg));
        result = snd_hw_params_input(&cfg);
        break;
    case CTRL_PCM_DEV_HW_FREE_IN:
        if (_snd.input_configured)
        {
            if (_snd.input_started)
            {
                virtio_snd_rx_stop(_snd.dev);
                (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_STOP,
                             (uint32_t)_snd.input_stream_id);
            }
            (void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_RELEASE,
                         (uint32_t)_snd.input_stream_id);
            virtio_snd_rx_reset(_snd.dev);
            _snd.input_configured = false;
            _snd.input_prepared = false;
            _snd.input_started = false;
            _snd.input_stream_id = -1;
            memset(&_snd.input_pcm_cfg, 0, sizeof(_snd.input_pcm_cfg));
            memset(&_snd.input_stream_info, 0, sizeof(_snd.input_stream_info));
            _snd.input_period_bytes = 0;
            _snd.input_buffer_bytes = 0;
        }
        result = 0;
        break;
    case CTRL_PCM_DEV_PRPARE_IN:
        result = snd_prepare_input_stream();
        break;
    case CTRL_PCM_BUF_AVAIL_IN:
        result = virtio_snd_rx_avail_bytes(_snd.dev);
        break;
    case CTRL_PCM_DEV_START_IN:
        result = snd_start_input_stream();
        break;
    case CTRL_SND_SET_VOLUME:
        result = proto_read_int(in);
        if (result >= 0 && result <= 100)
        {
            _snd.volume_percent = result;
            result = 0;
        }
        else
        {
            result = -1;
        }
        break;
    case CTRL_SND_GET_VOLUME:
        result = _snd.volume_percent;
        break;
    case CTRL_SND_GET_STATS:
        /* Return {xrun_count, last_error} packed as two ints.
           Caller passes an int[2] buffer via proto_t. */
        PF->addi(ret, (int)_snd.xrun_count);
        PF->addi(ret, virtio_snd_last_error(_snd.dev));
        return 0;
    default:
        result = -1;
        break;
    }

    PF->addi(ret, result);
    return 0;
}

static int snd_loop_step(vdevice_t *dev, void *p)
{
    (void)dev;
    (void)p;

    int err = virtio_snd_process_events(_snd.dev);

    int tx_avail = 0;
    int rx_avail = 0;
    bool busy = false;

    if (_snd.configured)
    {
        if (err < 0)
        {
            _snd.prepared = false;
            _snd.started = false;
            _snd.xrun_count++;
        }

        tx_avail = virtio_snd_tx_avail_bytes(_snd.dev);
        if (tx_avail >= (int)_snd.period_bytes)
        {
            vfs_wakeup(dev->mnt_info.node, VFS_EVT_WR);
            busy = true;
        }
    }

    if (_snd.input_configured)
    {
        if (err < 0)
        {
            _snd.input_prepared = false;
            _snd.input_started = false;
        }

        rx_avail = virtio_snd_rx_avail_bytes(_snd.dev);
        if (rx_avail >= (int)_snd.input_period_bytes)
        {
            vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
            busy = true;
        }
    }

    /*
     * Adaptive sleep:
     *   busy (data ready or wakeup needed) -> 5 ms
     *   idle                               -> ramp 5 -> 50 ms
     */
    static uint32_t idle_ms = 5;

    if (busy)
        idle_ms = 5;
    else if (idle_ms < 50)
        idle_ms += 5;

    proc_usleep(idle_ms * 1000);
    return err;
}

static int snd_init_device(void)
{
    _mmio_base = mmio_map();
    _snd.dev = virtio_snd_get();
    if (_snd.dev == NULL)
    {
        klog("virtio-snd: no sound device found\n");
        return -1;
    }
    if (virtio_snd_init(_snd.dev) != 0 ||
        virtio_snd_read_config(_snd.dev, &_snd.config) != 0)
    {
        return -1;
    }
    if (_snd.config.streams == 0)
    {
        klog("virtio-snd: device reports zero pcm streams\n");
        return -1;
    }

    virtio_snd_enable_interrupts(_snd.dev);

    //klog("virtio-snd: streams=%u jacks=%u chmaps=%u\n",
        // _snd.config.streams, _snd.config.jacks, _snd.config.chmaps);
    return 0;
}

int main(int argc, char **argv)
{
    const char *mnt_point = argc > 1 ? argv[1] : "/dev/sound0";

    if (snd_init_device() != 0)
    {
        return -1;
    }

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.name, "virtio-snd");
    dev.open = snd_open;
    dev.close = snd_close;
    dev.read = snd_read;
    dev.write = snd_write;
    dev.dev_cntl = snd_dcntl;
    dev.loop_step = snd_loop_step;

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
    return 0;
}
