#include "types.h"
#include "cfg80211.h"


s32 brcmf_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev,
               struct cfg80211_connect_params *sme)
{
    struct brcmf_cfg80211_info *cfg = wiphy_to_cfg(wiphy);
    struct brcmf_if *ifp = netdev_priv(ndev);
    struct brcmf_cfg80211_profile *profile = &ifp->vif->profile;
    struct ieee80211_channel *chan = sme->channel;
    struct brcmf_pub *drvr = ifp->drvr;
    struct brcmf_join_params join_params;
    size_t join_params_size;
    const struct brcmf_tlv *rsn_ie;
    const struct brcmf_vs_tlv *wpa_ie;
    const void *ie;
    u32 ie_len;
    struct brcmf_ext_join_params_le *ext_join_params;
    u16 chanspec;
    s32 err = 0;
    u32 ssid_len;

    brcmf_dbg(TRACE, "Enter\n");
    if (!check_vif_up(ifp->vif))
        return -EIO;

    if (!sme->ssid) {
        bphy_err(drvr, "Invalid ssid\n");
        return -EOPNOTSUPP;
    }

    if (ifp->vif == cfg->p2p.bss_idx[P2PAPI_BSSCFG_PRIMARY].vif) {
        /* A normal (non P2P) connection request setup. */
        ie = NULL;
        ie_len = 0;
        /* find the WPA_IE */
        wpa_ie = brcmf_find_wpaie((u8 *)sme->ie, sme->ie_len);
        if (wpa_ie) {
            ie = wpa_ie;
            ie_len = wpa_ie->len + TLV_HDR_LEN;
        } else {
            /* find the RSN_IE */
            rsn_ie = brcmf_parse_tlvs((const u8 *)sme->ie,
                          sme->ie_len,
                          WLAN_EID_RSN);
            if (rsn_ie) {
                ie = rsn_ie;
                ie_len = rsn_ie->len + TLV_HDR_LEN;
            }
        }
        brcmf_fil_iovar_data_set(ifp, "wpaie", ie, ie_len);
    }

    err = brcmf_vif_set_mgmt_ie(ifp->vif, BRCMF_VNDR_IE_ASSOCREQ_FLAG,
                    sme->ie, sme->ie_len);
    if (err)
        bphy_err(drvr, "Set Assoc REQ IE Failed\n");
    else
        brcmf_dbg(TRACE, "Applied Vndr IEs for Assoc request\n");

    set_bit(BRCMF_VIF_STATUS_CONNECTING, &ifp->vif->sme_state);

    if (chan) {
        cfg->channel =
            ieee80211_frequency_to_channel(chan->center_freq);
        chanspec = channel_to_chanspec(&cfg->d11inf, chan);
        brcmf_dbg(CONN, "channel=%d, center_req=%d, chanspec=0x%04x\n",
              cfg->channel, chan->center_freq, chanspec);
    } else {
        cfg->channel = 0;
        chanspec = 0;
    }

    brcmf_dbg(INFO, "ie (%p), ie_len (%zd)\n", sme->ie, sme->ie_len);

    err = brcmf_set_wpa_version(ndev, sme);
    if (err) {
        bphy_err(drvr, "wl_set_wpa_version failed (%d)\n", err);
        goto done;
    }

    sme->auth_type = brcmf_war_auth_type(ifp, sme->auth_type);
    err = brcmf_set_auth_type(ndev, sme);
    if (err) {
        bphy_err(drvr, "wl_set_auth_type failed (%d)\n", err);
        goto done;
    }

    err = brcmf_set_wsec_mode(ndev, sme);
    if (err) {
        bphy_err(drvr, "wl_set_set_cipher failed (%d)\n", err);
        goto done;
    }

    err = brcmf_set_key_mgmt(ndev, sme);
    if (err) {
        bphy_err(drvr, "wl_set_key_mgmt failed (%d)\n", err);
        goto done;
    }

    err = brcmf_set_sharedkey(ndev, sme);
    if (err) {
        bphy_err(drvr, "brcmf_set_sharedkey failed (%d)\n", err);
        goto done;
    }

    if (sme->crypto.psk &&
        profile->use_fwsup != BRCMF_PROFILE_FWSUP_SAE) {
        if (WARN_ON(profile->use_fwsup != BRCMF_PROFILE_FWSUP_NONE)) {
            err = -EINVAL;
            goto done;
        }
        brcmf_dbg(INFO, "using PSK offload\n");
        profile->use_fwsup = BRCMF_PROFILE_FWSUP_PSK;
    }

    if (profile->use_fwsup != BRCMF_PROFILE_FWSUP_NONE) {
        /* enable firmware supplicant for this interface */
        err = brcmf_fil_iovar_int_set(ifp, "sup_wpa", 1);
        if (err < 0) {
            bphy_err(drvr, "failed to enable fw supplicant\n");
            goto done;
        }
    }

    if (profile->use_fwsup == BRCMF_PROFILE_FWSUP_PSK)
        err = brcmf_set_pmk(ifp, sme->crypto.psk,
                    BRCMF_WSEC_MAX_PSK_LEN);
    else if (profile->use_fwsup == BRCMF_PROFILE_FWSUP_SAE) {
        /* clean up user-space RSNE */
        err = brcmf_fil_iovar_data_set(ifp, "wpaie", NULL, 0);
        if (err) {
            bphy_err(drvr, "failed to clean up user-space RSNE\n");
            goto done;
        }
        err = brcmf_set_sae_password(ifp, sme->crypto.sae_pwd,
                         sme->crypto.sae_pwd_len);
        if (!err && sme->crypto.psk)
            err = brcmf_set_pmk(ifp, sme->crypto.psk,
                        BRCMF_WSEC_MAX_PSK_LEN);
    }
    if (err)
        goto done;

    /* Join with specific BSSID and cached SSID
     * If SSID is zero join based on BSSID only
     */
    join_params_size = offsetof(struct brcmf_ext_join_params_le, assoc_le) +
        offsetof(struct brcmf_assoc_params_le, chanspec_list);
    if (cfg->channel)
        join_params_size += sizeof(u16);
    ext_join_params = kzalloc(join_params_size, GFP_KERNEL);
    if (ext_join_params == NULL) {
        err = -ENOMEM;
        goto done;
    }
    ssid_len = min_t(u32, sme->ssid_len, IEEE80211_MAX_SSID_LEN);
    ext_join_params->ssid_le.SSID_len = cpu_to_le32(ssid_len);
    memcpy(&ext_join_params->ssid_le.SSID, sme->ssid, ssid_len);
    if (ssid_len < IEEE80211_MAX_SSID_LEN)
        brcmf_dbg(CONN, "SSID \"%s\", len (%d)\n",
              ext_join_params->ssid_le.SSID, ssid_len);

    /* Set up join scan parameters */
    ext_join_params->scan_le.scan_type = -1;
    ext_join_params->scan_le.home_time = cpu_to_le32(-1);

    if (sme->bssid)
        memcpy(&ext_join_params->assoc_le.bssid, sme->bssid, ETH_ALEN);
    else
        eth_broadcast_addr(ext_join_params->assoc_le.bssid);

    if (cfg->channel) {
        ext_join_params->assoc_le.chanspec_num = cpu_to_le32(1);

        ext_join_params->assoc_le.chanspec_list[0] =
            cpu_to_le16(chanspec);
        /* Increase dwell time to receive probe response or detect
         * beacon from target AP at a noisy air only during connect
         * command.
         */
        ext_join_params->scan_le.active_time =
            cpu_to_le32(BRCMF_SCAN_JOIN_ACTIVE_DWELL_TIME_MS);
        ext_join_params->scan_le.passive_time =
            cpu_to_le32(BRCMF_SCAN_JOIN_PASSIVE_DWELL_TIME_MS);
        /* To sync with presence period of VSDB GO send probe request
         * more frequently. Probe request will be stopped when it gets
         * probe response from target AP/GO.
         */
        ext_join_params->scan_le.nprobes =
            cpu_to_le32(BRCMF_SCAN_JOIN_ACTIVE_DWELL_TIME_MS /
                    BRCMF_SCAN_JOIN_PROBE_INTERVAL_MS);
    } else {
        ext_join_params->scan_le.active_time = cpu_to_le32(-1);
        ext_join_params->scan_le.passive_time = cpu_to_le32(-1);
        ext_join_params->scan_le.nprobes = cpu_to_le32(-1);
    }

    brcmf_set_join_pref(ifp, &sme->bss_select);

    err  = brcmf_fil_bsscfg_data_set(ifp, "join", ext_join_params,
                     join_params_size);
    kfree(ext_join_params);
    if (!err)
        /* This is it. join command worked, we are done */
        goto done;

    /* join command failed, fallback to set ssid */
    memset(&join_params, 0, sizeof(join_params));
    join_params_size = sizeof(join_params.ssid_le);

    memcpy(&join_params.ssid_le.SSID, sme->ssid, ssid_len);
    join_params.ssid_le.SSID_len = cpu_to_le32(ssid_len);

    if (sme->bssid)
        memcpy(join_params.params_le.bssid, sme->bssid, ETH_ALEN);
    else
        eth_broadcast_addr(join_params.params_le.bssid);

    if (cfg->channel) {
        join_params.params_le.chanspec_list[0] = cpu_to_le16(chanspec);
        join_params.params_le.chanspec_num = cpu_to_le32(1);
        join_params_size += sizeof(join_params.params_le);
    }
    err = brcmf_fil_cmd_data_set(ifp, BRCMF_C_SET_SSID,
                     &join_params, join_params_size);
    if (err)
        bphy_err(drvr, "BRCMF_C_SET_SSID failed (%d)\n", err);

done:
    if (err)
        clear_bit(BRCMF_VIF_STATUS_CONNECTING, &ifp->vif->sme_state);
    brcmf_dbg(TRACE, "Exit\n");
    return err;
}

static s32
brcmf_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *ndev,
               u16 reason_code)
{
    struct brcmf_cfg80211_info *cfg = wiphy_to_cfg(wiphy);
    struct brcmf_if *ifp = netdev_priv(ndev);
    struct brcmf_cfg80211_profile *profile = &ifp->vif->profile;
    struct brcmf_pub *drvr = cfg->pub;
    struct brcmf_scb_val_le scbval;
    s32 err = 0;

    brcmf_dbg(TRACE, "Enter. Reason code = %d\n", reason_code);
    if (!check_vif_up(ifp->vif))
        return -EIO;

    clear_bit(BRCMF_VIF_STATUS_CONNECTED, &ifp->vif->sme_state);
    clear_bit(BRCMF_VIF_STATUS_CONNECTING, &ifp->vif->sme_state);
    cfg80211_disconnected(ndev, reason_code, NULL, 0, true, GFP_KERNEL);

    memcpy(&scbval.ea, &profile->bssid, ETH_ALEN);
    scbval.val = cpu_to_le32(reason_code);
    err = brcmf_fil_cmd_data_set(ifp, BRCMF_C_DISASSOC,
                     &scbval, sizeof(scbval));
    if (err)
        bphy_err(drvr, "error (%d)\n", err);

    brcmf_dbg(TRACE, "Exit\n");
    return err;
}