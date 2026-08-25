/*
 * usb_defs.h: hardware-independent USB specification constants and
 * descriptor layouts shared by the usbhostd implementations across
 * platforms (DWC2 on raspix, xHCI on raspi5, and future HCDs).
 *
 * Only spec-level definitions live here: request codes, descriptor
 * structures, class/protocol numbers, hub features, HID usages and the
 * mass-storage bulk-only wrappers. Controller registers and driver
 * policies belong to the per-machine bsp/arch layers.
 */
#ifndef __USB_DEFS_H__
#define __USB_DEFS_H__

#include <stdint.h>

/* /dev/hid0 event fan-out: report IDs delivered to subscribers via
   fcntl(cmd 0), and the fixed event sizes read back from the device */
#define USB_REPORT_ID_MOUSE 1u
#define USB_REPORT_ID_KEYBOARD 2u
#define USB_REPORT_ID_TOUCH 3u

#define USB_QUEUE_DEPTH 32
#define USB_MAX_EVENT_SIZE 8
#define USB_POINTER_EVENT_SIZE 7
#define USB_KEYBOARD_EVENT_SIZE 8
#define USB_MAX_REPORT 64
#define USB_MAX_CANDIDATES 8
#define USB_MAX_USAGE_LIST 32

/* standard device requests (USB 2.0 spec table 9-4) */
#define USB_REQ_GET_STATUS 0x00
#define USB_REQ_CLEAR_FEATURE 0x01
#define USB_REQ_SET_FEATURE 0x03
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_CONFIGURATION 0x09
/* HID class requests */
#define USB_REQ_SET_IDLE 0x0A
#define USB_REQ_SET_PROTOCOL 0x0B

/* bmRequestType recipes used by the enumeration/HID/hub paths */
#define USB_REQTYPE_STD_IN 0x80
#define USB_REQTYPE_STD_OUT 0x00
#define USB_REQTYPE_STD_IFACE_IN 0x81
#define USB_REQTYPE_STD_EP_OUT 0x02
#define USB_REQTYPE_CLASS_IFACE_OUT 0x21
#define USB_REQTYPE_CLASS_DEV_IN 0xA0
#define USB_REQTYPE_CLASS_PORT_OUT 0x23
#define USB_REQTYPE_CLASS_PORT_IN 0xA3

/* descriptor types */
#define USB_DESC_DEVICE 0x01
#define USB_DESC_CONFIG 0x02
#define USB_DESC_INTERFACE 0x04
#define USB_DESC_ENDPOINT 0x05
#define USB_DESC_HID 0x21
#define USB_DESC_HID_REPORT 0x22
#define USB_DESC_HUB 0x29

/* device/interface classes */
#define USB_CLASS_HID 0x03
#define USB_CLASS_HUB 0x09
#define USB_CLASS_MSC 0x08
#define USB_SUBCLASS_BOOT 0x01
#define USB_PROTOCOL_KEYBOARD 0x01
#define USB_PROTOCOL_MOUSE 0x02

/* standard feature selectors */
#define USB_FEAT_ENDPOINT_HALT 0

/* endpoint addressing / transfer types */
#define USB_ENDPOINT_IN 0x80
#define USB_ENDPOINT_XFER_BULK 0x02
#define USB_ENDPOINT_XFER_INTERRUPT 0x03

/* hub port features (USB 2.0 spec table 11-17) */
#define USB_HUB_FEAT_PORT_RESET 4
#define USB_HUB_FEAT_PORT_POWER 8
#define USB_HUB_FEAT_C_PORT_CONNECTION 16
#define USB_HUB_FEAT_C_PORT_ENABLE 17
#define USB_HUB_FEAT_C_PORT_RESET 20

/* hub wPortStatus bits */
#define USB_HUB_PS_CONNECTION (1u << 0)
#define USB_HUB_PS_ENABLE (1u << 1)
#define USB_HUB_PS_RESET (1u << 4)
#define USB_HUB_PS_POWER (1u << 8)
#define USB_HUB_PS_LOW_SPEED (1u << 9)
#define USB_HUB_PS_HIGH_SPEED (1u << 10)

/* hub wPortChange bits (positions mirror the C_PORT_* feature selectors) */
#define USB_HUB_PC_CONNECTION (1u << 0)
#define USB_HUB_PC_ENABLE (1u << 1)
#define USB_HUB_PC_RESET (1u << 4)

/* mass storage class: bulk-only transport over SCSI transparent command set */
#define USB_MSC_SUBCLASS_SCSI 0x06
#define USB_MSC_SUBCLASS_UFI 0x04
#define USB_MSC_PROTO_BBB 0x50
#define USB_MSC_REQ_RESET 0xFF
#define USB_MSC_REQ_GET_MAX_LUN 0xFE
#define USB_MSC_CBW_SIG 0x43425355u /* "USBC" */
#define USB_MSC_CSW_SIG 0x53425355u /* "USBS" */
#define USB_MSC_SECTOR_SIZE 512u

#define SCSI_OPCODE_TEST_UNIT_READY 0x00
#define SCSI_OPCODE_INQUIRY 0x12
#define SCSI_OPCODE_READ_CAPACITY10 0x25
#define SCSI_OPCODE_READ10 0x28
#define SCSI_OPCODE_WRITE10 0x2A
#define SCSI_OPCODE_SYNC_CACHE10 0x35

/* HID usage pages and usages referenced by the report parsers */
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_BUTTON 0x09
#define HID_USAGE_PAGE_DIGITIZER 0x0D
#define HID_USAGE_POINTER 0x01
#define HID_USAGE_MOUSE 0x02
#define HID_USAGE_JOYSTICK 0x04
#define HID_USAGE_GAMEPAD 0x05
#define HID_USAGE_KEYBOARD 0x06
#define HID_USAGE_TOUCH_SCREEN 0x04
#define HID_USAGE_TOUCH_PAD 0x05
#define HID_USAGE_FINGER 0x22
#define HID_USAGE_TIP_SWITCH 0x42
#define HID_USAGE_X 0x30
#define HID_USAGE_Y 0x31
#define HID_USAGE_WHEEL 0x38

typedef struct __attribute__((packed)) {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_pkt_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} usb_device_desc_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} usb_config_desc_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_iface_desc_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} usb_ep_desc_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bReportDescriptorType;
    uint16_t wReportDescriptorLength;
} usb_hid_desc_t;

/* mass storage bulk-only transport: command block / command status wrappers */
typedef struct __attribute__((packed)) {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
    uint8_t CBWCB[16];
} usb_cbw_t;

typedef struct __attribute__((packed)) {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} usb_csw_t;

#endif /* __USB_DEFS_H__ */
