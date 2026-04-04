#ifndef _VIRTIO_SND_H_
#define _VIRTIO_SND_H_

#include <stdint.h>

#include <arch/virt/virtio.h>

#define VIRTIO_SND_VQ_CONTROL 0
#define VIRTIO_SND_VQ_EVENT 1
#define VIRTIO_SND_VQ_TX 2
#define VIRTIO_SND_VQ_RX 3

#define VIRTIO_SND_R_PCM_INFO 0x0100
#define VIRTIO_SND_R_PCM_SET_PARAMS 0x0101
#define VIRTIO_SND_R_PCM_PREPARE 0x0102
#define VIRTIO_SND_R_PCM_RELEASE 0x0103
#define VIRTIO_SND_R_PCM_START 0x0104
#define VIRTIO_SND_R_PCM_STOP 0x0105

#define VIRTIO_SND_EVT_PCM_PERIOD_ELAPSED 0x1100
#define VIRTIO_SND_EVT_PCM_XRUN 0x1101

#define VIRTIO_SND_S_OK 0x8000
#define VIRTIO_SND_S_BAD_MSG 0x8001
#define VIRTIO_SND_S_NOT_SUPP 0x8002
#define VIRTIO_SND_S_IO_ERR 0x8003

#define VIRTIO_SND_D_OUTPUT 0
#define VIRTIO_SND_D_INPUT 1

#define VIRTIO_SND_PCM_FMT_S16 5
#define VIRTIO_SND_PCM_FMT_S24_3 11
#define VIRTIO_SND_PCM_FMT_S32 17

#define VIRTIO_SND_PCM_RATE_8000 1
#define VIRTIO_SND_PCM_RATE_16000 3
#define VIRTIO_SND_PCM_RATE_32000 5
#define VIRTIO_SND_PCM_RATE_44100 6
#define VIRTIO_SND_PCM_RATE_48000 7
#define VIRTIO_SND_PCM_RATE_96000 10

#define VIRTIO_SND_TX_SLOT_MAX 8

struct virtio_snd_config
{
	uint32_t jacks;
	uint32_t streams;
	uint32_t chmaps;
	uint32_t controls;
} __attribute__((packed));

struct virtio_snd_hdr
{
	uint32_t code;
} __attribute__((packed));

struct virtio_snd_query_info
{
	struct virtio_snd_hdr hdr;
	uint32_t start_id;
	uint32_t count;
	uint32_t size;
} __attribute__((packed));

struct virtio_snd_info
{
	uint32_t hda_fn_nid;
} __attribute__((packed));

struct virtio_snd_pcm_hdr
{
	struct virtio_snd_hdr hdr;
	uint32_t stream_id;
} __attribute__((packed));

struct virtio_snd_pcm_info
{
	struct virtio_snd_info hdr;
	uint32_t features;
	uint64_t formats;
	uint64_t rates;
	uint8_t direction;
	uint8_t channels_min;
	uint8_t channels_max;
	uint8_t padding[5];
} __attribute__((packed));

struct virtio_snd_pcm_set_params
{
	struct virtio_snd_pcm_hdr hdr;
	uint32_t buffer_bytes;
	uint32_t period_bytes;
	uint32_t features;
	uint8_t channels;
	uint8_t format;
	uint8_t rate;
	uint8_t padding;
} __attribute__((packed));

struct virtio_snd_pcm_xfer
{
	uint32_t stream_id;
} __attribute__((packed));

struct virtio_snd_pcm_status
{
	uint32_t status;
	uint32_t latency_bytes;
} __attribute__((packed));

struct virtio_snd_event
{
	struct virtio_snd_hdr hdr;
	uint32_t data;
} __attribute__((packed));

virtio_dev_t virtio_snd_get(void);
int virtio_snd_init(virtio_dev_t dev);
int virtio_snd_read_config(virtio_dev_t dev, struct virtio_snd_config *cfg);
int virtio_snd_ctl(virtio_dev_t dev, const void *req, uint32_t req_len, void *resp, uint32_t resp_len);
int virtio_snd_query_pcm_info(virtio_dev_t dev, uint32_t stream_id, struct virtio_snd_pcm_info *info);
int virtio_snd_pcm_set_params(virtio_dev_t dev, uint32_t stream_id, uint32_t buffer_bytes,
							  uint32_t period_bytes, uint32_t features, uint8_t channels,
							  uint8_t format, uint8_t rate);
int virtio_snd_pcm_ctl(virtio_dev_t dev, uint32_t code, uint32_t stream_id);
int virtio_snd_poll(virtio_dev_t dev);
int virtio_snd_last_error(virtio_dev_t dev);
void virtio_snd_clear_error(virtio_dev_t dev);
int virtio_snd_tx_init(virtio_dev_t dev, uint32_t slot_count, uint32_t period_bytes);
void virtio_snd_tx_reset(virtio_dev_t dev);
int virtio_snd_tx_avail_bytes(virtio_dev_t dev);
int virtio_snd_tx_write(virtio_dev_t dev, uint32_t stream_id, const void *data, uint32_t size,
						uint32_t timeout_ms);

#endif
