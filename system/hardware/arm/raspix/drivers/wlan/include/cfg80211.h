// SPDX-License-Identifier: ISC
/*
 * Copyright (c) 2010 Broadcom Corporation
 */

#ifndef BRCMFMAC_CFG80211_H
#define BRCMFMAC_CFG80211_H

/* for brcmu_d11inf */
#include "types.h"

#include <netinet/ether.h>

#include "brcmu_d11.h"
#include "fwil_types.h"

//#include "core.h"
// #include "p2p.h"

#define BRCMF_SCAN_IE_LEN_MAX		2048

#define WL_NUM_SCAN_MAX			10
#define WL_TLV_INFO_MAX			1024
#define WL_BSS_INFO_MAX			2048
#define WL_ASSOC_INFO_MAX		512	/* assoc related fil max buf */
#define WL_EXTRA_BUF_MAX		2048
#define WL_ROAM_TRIGGER_LEVEL		-75
#define WL_ROAM_DELTA			20

/* WME Access Category Indices (ACIs) */
#define AC_BE			0	/* Best Effort */
#define AC_BK			1	/* Background */
#define AC_VI			2	/* Video */
#define AC_VO			3	/* Voice */
#define EDCF_AC_COUNT		4
#define MAX_8021D_PRIO		8

#define EDCF_ACI_MASK			0x60
#define EDCF_ACI_SHIFT			5
#define EDCF_ACM_MASK                  0x10
#define EDCF_ECWMIN_MASK		0x0f
#define EDCF_ECWMAX_SHIFT		4
#define EDCF_AIFSN_MASK			0x0f
#define EDCF_AIFSN_MAX			15
#define EDCF_ECWMAX_MASK		0xf0

/* Keep BRCMF_ESCAN_BUF_SIZE below 64K (65536). Allocing over 64K can be
 * problematic on some systems and should be avoided.
 */
#define BRCMF_ESCAN_BUF_SIZE		65000
#define BRCMF_ESCAN_TIMER_INTERVAL_MS	10000	/* E-Scan timeout */

#define WL_ESCAN_ACTION_START		1
#define WL_ESCAN_ACTION_CONTINUE	2
#define WL_ESCAN_ACTION_ABORT		3

#define WL_AUTH_SHARED_KEY		1	/* d11 shared authentication */
#define IE_MAX_LEN			512

/* IE TLV processing */
#define TLV_LEN_OFF			1	/* length offset */
#define TLV_HDR_LEN			2	/* header length */
#define TLV_BODY_OFF			2	/* body offset */
#define TLV_OUI_LEN			3	/* oui id length */

/* 802.11 Mgmt Packet flags */
#define BRCMF_VNDR_IE_BEACON_FLAG	0x1
#define BRCMF_VNDR_IE_PRBRSP_FLAG	0x2
#define BRCMF_VNDR_IE_ASSOCRSP_FLAG	0x4
#define BRCMF_VNDR_IE_AUTHRSP_FLAG	0x8
#define BRCMF_VNDR_IE_PRBREQ_FLAG	0x10
#define BRCMF_VNDR_IE_ASSOCREQ_FLAG	0x20
/* vendor IE in IW advertisement protocol ID field */
#define BRCMF_VNDR_IE_IWAPID_FLAG	0x40
/* allow custom IE id */
#define BRCMF_VNDR_IE_CUSTOM_FLAG	0x100

/* P2P Action Frames flags (spec ordered) */
#define BRCMF_VNDR_IE_GONREQ_FLAG     0x001000
#define BRCMF_VNDR_IE_GONRSP_FLAG     0x002000
#define BRCMF_VNDR_IE_GONCFM_FLAG     0x004000
#define BRCMF_VNDR_IE_INVREQ_FLAG     0x008000
#define BRCMF_VNDR_IE_INVRSP_FLAG     0x010000
#define BRCMF_VNDR_IE_DISREQ_FLAG     0x020000
#define BRCMF_VNDR_IE_DISRSP_FLAG     0x040000
#define BRCMF_VNDR_IE_PRDREQ_FLAG     0x080000
#define BRCMF_VNDR_IE_PRDRSP_FLAG     0x100000

#define BRCMF_VNDR_IE_P2PAF_SHIFT	12

#define BRCMF_MAX_DEFAULT_KEYS		6

/* beacon loss timeout defaults */
#define BRCMF_DEFAULT_BCN_TIMEOUT_ROAM_ON	2
#define BRCMF_DEFAULT_BCN_TIMEOUT_ROAM_OFF	4

#define BRCMF_VIF_EVENT_TIMEOUT		msecs_to_jiffies(1500)

/**
 * enum brcmf_scan_status - scan engine status
 *
 * @BRCMF_SCAN_STATUS_BUSY: scanning in progress on dongle.
 * @BRCMF_SCAN_STATUS_ABORT: scan being aborted on dongle.
 * @BRCMF_SCAN_STATUS_SUPPRESS: scanning is suppressed in driver.
 */
enum brcmf_scan_status {
	BRCMF_SCAN_STATUS_BUSY,
	BRCMF_SCAN_STATUS_ABORT,
	BRCMF_SCAN_STATUS_SUPPRESS,
};

/* dongle configuration */
struct brcmf_cfg80211_conf {
	u32 frag_threshold;
	u32 rts_threshold;
	u32 retry_short;
	u32 retry_long;
};

/* security information with currently associated ap */
struct brcmf_cfg80211_security {
	u32 wpa_versions;
	u32 auth_type;
	u32 cipher_pairwise;
	u32 cipher_group;
};

enum brcmf_profile_fwsup {
	BRCMF_PROFILE_FWSUP_NONE,
	BRCMF_PROFILE_FWSUP_PSK,
	BRCMF_PROFILE_FWSUP_1X,
	BRCMF_PROFILE_FWSUP_SAE
};

/**
 * enum brcmf_profile_fwauth - firmware authenticator profile
 *
 * @BRCMF_PROFILE_FWAUTH_NONE: no firmware authenticator
 * @BRCMF_PROFILE_FWAUTH_PSK: authenticator for WPA/WPA2-PSK
 * @BRCMF_PROFILE_FWAUTH_SAE: authenticator for SAE
 */
enum brcmf_profile_fwauth {
	BRCMF_PROFILE_FWAUTH_NONE,
	BRCMF_PROFILE_FWAUTH_PSK,
	BRCMF_PROFILE_FWAUTH_SAE
};

enum nl80211_band {
	NL80211_BAND_2GHZ,
	NL80211_BAND_5GHZ,
	NL80211_BAND_60GHZ,
	NL80211_BAND_6GHZ,
	NL80211_BAND_S1GHZ,
	NL80211_BAND_LC,

	NUM_NL80211_BANDS,
};

enum nl80211_dfs_state {
	NL80211_DFS_USABLE,
	NL80211_DFS_UNAVAILABLE,
	NL80211_DFS_AVAILABLE,
};

enum nl80211_auth_type {
	NL80211_AUTHTYPE_OPEN_SYSTEM,
	NL80211_AUTHTYPE_SHARED_KEY,
	NL80211_AUTHTYPE_FT,
	NL80211_AUTHTYPE_NETWORK_EAP,
	NL80211_AUTHTYPE_SAE,
	NL80211_AUTHTYPE_FILS_SK,
	NL80211_AUTHTYPE_FILS_SK_PFS,
	NL80211_AUTHTYPE_FILS_PK,

	/* keep last */
	__NL80211_AUTHTYPE_NUM,
	NL80211_AUTHTYPE_MAX = __NL80211_AUTHTYPE_NUM - 1,
	NL80211_AUTHTYPE_AUTOMATIC
};

enum nl80211_mfp {
	NL80211_MFP_NO,
	NL80211_MFP_REQUIRED,
	NL80211_MFP_OPTIONAL,
};

enum nl80211_sae_pwe_mechanism {
	NL80211_SAE_PWE_UNSPECIFIED,
	NL80211_SAE_PWE_HUNT_AND_PECK,
	NL80211_SAE_PWE_HASH_TO_ELEMENT,
	NL80211_SAE_PWE_BOTH,
};

enum nl80211_bss_select_attr {
	__NL80211_BSS_SELECT_ATTR_INVALID,
	NL80211_BSS_SELECT_ATTR_RSSI,
	NL80211_BSS_SELECT_ATTR_BAND_PREF,
	NL80211_BSS_SELECT_ATTR_RSSI_ADJUST,

	/* keep last */
	__NL80211_BSS_SELECT_ATTR_AFTER_LAST,
	NL80211_BSS_SELECT_ATTR_MAX = __NL80211_BSS_SELECT_ATTR_AFTER_LAST - 1
};

enum ieee80211_edmg_bw_config {
	IEEE80211_EDMG_BW_CONFIG_4	= 4,
	IEEE80211_EDMG_BW_CONFIG_5	= 5,
	IEEE80211_EDMG_BW_CONFIG_6	= 6,
	IEEE80211_EDMG_BW_CONFIG_7	= 7,
	IEEE80211_EDMG_BW_CONFIG_8	= 8,
	IEEE80211_EDMG_BW_CONFIG_9	= 9,
	IEEE80211_EDMG_BW_CONFIG_10	= 10,
	IEEE80211_EDMG_BW_CONFIG_11	= 11,
	IEEE80211_EDMG_BW_CONFIG_12	= 12,
	IEEE80211_EDMG_BW_CONFIG_13	= 13,
	IEEE80211_EDMG_BW_CONFIG_14	= 14,
	IEEE80211_EDMG_BW_CONFIG_15	= 15,
};

/**
 * struct brcmf_cfg80211_profile - profile information.
 *
 * @bssid: bssid of joined/joining ibss.
 * @sec: security information.
 * @key: key information
 */
struct brcmf_cfg80211_profile {
	u8 bssid[ETH_ALEN];
	struct brcmf_cfg80211_security sec;
	struct brcmf_wsec_key key[BRCMF_MAX_DEFAULT_KEYS];
	enum brcmf_profile_fwsup use_fwsup;
	u16 use_fwauth;
	bool is_ft;
};

/**
 * enum brcmf_vif_status - bit indices for vif status.
 *
 * @BRCMF_VIF_STATUS_READY: ready for operation.
 * @BRCMF_VIF_STATUS_CONNECTING: connect/join in progress.
 * @BRCMF_VIF_STATUS_CONNECTED: connected/joined successfully.
 * @BRCMF_VIF_STATUS_DISCONNECTING: disconnect/disable in progress.
 * @BRCMF_VIF_STATUS_AP_CREATED: AP operation started.
 * @BRCMF_VIF_STATUS_EAP_SUCCUSS: EAPOL handshake successful.
 * @BRCMF_VIF_STATUS_ASSOC_SUCCESS: successful SET_SSID received.
 */
enum brcmf_vif_status {
	BRCMF_VIF_STATUS_READY,
	BRCMF_VIF_STATUS_CONNECTING,
	BRCMF_VIF_STATUS_CONNECTED,
	BRCMF_VIF_STATUS_DISCONNECTING,
	BRCMF_VIF_STATUS_AP_CREATED,
	BRCMF_VIF_STATUS_EAP_SUCCESS,
	BRCMF_VIF_STATUS_ASSOC_SUCCESS,
};

struct ieee80211_channel {
	enum nl80211_band band;
	u32 center_freq;
	u16 freq_offset;
	u16 hw_value;
	u32 flags;
	int max_antenna_gain;
	int max_power;
	int max_reg_power;
	bool beacon_found;
	u32 orig_flags;
	int orig_mag, orig_mpwr;
	enum nl80211_dfs_state dfs_state;
	unsigned long dfs_state_entered;
	unsigned int dfs_cac_ms;
	s8 psd;
};

struct cfg80211_crypto_settings {
	u32 wpa_versions;
	u32 cipher_group;
	int n_ciphers_pairwise;
	u32 ciphers_pairwise[5];
	int n_akm_suites;
	u32 akm_suites[10];
	bool control_port;
	__be16 control_port_ethertype;
	bool control_port_no_encrypt;
	bool control_port_over_nl80211;
	bool control_port_no_preauth;
	const u8 *psk;
	const u8 *sae_pwd;
	u8 sae_pwd_len;
	enum nl80211_sae_pwe_mechanism sae_pwe;
};

struct ieee80211_mcs_info {
	u8 rx_mask[10];
	__le16 rx_highest;
	u8 tx_params;
	u8 reserved[3];
} __packed;

struct ieee80211_vht_mcs_info {
	__le16 rx_mcs_map;
	__le16 rx_highest;
	__le16 tx_mcs_map;
	__le16 tx_highest;
} __packed;

struct ieee80211_ht_cap {
	__le16 cap_info;
	u8 ampdu_params_info;

	/* 16 bytes MCS information */
	struct ieee80211_mcs_info mcs;

	__le16 extended_ht_cap_info;
	__le32 tx_BF_cap_info;
	u8 antenna_selection_info;
} __packed;

struct ieee80211_vht_cap {
	__le32 vht_cap_info;
	struct ieee80211_vht_mcs_info supp_mcs;
} __packed;

struct cfg80211_bss_select_adjust {
	enum nl80211_band band;
	s8 delta;
};

struct cfg80211_bss_selection {
	enum nl80211_bss_select_attr behaviour;
	union {
		enum nl80211_band band_pref;
		struct cfg80211_bss_select_adjust adjust;
	} param;
};

struct ieee80211_edmg {
	u8 channels;
	enum ieee80211_edmg_bw_config bw_config;
};

struct cfg80211_connect_params {
	struct ieee80211_channel *channel;
	struct ieee80211_channel *channel_hint;
	const u8 *bssid;
	const u8 *bssid_hint;
	const u8 *ssid;
	size_t ssid_len;
	enum nl80211_auth_type auth_type;
	const u8 *ie;
	size_t ie_len;
	bool privacy;
	enum nl80211_mfp mfp;
	struct cfg80211_crypto_settings crypto;
	const u8 *key;
	u8 key_len, key_idx;
	u32 flags;
	int bg_scan_period;
	struct ieee80211_ht_cap ht_capa;
	struct ieee80211_ht_cap ht_capa_mask;
	struct ieee80211_vht_cap vht_capa;
	struct ieee80211_vht_cap vht_capa_mask;
	bool pbss;
	struct cfg80211_bss_selection bss_select;
	const u8 *prev_bssid;
	const u8 *fils_erp_username;
	size_t fils_erp_username_len;
	const u8 *fils_erp_realm;
	size_t fils_erp_realm_len;
	u16 fils_erp_next_seq_num;
	const u8 *fils_erp_rrk;
	size_t fils_erp_rrk_len;
	bool want_1x;
	struct ieee80211_edmg edmg;
};

#endif /* BRCMFMAC_CFG80211_H */
