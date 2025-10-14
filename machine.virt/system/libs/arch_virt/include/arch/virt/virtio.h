#ifndef _VIRTIO_H_
#define _VIRTIO_H_
#include <stdint.h>
#include <stdlib.h>

#define VIRTIO_ID_NET           1 /* virtio net */
#define VIRTIO_ID_BLOCK         2 /* virtio block */
#define VIRTIO_ID_CONSOLE       3 /* virtio console */
#define VIRTIO_ID_RNG           4 /* virtio rng */
#define VIRTIO_ID_BALLOON       5 /* virtio balloon */
#define VIRTIO_ID_IOMEM         6 /* virtio ioMemory */
#define VIRTIO_ID_RPMSG         7 /* virtio remote processor messaging */
#define VIRTIO_ID_SCSI          8 /* virtio scsi */
#define VIRTIO_ID_9P            9 /* 9p virtio console */
#define VIRTIO_ID_MAC80211_WLAN     10 /* virtio WLAN MAC */
#define VIRTIO_ID_RPROC_SERIAL      11 /* virtio remoteproc serial link */
#define VIRTIO_ID_CAIF          12 /* Virtio caif */
#define VIRTIO_ID_MEMORY_BALLOON    13 /* virtio memory balloon */
#define VIRTIO_ID_GPU           16 /* virtio GPU */
#define VIRTIO_ID_CLOCK         17 /* virtio clock/timer */
#define VIRTIO_ID_INPUT         18 /* virtio input */
#define VIRTIO_ID_VSOCK         19 /* virtio vsock transport */
#define VIRTIO_ID_CRYPTO        20 /* virtio crypto */
#define VIRTIO_ID_SIGNAL_DIST       21 /* virtio signal distribution device */
#define VIRTIO_ID_PSTORE        22 /* virtio pstore device */
#define VIRTIO_ID_IOMMU         23 /* virtio IOMMU */
#define VIRTIO_ID_MEM           24 /* virtio mem */
#define VIRTIO_ID_SOUND         25 /* virtio sound */
#define VIRTIO_ID_FS            26 /* virtio filesystem */
#define VIRTIO_ID_PMEM          27 /* virtio pmem */
#define VIRTIO_ID_RPMB          28 /* virtio rpmb */
#define VIRTIO_ID_MAC80211_HWSIM    29 /* virtio mac80211-hwsim */
#define VIRTIO_ID_VIDEO_ENCODER     30 /* virtio video encoder */
#define VIRTIO_ID_VIDEO_DECODER     31 /* virtio video decoder */
#define VIRTIO_ID_SCMI          32 /* virtio SCMI */
#define VIRTIO_ID_NITRO_SEC_MOD     33 /* virtio nitro secure module*/
#define VIRTIO_ID_I2C_ADAPTER       34 /* virtio i2c adapter */
#define VIRTIO_ID_WATCHDOG      35 /* virtio watchdog */
#define VIRTIO_ID_CAN           36 /* virtio can */
#define VIRTIO_ID_DMABUF        37 /* virtio dmabuf */
#define VIRTIO_ID_PARAM_SERV        38 /* virtio parameter server */
#define VIRTIO_ID_AUDIO_POLICY      39 /* virtio audio policy */
#define VIRTIO_ID_BT            40 /* virtio bluetooth */
#define VIRTIO_ID_GPIO          41 /* virtio gpio */
#define VIRTIO_ID_MAX           42 /* max virtio device id */
    
typedef void* virtio_dev_t;

virtio_dev_t virtio_get(int id);
virtio_dev_t *virtio_input_get(const char* name);

void virtio_free(virtio_dev_t dev);

int virtio_init(virtio_dev_t dev, uint32_t feature);
int virtio_blk_transfer(virtio_dev_t dev, uint64_t sector, void *buffer, uint32_t count, int isWrite);
int virtio_input_read(virtio_dev_t *dev, void *buffer, uint32_t size);

#endif