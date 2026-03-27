#include <types.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <utils/log.h>
#include <utils/utils.h>

#include "fwil_types.h"
#include "command.h"
#include "brcm.h"
#include "core.h"
#include "chip.h"
#include "brcmu_wifi.h"
#include "nl80211.h"
#include "firmware.h"

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
    __le32 cmd; /* dongle command value */
    __le32 len; /* lower 16: output buflen;
             * upper 16: input buflen (excludes header) */
    __le32 flags;   /* flag defns given below */
    __le32 status;  /* status code returned from the device */
};

static uint8_t proto_buf[8192];
static uint8_t temp[8192];
static uint8_t mac_addr[6];
static uint32_t reqid = 0;


void
hexdump(const char* lable, const void *data, size_t size)
{
    unsigned char *src;
    int offset, index;
    src = (unsigned char *)data;
    brcm_log("%s %d:\n", lable, size);
    brcm_log("+------+-------------------------------------------------+------------------+\n");
    for(offset = 0; offset < (int)size; offset += 16) {
		brcm_log("| %04x | ", offset);
        for(index = 0; index < 16; index++) {
            if(offset + index < (int)size) {
                brcm_log("%02x ", 0xff & src[offset + index]);
            } else {
                brcm_log("   ");
            }
        }
        brcm_log("| ");
        for(index = 0; index < 16; index++) {
            if(offset + index < (int)size) {
            if(src[offset + index] >= 32  && src[offset + index] <= 126) {
                    brcm_log("%c", src[offset + index]);
                } else {
                    brcm_log(".");
                }
            } else {
                brcm_log(" ");
            }
        }
        brcm_log(" |\n");
    }
    brcm_log("+------+-------------------------------------------------+------------------+\n");
}

static int
brcmf_proto_bcdc_msg(int ifidx, uint cmd, void *buf,
             uint len, bool set)
{
    uint32_t flags;
    struct brcmf_proto_bcdc_dcmd *msg = (struct brcmf_proto_bcdc_dcmd *)(temp + 32);
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
        memcpy( temp + 32 + sizeof(struct brcmf_proto_bcdc_dcmd), buf, len);

    len += sizeof(struct brcmf_proto_bcdc_dcmd);
    if (len > BRCMF_TX_IOCTL_MAX_MSG_SIZE)
        len = BRCMF_TX_IOCTL_MAX_MSG_SIZE;

    //hexdump(__func__, msg, len);
    /* Send request */
    return brcmf_sdio_bus_txctl((unsigned char *)msg, len);
}


static int brcmf_proto_bcdc_cmplt(uint32_t id, uint32_t len)
{
    int ret;

    struct brcmf_proto_bcdc_dcmd *msg = (struct brcmf_proto_bcdc_dcmd *)(temp + 32);
    memset(msg, 0, sizeof(struct brcmf_proto_bcdc_dcmd));

    len += sizeof(struct brcmf_proto_bcdc_dcmd);
    do {
        ret = brcmf_sdio_bus_rxctl((unsigned char*)msg, len);
        if (ret < 0)
            break;
    } while (BCDC_DCMD_ID(le32_to_cpu(msg->flags)) != id);

    //hexdump(__func__, msg, ret);

    return ret;
}

static int
brcmf_proto_bcdc_set_dcmd(int ifidx, uint cmd,
              void *buf, uint len, int32_t *fwerr)
{
    struct brcmf_proto_bcdc_dcmd *msg = (struct brcmf_proto_bcdc_dcmd *)(temp + 32);
    int ret;
    uint32_t flags, id;

    // brcm_log("brcmf_proto_bcdc_set_dcmd cmd %d len %d\n", cmd, len);

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
        brcm_log("unexpected request id %d (expected %d)\n", id, reqid);
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
                void *buf, uint len, int32_t *fwerr)
{
    struct brcmf_proto_bcdc_dcmd *msg = (struct brcmf_proto_bcdc_dcmd *)(temp + 32);
    int ret = 0, retries = 0;
    uint32_t id, flags;

    *fwerr = 0;
    ret = brcmf_proto_bcdc_msg(ifidx, cmd, buf, len, false);
    if (ret < 0) {
        brcm_log("brcmf_proto_bcdc_msg failed w/status %d\n",
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
        brcm_log("unexpected request id %d (expected %d)\n", id, reqid);
        ret = -EINVAL;
        goto done;
    }

    /* Copy info buffer */
    if (buf) {
        if (ret < (int)len)
            len = ret;
        memcpy(buf, temp + 32 + sizeof(struct brcmf_proto_bcdc_dcmd), len);
    }

    ret = 0;

    /* Check the ERROR flag */
    if (flags & BCDC_DCMD_ERROR)
        *fwerr = le32_to_cpu(msg->status);
done:
    return ret;
}

static int32_t
brcmf_fil_cmd_data(int ifidx, uint32_t cmd, void *data, uint32_t len, bool set)
{
    int32_t err, fwerr;

    if (data != NULL)
        len = min_t(uint, len, BRCMF_DCMD_MAXLEN);
    if (set)
        err = brcmf_proto_bcdc_set_dcmd(ifidx, cmd,
                       data, len, &fwerr);
    else
        err = brcmf_proto_bcdc_query_dcmd(ifidx, cmd,
                         data, len, &fwerr);

    if (err) {
        brcm_log("Failed: error=%d\n", err);
    } else if (fwerr < 0) {
        brcm_log("Firmware error: %d\n", fwerr);
        err = -EBADE;
    }
    return err;
}

int32_t brcmf_fil_cmd_data_set(int ifidx, uint32_t cmd, void *data, uint32_t len)
{
    int32_t err;
    err = brcmf_fil_cmd_data(ifidx, cmd, data, len, true);
    return err;
}

int32_t brcmf_fil_cmd_data_get(int ifidx, uint32_t cmd, void *data, uint32_t len)
{
    int32_t err;

    err = brcmf_fil_cmd_data(ifidx, cmd, data, len, false);

    return err;
}

int32_t brcmf_fil_cmd_int_set(int ifidx, uint32_t cmd, uint32_t data)
{
    return brcmf_fil_cmd_data(ifidx, cmd, &data, sizeof(data), true);
}

int32_t brcmf_fil_cmd_int_get(int ifidx, uint32_t cmd, uint32_t *data)
{
    int32_t err;
    __le32 data_le = cpu_to_le32(*data);

    err = brcmf_fil_cmd_data(ifidx, cmd, &data_le, sizeof(data_le), false);
    *data = le32_to_cpu(data_le);

    return err;
}

static uint32_t
brcmf_create_iovar(char *name, const char *data, uint32_t datalen,
           uint8_t *buf, uint32_t buflen)
{
    uint32_t len;

    len = strlen(name) + 1;

    if ((len + datalen) > buflen)
        return 0;

    memcpy(buf, name, len);

    /* append data onto the end of the name string */
    if (data && datalen)
        memcpy(&buf[len], data, datalen);

    return len + datalen;
}

int32_t brcmf_fil_iovar_data_set(int ifidx, char *name, const void *data,
             uint32_t len)
{
    int32_t err;
    uint32_t buflen;

    buflen = brcmf_create_iovar(name, data, len, proto_buf,
                    sizeof(proto_buf));
    if (buflen) {
        err = brcmf_fil_cmd_data(ifidx, BRCMF_C_SET_VAR, proto_buf,
                     buflen, true);
    } else {
        err = -EPERM;
        brcm_log("Creating iovar failed\n");
    }

    return err;
}

int32_t brcmf_fil_iovar_data_get(int ifidx, char *name, void *data,
             uint32_t len)
{
    int32_t err;
    uint32_t buflen;


    buflen = brcmf_create_iovar(name, data, len, proto_buf,
                    sizeof(proto_buf));
    if (buflen) {
        err = brcmf_fil_cmd_data(ifidx, BRCMF_C_GET_VAR, proto_buf,
                     buflen, false);
        if (err == 0)
            memcpy(data, proto_buf, len);
    } else {
        err = -EPERM;
        brcm_log("Creating iovar failed\n");
    }

    return err;
}


int32_t brcmf_fil_iovar_int_set(int ifidx, char *name, uint32_t data)
{
    return brcmf_fil_iovar_data_set(ifidx, name, &data, sizeof(data));
}

int32_t brcmf_fil_iovar_int_get(int ifidx, char *name, uint32_t *data)
{
    __le32 data_le = cpu_to_le32(*data);
    int32_t err;

    err = brcmf_fil_iovar_data_get(ifidx, name, &data_le, sizeof(data_le));
    if (err == 0)
        *data = le32_to_cpu(data_le);
    return err;
}

static int brcmf_c_download(int ifidx, uint16_t flag,
                struct brcmf_dload_data_le *dload_buf,
                uint32_t len)
{
    int32_t err;

    flag |= (DLOAD_HANDLER_VER << DLOAD_FLAG_VER_SHIFT);
    dload_buf->flag = cpu_to_le16(flag);
    dload_buf->dload_type = cpu_to_le16(DL_TYPE_CLM);
    dload_buf->len = cpu_to_le32(len);
    dload_buf->crc = cpu_to_le32(0);
    len = sizeof(*dload_buf) + len - 1;

    err = brcmf_fil_iovar_data_set(ifidx, "clmload", dload_buf, len);

    return err;
}

static int brcmf_set_join_pref(int ifidx)
{
    struct brcmf_join_pref_params join_pref_params[2];

    //brcmf_fil_cmd_int_set(ifidx, BRCMF_C_SET_ASSOC_PREFER, WLC_BAND_AUTO);
    brcmf_fil_cmd_int_set(ifidx, BRCMF_C_SET_ASSOC_PREFER, WLC_BAND_ALL);

    join_pref_params[0].type = BRCMF_JOIN_PREF_RSSI_DELTA;
    join_pref_params[0].len = 2;
    join_pref_params[0].rssi_gain = 0x8;
    join_pref_params[0].band = 0x1;
    join_pref_params[1].type = BRCMF_JOIN_PREF_RSSI;
    join_pref_params[1].len = 2;
    join_pref_params[1].rssi_gain = 0;
    join_pref_params[1].band = 0;
    int err = brcmf_fil_iovar_data_set(0, "join_pref", &join_pref_params,
                       sizeof(join_pref_params));
    return err;
}

/* Join with specific  SSID
*/
static int brcmf_join(int ifidx, const char* ssid)
{
    struct brcmf_ext_join_params_le ext_join_params = {0};

    int ssid_len = min(strlen(ssid), 32);
    ext_join_params.ssid_le.SSID_len = cpu_to_le32(ssid_len);
    memcpy(&ext_join_params.ssid_le.SSID, ssid, strlen(ssid));
    ext_join_params.scan_le.scan_type = -1;
    ext_join_params.scan_le.home_time = cpu_to_le32(-1);
    memset(ext_join_params.assoc_le.bssid, 0xFF, 6);
    ext_join_params.scan_le.active_time = cpu_to_le32(-1);
    ext_join_params.scan_le.passive_time = cpu_to_le32(-1);
    ext_join_params.scan_le.nprobes = cpu_to_le32(-1);
    ext_join_params.assoc_le.chanspec_num = cpu_to_le32(0);

    brcmf_set_join_pref(0);

    int err  = brcmf_fil_iovar_data_set(ifidx, "join", &ext_join_params,
                     sizeof(ext_join_params) - 4);

    return err;
}

static int brcmf_c_process_clm_blob(int ifidx)
{
    struct brcmf_dload_data_le *chunk_buf;
    uint8_t *clm = NULL;
    uint32_t datalen;
    uint32_t chunk_len;
    uint32_t cumulative_len;
    uint16_t dl_flag = DL_BEGIN;
    uint32_t status;
    int32_t err;

    clm = brcmf_fw_get_clm(&datalen);

    chunk_buf = malloc(sizeof(*chunk_buf) + MAX_CHUNK_LEN - 1);
    if (!chunk_buf) {
        err = -ENOMEM;
        return err;
    }

    cumulative_len = 0;
    do {
        if (datalen > MAX_CHUNK_LEN) {
            chunk_len = MAX_CHUNK_LEN;
        } else {
            chunk_len = datalen;
            dl_flag |= DL_END;
        }
        memcpy(chunk_buf->data, clm + cumulative_len, chunk_len);

        err = brcmf_c_download(ifidx, dl_flag, chunk_buf, chunk_len);

        dl_flag &= ~DL_BEGIN;

        cumulative_len += chunk_len;
        datalen -= chunk_len;
    } while ((datalen > 0) && (err == 0));

    if (err) {
        brcm_log("clmload failed (%d)\n", err);
        /* Retrieve clmload_status and print */
        err = brcmf_fil_iovar_int_get(ifidx, "clmload_status", &status);
        if (err)
            brcm_log("get clmload_status failed (%d)\n", err);
        else
            brcm_log("clmload_status=%d\n", status);
        err = -EIO;
    }

    free(chunk_buf);
    return err;
}


static int brcmf_dump_revinfo(struct brcmf_rev_info *ri)
{
    brcm_log("vendorid: 0x%04x\n", ri->vendorid);
    brcm_log("deviceid: 0x%04x\n", ri->deviceid);
    brcm_log("radiorev: %x\n", ri->radiorev);
    brcm_log("chip: %s\n", ri->chipname);
    brcm_log("chippkg: %u\n", ri->chippkg);
    brcm_log("corerev: %u\n", ri->corerev);
    brcm_log("boardid: 0x%04x\n", ri->boardid);
    brcm_log("boardvendor: 0x%04x\n", ri->boardvendor);
    brcm_log("boardrev: %x\n", ri->boardrev);
    brcm_log("driverrev: %x\n", ri->driverrev);
    brcm_log("ucoderev: %u\n", ri->ucoderev);
    brcm_log("bus: %u\n", ri->bus);
    brcm_log("phytype: %u\n", ri->phytype);
    brcm_log("phyrev: %u\n", ri->phyrev);
    brcm_log("anarev: %u\n", ri->anarev);
    brcm_log("nvramrev: %08x\n", ri->nvramrev);


    return 0;
}

int brcmf_c_preinit_dcmds(void)
{
    uint8_t buf[BRCMF_DCMD_SMLEN];
    struct brcmf_rev_info_le revinfo;
    struct brcmf_rev_info ri;
    int32_t err = 0;
    int chip = 0;
    int chiprev = 0;

    /* retreive mac address */
    err = brcmf_fil_iovar_data_get(0, "cur_etheraddr", mac_addr,
                       sizeof(mac_addr));
    if (err < 0) {
        brcm_log("Retrieving cur_etheraddr failed, %d\n", err);
        goto done;
    }

    err = brcmf_fil_cmd_data_get(0, BRCMF_C_GET_REVINFO,
                     &revinfo, sizeof(revinfo));
    if (err < 0) {
        brcm_log("retrieving revision info failed, %d\n", err);
        strlcpy(ri.chipname, "UNKNOWN", sizeof(ri.chipname));
    } else {
        ri.vendorid = le32_to_cpu(revinfo.vendorid);
        ri.deviceid = le32_to_cpu(revinfo.deviceid);
        ri.radiorev = le32_to_cpu(revinfo.radiorev);
        ri.corerev = le32_to_cpu(revinfo.corerev);
        ri.boardid = le32_to_cpu(revinfo.boardid);
        ri.boardvendor = le32_to_cpu(revinfo.boardvendor);
        ri.boardrev = le32_to_cpu(revinfo.boardrev);
        ri.driverrev = le32_to_cpu(revinfo.driverrev);
        ri.ucoderev = le32_to_cpu(revinfo.ucoderev);
        ri.bus = le32_to_cpu(revinfo.bus);
        ri.phytype = le32_to_cpu(revinfo.phytype);
        ri.phyrev = le32_to_cpu(revinfo.phyrev);
        ri.anarev = le32_to_cpu(revinfo.anarev);
        ri.chippkg = le32_to_cpu(revinfo.chippkg);
        ri.nvramrev = le32_to_cpu(revinfo.nvramrev);

        /* use revinfo if not known yet */
        if (!chip) {
            chip = le32_to_cpu(revinfo.chipnum);
            chiprev = le32_to_cpu(revinfo.chiprev);
        }
    }
    ri.result = err;

    if (chip)
        brcmf_chip_name(chip, chiprev,
                ri.chipname, sizeof(ri.chipname));

    brcmf_dump_revinfo(&ri);

    /* Do any CLM downloading */
    err = brcmf_c_process_clm_blob(0);
    if (err < 0) {
        brcm_log("download CLM blob file failed, %d\n", err);
        goto done;
    }

    /* query for 'ver' to get version info from firmware */
    memset(buf, 0, sizeof(buf));
    err = brcmf_fil_iovar_data_get(0, "ver", buf, sizeof(buf));
    if (err < 0) {
        brcm_log("Retrieving version information failed, %d\n", err);
        goto done;
    }
    char* ptr = (char *)buf;
    strsep(&ptr, "\n");

    /* Print fw version info */
    brcm_log("Firmware: %s %s\n", ri.chipname, buf);

    /* Query for 'clmver' to get CLM version info from firmware */
    memset(buf, 0, sizeof(buf));
    err = brcmf_fil_iovar_data_get(0, "clmver", buf, sizeof(buf));
    if (err) {
        brcm_log("retrieving clmver failed, %d\n", err);
    } else {
        brcm_log("CLM version = %s\n", buf);
    }

    /* set mpc */
    err = brcmf_fil_iovar_int_set(0, "mpc", 1);
    if (err) {
        brcm_log("failed setting mpc\n");
        goto done;
    }

    /* Setup event_msgs, enable E_IF */
    int8_t  eventmask[18] = {0x61, 0x15, 0x0b, 0x00, 0x02, 0x42, 0xc0, 0x11, 
                             0x60, 0x09, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
    err = brcmf_fil_iovar_data_set(0, "event_msgs", eventmask, BRCMF_EVENTING_MASK_LEN);
    if (err) {
        brcm_log("Set event_msgs error (%d)\n", err);
        goto done;
    }

    /* Setup default scan unassoc time */
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_SET_SCAN_UNASSOC_TIME, 40);
    if (err) {
        brcm_log("BRCMF_C_SET_SCAN_UNASSOC_TIME error (%d)\n",
             err);
        goto done;
    }

    /* Enable tx beamforming, errors can be ignored (not supported) */
    (void)brcmf_fil_iovar_int_set(0, "txbf", 1);

    struct brcmf_fil_bwcap_le band_bwcap;
    band_bwcap.band = 2;
    band_bwcap.bw_cap = 3;
    err = brcmf_fil_iovar_data_set(0, "bw_cap", &band_bwcap, sizeof(band_bwcap));
    if (err < 0)
        brcm_log("set bw_cap error %d\n", err);

    brcmf_fil_iovar_int_set(0, "tdls_enable", 1);
    //set contry code to CN
    uint8_t ccreq[12] = {0x43, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4e, 0x00, 0x00};
    err = brcmf_fil_iovar_data_set(0, "country", &ccreq, sizeof(ccreq));
    if (err) {
        brcm_log("Firmware rejected country setting\n");
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "bcn_timeout", 4);
    if (err) {
        brcm_log("bcn_timeout error (%d)\n", err);
    }

    err =  brcmf_fil_iovar_int_set(0, "roam_off", 1);
    if (err) {
        brcm_log("set roam error (%d)\n", err);
    }

    err = brcmf_fil_iovar_int_set(0, "pm2_sleep_ret", 2000);
    if (err){
        brcm_log("Unable to set pm timeout, (%d)\n", err);
    }

    brcmf_fil_iovar_int_set(0, "apsta", 1);

    brcmf_fil_iovar_int_set(0, "p2p_disc", 1);

    /* Setup default scan channel time */
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_SET_SCAN_CHANNEL_TIME,
                    40);
    if (err) {
        brcm_log("BRCMF_C_SET_SCAN_CHANNEL_TIME error (%d)\n",
             err);
        goto done;
    }

    /* Bring device back up*/
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_UP, 1);
    if (err < 0)
        brcm_log("BRCMF_C_UP error %d\n", err);

done:
    return err;
}

static void brcmf_escan_prep(struct brcmf_scan_params_le *params_le)
{
    uint32_t n_ssids;
    uint32_t n_channels;

    const uint8_t ch_list[44] = {0x01,0x10,0x02,0x10,0x03,0x10,0x04,0x10, 
        0x05,0x10,0x06,0x10,0x07,0x10,0x08,0x10,0x09,0x10,0x0a,0x10, 
        0x0b,0x10,0x0c,0x10,0x0d,0x10,0x24,0xd0,0x28,0xd0,0x2c,0xd0, 
        0x30,0xd0,0x95,0xd0,0x99,0xd0,0x9d,0xd0,0xa1,0xd0,0xa5,0xd0 
    };

    memset(params_le->bssid, 0xff, 6);
    params_le->bss_type = DOT11_BSSTYPE_ANY;
    params_le->scan_type = BRCMF_SCANTYPE_ACTIVE;
    params_le->nprobes = cpu_to_le32(-1);
    params_le->active_time = cpu_to_le32(-1);
    params_le->passive_time = cpu_to_le32(-1);
    params_le->home_time = cpu_to_le32(-1);
    memset(&params_le->ssid_le, 0, sizeof(params_le->ssid_le));

    n_ssids = 1;
    n_channels = 22;
    memcpy(params_le->channel_list, ch_list, sizeof(ch_list));

    params_le->scan_type = BRCMF_SCANTYPE_PASSIVE;
    /* Adding mask to channel numbers */
    params_le->channel_num =
        cpu_to_le32((n_ssids << BRCMF_SCAN_PARAMS_NSSID_SHIFT) |
            (n_channels & BRCMF_SCAN_PARAMS_COUNT_MASK));
}


void scan(void)
{
    int32_t params_size = BRCMF_SCAN_PARAMS_FIXED_SIZE +
              offsetof(struct brcmf_escan_params_le, params_le) + 44 + 36;
    struct brcmf_escan_params_le *params;
    int32_t err;

    brcm_log("E-SCAN START\n");

    params = calloc(1, params_size);
    if (!params) {
        err = -ENOMEM;
        goto exit;
    }
    brcmf_escan_prep(&params->params_le);
    params->version = cpu_to_le32(BRCMF_ESCAN_REQ_VERSION);
    params->action = cpu_to_le16(1);
    params->sync_id = cpu_to_le16(0x1234);

    err = brcmf_fil_iovar_data_set(0, "escan", params, params_size);
    if (err) {
        if (err == -EBUSY)
            brcm_log("system busy : escan canceled\n");
        else
            brcm_log("error (%d)\n", err);
    }

    free(params);
exit:
    return;
}

void get_ethaddr(char* mac){
    memcpy(mac, mac_addr, 6);
}


int connect(const char*ssid, const char* pmk)
{
    int32_t err = 0;

    if(strlen(pmk) < 64){
        brcm_log("Wrong PMK lens\n");
        return -1;
    }
    brcm_log("Connect to %s...\n", ssid);

    /* A normal (non P2P) connection request setup. */
    const char ie[22] = {0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 
                         0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 
                         0x00, 0x0f, 0xac, 0x02, 0x80, 0x00};

    brcmf_fil_iovar_data_set(0, "wpaie", ie, sizeof(ie));

    err = brcmf_fil_iovar_int_set(0, "wpa_auth", WPA2_AUTH_UNSPECIFIED|WPA2_AUTH_PSK);
    if (err) {
        brcm_log("clear wpa_auth failed (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "auth", 0);
    if (err) {
        brcm_log("set auth failed (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "wsec", AES_ENABLED|TKIP_ENABLED);
    if (err) {
        brcm_log("error (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "wpa_auth", 0);
    if (err) {
        brcm_log("clear wpa_auth failed (%d)\n", err);
        return err;
    }

    brcmf_fil_iovar_int_set(0, "mfp", BRCMF_MFP_CAPABLE);
    if (err) {
        brcm_log("set mfp failed (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "wpa_auth", WPA2_AUTH_PSK);
    if (err) {
        brcm_log("set wpa_auth failed (%d)\n", err);
        return err;
    }

    /* enable firmware supplicant for this interface */
    err = brcmf_fil_iovar_int_set(0, "sup_wpa", 1);
    if (err < 0) {
        klog("failed to enable fw supplicant\n");
        goto done;
    }

    struct brcmf_wsec_pmk_le pmk_le = {0};

    /* pass pmk */
    pmk_le.key_len = cpu_to_le16(32);
    pmk_le.flags = cpu_to_le16(0);
    to_hex(pmk, pmk_le.key, 32);

     /* store psk in firmware */
    err = brcmf_fil_cmd_data_set(0, BRCMF_C_SET_WSEC_PMK, &pmk_le, sizeof(pmk_le));
    if (err < 0){
        brcm_log("failed to change PSK in firmware\n");
        goto done;
    }


    err = brcmf_join(0, ssid);
    if (err < 0){
        brcm_log("failed to join SSID:%s\n", ssid);
        goto done;
    }

done:
    return err;
}