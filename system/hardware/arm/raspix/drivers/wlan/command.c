#include "include/types.h"
#include "include/fwil_types.h"
#include "include/command.h"
#include "include/core.h"
#include "log.h"
#include "brcmu_wifi.h"
#include "nl80211.h"

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
static int bands[256];
static int reqid = 0;

int debug_dump = 0;

void
hexdump(const char* lable, const void *data, size_t size)
{
    unsigned char *src;
    int offset, index;
	while(debug_dump){usleep(1);};
    debug_dump = 1;
    src = (unsigned char *)data;
    brcm_klog("%s %d:\n", lable, size);
    brcm_klog("+------+-------------------------------------------------+------------------+\n");
    for(offset = 0; offset < (int)size; offset += 16) {
		brcm_klog("| %04x | ", offset);
        for(index = 0; index < 16; index++) {
            if(offset + index < (int)size) {
                brcm_klog("%02x ", 0xff & src[offset + index]);
            } else {
                brcm_klog("   ");
            }
        }
        brcm_klog("| ");
        for(index = 0; index < 16; index++) {
            if(offset + index < (int)size) {
                if(isascii(src[offset + index]) && isprint(src[offset + index])) {
                    brcm_klog("%c", src[offset + index]);
                } else {
                    brcm_klog(".");
                }
            } else {
                brcm_klog(" ");
            }
        }
        brcm_klog(" |\n");
    }
    brcm_klog("+------+-------------------------------------------------+------------------+\n");
    debug_dump = 0;
}

static int
brcmf_proto_bcdc_msg(int ifidx, uint cmd, void *buf,
             uint len, bool set)
{
    u32 flags;
    struct brcmf_proto_bcdc_dcmd *msg = temp + 32;
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


static int brcmf_proto_bcdc_cmplt(u32 id, u32 len)
{
    int ret;

    struct brcmf_proto_bcdc_dcmd *msg = temp + 32;
    memset(msg, 0, sizeof(struct brcmf_proto_bcdc_dcmd));

    len += sizeof(struct brcmf_proto_bcdc_dcmd);
    do {
        ret = brcmf_sdio_bus_rxctl(msg, len);
        if (ret < 0)
            break;
    } while (BCDC_DCMD_ID(le32_to_cpu(msg->flags)) != id);

    //hexdump(__func__, msg, ret);

    return ret;
}

static int
brcmf_proto_bcdc_set_dcmd(int ifidx, uint cmd,
              void *buf, uint len, int *fwerr)
{
    struct brcmf_proto_bcdc_dcmd *msg = temp + 32;
    int ret;
    u32 flags, id;

    // brcm_klog("brcmf_proto_bcdc_set_dcmd cmd %d len %d\n", cmd, len);

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
        brcm_klog("unexpected request id %d (expected %d)\n", id, reqid);
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
    struct brcmf_proto_bcdc_dcmd *msg = temp + 32;
    void *info;
    int ret = 0, retries = 0;
    u32 id, flags;

    *fwerr = 0;
    ret = brcmf_proto_bcdc_msg(ifidx, cmd, buf, len, false);
    if (ret < 0) {
        brcm_klog("brcmf_proto_bcdc_msg failed w/status %d\n",
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
        brcm_klog("unexpected request id %d (expected %d)\n", id, reqid);
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

static s32
brcmf_fil_cmd_data(int ifidx, u32 cmd, void *data, u32 len, bool set)
{
    s32 err, fwerr;

    if (data != NULL)
        len = min_t(uint, len, BRCMF_DCMD_MAXLEN);
    if (set)
        err = brcmf_proto_bcdc_set_dcmd(ifidx, cmd,
                       data, len, &fwerr);
    else
        err = brcmf_proto_bcdc_query_dcmd(ifidx, cmd,
                         data, len, &fwerr);

    if (err) {
        brcm_klog("Failed: error=%d\n", err);
    } else if (fwerr < 0) {
        brcm_klog("Firmware error: %d\n", fwerr);
        err = -EBADE;
    }
    return err;
}

s32 brcmf_fil_cmd_data_set(int ifidx, u32 cmd, void *data, u32 len)
{
    s32 err;
    err = brcmf_fil_cmd_data(ifidx, cmd, data, len, true);
    return err;
}

s32 brcmf_fil_cmd_data_get(int ifidx, u32 cmd, void *data, u32 len)
{
    s32 err;

    err = brcmf_fil_cmd_data(ifidx, cmd, data, len, false);

    return err;
}

s32 brcmf_fil_cmd_int_set(int ifidx, u32 cmd, u32 data)
{
    return brcmf_fil_cmd_data(ifidx, cmd, &data, sizeof(data), true);
}

s32 brcmf_fil_cmd_int_get(int ifidx, u32 cmd, u32 *data)
{
    s32 err;
    __le32 data_le = cpu_to_le32(*data);

    err = brcmf_fil_cmd_data(ifidx, cmd, &data_le, sizeof(data_le), false);
    *data = le32_to_cpu(data_le);

    return err;
}

static u32
brcmf_create_iovar(char *name, const char *data, u32 datalen,
           char *buf, u32 buflen)
{
    u32 len;

    len = strlen(name) + 1;

    if ((len + datalen) > buflen)
        return 0;

    memcpy(buf, name, len);

    /* append data onto the end of the name string */
    if (data && datalen)
        memcpy(&buf[len], data, datalen);

    return len + datalen;
}

s32 brcmf_fil_iovar_data_set(int ifidx, char *name, const void *data,
             u32 len)
{
    s32 err;
    u32 buflen;

    buflen = brcmf_create_iovar(name, data, len, proto_buf,
                    sizeof(proto_buf));
    if (buflen) {
        err = brcmf_fil_cmd_data(ifidx, BRCMF_C_SET_VAR, proto_buf,
                     buflen, true);
    } else {
        err = -EPERM;
        brcm_klog("Creating iovar failed\n");
    }

    return err;
}

s32 brcmf_fil_iovar_data_get(int ifidx, char *name, void *data,
             u32 len)
{
    s32 err;
    u32 buflen;


    buflen = brcmf_create_iovar(name, data, len, proto_buf,
                    sizeof(proto_buf));
    if (buflen) {
        err = brcmf_fil_cmd_data(ifidx, BRCMF_C_GET_VAR, proto_buf,
                     buflen, false);
        if (err == 0)
            memcpy(data, proto_buf, len);
    } else {
        err = -EPERM;
        brcm_klog("Creating iovar failed\n");
    }

    return err;
}


s32 brcmf_fil_iovar_int_set(int ifidx, char *name, u32 data)
{
    return brcmf_fil_iovar_data_set(ifidx, name, &data, sizeof(data));
}

s32 brcmf_fil_iovar_int_get(int ifidx, char *name, u32 *data)
{
    __le32 data_le = cpu_to_le32(*data);
    s32 err;

    err = brcmf_fil_iovar_data_get(ifidx, name, &data_le, sizeof(data_le));
    if (err == 0)
        *data = le32_to_cpu(data_le);
    return err;
}

static int brcmf_c_download(int ifidx, u16 flag,
                struct brcmf_dload_data_le *dload_buf,
                u32 len)
{
    s32 err;

    flag |= (DLOAD_HANDLER_VER << DLOAD_FLAG_VER_SHIFT);
    dload_buf->flag = cpu_to_le16(flag);
    dload_buf->dload_type = cpu_to_le16(DL_TYPE_CLM);
    dload_buf->len = cpu_to_le32(len);
    dload_buf->crc = cpu_to_le32(0);
    len = sizeof(*dload_buf) + len - 1;

    err = brcmf_fil_iovar_data_set(ifidx, "clmload", dload_buf, len);

    return err;
}

void brcmf_c_set_joinpref_default(int ifidx)
{
    struct brcmf_join_pref_params join_pref_params[2];
    int err;

    /* Setup join_pref to select target by RSSI (boost on 5GHz) */
    join_pref_params[0].type = BRCMF_JOIN_PREF_RSSI_DELTA;
    join_pref_params[0].len = 2;
    join_pref_params[0].rssi_gain = 8;
    join_pref_params[0].band = 1;

    join_pref_params[1].type = BRCMF_JOIN_PREF_RSSI;
    join_pref_params[1].len = 2;
    join_pref_params[1].rssi_gain = 0;
    join_pref_params[1].band = 0;
    err = brcmf_fil_iovar_data_set(ifidx, "join_pref", join_pref_params,
                       sizeof(join_pref_params));
    if (err)
        brcm_klog("Set join_pref error (%d)\n", err);
}

static int brcmf_c_process_clm_blob(int ifidx)
{
    struct brcmf_dload_data_le *chunk_buf;
    uint8_t *clm = NULL;
    u8 clm_name[320];
    int datalen;
    u32 chunk_len;
    u32 cumulative_len;
    u16 dl_flag = DL_BEGIN;
    u32 status;
    s32 err;

    brcm_klog("%s\n", __func__);

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
        brcm_klog("clmload failed (%d)\n", err);
        /* Retrieve clmload_status and print */
        err = brcmf_fil_iovar_int_get(ifidx, "clmload_status", &status);
        if (err)
            brcm_klog("get clmload_status failed (%d)\n", err);
        else
            brcm_klog("clmload_status=%d\n", status);
        err = -EIO;
    }

    free(chunk_buf);
    return err;
}


static int brcmf_dump_revinfo(struct brcmf_rev_info *ri)
{
    brcm_klog("vendorid: 0x%04x\n", ri->vendorid);
    brcm_klog("deviceid: 0x%04x\n", ri->deviceid);
    brcm_klog("radiorev: %x\n", ri->radiorev);
    brcm_klog("chip: %s\n", ri->chipname);
    brcm_klog("chippkg: %u\n", ri->chippkg);
    brcm_klog("corerev: %u\n", ri->corerev);
    brcm_klog("boardid: 0x%04x\n", ri->boardid);
    brcm_klog("boardvendor: 0x%04x\n", ri->boardvendor);
    brcm_klog("boardrev: %x\n", ri->boardrev);
    brcm_klog("driverrev: %x\n", ri->driverrev);
    brcm_klog("ucoderev: %u\n", ri->ucoderev);
    brcm_klog("bus: %u\n", ri->bus);
    brcm_klog("phytype: %u\n", ri->phytype);
    brcm_klog("phyrev: %u\n", ri->phyrev);
    brcm_klog("anarev: %u\n", ri->anarev);
    brcm_klog("nvramrev: %08x\n", ri->nvramrev);


    return 0;
}

// struct brcmf_p2p_disc_st_le {
//     u8 state;
//     __le16 chspec;
//     __le16 dwell;
// };

// static s32 brcmf_p2p_set_discover_state(struct brcmf_if *ifp, u8 state,
//                     u16 chanspec, u16 listen_ms)
// {
//     struct brcmf_p2p_disc_st_le discover_state;
//     s32 ret = 0;

//     discover_state.state = state;
//     discover_state.chspec = cpu_to_le16(chanspec);
//     discover_state.dwell = cpu_to_le16(listen_ms);
//     ret = brcmf_fil_bsscfg_data_set(ifp, "p2p_state", &discover_state,
//                     sizeof(discover_state));
//     return ret;
// }

static void brcmf_get_bwcap(int ifidx, u32 bw_cap[])
{
    u32 band, mimo_bwcap;
    int err;

    band = WLC_BAND_2G;
    err = brcmf_fil_iovar_int_get(ifidx, "bw_cap", &band);
    if (!err) {
        bw_cap[NL80211_BAND_2GHZ] = band;
        band = WLC_BAND_5G;
        err = brcmf_fil_iovar_int_get(ifidx, "bw_cap", &band);
        if (!err) {
            bw_cap[NL80211_BAND_5GHZ] = band;
            return;
        }
        return;
    }
    brcm_klog("fallback to mimo_bw_cap info\n");
    mimo_bwcap = 0;
    err = brcmf_fil_iovar_int_get(0, "mimo_bw_cap", &mimo_bwcap);
    if (err)
        /* assume 20MHz if firmware does not give a clue */
        mimo_bwcap = WLC_N_BW_20ALL;

    switch (mimo_bwcap) {
    case WLC_N_BW_40ALL:
        bw_cap[NL80211_BAND_2GHZ] |= WLC_BW_40MHZ_BIT;
    case WLC_N_BW_20IN2G_40IN5G:
        bw_cap[NL80211_BAND_5GHZ] |= WLC_BW_40MHZ_BIT;
    case WLC_N_BW_20ALL:
        bw_cap[NL80211_BAND_2GHZ] |= WLC_BW_20MHZ_BIT;
        bw_cap[NL80211_BAND_5GHZ] |= WLC_BW_20MHZ_BIT;
        break;
    default:
        brcm_klog("invalid mimo_bw_cap value\n");
    }
}
#if 0
static int brcmf_construct_chaninfo(int ifidx, struct brcmf_cfg80211_info *cfg,
                    u32 bw_cap[])
{
    struct ieee80211_supported_band *band;
    struct ieee80211_channel *channel;
    struct brcmf_chanspec_list *list;
    struct brcmu_chan ch;
    int err;
    u8 *pbuf;
    u32 i, j;
    u32 total;
    u32 chaninfo;

    pbuf = malloc(BRCMF_DCMD_MEDLEN);

    if (pbuf == NULL)
        return -ENOMEM;

    list = (struct brcmf_chanspec_list *)pbuf;

    err = brcmf_fil_iovar_data_get(ifidx, "chanspecs", pbuf,
                       BRCMF_DCMD_MEDLEN);
    if (err) {
        brcm_klog("get chanspecs error (%d)\n", err);
        goto fail_pbuf;
    }

    band = bands[NL80211_BAND_2GHZ];
    if (band)
        for (i = 0; i < band->n_channels; i++)
            band->channels[i].flags = IEEE80211_CHAN_DISABLED;
    band = bands[NL80211_BAND_5GHZ];
    if (band)
        for (i = 0; i < band->n_channels; i++)
            band->channels[i].flags = IEEE80211_CHAN_DISABLED;

    total = le32_to_cpu(list->count);
    for (i = 0; i < total; i++) {
        ch.chspec = (u16)le32_to_cpu(list->element[i]);
        cfg->d11inf.decchspec(&ch);

        if (ch.band == BRCMU_CHAN_BAND_2G) {
            band = wiphy->bands[NL80211_BAND_2GHZ];
        } else if (ch.band == BRCMU_CHAN_BAND_5G) {
            band = wiphy->bands[NL80211_BAND_5GHZ];
        } else {
            brcm_klog("Invalid channel Spec. 0x%x.\n",
                 ch.chspec);
            continue;
        }
        if (!band)
            continue;
        if (!(bw_cap[band->band] & WLC_BW_40MHZ_BIT) &&
            ch.bw == BRCMU_CHAN_BW_40)
            continue;
        if (!(bw_cap[band->band] & WLC_BW_80MHZ_BIT) &&
            ch.bw == BRCMU_CHAN_BW_80)
            continue;

        channel = NULL;
        for (j = 0; j < band->n_channels; j++) {
            if (band->channels[j].hw_value == ch.control_ch_num) {
                channel = &band->channels[j];
                break;
            }
        }
        if (!channel) {
            /* It seems firmware supports some channel we never
             * considered. Something new in IEEE standard?
             */
            brcm_klog("Ignoring unexpected firmware channel %d\n",
                 ch.control_ch_num);
            continue;
        }

        if (channel->orig_flags & IEEE80211_CHAN_DISABLED)
            continue;

        /* assuming the chanspecs order is HT20,
         * HT40 upper, HT40 lower, and VHT80.
         */
        switch (ch.bw) {
        case BRCMU_CHAN_BW_160:
            channel->flags &= ~IEEE80211_CHAN_NO_160MHZ;
            break;
        case BRCMU_CHAN_BW_80:
            channel->flags &= ~IEEE80211_CHAN_NO_80MHZ;
            break;
        case BRCMU_CHAN_BW_40:
            brcmf_update_bw40_channel_flag(channel, &ch);
            break;
        default:
            brcm_klog("Firmware reported unsupported bandwidth %d\n",
                   ch.bw);
        case BRCMU_CHAN_BW_20:
            /* enable the channel and disable other bandwidths
             * for now as mentioned order assure they are enabled
             * for subsequent chanspecs.
             */
            channel->flags = IEEE80211_CHAN_NO_HT40 |
                     IEEE80211_CHAN_NO_80MHZ |
                     IEEE80211_CHAN_NO_160MHZ;
            ch.bw = BRCMU_CHAN_BW_20;
            cfg->d11inf.encchspec(&ch);
            chaninfo = ch.chspec;
            err = brcmf_fil_bsscfg_int_get(ifidx, "per_chan_info",
                               &chaninfo);
            if (!err) {
                if (chaninfo & WL_CHAN_RADAR)
                    channel->flags |=
                        (IEEE80211_CHAN_RADAR |
                         IEEE80211_CHAN_NO_IR);
                if (chaninfo & WL_CHAN_PASSIVE)
                    channel->flags |=
                        IEEE80211_CHAN_NO_IR;
            }
        }
    }

fail_pbuf:
    free(pbuf);
    return err;
}

static void brcmf_update_ht_cap(struct ieee80211_supported_band *band,
                u32 bw_cap[2], u32 nchain)
{
    band->ht_cap.ht_supported = true;
    if (bw_cap[band->band] & WLC_BW_40MHZ_BIT) {
        band->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
        band->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
    }
    band->ht_cap.cap |= IEEE80211_HT_CAP_SGI_20;
    band->ht_cap.cap |= IEEE80211_HT_CAP_DSSSCCK40;
    band->ht_cap.ampdu_factor = IEEE80211_HT_MAX_AMPDU_64K;
    band->ht_cap.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16;
    memset(band->ht_cap.mcs.rx_mask, 0xff, nchain);
    band->ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;
}

static void brcmf_update_vht_cap(struct ieee80211_supported_band *band,
                 u32 bw_cap[2], u32 nchain, u32 txstreams,
                 u32 txbf_bfe_cap, u32 txbf_bfr_cap)
{
    __le16 mcs_map;

    /* not allowed in 2.4G band */
    if (band->band == NL80211_BAND_2GHZ)
        return;

    band->vht_cap.vht_supported = true;
    /* 80MHz is mandatory */
    band->vht_cap.cap |= IEEE80211_VHT_CAP_SHORT_GI_80;
    if (bw_cap[band->band] & WLC_BW_160MHZ_BIT) {
        band->vht_cap.cap |= IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160MHZ;
        band->vht_cap.cap |= IEEE80211_VHT_CAP_SHORT_GI_160;
    }
    /* all support 256-QAM */
    mcs_map = brcmf_get_mcs_map(nchain, IEEE80211_VHT_MCS_SUPPORT_0_9);
    band->vht_cap.vht_mcs.rx_mcs_map = mcs_map;
    band->vht_cap.vht_mcs.tx_mcs_map = mcs_map;

    /* Beamforming support information */
    if (txbf_bfe_cap & BRCMF_TXBF_SU_BFE_CAP)
        band->vht_cap.cap |= IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE;
    if (txbf_bfe_cap & BRCMF_TXBF_MU_BFE_CAP)
        band->vht_cap.cap |= IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE;
    if (txbf_bfr_cap & BRCMF_TXBF_SU_BFR_CAP)
        band->vht_cap.cap |= IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE;
    if (txbf_bfr_cap & BRCMF_TXBF_MU_BFR_CAP)
        band->vht_cap.cap |= IEEE80211_VHT_CAP_MU_BEAMFORMER_CAPABLE;

    if ((txbf_bfe_cap || txbf_bfr_cap) && (txstreams > 1)) {
        band->vht_cap.cap |=
            (2 << IEEE80211_VHT_CAP_BEAMFORMEE_STS_SHIFT);
        band->vht_cap.cap |= ((txstreams - 1) <<
                IEEE80211_VHT_CAP_SOUNDING_DIMENSIONS_SHIFT);
        band->vht_cap.cap |=
            IEEE80211_VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB;
    }
}

static int brcmf_setup_wiphybands(int ifidx)
{
    u32 nmode = 0;
    u32 vhtmode = 0;
    u32 bw_cap[2] = { 0x1, 0x1 };
    u32 rxchain;
    u32 nchain;
    int err;
    s32 i;
    struct ieee80211_supported_band *band;
    u32 txstreams = 0;
    u32 txbf_bfe_cap = 0;
    u32 txbf_bfr_cap = 0;

    (void)brcmf_fil_iovar_int_get(0, "vhtmode", &vhtmode);
    err = brcmf_fil_iovar_int_get(0, "nmode", &nmode);
    if (err) {
        brcm_klog("nmode error (%d)\n", err);
    } else {
        brcmf_get_bwcap(ifidx, bw_cap);
    }
    brcm_klog("nmode=%d, vhtmode=%d, bw_cap=(%d, %d)\n",
          nmode, vhtmode, bw_cap[NL80211_BAND_2GHZ],
          bw_cap[NL80211_BAND_5GHZ]);

    err = brcmf_fil_iovar_int_get(ifidx, "rxchain", &rxchain);
    if (err) {
        brcm_klog("rxchain error (%d)\n", err);
        nchain = 1;
    } else {
        for (nchain = 0; rxchain; nchain++)
            rxchain = rxchain & (rxchain - 1);
    }
    dklog("nchain=%d\n", nchain);

    err = brcmf_construct_chaninfo(cfg, bw_cap);
    if (err) {
        brcm_klog("brcmf_construct_chaninfo failed (%d)\n", err);
        return err;
    }

    if (vhtmode) {
        (void)brcmf_fil_iovar_int_get(ifidx, "txstreams", &txstreams);
        (void)brcmf_fil_iovar_int_get(ifidx, "txbf_bfe_cap",
                          &txbf_bfe_cap);
        (void)brcmf_fil_iovar_int_get(ifidx, "txbf_bfr_cap",
                          &txbf_bfr_cap);
    }

    for (i = 0; i < ARRAY_SIZE(bands); i++) {
        band = bands[i];
        if (band == NULL)
            continue;

        if (nmode)
            brcmf_update_ht_cap(band, bw_cap, nchain);
        if (vhtmode)
            brcmf_update_vht_cap(band, bw_cap, nchain, txstreams,
                         txbf_bfe_cap, txbf_bfr_cap);
    }

    return 0;
}
#endif

int brcmf_c_preinit_dcmds(void)
{
    uint8_t mac_addr[6];
    u8 buf[BRCMF_DCMD_SMLEN];
    struct brcmf_rev_info_le revinfo;
    struct brcmf_rev_info ri;
    char *clmver;
    char *ptr;
    s32 err;
    int chip = 0;
    int chiprev = 0;

    /* retreive mac address */
    err = brcmf_fil_iovar_data_get(0, "cur_etheraddr", mac_addr,
                       sizeof(mac_addr));
    if (err < 0) {
        brcm_klog("Retrieving cur_etheraddr failed, %d\n", err);
        goto done;
    }

    err = brcmf_fil_cmd_data_get(0, BRCMF_C_GET_REVINFO,
                     &revinfo, sizeof(revinfo));
    if (err < 0) {
        brcm_klog("retrieving revision info failed, %d\n", err);
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
        brcm_klog("download CLM blob file failed, %d\n", err);
        goto done;
    }

    /* query for 'ver' to get version info from firmware */
    memset(buf, 0, sizeof(buf));
    err = brcmf_fil_iovar_data_get(0, "ver", buf, sizeof(buf));
    if (err < 0) {
        brcm_klog("Retrieving version information failed, %d\n",
             err);
        goto done;
    }
    ptr = (char *)buf;
    strsep(&ptr, "\n");

    /* Print fw version info */
    brcm_klog("Firmware: %s %s\n", ri.chipname, buf);

    /* Query for 'clmver' to get CLM version info from firmware */
    memset(buf, 0, sizeof(buf));
    err = brcmf_fil_iovar_data_get(0, "clmver", buf, sizeof(buf));
    if (err) {
        brcm_klog("retrieving clmver failed, %d\n", err);
    } else {
        clmver = (char *)buf;
        /* store CLM version for adding it to revinfo debugfs file */
        //memcpy(ifp->drvr->clmver, clmver, sizeof(ifp->drvr->clmver));

        /* Replace all newline/linefeed characters with space
         * character
         */

        brcm_klog("CLM version = %s\n", clmver);
    }

    /* set mpc */
    err = brcmf_fil_iovar_int_set(0, "mpc", 1);
    if (err) {
        brcm_klog("failed setting mpc\n");
        goto done;
    }

    brcmf_c_set_joinpref_default(0);

    /* Setup event_msgs, enable E_IF */
    int8_t  eventmask[18] = {0x61, 0x15, 0x0b, 0x00, 0x02, 0x42, 0xc0, 0x11, 
                             0x60, 0x09, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
    // err = brcmf_fil_iovar_data_get(0, "event_msgs", eventmask,
    //                    BRCMF_EVENTING_MASK_LEN);
    // if (err) {
    //     brcm_klog("Get event_msgs error (%d)\n", err);
    //     goto done;
    // }
    //setbit(eventmask, BRCMF_E_IF);
    err = brcmf_fil_iovar_data_set(0, "event_msgs", eventmask,
                       BRCMF_EVENTING_MASK_LEN);
    if (err) {
        brcm_klog("Set event_msgs error (%d)\n", err);
        goto done;
    }

    /* Setup default scan channel time */
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_SET_SCAN_CHANNEL_TIME, 40);
    if (err) {
        brcm_klog("BRCMF_C_SET_SCAN_CHANNEL_TIME error (%d)\n",
             err);
        goto done;
    }

    /* Setup default scan unassoc time */
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_SET_SCAN_UNASSOC_TIME, 40);
    if (err) {
        brcm_klog("BRCMF_C_SET_SCAN_UNASSOC_TIME error (%d)\n",
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
        brcm_klog("set bw_cap error %d\n", err);

    brcmf_fil_iovar_int_set(0, "tdls_enable", 1);

    //set contry code to CN
    uint8_t ccreq[12] = {0x43, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4e, 0x00, 0x00};
    err = brcmf_fil_iovar_data_set(0, "country", &ccreq, sizeof(ccreq));
    if (err) {
        brcm_klog("Firmware rejected country setting\n");
        return;
    }

    // err = brcmf_p2p_set_discover_state(0, 0, 0, 0);
    // if (err < 0) {
    //     brcm_klog("unable to set WL_P2P_DISC_ST_SCAN\n");
    //     goto done;
    // }

    err = brcmf_fil_iovar_int_set(0, "bcn_timeout", 4);
    if (err) {
        brcm_klog("bcn_timeout error (%d)\n", err);
    }
    brcmf_fil_iovar_int_set(0, "roam_off", 1);

    err = brcmf_fil_iovar_int_set(0, "pm2_sleep_ret", 2000);
    if (err){
        brcm_klog("Unable to set pm timeout, (%d)\n", err);
    }

    brcmf_fil_iovar_int_set(0, "apsta", 1);

    brcmf_fil_iovar_int_set(0, "p2p_disc", 1);
    
    brcmf_fil_iovar_int_set(0, "bsscfg:p2p_state", 1);
    brcmf_fil_iovar_int_set(0, "bsscfg:wsec", 1);

    /* Setup default scan channel time */
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_SET_SCAN_CHANNEL_TIME,
                    40);
    if (err) {
        brcm_klog("BRCMF_C_SET_SCAN_CHANNEL_TIME error (%d)\n",
             err);
        goto done;
    }

    /* Bring device back up*/
    err = brcmf_fil_cmd_int_set(0, BRCMF_C_UP, 1);
    if (err < 0)
        brcm_klog("BRCMF_C_UP error %d\n", err);

done:
    return err;
}


//+------+-------------------------------------------------+------------------+
//| 0000 | c4 00 3b ff  be  00 00 01 00 00 02 00 8c 00 00 14 | ..;............. |
//         len:196      seq ch:0    glom
//| 0010 | 00 00 00 00  07 01 00 00  9e 00 00 00  02 00 8e 00 | ................ |
//                      cmd:107      len:158      flag
//| 0020 | 00 00 00 00  65 73 63 61 6e 00  01 00 00 00  01 00 | ....escan....... |
//         state        name                version     action
//| 0030 | 34 12    00 00 00 00  00 00 00 00 00 00 00 00 00 00 | 4............... |
//          sync id  ssid len    ssid
//| 0040 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | ................ |
//| 0050 | 00 00 00 00 00 00    ff ff ff ff ff ff  02   00  ff ff | ................ |
//                              boardcast address  type     nprobes:-1
//| 0060 | ff ff  ff ff ff ff     ff ff ff ff     ff ff ff ff   16 00 | ................ |
//                active_time:-1  passive_time:-1  home_time:-1  ch num:16
//| 0070 | 01 00 01 10 02 10 03 10 04 10 05 10 06 10 07 10 | ................ |
//
//| 0080 | 08 10 09 10 0a 10 0b 10 0c 10 0d 10 24 d0 28 d0 | ............$.(. |
//| 0090 | 2c d0 30 d0 95 d0 99 d0 9d d0 a1 d0 a5 d0 00 00 | ,.0............. |
//| 00a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | ................ |
//| 00b0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | ................ |
//| 00c0 | 00 00 00 00                                     | ....             |
//+------+-------------------------------------------------+------------------+

const uint8_t ch_list[44] = {
    0x01,0x10,0x02,0x10, 
    0x03,0x10,0x04,0x10, 
    0x05,0x10,0x06,0x10, 
    0x07,0x10,0x08,0x10, 
    0x09,0x10,0x0a,0x10, 
    0x0b,0x10,0x0c,0x10, 
    0x0d,0x10,0x24,0xd0, 
    0x28,0xd0,0x2c,0xd0, 
    0x30,0xd0,0x95,0xd0, 
    0x99,0xd0,0x9d,0xd0, 
    0xa1,0xd0,0xa5,0xd0 
};

static void brcmf_escan_prep(struct brcmf_scan_params_le *params_le)
{
    u32 n_ssids;
    u32 n_channels;
    s32 i;
    s32 offset;
    u16 chanspec;
    char *ptr;
    struct brcmf_ssid_le ssid_le;

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
    s32 params_size = BRCMF_SCAN_PARAMS_FIXED_SIZE +
              offsetof(struct brcmf_escan_params_le, params_le) + 44 + 36;
    struct brcmf_escan_params_le *params;
    s32 err = 0;

    brcm_klog("E-SCAN START\n");

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
            brcm_klog("system busy : escan canceled\n");
        else
            brcm_klog("error (%d)\n", err);
    }

    free(params);
exit:
    return err;
}

void scan_result(void){

    struct brcmf_scan_results result;
    result.buflen = sizeof(struct brcmf_scan_results);
    result.version = 0;
    result.count = 1;
    int err = brcmf_fil_cmd_data_set(0, BRCMF_C_SCAN_RESULTS,
                        &result, sizeof(result));
}

void get_ethaddr(void){

}

static s32 brcmf_set_wpa_version(int ifidx)
{
    s32 val = 0;
    s32 err = 0;

    //val = WPA_AUTH_PSK | WPA_AUTH_UNSPECIFIED;
    //val = WPA2_AUTH_PSK | WPA2_AUTH_UNSPECIFIED;
    val = WPA2_AUTH_PSK;
    //val = WPA3_AUTH_SAE_PSK;
    //val = WPA_AUTH_DISABLED;
    err = brcmf_fil_iovar_int_set(ifidx, "wpa_auth", val);
    if (err) {
        brcm_klog("set wpa_auth failed (%d)\n", err);
        return err;
    }
    return err;
}

static int brcmf_set_pmk(int ifidx, const u8 *pmk_data, u16 pmk_len)
{
    struct brcmf_wsec_pmk_le pmk;
    int err;

    memset(&pmk, 0, sizeof(pmk));

    /* pass pmk directly */
    pmk.key_len = cpu_to_le16(pmk_len);
    pmk.flags = cpu_to_le16(0);
    memcpy(pmk.key, pmk_data, pmk_len);

    /* store psk in firmware */
    err = brcmf_fil_cmd_data_set(ifidx, BRCMF_C_SET_WSEC_PMK,
                     &pmk, sizeof(pmk));
    if (err < 0)
        brcm_klog("failed to change PSK in firmware (len=%u)\n",
             pmk_len);

    return err;
}


static void brcmf_set_join_pref(int ifidx)
{
    struct brcmf_join_pref_params join_pref_params[2];
    enum nl80211_band band;
    join_pref_params[0].type = 0x4;
    join_pref_params[0].len = 2;
    join_pref_params[0].rssi_gain = 0x8;
    join_pref_params[0].band = 0x1;
    join_pref_params[1].type = BRCMF_JOIN_PREF_RSSI;
    join_pref_params[1].len = 2;
    join_pref_params[1].rssi_gain = 0;
    join_pref_params[1].band = 0;
    int err = brcmf_fil_iovar_data_set(0, "join_pref", &join_pref_params,
                       sizeof(join_pref_params));
    if (err)
        brcm_klog("Set join_pref error (%d)\n", err);
}

int connect(const char*ssid, const char* pmk)
{
    struct brcmf_join_params join_params;
    size_t join_params_size;
    const struct brcmf_tlv *rsn_ie;
    const struct brcmf_vs_tlv *wpa_ie;
    u32 ie_len;
    struct brcmf_ext_join_params_le *ext_join_params;
    u16 chanspec;
    s32 err = 0;
    u32 ssid_len;

    brcm_klog("%s\n", __func__);

    /* A normal (non P2P) connection request setup. */
    const char ie[22] = {0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 
                         0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 
                         0x00, 0x0f, 0xac, 0x02, 0x80, 0x00};
                         
    brcmf_fil_iovar_data_set(0, "wpaie", ie, sizeof(ie));

    err = brcmf_fil_iovar_int_set(0, "wpa_auth", WPA2_AUTH_UNSPECIFIED|WPA2_AUTH_PSK);
    if (err) {
        brcm_klog("clear wpa_auth failed (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "auth", 0);
    if (err) {
        brcm_klog("set auth failed (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "wsec", AES_ENABLED);
    if (err) {
        brcm_klog("error (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "wpa_auth", 0);
    if (err) {
        brcm_klog("clear wpa_auth failed (%d)\n", err);
        return err;
    }

    brcmf_fil_iovar_int_set(0, "mfp", BRCMF_MFP_CAPABLE);
    if (err) {
        brcm_klog("set mfp failed (%d)\n", err);
        return err;
    }

    err = brcmf_fil_iovar_int_set(0, "wpa_auth", WPA2_AUTH_PSK);
    if (err) {
        brcm_klog("set wpa_auth failed (%d)\n", err);
        return err;
    }

    /* enable firmware supplicant for this interface */
    err = brcmf_fil_iovar_int_set(0, "sup_wpa", 1);
    if (err < 0) {
        klog("failed to enable fw supplicant\n");
        goto done;
    }

    brcmf_set_pmk(0, pmk, 32);
     /* Join with specific BSSID and cached SSID
     * If SSID is zero join based on BSSID only
     */
    join_params_size = offsetof(struct brcmf_ext_join_params_le, assoc_le) +
        offsetof(struct brcmf_assoc_params_le, chanspec_list);
    ext_join_params = calloc(1,join_params_size);
    if (ext_join_params == NULL) {
        err = -ENOMEM;
        goto done;
    }
    ssid_len = min(strlen(ssid), 32);
    ext_join_params->ssid_le.SSID_len = cpu_to_le32(ssid_len);
    memcpy(&ext_join_params->ssid_le.SSID, ssid, strlen(ssid));
    if (ssid_len < 32)
        brcm_klog("SSID \"%s\", len (%d)\n",
              ext_join_params->ssid_le.SSID, ssid_len);

    /* Set up join scan parameters */
    ext_join_params->scan_le.scan_type = -1;
    ext_join_params->scan_le.home_time = cpu_to_le32(-1);
    memset(ext_join_params->assoc_le.bssid, 0xFF, 6);
    ext_join_params->scan_le.active_time = cpu_to_le32(-1);
    ext_join_params->scan_le.passive_time = cpu_to_le32(-1);
    ext_join_params->scan_le.nprobes = cpu_to_le32(-1);

    brcmf_set_join_pref(0);

    err  = brcmf_fil_iovar_data_set(0, "join", ext_join_params,
                     join_params_size);
    free(ext_join_params);
done:
    return err;
}

//bw_cap
//event_msgs
//tdls_enable
//event_msgs
//BRCMF_C_SET_PM
//bcn_timeout
//roam_off
//arp_ol
//arpoe
//ndoe
//pm2_sleep_re
//mcast_list
//allmulti
//arp_ol
//arpoe
//ndoeo
//pmkid_info