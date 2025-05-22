#include <bcm283x/board.h>
#include <kstring.h>
#include <bcm283x/mailbox.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>

#define GET_BOARD_REVISION  0x00010002

static __attribute__((__aligned__(16))) uint32_t data[6];
static uint32_t bcm283x_board_revision(void) {
	memset(data, 0, 6*4);
	data[0] = 6*4;
	data[1] = 0;
	data[2] = GET_BOARD_REVISION;
	data[3] = 4;
	data[4] = 4;
	data[5] = 0;

#if __aarch64__
    clear_cache((char *)data, (char *)data + sizeof(data));
#endif

	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));
	msg.data = ((uint32_t)data + 0x40000000) >> 4;
	mailbox_send(PROPERTY_CHANNEL, &msg);
	mailbox_read(PROPERTY_CHANNEL, &msg);

#if __aarch64__
    clear_cache((char *)data, (char *)data + sizeof(data));
#endif
	return data[5];
}

uint32_t bcm283x_board(void) {
	uint32_t revision = bcm283x_board_revision();
	//uint32_t revision = 0xb04170;
	if(revision == 0xb04170 ||
			revision == 0xb04171 )
		return PI_5_2G;
	else if(revision == 0xc04170 ||
			revision == 0xc04171 )
		return PI_5_4G;
	else if(revision == 0xd04170 ||
			revision == 0xd04171 )
		return PI_5_8G;
	else if(revision == 0xe04171) 
		return PI_5_16G;
	else if(revision == 0xa03111 ||
			revision == 0xa03112 ||
			revision == 0xa03115)
		return PI_4B_1G;
	else if(revision == 0xb03111 ||
			revision == 0xb03112 ||
			revision == 0xb03114 ||
			revision == 0xb03115)
		return PI_4B_2G;
	else if(revision == 0xc03111 ||
			revision == 0xc03112 ||
			revision == 0xc03114 ||
			revision == 0xc03115)
		return PI_4B_4G;
	else if(revision == 0xd03114 ||
			revision == 0xd03115)
		return PI_4B_8G;
	else if(revision == 0xa03140 ||
			revision == 0xa03141)
		return PI_CM4_1G;
	else if(revision == 0xb03140 ||
			revision == 0xb03141)
		return PI_CM4_2G;
	else if(revision == 0xc03140 ||
			revision == 0xc03141)
		return PI_CM4_4G;
	else if(revision == 0xd03140 ||
			revision == 0xd03141)
		return PI_CM4_8G;
	else if(revision == 0xa020a0 ||
			revision == 0xa02100 ||
			revision == 0xa220a0)
		return PI_CM3_1G;
	else if(revision == 0xa01040 ||
			revision == 0xa01041 ||
			revision == 0xa21041 ||
			revision == 0xa02042 ||
			revision == 0xa22042)
		return PI_2B;
	else if(revision == 0x9020e0)
		return PI_3A;
	else if(revision == 0x902120)
		return PI_0_2W;
	else if(revision == 0xa020a0 ||
			revision == 0xa02082 ||
			revision == 0xa020d3 ||
			revision == 0xa22082 ||
			revision == 0xa220a0 ||
			revision == 0xa02100 ||
			revision == 0xa32082 ||
			revision == 0xa52082 ||
			revision == 0xa22083)
		return PI_3B;
	else if(revision == 0x9200c1 ||
			revision == 0x9000c1)
		return PI_0_W;
	else if(revision == 0x900092 ||
			revision == 0x900093 || 
			revision == 0x920093)
		return PI_0;
	else if(revision == 0x900021 ||
			revision == 0x12 ||
			revision == 0x15)
		return PI_1A;
	else if(revision == 0x0d ||
			revision == 0x0e ||
			revision == 0x0f ||
			revision == 0x10 ||
			revision == 0x13 ||
			revision == 0x900032)
		return PI_1B;
	return PI_UNKNOWN;
}
