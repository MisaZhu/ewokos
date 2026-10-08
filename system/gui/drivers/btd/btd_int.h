/* btd_int.h - shared internal interface for the btd daemon.
   Carved from the former monolithic btd.c: every translation unit
   includes this for the common constants, types, shared globals
   (extern) and the cross-module function prototypes.
   The daemon is machine-neutral: all UART/pin/firmware access goes
   through the bsp_bt hooks (see system/gui/libs/bt), which each
   machine's libbsp implements (raspi5, raspix; no-op stubs elsewhere). */
#ifndef BTD_INT_H
#define BTD_INT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/kernel_tic.h>

#include <bt/bsp_bt.h>

/* the report-descriptor parser and the /dev/bt0 subscriber fan-out are
   shared with usbhostd, so a report decoded over HIDP (classic) or HOGP
   (GATT) produces the identical bytes for hid_keybd/hid_moused */
#include <hid/hid_defs.h>
#include <hid/hid_report.h>
#include <hid/hid_srv.h>


#define MAX_BT_DEVICES 32
#define MAX_BT_KNOWN 32
#define MAX_HCI_PAYLOAD 260
#define MAX_EVT_LINE 256
#define BT_CMD_RET_SZ 2048
#define BT_UART_PKT_FOLLOW_TIMEOUT_MS 120

/* persistent store of every device we connected (or paired) with, so a
   future adapter power-on can page them again without a manual scan */
#define BT_KNOWN_DIR "/etc/bt"
#define BT_KNOWN_FILE "/etc/bt/bt.json"
#define BT_KNOWN_MAX_FILE 16384

#define HCI_PKT_COMMAND 0x01
#define HCI_PKT_ACL 0x02
#define HCI_PKT_EVENT 0x04

#define HCI_OGF_LINK_CTRL 0x01
#define HCI_OGF_HOST_CTRL 0x03
#define HCI_OGF_INFO 0x04
#define HCI_OGF_STATUS 0x05
#define HCI_OGF_LE 0x08
#define HCI_OGF_VENDOR 0x3f

#define HCI_OCF_INQUIRY 0x0001
#define HCI_OCF_INQUIRY_CANCEL 0x0002
#define HCI_OCF_CREATE_CONN 0x0005
#define HCI_OCF_DISCONNECT 0x0006
#define HCI_OCF_ACCEPT_CONN_REQ 0x0009
#define HCI_OCF_REJECT_CONN_REQ 0x000a
#define HCI_OCF_LINK_KEY_REQ_REPLY 0x000b
#define HCI_OCF_LINK_KEY_REQ_NEG_REPLY 0x000c
#define HCI_OCF_PIN_CODE_REQ_REPLY 0x000d
#define HCI_OCF_PIN_CODE_REQ_NEG_REPLY 0x000e
#define HCI_OCF_AUTH_REQ 0x0011
#define HCI_OCF_SET_CONN_ENCRYPT 0x0013
#define HCI_OCF_REMOTE_NAME_REQ 0x0019
#define HCI_OCF_IO_CAPABILITY_REQ_REPLY 0x002b
#define HCI_OCF_USER_CONFIRM_REQ_REPLY 0x002c
#define HCI_OCF_USER_CONFIRM_REQ_NEG_REPLY 0x002d
#define HCI_OCF_USER_PASSKEY_REQ_REPLY 0x002e
#define HCI_OCF_USER_PASSKEY_REQ_NEG_REPLY 0x002f

#define HCI_OCF_SET_EVENT_MASK 0x0001
#define HCI_OCF_RESET 0x0003
#define HCI_OCF_READ_BUFFER_SIZE 0x0005
#define HCI_OCF_READ_LOCAL_VERSION 0x0001
#define HCI_OCF_WRITE_SCAN_ENABLE 0x001a
#define HCI_OCF_WRITE_AUTH_ENABLE 0x0020
/* declaring LE Host Support makes the controller escalate a peer's
   LL_CONNECTION_PARAM_REQ to the host (LE sub-event 0x06) instead of
   answering it autonomously */
#define HCI_OCF_WRITE_LE_HOST_SUPPORTED 0x006d
#define HCI_OCF_WRITE_INQUIRY_MODE 0x0045
#define HCI_OCF_WRITE_SIMPLE_PAIRING_MODE 0x0056

#define HCI_OCF_VENDOR_RESET_CHIP 0x0003
#define HCI_OCF_VENDOR_LOAD_FIRMWARE 0x002e

/* Status parameters (OGF 0x05): live link quality of a connected handle */
#define HCI_OCF_READ_RSSI 0x0005

/* LE controller commands (OGF 0x08) */
#define HCI_OCF_LE_SET_EVENT_MASK 0x0001
#define HCI_OCF_LE_READ_BUFFER_SIZE 0x0002
#define HCI_OCF_LE_SET_RANDOM_ADDRESS 0x0005
#define HCI_OCF_LE_SET_SCAN_PARAMS 0x000b
#define HCI_OCF_LE_SET_SCAN_ENABLE 0x000c
#define HCI_OCF_LE_CREATE_CONNECTION 0x000d
#define HCI_OCF_LE_CREATE_CONN_CANCEL 0x000e
#define HCI_OCF_LE_CONNECTION_UPDATE 0x0013
#define HCI_OCF_LE_RAND 0x0018
#define HCI_OCF_LE_START_ENCRYPTION 0x0019
#define HCI_OCF_LE_LTK_REQ_REPLY 0x001a
#define HCI_OCF_LE_LTK_REQ_NEG_REPLY 0x001b
#define HCI_OCF_LE_REMOTE_CONN_PARAM_REPLY 0x0020
#define HCI_OCF_LE_REMOTE_CONN_PARAM_NEG_REPLY 0x0021
/* LE Secure Connections: the controller does the P-256 ECDH. Read_Local_P256
   and Generate_DHKey both answer with a Command Complete (status only) and
   then deliver the result asynchronously via an LE Meta sub-event. */
#define HCI_OCF_LE_READ_LOCAL_P256 0x0025
#define HCI_OCF_LE_GENERATE_DHKEY 0x0026
/* LE privacy (resolving list): hand the controller a peer's IRK + identity
   address once and it maps that peer's rotating private address back to the
   stable identity address in every advertising report and connection
   complete, so one device stops looking like a new one per rotation. */
#define HCI_OCF_LE_ADD_DEV_RESOLV_LIST 0x0027
#define HCI_OCF_LE_REMOVE_DEV_RESOLV_LIST 0x0028
#define HCI_OCF_LE_CLEAR_RESOLV_LIST 0x0029
#define HCI_OCF_LE_READ_RESOLV_LIST_SIZE 0x002a
#define HCI_OCF_LE_SET_ADDR_RESOLUTION_ENABLE 0x002d
/* LE 5.0 extended scanning / initiating. A BLE 5.0 peripheral (many
   keyboards) advertises with ADV_EXT_IND, which a legacy LE_Set_Scan_Enable
   scan physically cannot receive: the controller only reports those PDUs to
   an extended scan, as an LE_Extended_Advertising_Report (subevent 0x0d).
   Extended scanning also receives legacy ADV_IND (reported through the same
   0x0d event with the "legacy PDU" bit set), so it supersedes the legacy
   path on any controller that supports it. Connecting to an extended-only
   advertiser likewise needs LE_Extended_Create_Connection. */
#define HCI_OCF_LE_SET_EXT_SCAN_PARAMS 0x0041
#define HCI_OCF_LE_SET_EXT_SCAN_ENABLE 0x0042
#define HCI_OCF_LE_EXT_CREATE_CONNECTION 0x0043

/* host-controller commands we need for the LE path (OGF 0x03) */
#define HCI_OCF_READ_BD_ADDR 0x0009

#define HCI_OPCODE(ogf, ocf) (uint16_t)((((ogf) & 0x3f) << 10) | ((ocf) & 0x03ff))

#define EVT_INQUIRY_COMPLETE 0x01
#define EVT_INQUIRY_RESULT 0x02
#define EVT_CONN_COMPLETE 0x03
#define EVT_CONN_REQUEST 0x04
#define EVT_DISCONN_COMPLETE 0x05
#define EVT_AUTH_COMPLETE 0x06
#define EVT_REMOTE_NAME_COMPLETE 0x07
#define EVT_CMD_COMPLETE 0x0e
#define EVT_CMD_STATUS 0x0f
#define EVT_HARDWARE_ERROR 0x10
#define EVT_PIN_CODE_REQUEST 0x16
#define EVT_LINK_KEY_REQUEST 0x17
#define EVT_LINK_KEY_NOTIFY 0x18
#define EVT_INQUIRY_RESULT_RSSI 0x22
#define EVT_IO_CAPABILITY_REQUEST 0x31
#define EVT_USER_CONFIRMATION_REQUEST 0x33
#define EVT_USER_PASSKEY_REQUEST 0x34
#define EVT_SIMPLE_PAIRING_COMPLETE 0x36
#define EVT_EXTENDED_INQUIRY_RESULT 0x2f
#define EVT_NUM_COMPLETED_PKTS 0x13
#define EVT_ENCRYPTION_CHANGE 0x08
/* every LE controller event is wrapped in this one; the first payload
   byte is the sub-event code */
#define EVT_LE_META 0x3e

/* LE meta sub-event codes */
#define LE_EVT_CONN_COMPLETE 0x01
#define LE_EVT_ADV_REPORT 0x02
#define LE_EVT_CONN_UPDATE 0x03
#define LE_EVT_LTK_REQUEST 0x05
#define LE_EVT_REMOTE_CONN_PARAM_REQ 0x06
#define LE_EVT_REMOTE_FEATURES 0x04
#define LE_EVT_READ_LOCAL_P256 0x08
#define LE_EVT_GENERATE_DHKEY 0x09
#define LE_EVT_ENHANCED_CONN_COMPLETE 0x0a
#define LE_EVT_EXT_ADV_REPORT 0x0d

/*
 * Standard Bluetooth mouse (HID over Bluetooth): after the ACL link is up
 * and authenticated, the host opens two L2CAP channels - HID Control on
 * PSM 0x0011 and HID Interrupt on PSM 0x0013 - sends SET_PROTOCOL(boot)
 * on the control channel and receives boot mouse reports
 * ([buttons, dx, dy, (wheel)]) as HIDP DATA|INPUT on the interrupt one.
 * A reconnecting mouse opens both channels itself, so the L2CAP layer
 * accepts inbound connection requests for the HID PSMs as well.
 */
#define L2CAP_CID_SIGNAL 0x0001

#define L2CAP_SIG_CMD_REJECT 0x01
#define L2CAP_SIG_CONN_REQ 0x02
#define L2CAP_SIG_CONN_RSP 0x03
#define L2CAP_SIG_CONF_REQ 0x04
#define L2CAP_SIG_CONF_RSP 0x05
#define L2CAP_SIG_DISCONN_REQ 0x06
#define L2CAP_SIG_DISCONN_RSP 0x07
#define L2CAP_SIG_ECHO_REQ 0x08
#define L2CAP_SIG_ECHO_RSP 0x09
#define L2CAP_SIG_INFO_REQ 0x0a
#define L2CAP_SIG_INFO_RSP 0x0b
#define L2CAP_INFO_EXT_FEATURES 0x0002
#define L2CAP_INFO_FIXED_CHANNELS 0x0003

#define L2CAP_CONN_SUCCESS 0x0000
#define L2CAP_CONN_PENDING 0x0001
#define L2CAP_CONN_PSM_UNSUPPORTED 0x0002
#define L2CAP_CONF_SUCCESS 0x0000
#define L2CAP_CONF_PENDING 0x0004

#define L2CAP_PSM_HID_CTRL 0x0011
#define L2CAP_PSM_HID_INTR 0x0013

#define L2CAP_MTU_DEFAULT 672

/* l2cap_chan_t.state */
#define L2CAP_STATE_CLOSED 0
#define L2CAP_STATE_CONN_REQ_SENT 1
#define L2CAP_STATE_CONF_SENT 2
#define L2CAP_STATE_OPEN 3
#define L2CAP_STATE_CLOSING 4

#define L2CAP_STEP_TIMEOUT_MS 2000
#define L2CAP_STEP_MAX_RETRIES 3

/*
 * Bluetooth LE hosts a different L2CAP flavour: the HID traffic does not
 * ride a PSM but the ATT fixed channel, and pairing rides the SMP fixed
 * channel. Both exist as soon as the LE link is up - there is no
 * connection/configuration handshake to run, and the peer's MTU is the
 * spec default until ATT Exchange MTU says otherwise.
 */
#define L2CAP_CID_ATT 0x0004
#define L2CAP_CID_LE_SIGNAL 0x0005
#define L2CAP_CID_SMP 0x0006

#define L2CAP_SIG_LE_CONN_PARAM_UPDATE_REQ 0x12
#define L2CAP_SIG_LE_CONN_PARAM_UPDATE_RSP 0x13

#define L2CAP_LE_MTU_DEFAULT 23
#define L2CAP_LE_MPS_DEFAULT 23

/* ATT opcodes (Vol 3 Part F 3.4) */
#define ATT_OP_ERROR_RSP 0x01
#define ATT_OP_MTU_REQ 0x02
#define ATT_OP_MTU_RSP 0x03
#define ATT_OP_FIND_INFO_REQ 0x04
#define ATT_OP_FIND_INFO_RSP 0x05
#define ATT_OP_FIND_BY_TYPE_VALUE_REQ 0x06
#define ATT_OP_FIND_BY_TYPE_VALUE_RSP 0x07
#define ATT_OP_READ_BY_TYPE_REQ 0x08
#define ATT_OP_READ_BY_TYPE_RSP 0x09
#define ATT_OP_READ_REQ 0x0a
#define ATT_OP_READ_RSP 0x0b
#define ATT_OP_READ_BLOB_REQ 0x0c
#define ATT_OP_READ_BLOB_RSP 0x0d
#define ATT_OP_READ_BY_GROUP_REQ 0x10
#define ATT_OP_READ_BY_GROUP_RSP 0x11
#define ATT_OP_WRITE_REQ 0x12
#define ATT_OP_WRITE_RSP 0x13
#define ATT_OP_HANDLE_NOTIFY 0x1b
#define ATT_OP_HANDLE_IND 0x1d
#define ATT_OP_IND_CONFIRM 0x1e
#define ATT_OP_WRITE_CMD 0x52

#define ATT_ERR_INVALID_HANDLE 0x01
#define ATT_ERR_ATTR_NOT_FOUND 0x0a
#define ATT_ERR_INSUFFICIENT_AUTHENTICATION 0x05
#define ATT_ERR_INSUFFICIENT_ENCRYPTION 0x0f

/* GATT attribute / service / characteristic UUIDs (16-bit form) */
#define GATT_UUID_PRIMARY_SERVICE 0x2800
#define GATT_UUID_INCLUDE 0x2802
#define GATT_UUID_CHARACTERISTIC 0x2803
#define GATT_UUID_CLIENT_CHAR_CFG 0x2902
#define GATT_UUID_EXT_REPORT_REF 0x2907
#define GATT_UUID_REPORT_REFERENCE 0x2908

#define GATT_SVC_HID 0x1812
#define GATT_CHR_BOOT_KBD_INPUT 0x2a22
#define GATT_CHR_BOOT_KBD_OUTPUT 0x2a32
#define GATT_CHR_BOOT_MOUSE_INPUT 0x2a33
#define GATT_CHR_HID_INFORMATION 0x2a4a
#define GATT_CHR_REPORT_MAP 0x2a4b
#define GATT_CHR_HID_CONTROL_POINT 0x2a4c
#define GATT_CHR_REPORT 0x2a4d
#define GATT_CHR_PROTOCOL_MODE 0x2a4e

/* characteristic declaration value: [props, value handle, uuid] */
#define GATT_CHR_PROP_READ 0x02
#define GATT_CHR_PROP_WRITE_NR 0x04
#define GATT_CHR_PROP_WRITE 0x08
#define GATT_CHR_PROP_NOTIFY 0x10
#define GATT_CHR_PROP_INDICATE 0x20

#define GATT_CCCD_NOTIFY 0x0001
#define GATT_CCCD_INDICATE 0x0002

#define GATT_PROTOCOL_MODE_BOOT 0x00
#define GATT_PROTOCOL_MODE_REPORT 0x01
#define GATT_HID_CTRL_SUSPEND 0x00
#define GATT_HID_CTRL_EXIT_SUSPEND 0x01

/* SMP over the fixed CID 0x0006 (Vol 3 Part H) */
#define SMP_CMD_PAIRING_REQUEST 0x01
#define SMP_CMD_PAIRING_RESPONSE 0x02
#define SMP_CMD_PAIRING_CONFIRM 0x03
#define SMP_CMD_PAIRING_RANDOM 0x04
#define SMP_CMD_PAIRING_FAILED 0x05
#define SMP_CMD_ENCRYPTION_INFO 0x06
#define SMP_CMD_MASTER_IDENT 0x07
#define SMP_CMD_IDENTITY_INFO 0x08
#define SMP_CMD_IDENTITY_ADDR_INFO 0x09
#define SMP_CMD_SIGNING_INFO 0x0a
#define SMP_CMD_SECURITY_REQUEST 0x0b
/* LE Secure Connections phase-2 PDUs (Vol 3 Part H 3.5) */
#define SMP_CMD_PAIRING_PUBLIC_KEY 0x0c
#define SMP_CMD_PAIRING_DHKEY_CHECK 0x0d
#define SMP_CMD_PAIRING_KEYPRESS 0x0e

#define SMP_IO_NO_INPUT_NO_OUTPUT 0x03

#define SMP_AUTHREQ_BONDING 0x01
#define SMP_AUTHREQ_MITM 0x04
#define SMP_AUTHREQ_SC 0x08
#define SMP_AUTHREQ_CT2 0x20

#define SMP_DIST_ENCKEY 0x01
#define SMP_DIST_IDKEY 0x02

#define SMP_REASON_CONFIRM_VALUE_FAILED 0x04
#define SMP_REASON_PAIRING_NOT_SUPPORTED 0x05
#define SMP_REASON_CMD_NOT_SUPPORTED 0x07
#define SMP_REASON_UNSPECIFIED 0x08
#define SMP_REASON_DHKEY_CHECK_FAILED 0x0b
#define SMP_REASON_TIMEOUT 0x0c

/* GAP AD structures inside an advertising / scan-response payload */
#define AD_TYPE_FLAGS 0x01
#define AD_TYPE_UUID16_INCOMPLETE 0x02
#define AD_TYPE_UUID16_COMPLETE 0x03
#define AD_TYPE_UUID128_INCOMPLETE 0x06
#define AD_TYPE_UUID128_COMPLETE 0x07
#define AD_TYPE_NAME_SHORT 0x08
#define AD_TYPE_NAME_COMPLETE 0x09
#define AD_TYPE_TX_POWER 0x0a
#define AD_TYPE_APPEARANCE 0x19
#define AD_TYPE_MFG_SPECIFIC 0xff

/* GAP appearance category (top 10 bits) 15 == HID; the sub-categories we
   care about are 961 keyboard, 962 mouse, 963 joystick, 964 gamepad */
#define AD_APPEARANCE_CATEGORY_HID 15

/*
 * LE discovery and classic inquiry never overlap: some Broadcom
 * firmwares answer LE_Set_Scan_Enable with Command Disallowed (0x0c)
 * while an inquiry is in flight and vice versa, so bt_loop runs them as
 * alternating slices. 100% scan duty (window == interval) keeps
 * discovery latency low - a BLE mouse only advertises for ~1s after it
 * wakes up, so missing an interval means missing the device.
 */
#define BT_LE_SCAN_SLICE_MS 2500
#define BT_CLASSIC_SCAN_SLICE_MS 2500
#define BT_LE_SCAN_INTERVAL 0x0030 /* 30 * 0.625ms */
#define BT_LE_SCAN_WINDOW 0x0030
#define BT_LE_SCAN_TYPE_ACTIVE 0x01 /* request SCAN_RSP, which carries the full name */
#define BT_LE_ADDR_TYPE_PUBLIC 0x00
#define BT_LE_ADDR_TYPE_RANDOM 0x01

/* Initial link establishment window. Every mainstream host (BlueZ, Android,
   iOS, Windows - i.e. every host an Xbox pad's firmware is validated
   against) creates LE connections at 30-50ms and only tightens later if
   needed. The old 7.5-15ms request was an outlier: it polls the peripheral
   up to 4x more often than it ever sees in the field, and on the Pi's
   shared WiFi/BT radio it multiplies the coex slots a BLE link can starve
   on while ssh/DHCP traffic runs. Peripherals that want a different pace
   ask for it themselves - both request paths (LE sub-event 0x06 and L2CAP
   0x12) are answered permissively. */
#define BT_LE_CONN_ITV_INIT_MIN 0x0018 /* 30ms */
#define BT_LE_CONN_ITV_INIT_MAX 0x0028 /* 50ms */

/* the fast window conn_update_fast pulls an input device into after READY.
   The old 30ms ceiling let a mouse settle at ~33Hz, which feels sluggish
   under fast motion; 15ms (66Hz) is the responsiveness floor for a pointing
   device and is well within what any BLE mouse sustains while awake. */
#define BT_LE_CONN_ITV_MIN 0x0006 /* 7.5ms */
#define BT_LE_CONN_ITV_MAX 0x000c /* 15ms */
#define BT_LE_CONN_LATENCY 0x0000
/* Supervision timeout: 10s instead of the BLE-minimum 5s. Xbox controllers
   need several seconds after SMP before they answer the first ATT request;
   with 5s the link dies at DISCOVERING (reason=0x08) before the MTU
   exchange even times out. 10s still detects a genuinely dead peer quickly
   enough for the UI. */
#define BT_LE_CONN_TIMEOUT 0x03e8 /* 10s */
#define BT_LE_CONN_CE_LEN 0x0000

/* Input-responsiveness floor for a peripheral's LE Connection Parameter
   Update Request. An idle mouse asks to stretch the live link to a long
   power-saving interval (typically 30-50ms => 20-33Hz), which is exactly the
   sluggish-cursor symptom; bt_le_l2cap_rx rejects any request whose fastest
   acceptable interval (interval_min) is slower than this, so the streaming
   input link stays at the fast interval bt_le_connect negotiated. Input
   latency outranks the peripheral's power preference while it is in use. */
#define BT_LE_INPUT_ITV_MAX 0x000c /* 15ms */

/* Ceiling on the LE bring-up's first phase (LE_Create_Connection ->
   LE_(Enhanced_)Connection_Complete). A peripheral that is actually in
   pairing mode answers CONNECT_IND within a couple of seconds; anything
   longer means the user has not put the peer into discoverable/pairing
   state yet, and holding the radio in initiating for the full window
   blocks every other client of /dev/bt0 while xbt's "connect" click feels
   dead. 8s is long enough to survive a slow advertiser's first connection
   event but short enough that a mistimed click retries quickly. */
#define BT_LE_CONNECT_TIMEOUT_MS 8000
#define BT_LE_ATT_TIMEOUT_MS 6000
#define BT_LE_SMP_TIMEOUT_MS 15000

/* our ATT client RX capability, offered in Exchange MTU. The reassembly
   buffer holds a 276-octet L2CAP PDU and l2cap_send_pdu 256 octets of
   payload, so 247 (the practical maximum) fits both ways. */
#define BT_LE_ATT_MTU_PREFERRED 247

/* a bonded LE peripheral only reconnects while it advertises, so the
   power-on autoconnect runs a bounded scan session and lets the first
   advertisement from a known device trigger the bring-up */
#define BT_LE_AUTOCONNECT_SCAN_S 10

/* HOGP fallback path: the Report Map is read in ATT_MTU-sized chunks */
#define BT_MAX_REPORT_MAP 512
#define BT_HOGP_MAX_REPORTS 4
#define BT_MAX_HID_ATTRS 24

/* subscriber fan-out comes from libhid (hid/hid_srv.h) and is identical to
   usbhostd's /dev/hid0: fcntl cmd 0 selects the report id, then read()
   pops fixed-size events from the per-fd queue. Report id 0 stays btd's
   own command/event text stream. */
#define BT_HID_REASSERT_MS 30

/* Idle polling backoff. The HCI UART runs in polled mode (IER=0, no IRQ),
   so an input report sits in the RX FIFO until the next bt_poll_once: the
   sleep ceiling is a hard floor on cursor latency. While a HID link is
   live, do NOT back off at all - stay at the 2ms floor so a report is
   picked up almost immediately (a BT mouse only reports on movement, so
   the extra polls are cheap LSR reads). When nothing is connected the
   daemon is only scanning, and polling may relax to the full ceiling. */
#define BT_IDLE_SLEEP_MIN_US 2000u
#define BT_IDLE_SLEEP_HID_US 2000u
#define BT_IDLE_SLEEP_MAX_US 20000u

/* HIDP transaction headers (high nibble = message type) */
#define HIDP_TRANS_HANDSHAKE 0x00
#define HIDP_TRANS_HID_CONTROL 0x10
#define HIDP_TRANS_SET_PROTOCOL 0x70
#define HIDP_TRANS_DATA 0xA0

#define HIDP_HANDSHAKE_SUCCESS 0x00
#define HIDP_HID_CONTROL_VC_UNPLUG 0x15
#define HIDP_DATA_INPUT 0xA1 /* DATA | input report */

#define HIDP_PROTOCOL_BOOT 0x00

typedef struct {
    bool used;
    uint8_t addr[6];
    uint32_t class_of_device;
    uint16_t clock_offset;
    uint8_t page_scan_rep_mode;
    int8_t rssi;
    uint16_t handle;
    bool connected;
    /* classic (BR/EDR) HID: the L2CAP HID channels are deferred until the
       link is authenticated AND encrypted, because an input peripheral will
       not send reports over a plain link. Set on connect, cleared when the
       security sequence finishes and bt_hid_start runs. */
    bool hid_after_sec;
    bool has_link_key;
    uint8_t link_key[16];
    char name[64];
    /* LE side. A dual-mode device is one entry carrying both flags; a
       BLE-only HID peripheral (every modern mouse/keyboard) has
       classic == false and is invisible to a BR/EDR inquiry.
       addr_type is part of the identity: the same six octets mean a
       public address or a random one, and LE_Create_Connection takes
       both. */
    bool le;
    bool classic;
    uint8_t addr_type;
    uint16_t appearance;   /* GAP appearance, 0 when not advertised */
    bool adv_hid;          /* advertised the HID Service or a HID appearance */
    /* set once we have seen a non-legacy (BLE 5.0 extended) advertising PDU
       from this peer: it can only be reached with LE_Extended_Create_
       Connection, so the connect path branches on it */
    bool ext_adv;
    uint64_t last_seen_ms; /* LE entries are evicted oldest-first when full */
    /* LE long-term key. A bonded peripheral re-encrypts with this instead
       of running SMP again. */
    bool has_ltk;
    uint8_t ltk[16];
    uint16_t ediv;
    uint8_t ltk_rand[8];
    /* LE privacy: the peer's IRK and its stable identity address, learned in
       SMP phase 3 when it honours the IdKey request. Held in the controller's
       resolving list they collapse a rotating private address to id_addr, so
       the device keeps ONE entry (and ONE bond, keyed by id_addr) across
       reconnects instead of a fresh pair per address rotation. */
    bool has_irk;
    uint8_t irk[16];
    bool has_id_addr;
    uint8_t id_addr[6];
    uint8_t id_addr_type;
} bt_device_t;

typedef enum {
    BT_PENDING_NONE = 0,
    BT_PENDING_CONNECT,
    BT_PENDING_PAIR
} bt_pending_type_t;

typedef struct {
    bt_pending_type_t type;
    uint8_t addr[6];
    char pin[17];
    uint16_t handle;
} bt_pending_t;

typedef struct {
    bool active;
    bool done;
    uint16_t opcode;
    int status;
    /* command-complete return parameters (everything after the status
       byte), captured for the synchronous commands that need them */
    bool got_ret;
    uint8_t ret[64];
    uint8_t ret_len;
} bt_wait_cmd_t;

typedef struct {
    uint32_t packets_seen;
    uint32_t event_packets;
    uint32_t acl_packets;
    uint32_t other_packets;
    uint8_t last_pkt_type;
    uint8_t last_event_code;
    uint8_t last_event_len;
    uint16_t last_opcode;
    int last_status;
} bt_wait_debug_t;

typedef struct {
    bool used;
    uint8_t addr[6];
    bool paired;
    bool has_key;
    uint8_t key[16];
    char name[64];
    /* LE bonds survive a reboot the same way classic link keys do */
    bool le;
    uint8_t addr_type;
    bool has_ltk;
    uint8_t ltk[16];
    uint16_t ediv;
    uint8_t ltk_rand[8];
    /* LE privacy: IRK + identity address, persisted so the resolving list is
       rebuilt at boot and a rotating-address peer keeps a single bond */
    bool has_irk;
    uint8_t irk[16];
    bool has_id_addr;
    uint8_t id_addr[6];
    uint8_t id_addr_type;
} bt_known_t;

/* one L2CAP connection-oriented channel (HID control or interrupt) */
typedef struct {
    bool used;
    bool incoming;       /* peer initiated this channel, independent of ACL role */
    uint16_t acl_handle;
    uint16_t psm;
    uint16_t local_cid;   /* our dynamic source cid (0x0040+) */
    uint16_t remote_cid;  /* 0 until the peer tells us */
    uint8_t state;        /* L2CAP_STATE_* */
    bool conf_req_sent;
    bool conf_rsp_recv;
    bool conf_req_recv;   /* peer asked us to configure its side */
    bool conf_rsp_sent;
    uint8_t sig_id;       /* identifier of the outstanding request */
    uint8_t retries;
    uint64_t retry_ms;
} l2cap_chan_t;

#define MAX_L2CAP_CHANS 4

/* HID host state for one ACL link (control + interrupt channel pair) */
typedef struct {
    bool active;
    uint16_t acl_handle;
    uint8_t addr[6];
    l2cap_chan_t* ctrl;
    l2cap_chan_t* intr;
    bool boot_protocol_pending;
    bool boot_protocol_ok; /* SET_PROTOCOL(boot) handshake succeeded */
    /* Class-of-Device says joystick/gamepad: reports are full-length
       report-protocol frames that must NOT go through the boot
       keyboard/mouse length heuristic, and SET_PROTOCOL(boot) is skipped
       (boot protocol is only defined for keyboards and mice). */
    bool is_gamepad;
    bool up;               /* both channels open: reports flow */
    uint64_t intr_wait_ms;  /* bounded wait for a peer-initiated interrupt channel */
} bt_hid_chan_t;

/* one /dev/bt0 subscriber fd lives in libhid now (fd_info_t): the queue,
   the report-id selection and the directed wakes are identical to
   usbhostd's /dev/hid0, so they are shared rather than duplicated. */

/* ---------------- Bluetooth LE (HOGP) state ----------------
   One LE link at a time: a keyboard or mouse is connected, brought up
   and left alone until it drops, and only then does the next candidate
   get a turn. The bring-up itself is a bounded blocking sequence in the
   same style as the classic pairing path - every wait is on an
   asynchronous controller event and every wait has a deadline. */
typedef enum {
    LE_ST_IDLE = 0,
    LE_ST_CONNECTING,  /* LE_Create_Connection sent */
    LE_ST_LINK_UP,     /* LE Connection Complete seen, link not encrypted */
    LE_ST_PAIRING,     /* SMP phase 1/2 in flight */
    LE_ST_ENCRYPTING,  /* LE_Start_Encryption sent */
    LE_ST_DISCOVERING, /* GATT discovery / CCCD writes in flight */
    LE_ST_READY,       /* input reports subscribed and flowing */
    LE_ST_FAILED
} le_state_t;

typedef struct {
    le_state_t state;
    uint8_t addr[6];
    uint8_t addr_type;
    uint16_t handle;
    bool handle_valid;
    bool encrypted;
    uint64_t deadline_ms;
} le_link_t;

/* ATT client on the fixed CID 0x0004. Exactly one request is outstanding
   at a time - the response, error response or deadline ends the wait. */
typedef struct {
    bool busy;
    uint8_t req_opcode;
    bool rsp_ready;
    uint8_t rsp_opcode;
    uint8_t rsp[256];
    uint16_t rsp_len;
    bool err;
    uint8_t err_code;
    uint16_t err_handle;
} att_client_t;

/* LE Security Manager on the fixed CID 0x0006. NoInputNoOutput on both
   sides makes Just Works the negotiated association model. The Pairing
   Request advertises the SC bit whenever the controller handed us a local
   P-256 public key, so a Secure-Connections-capable peripheral (an Xbox
   pad mandates it) negotiates the LESC flow in smp_run_lesc; a peer that
   clears SC in its response falls back to the legacy c1/s1 exchange. */
typedef struct {
    bool active;
    uint8_t preq[7];
    uint8_t pres[7];
    uint8_t mrand[16];   /* our random, over-the-air (little-endian) order */
    uint8_t srand[16];   /* the peer's random, same order */
    uint8_t mconfirm[16];
    uint8_t sconfirm[16];
    uint8_t ltk[16];     /* the LTK we generate and distribute */
    uint16_t ediv;
    uint8_t ltk_rand[8];
    uint8_t peer_ltk[16];
    uint16_t peer_ediv;
    uint8_t peer_rand[8];
    bool got_pres;
    bool got_sconfirm;
    bool got_srand;
    bool got_peer_ltk;
    bool got_peer_ident;
    uint8_t peer_irk[16];       /* Identity Information (IRK) */
    uint8_t peer_id_addr[6];    /* Identity Address Information */
    uint8_t peer_id_addr_type;
    bool got_peer_irk;
    bool got_peer_id_addr;
    bool enc_changed;
    bool failed;
    uint8_t fail_reason;
    /* a peripheral usually asks for pairing itself right after the link
       comes up; honour that instead of racing it with our own request */
    bool security_request_seen;
    uint8_t security_request_auth;
    /* ---- LE Secure Connections (LESC) ---- */
    bool sc_local;              /* our Pairing Request offered the SC bit */
    uint8_t peer_pub[64];       /* peer P-256 public key, air order (Qx||Qy) */
    bool got_peer_pub;
    uint8_t peer_dhkey_check[16];
    bool got_dhkey_check;
} smp_state_t;

/* one discovered characteristic of the HID Service */
typedef struct {
    uint16_t uuid;
    uint8_t props;
    uint16_t decl_handle;  /* the 0x2803 declaration: bounds the descriptor walk */
    uint16_t value_handle;
    uint16_t cccd_handle; /* 0 when it has no Client Characteristic Configuration */
    bool has_report_ref;
    uint8_t report_id;    /* from the Report Reference descriptor */
    uint8_t report_type;  /* 1 = input, 2 = output, 3 = feature */
} hogp_attr_t;

typedef struct {
    bool active;
    uint16_t svc_start;
    uint16_t svc_end;
    uint16_t mtu;
    int n_attrs;
    hogp_attr_t attrs[BT_MAX_HID_ATTRS];
    uint8_t report_map[BT_MAX_REPORT_MAP];
    uint16_t report_map_len;
    bool report_map_complete;
    bool boot_mode_ok;   /* Protocol Mode accepted GATT_PROTOCOL_MODE_BOOT */
    mouse_parser_t mouse;
    bool mouse_ok;       /* Report Map yielded a usable mouse bit layout */
    /* Report ID of the keyboard application collection in the Report Map
       (0 when the keyboard collection carries no Report ID, i.e. plain boot
       layout). A composite HOGP peripheral puts mouse and keyboard reports on
       separate Report characteristics that differ only by this ID, and a
       keyboard body shorter than 8 octets cannot be told from a mouse by
       length - so routing must key off this, exactly like usbhostd does. */
    uint8_t kbd_report_id;
    int n_subscribed;
    /* A LE gamepad (an Xbox/8BitDo pad) exposes a Generic Desktop Joystick
       or Game Pad collection in its Report Map rather than a mouse or
       keyboard. Detect it here so the notify path routes its Report
       characteristic to the joystick dispatch instead of the boot
       keyboard/mouse length heuristic, and hand the parser to libhid's
       descriptor-driven normalizer. */
    bool is_gamepad;
    joystick_parser_t joystick;
    bool joystick_ok;
    uint8_t joystick_report_id;
} hogp_state_t;

/* LE link, ATT/GATT client, SMP and HID-over-GATT state.

   The BCM43455 controller supports several simultaneous LE connections, so
   these are bundled into a session struct and kept in a small array rather
   than as singletons: one BLE keyboard and one BLE mouse can then be live at
   the same time. The macro shim below keeps every existing _le.xxx /
   _att.xxx / _smp.xxx / _hogp.xxx reference working, resolving them against
   the session currently being set up (_le_cur). Only the routing logic and
   the step/auto-connect machinery need to address sessions explicitly. */
#define MAX_LE_SESSIONS 2

typedef struct {
    le_link_t    le;
    att_client_t att;
    smp_state_t  smp;
    hogp_state_t hogp;
} le_session_t;

/* discovery slicing: _scanning says "a scan session is running",
   _scan_slice says which radio mode currently owns the controller */
typedef enum {
    BT_SCAN_SLICE_NONE = 0,
    BT_SCAN_SLICE_LE,
    BT_SCAN_SLICE_CLASSIC
} bt_scan_slice_t;

/* ---------- bounded waits ----------
   Every LE bring-up step ends in "wait for the controller or the peer to
   answer", and all of them go through here so the deadline is real and
   bt_poll_once keeps the classic paths alive in between. */
typedef bool (*bt_pred_fn)(void* ctx);

/* ---- shared globals (each defined once in its owning module) ---- */
/* btd.c */
extern vdevice_t* _bt_dev;
extern charbuf_t* _evt_buf;
extern uint32_t _idle_sleep_us;
extern bool _ready;
extern bool _powered;
extern uint16_t _sec_encrypt_handle;
extern bt_wait_cmd_t _wait_cmd;
extern bt_wait_debug_t _wait_debug;

/* btd_known.c */
extern bt_device_t _devices[MAX_BT_DEVICES];
extern bt_known_t _known[MAX_BT_KNOWN];
extern bt_pending_t _pending;

/* btd_l2cap.c */
extern uint16_t _acl_credits;
void l2cap_rx_reset(void);
void l2cap_link_closed(uint16_t handle);
void l2cap_recv_acl(uint16_t handle, uint8_t pb, const uint8_t* data, size_t len);
extern l2cap_chan_t _l2chans[MAX_L2CAP_CHANS];
extern uint16_t _l2_next_cid;
extern uint8_t _l2_next_sig_id;

/* btd_hid.c */
extern bt_hid_chan_t _hid;
extern uint64_t _sub_reassert_ms;

/* btd_le.c */
extern bool _scanning;
extern le_session_t _les[MAX_LE_SESSIONS];
extern int _le_cur;
extern uint8_t _local_addr[6];
extern uint8_t _local_addr_type;
extern bool _le_supported;
extern bool _le_req_active;
extern uint8_t _le_req_addr[6];
extern bool _le_req_pair;
extern uint64_t _le_req_ms;
extern int _le_req_slot;
extern bool _le_autoconnect;
extern bt_scan_slice_t _scan_slice;
extern uint64_t _scan_slice_end_ms;
extern uint64_t _scan_total_end_ms;
extern bool _le_scan_enabled;
extern bool _le_scan_extended;
extern int _le_ext_scan_supp;
extern bool _inquiry_running;
extern uint8_t _aes_sbox[256];
extern bool _aes_sbox_ready;
extern uint32_t _rng_state;
extern bool _le_resolving;

/* ---- LE session accessor macros ---- */
#define _le    (_les[_le_cur].le)
#define _att   (_les[_le_cur].att)
#define _smp   (_les[_le_cur].smp)
#define _hogp  (_les[_le_cur].hogp)

/* ---- cross-module function prototypes ---- */
/* btd.c */
void bt_emit(const char* fmt, ...);
void bt_ret_append(char* ret, size_t ret_sz, const char* fmt, ...);
int bt_hci_send_packet(uint8_t pkt_type, const uint8_t* data, size_t len);
int bt_hci_send_command(uint16_t ogf, uint16_t ocf, const uint8_t* params, uint8_t param_len);
int bt_hci_command_sync_ret(uint16_t ogf, uint16_t ocf, const uint8_t* params, uint8_t param_len, uint32_t timeout_ms, uint8_t* ret_params, uint8_t ret_cap, uint8_t* ret_len);
int bt_poll_once(uint32_t first_timeout_ms);
int bt_wait_for_opcode(uint16_t opcode, uint32_t timeout_ms);
int bt_hci_command_sync(uint16_t ogf, uint16_t ocf, const uint8_t* params, uint8_t param_len, uint32_t timeout_ms);
void bt_autoconnect_known(void);
int bt_loop(vdevice_t* dev, void* p);
/* true while a classic HID link is up or any LE/HOGP session is READY:
   used both to keep bt_loop tightly polled and to keep discovery scanning
   off the radio so it cannot starve a live input connection (bt_le_step). */
bool bt_hid_live(void);

/* btd_known.c */
uint8_t hci_opcode_lo(uint16_t opcode);
uint8_t hci_opcode_hi(uint16_t opcode);
bool bt_addr_equal(const uint8_t* a, const uint8_t* b);
void bt_addr_to_str(const uint8_t* addr, char* out, size_t size);
bool bt_parse_addr(const char* str, uint8_t* addr);
int bt_pending_matches_addr(const uint8_t* addr);
void bt_clear_pending(void);
bt_device_t* bt_find_device(const uint8_t* addr, bool create);
bt_device_t* bt_find_device_by_handle(uint16_t handle);
void bt_trim_name(char* name);
bt_known_t* bt_known_find(const uint8_t* addr);
bt_known_t* bt_known_find_by_ltk(uint16_t ediv, const uint8_t* rand8);
bt_known_t* bt_known_find_le_by_name(const char* name);
int bt_known_dedup_le(void);
int bt_known_load(void);
int bt_known_save(void);
void bt_known_touch_from_device(const bt_device_t* dev);
void bt_known_seed_devices(void);
void bt_parse_eir_name(const uint8_t* eir, size_t len, char* out, size_t out_sz);
void bt_emit_device_line(const char* prefix, const bt_device_t* dev);
void bt_ret_append_device_line(int dev_id, char* ret, size_t ret_sz, const char* prefix, const bt_device_t* dev);

/* btd_classic.c */
int bt_hci_auth_request(uint16_t handle);
int bt_hci_disconnect(uint16_t handle);
int bt_hci_create_connection(const bt_device_t* dev);
int bt_hci_request_remote_name(const bt_device_t* dev);
void bt_handle_inquiry_result(const uint8_t* payload, size_t len);
void bt_handle_inquiry_result_rssi(const uint8_t* payload, size_t len);
void bt_handle_extended_inquiry_result(const uint8_t* payload, size_t len);
void bt_handle_remote_name_complete(const uint8_t* payload, size_t len);
bool bt_hogp_blocks_classic(bt_device_t* dev);
void bt_handle_connection_complete(const uint8_t* payload, size_t len);
void bt_handle_disconnection_complete(const uint8_t* payload, size_t len);
void bt_handle_auth_complete(const uint8_t* payload, size_t len);
void bt_handle_pin_code_request(const uint8_t* payload, size_t len);
void bt_handle_link_key_request(const uint8_t* payload, size_t len);
void bt_handle_link_key_notify(const uint8_t* payload, size_t len);
void bt_handle_conn_request(const uint8_t* payload, size_t len);
void bt_handle_io_capability_request(const uint8_t* payload, size_t len);
void bt_handle_user_confirmation_request(const uint8_t* payload, size_t len);
void bt_handle_user_passkey_request(const uint8_t* payload, size_t len);
void bt_handle_simple_pairing_complete(const uint8_t* payload, size_t len);

/* btd_l2cap.c */
void bt_handle_num_completed_pkts(const uint8_t* payload, size_t len);
int l2cap_send_pdu(uint16_t handle, uint16_t cid, const uint8_t* payload, uint16_t payload_len);
l2cap_chan_t* l2cap_chan_open(uint16_t handle, uint16_t psm);
void l2cap_chan_close(l2cap_chan_t* ch, bool send_req);
void l2cap_step(void);
void l2cap_dispatch(uint16_t handle, const uint8_t* pdu, size_t len);

/* btd_hid.c */
void bt_hid_check_up(void);
void bt_hid_step(void);
void bt_hid_link_closed(uint16_t handle, const char* reason);
void bt_hid_stop(void);
void bt_hid_handle_report(const uint8_t* data, size_t len);
void bt_hid_handle_ctrl(l2cap_chan_t* ch, const uint8_t* data, size_t len);
void bt_hid_dispatch_mouse(const uint8_t* evt);
void bt_hid_dispatch_keyboard(const uint8_t* evt);
/* forward the raw gamepad report prefix under HID_REPORT_ID_JOYSTICK; len is
   the report length, truncated/zero-padded to HID_JOYSTICK_RAW_SIZE */
void bt_hid_dispatch_joystick(const uint8_t* raw, size_t len);
void bt_hid_accept(l2cap_chan_t* ch);
void bt_hid_start(uint16_t handle, const uint8_t* addr);
void bt_hid_stack_reset(void);

/* btd_classic.c */
/* CoD major 0x05 (Peripheral) whose minor marks a joystick/gamepad, as
   opposed to a keyboard/pointing device. DualShock 4/DualSense report a
   gamepad CoD (0x002508); many Xbox-layout pads report joystick (0x002504). */
bool bt_cod_is_gamepad(uint32_t cod);

/* btd_le.c */
int le_session_free(void);
int le_session_by_handle(uint16_t handle);
int le_session_ready_by_addr(const uint8_t* addr);
int bt_le_controller_init(void);
void bt_handle_le_meta(const uint8_t* payload, size_t len);
void bt_handle_encryption_change(const uint8_t* payload, size_t len);
void bt_le_stack_reset(void);
int bt_le_connect(bt_device_t* dev, bool pair, char* ret_text, size_t ret_text_sz);
void bt_le_link_closed(uint16_t handle, uint8_t reason);
void bt_le_l2cap_rx(uint16_t handle, uint16_t cid, const uint8_t* data, size_t len);
int bt_le_request(bt_device_t* dev, bool pair, char* ret_text, size_t ret_text_sz);
void bt_le_step(bool from_loop);
int bt_start_scan(int seconds);
int bt_stop_scan(void);
void bt_le_autoconnect_kick(void);

#endif /* BTD_INT_H */
