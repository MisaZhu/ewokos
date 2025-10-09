#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sysinfo.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <arch/bcm283x/pl011_uart.h>
#include "firmware_3.h"

inline uint8_t lo(uint16_t val) { return (uint8_t)(val & 0xff); }
inline uint8_t hi(uint16_t val) { return (uint8_t)((val & 0xff00) >> 8); }

enum {
    OGF_HOST_CONTROL          = 0x03,
    OGF_LE_CONTROL            = 0x08,
    OGF_VENDOR                = 0x3f,

    COMMAND_SET_BDADDR        = 0x01,
    COMMAND_RESET_CHIP        = 0x03,
    COMMAND_SET_BAUD          = 0x18,
    COMMAND_LOAD_FIRMWARE     = 0x2e,

    HCI_COMMAND_PKT           = 0x01,
    HCI_ACL_PKT               = 0x02,
    HCI_EVENT_PKT             = 0x04,
    COMMAND_COMPLETE_CODE     = 0x0e,
    CONNECT_COMPLETE_CODE     = 0x0f,

    LL_SCAN_ACTIVE            = 0x01,
    LL_ADV_NONCONN_IND        = 0x03
};

// 发送HCI命令
int32_t bt_send_hci_command_bytes(uint8_t* opcodebytes, uint8_t* data, uint8_t len) {
    // HCI命令包格式: 0x01 + opcode (2字节) + 参数长度 (1字节) + 参数
    bcm283x_pl011_uart_send(HCI_COMMAND_PKT); // HCI命令包标识
	bcm283x_pl011_uart_send(opcodebytes[0]);
	bcm283x_pl011_uart_send(opcodebytes[1]);
	bcm283x_pl011_uart_send(len);
    
    for (uint8_t i = 0; i < len; i++) {
        bcm283x_pl011_uart_send(data[i]);
    }

	uint8_t res = bcm283x_pl011_uart_recv(100);
	if(res != HCI_EVENT_PKT)  {
		slog("bt_send_hci_command_bytes failed, res=0x%X\n", res);
		return 1;
	}

    uint8_t code = bcm283x_pl011_uart_recv(100);
    if (code == CONNECT_COMPLETE_CODE) {
		if (bcm283x_pl011_uart_recv(100) != 4)
			return 2;

		res = bcm283x_pl011_uart_recv(100);
		if (res != 0) {
				slog("Saw HCI COMMAND STATUS error %d\n", res);
				return 12;
		}

		if (bcm283x_pl011_uart_recv(100) == 0)
			return 3;
		if (bcm283x_pl011_uart_recv(100) != opcodebytes[0])
			return 4;
		if (bcm283x_pl011_uart_recv(100) != opcodebytes[1])
			return 5;
    } 
	else if (code == COMMAND_COMPLETE_CODE) {
		if (bcm283x_pl011_uart_recv(100) != 4)
			return 6;
		if (bcm283x_pl011_uart_recv(100) == 0)
			return 7;
		if (bcm283x_pl011_uart_recv(100) != opcodebytes[0])	
			return 8;
		if (bcm283x_pl011_uart_recv(100) != opcodebytes[1])
			return 9;
		if (bcm283x_pl011_uart_recv(100) != 0)
			return 10;
    }
	else
		return 11;

    return 0;
}

// 发送HCI命令
int32_t bt_send_hci_command(uint16_t ogf, uint16_t ocf, uint8_t* data, uint32_t len) {
	uint16_t opcode = ogf << 10 | ocf;
    uint8_t  opcodebytes[2] = { lo(opcode), hi(opcode) };
	return bt_send_hci_command_bytes(opcodebytes, data, len);
}

// 蓝牙固件下载
void bt_load_firmware(void) {
	volatile unsigned char empty[] = {};
	int32_t res = bt_send_hci_command(OGF_VENDOR, COMMAND_LOAD_FIRMWARE, empty, 0);
	if(res != 0) {
		slog("loadFirmware() failed: %d\n", res);
		return;
	}

	uint32_t offset = 0;
	uint32_t size = brcmfmac43430_sdio_bin_len;
	uint8_t opcodebytes[2];
	uint8_t length;
	uint8_t *data = brcmfmac43430_sdio_bin;

	while (offset < size) {
		opcodebytes[0] = *data;
		opcodebytes[1] = *(data+1);
		length =         *(data+2);
		data += 3;

		int32_t res = bt_send_hci_command_bytes(opcodebytes, data, length);
		if(res != 0) {
			slog("bt_send_hci_command_bytes failed, res=%d\n", res);
			break;
		}
		data += length;
		offset += 3 + length;
	}
}

int main(int argc, char* argv[]) {
	_mmio_base = mmio_map();

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	if(strcmp(sysinfo.machine, "raspberry-pi1") == 0 ||
			strcmp(sysinfo.machine, "raspberry-pi2b") == 0)  {
		printf("bt not support with pl011_uart\n");
		return -1;
	}
	slog("bt: init pl011_uart\n");
	bcm283x_pl011_uart_init();
	bcm283x_pl011_uart_recv(100); // 清空接收缓冲区
	sleep(1);

    // 重置蓝牙芯片
	slog("reset firmware ... ");
    volatile uint8_t empty[] = {};
	int32_t res = bt_send_hci_command(OGF_HOST_CONTROL, COMMAND_RESET_CHIP, empty, 0);
	if(res != 0) {
		slog("failed: %d\n", res);
		return -1;
	}
    usleep(1000000); // 等待重置完成
	slog("done\n");

	// 加载蓝牙固件
	slog("load firmware ... ");
    bt_load_firmware();
	slog("done\n");
    
	return 0;
}
