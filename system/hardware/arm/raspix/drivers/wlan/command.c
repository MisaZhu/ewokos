#include "include/types.h"
#include "include/fwil_types.h"
#include "include/command.h"


#define TOE_TX_CSUM_OL      0x00000001
#define TOE_RX_CSUM_OL      0x00000002

/* For supporting multiple interfaces */
#define BRCMF_MAX_IFS   16

/* Small, medium and maximum buffer size for dcmd
 */
#define BRCMF_DCMD_SMLEN    256
#define BRCMF_DCMD_MEDLEN   1536
#define BRCMF_DCMD_MAXLEN   8192

/* IOCTL from host to device are limited in length. A device can only handle
 * ethernet frame size. This limitation is to be applied by protocol layer.
 */
#define BRCMF_TX_IOCTL_MAX_MSG_SIZE (1518)

#define BRCMF_AMPDU_RX_REORDER_MAXFLOWS     256

/* Length of firmware version string stored for
 * ethtool driver info which uses 32 bytes as well.
 */
#define BRCMF_DRIVER_FIRMWARE_VERSION_LEN   32

#define NDOL_MAX_ENTRIES    8

/* BCDC flag definitions */
#define BCDC_DCMD_ERROR     0x01        /* 1=cmd failed */
#define BCDC_DCMD_SET       0x02        /* 0=get, 1=set cmd */
#define BCDC_DCMD_IF_MASK   0xF000      /* I/F index */
#define BCDC_DCMD_IF_SHIFT  12
#define BCDC_DCMD_ID_MASK   0xFFFF0000  /* id an cmd pairing */
#define BCDC_DCMD_ID_SHIFT  16      /* ID Mask shift bits */
#define BCDC_DCMD_ID(flags) \
    (((flags) & BCDC_DCMD_ID_MASK) >> BCDC_DCMD_ID_SHIFT)

/*
 * BCDC header - Broadcom specific extension of CDC.
 * Used on data packets to convey priority across USB.
 */
#define BCDC_HEADER_LEN     4
#define BCDC_PROTO_VER      2   /* Protocol version */
#define BCDC_FLAG_VER_MASK  0xf0    /* Protocol version mask */
#define BCDC_FLAG_VER_SHIFT 4   /* Protocol version shift */
#define BCDC_FLAG_SUM_GOOD  0x04    /* Good RX checksums */
#define BCDC_FLAG_SUM_NEEDED    0x08    /* Dongle needs to do TX checksums */
#define BCDC_PRIORITY_MASK  0x7
#define BCDC_FLAG2_IF_MASK  0x0f    /* packet rx interface in APSTA */
#define BCDC_FLAG2_IF_SHIFT 0

#define BCDC_GET_IF_IDX(hdr) \
    ((int)((((hdr)->flags2) & BCDC_FLAG2_IF_MASK) >> BCDC_FLAG2_IF_SHIFT))
#define BCDC_SET_IF_IDX(hdr, idx) \
    ((hdr)->flags2 = (((hdr)->flags2 & ~BCDC_FLAG2_IF_MASK) | \
    ((idx) << BCDC_FLAG2_IF_SHIFT)))

struct brcmf_proto_bcdc_dcmd {
    uint8_t header[32];
    __le32 cmd; /* dongle command value */
    __le32 len; /* lower 16: output buflen;
             * upper 16: input buflen (excludes header) */
    __le32 flags;   /* flag defns given below */
    __le32 status;  /* status code returned from the device */
    uint8_t buf[BRCMF_DCMD_SMLEN];
};

static struct brcmf_proto_bcdc_dcmd _msg;
static int reqid = 0;

static int
brcmf_proto_bcdc_msg(int ifidx, uint cmd, void *buf,
             uint len, bool set)
{
    struct brcmf_proto_bcdc_dcmd *msg = &_msg;
    u32 flags;


    memset(msg, 0, sizeof(struct brcmf_proto_bcdc_dcmd));

    msg->cmd = cpu_to_le32(cmd);
    msg->len = cpu_to_le32(len);
    flags = (++reqid << BCDC_DCMD_ID_SHIFT);
    if (set)
        flags |= BCDC_DCMD_SET;
    flags = (flags & ~BCDC_DCMD_IF_MASK) |
        (ifidx << BCDC_DCMD_IF_SHIFT);
    msg->flags = cpu_to_le32(flags);

    if (buf)
        memcpy(msg->buf, buf, len);

    len += sizeof(*msg);
    if (len > BRCMF_TX_IOCTL_MAX_MSG_SIZE)
        len = BRCMF_TX_IOCTL_MAX_MSG_SIZE;

    /* Send request */
    return brcmf_sdio_bus_txctl((unsigned char *)&_msg.cmd, len);
}

static int brcmf_proto_bcdc_cmplt(u32 id, u32 len)
{
    int ret;

    len += sizeof(struct brcmf_proto_bcdc_dcmd);
    do {
        ret = brcmf_sdio_bus_rxctl((unsigned char *)&_msg.cmd,
                      len);
        if (ret < 0)
            break;
    } while (BCDC_DCMD_ID(le32_to_cpu(_msg.flags)) != id);

    return ret;
}

static int
brcmf_proto_bcdc_set_dcmd(int ifidx, uint cmd,
              void *buf, uint len, int *fwerr)
{
    struct brcmf_proto_bcdc_dcmd *msg = &_msg;
    int ret;
    u32 flags, id;

    klog("Enter, cmd %d len %d\n", cmd, len);

    *fwerr = 0;
    ret = brcmf_proto_bcdc_msg(ifidx, cmd, buf, len, true);
    if (ret < 0)
        goto done;

    ret = brcmf_proto_bcdc_cmplt(reqid, len);
    if (ret < 0)
        goto done;

    flags = le32_to_cpu(msg->flags);
    id = (flags & BCDC_DCMD_ID_MASK) >> BCDC_DCMD_ID_SHIFT;

    if (id != reqid) {
        klog("unexpected request id %d (expected %d)\n", id, reqid);
        ret = -EINVAL;
        goto done;
    }

    ret = 0;

    /* Check the ERROR flag */
    if (flags & BCDC_DCMD_ERROR)
        *fwerr = le32_to_cpu(msg->status);

done:
    return ret;
}


static int
brcmf_proto_bcdc_query_dcmd(int ifidx, uint cmd,
                void *buf, uint len, int *fwerr)
{
    struct brcmf_proto_bcdc_dcmd *msg = &_msg;
    void *info;
    int ret = 0, retries = 0;
    u32 id, flags;

    *fwerr = 0;
    ret = brcmf_proto_bcdc_msg(ifidx, cmd, buf, len, false);
    if (ret < 0) {
        klog("brcmf_proto_bcdc_msg failed w/status %d\n",
             ret);
        goto done;
    }

retry:
    /* wait for interrupt and get first fragment */
    ret = brcmf_proto_bcdc_cmplt(reqid, len);
    if (ret < 0)
        goto done;

    flags = le32_to_cpu(msg->flags);
    id = (flags & BCDC_DCMD_ID_MASK) >> BCDC_DCMD_ID_SHIFT;

    if ((id < reqid) && (++retries < 2))
        goto retry;
    if (id != reqid) {
        bphy_err("unexpected request id %d (expected %d)\n", id, reqid);
        ret = -EINVAL;
        goto done;
    }

    /* Copy info buffer */
    if (buf) {
        if (ret < (int)len)
            len = ret;
        memcpy(buf, msg->buf, len);
    }

    ret = 0;

    /* Check the ERROR flag */
    if (flags & BCDC_DCMD_ERROR)
        *fwerr = le32_to_cpu(msg->status);
done:
    return ret;
}

static s32
brcmf_fil_cmd_data(int ifidx, u32 cmd, void *data, u32 len, bool set)
{
    s32 err, fwerr;

    if (data != NULL)
        len = min_t(uint, len, BRCMF_DCMD_SMLEN);
    if (set)
        err = brcmf_proto_bcdc_set_dcmd(ifidx, cmd,
                       data, len, &fwerr);
    else
        err = brcmf_proto_bcdc_query_dcmd(ifidx, cmd,
                         data, len, &fwerr);

    if (err) {
        klog("Failed: error=%d\n", err);
    } else if (fwerr < 0) {
        klog("Firmware error: %d\n", fwerr);
        err = -EBADE;
    }
    return err;
}

s32
brcmf_fil_cmd_data_set(u32 cmd, void *data, u32 len)
{
    s32 err;
    err = brcmf_fil_cmd_data(0, cmd, data, len, true);
    klog("ifidx=%d, cmd=%d, len=%d ret:%d\n", 0, cmd, len, err);
    return err;
}


void scan(void){

    struct brcmf_scan_params_le params_le;

    memset(&params_le, 0, sizeof(params_le));
    memset(params_le.bssid, 0xff, sizeof(params_le.bssid));
    params_le.bss_type = DOT11_BSSTYPE_ANY;
    params_le.scan_type = 0;
    params_le.channel_num = cpu_to_le32(1);
    params_le.nprobes = cpu_to_le32(1);
    params_le.active_time = cpu_to_le32(-1);
    params_le.passive_time = cpu_to_le32(-1);
    params_le.home_time = cpu_to_le32(-1);
    /* Scan is aborted by setting channel_list[0] to -1 */
    params_le.channel_list[0] = cpu_to_le16(-1);
    /* E-Scan (or anyother type) can be aborted by SCAN */
    int err = brcmf_fil_cmd_data_set(BRCMF_C_SCAN,
                     &params_le, sizeof(params_le));
    klog("scane %d\n");
}