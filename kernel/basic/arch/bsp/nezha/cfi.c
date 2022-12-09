
#include "cfi.h"
#include <mm/mmudef.h>

#define debug printf

uint32_t flash_read8(void *addr)
{
	return get8(addr);
}

uint32_t flash_read16(void *addr)
{
	return get16(addr);
}

uint32_t flash_read32(void *addr)
{
	return get32(addr);
}

uint32_t flash_read64(void *addr)
{
	return get64(addr);
}

uint32_t flash_write8(uint8_t v, void *addr)
{
	return put8(addr, v);
}

uint32_t flash_write16(uint16_t v, void *addr)
{
	return put16(addr, v);
}

uint32_t flash_write32(uint32_t v, void *addr)
{
	return put32(addr, v);
}

uint32_t flash_write64(uint64_t v, void *addr)
{
	return put64(addr, v);
}


static inline void *
flash_map(flash_info_t *info, flash_sect_t sect, uint offset)
{
	unsigned int byte_offset = offset * info->portwidth;
printf("%d\n", byte_offset);
	return (void *)(info->start[sect] + (byte_offset << info->chip_lsb));
}

static inline void flash_unmap(flash_info_t *info, flash_sect_t sect,
			       unsigned int offset, void *addr)
{
}

static void flash_make_cmd(flash_info_t *info, uint32_t cmd, void *cmdbuf)
{
	int i;
	int cword_offset;
	int cp_offset;
	uchar val;
	uchar *cp = (uchar *) cmdbuf;

	for (i = info->portwidth; i > 0; i--) {
		cword_offset = (info->portwidth - i) % info->chipwidth;
		cp_offset = i - 1;
		val = *((uchar *)&cmd + sizeof(uint32_t) - cword_offset - 1);
		cp[cp_offset] = (cword_offset >= sizeof(uint32_t)) ? 0x00 : val;
	}
}

static void flash_write_cmd(flash_info_t *info, flash_sect_t sect,
			    uint offset, uint32_t cmd)
{
	void *addr;
	cfiword_t cword;

	addr = flash_map(info, sect, offset);
	flash_make_cmd(info, cmd, &cword);
	switch (info->portwidth) {
	case FLASH_CFI_8BIT:
		debug("fwc addr %08x cmd %x %x 8bit x %d bit\n", addr, cmd,
		      cword.w8, info->chipwidth << CFI_FLASH_SHIFT_WIDTH);
		flash_write8(cword.w8, addr);
		break;
	case FLASH_CFI_16BIT:
		debug("fwc addr %08x cmd %x %04x 16bit x %d bit\n", addr,
		      cmd, cword.w16,
		      info->chipwidth << CFI_FLASH_SHIFT_WIDTH);
		flash_write16(cword.w16, addr);
		break;
	case FLASH_CFI_32BIT:
		debug("fwc addr %08x cmd %x %08x 32bit x %d bit\n", addr,
		      cmd, cword.w32,
		      info->chipwidth << CFI_FLASH_SHIFT_WIDTH);
		flash_write32(cword.w32, addr);
		break;
	case FLASH_CFI_64BIT:
#ifdef DEBUG
		{
			char str[20];

			print_longlong(str, cword.w64);

			debug("fwrite addr %p cmd %x %s 64 bit x %d bit\n",
			      addr, cmd, str,
			      info->chipwidth << CFI_FLASH_SHIFT_WIDTH);
		}
#endif
		flash_write64(cword.w64, addr);
		break;
	}

	/* Ensure all the instructions are fully finished */
	//sync();

	flash_unmap(info, sect, offset, addr);
}



static inline uchar flash_read_uchar(flash_info_t *info, uint offset)
{
	uchar *cp;
	uchar retval;

	cp = flash_map(info, 0, offset);
	retval = flash_read8(cp + info->portwidth - 1);
	flash_unmap(info, 0, offset, cp);
	return retval;
}



static int flash_isequal(flash_info_t *info, flash_sect_t sect, uint offset,
			 uchar cmd)
{
	void *addr;
	cfiword_t cword;
	int retval;

	addr = flash_map(info, sect, offset);
	flash_make_cmd(info, cmd, &cword);

	debug("is= cmd %x(%c) addr %08x ", cmd, cmd, addr);
	switch (info->portwidth) {
	case FLASH_CFI_8BIT:
		debug("is= %x %x\n", flash_read8(addr), cword.w8);
		retval = (flash_read8(addr) == cword.w8);
		break;
	case FLASH_CFI_16BIT:
		debug("is= %04x %04x\n", flash_read16(addr), cword.w16);
		retval = (flash_read16(addr) == cword.w16);
		break;
	case FLASH_CFI_32BIT:
		debug("is= %08x %08x\n", flash_read32(addr), cword.w32);
		retval = (flash_read32(addr) == cword.w32);
		break;
	case FLASH_CFI_64BIT:
		retval = (flash_read64(addr) == cword.w64);
		break;
	default:
		retval = 0;
		break;
	}
	flash_unmap(info, sect, offset, addr);

	return retval;
}

static void flash_read_cfi(flash_info_t *info, void *buf, unsigned int start,
			   uint32_t len)
{
	uint8_t *p = buf;
	unsigned int i;

	for (i = 0; i < len; i++)
		p[i] = flash_read_uchar(info, start + i);
}

static void flash_cmd_reset(flash_info_t *info)
{
	/*
	 * We do not yet know what kind of commandset to use, so we issue
	 * the reset command in both Intel and AMD variants, in the hope
	 * that AMD flash roms ignore the Intel command.
	 */
	flash_write_cmd(info, 0, 0, AMD_CMD_RESET);
	_delay(1);
	flash_write_cmd(info, 0, 0, FLASH_CMD_RESET);
}

static uint flash_offset_cfi[2] = { FLASH_OFFSET_CFI, FLASH_OFFSET_CFI_ALT };

int flash_detect_cfi(flash_info_t *info, struct cfi_qry *qry)
{
	int cfi_offset;

	/* Issue FLASH reset command */
	flash_cmd_reset(info);
#if 1
	for (cfi_offset = 0; cfi_offset < 2;
	     cfi_offset++) {
		flash_write_cmd(info, 0, flash_offset_cfi[cfi_offset],
				FLASH_CMD_CFI);
		if (flash_isequal(info, 0, FLASH_OFFSET_CFI_RESP, 'Q') &&
		    flash_isequal(info, 0, FLASH_OFFSET_CFI_RESP + 1, 'R') &&
		    flash_isequal(info, 0, FLASH_OFFSET_CFI_RESP + 2, 'Y')) {
			flash_read_cfi(info, qry, FLASH_OFFSET_CFI_RESP,
				       sizeof(struct cfi_qry));
			info->interface	= qry->interface_desc;
			/* Some flash chips can support multiple bus widths.
			 * In this case, override the interface width and
			 * limit it to the port width.
			 */
			if ((info->interface == FLASH_CFI_X8X16) &&
					(info->portwidth == FLASH_CFI_8BIT)) {
				debug("Overriding 16-bit interface width to"
						" 8-bit port width\n");
				info->interface = FLASH_CFI_X8;
			} else if ((info->interface == FLASH_CFI_X16X32) &&
					(info->portwidth == FLASH_CFI_16BIT)) {
				debug("Overriding 16-bit interface width to"
						" 16-bit port width\n");
				info->interface = FLASH_CFI_X16;
			}

			info->cfi_offset = flash_offset_cfi[cfi_offset];
			debug("device interface is %d\n",
			      info->interface);
			debug("found port %d chip %d chip_lsb %d ",
			      info->portwidth, info->chipwidth, info->chip_lsb);
			debug("port %d bits chip %d bits\n",
			      info->portwidth << CFI_FLASH_SHIFT_WIDTH,
			      info->chipwidth << CFI_FLASH_SHIFT_WIDTH);

			/* calculate command offsets as in the Linux driver */
			info->addr_unlock1 = 0x555;
			info->addr_unlock2 = 0x2aa;

			/*
			 * modify the unlock address if we are
			 * in compatibility mode
			 */
			if (/* x8/x16 in x8 mode */
			    (info->chipwidth == FLASH_CFI_BY8 &&
				info->interface == FLASH_CFI_X8X16) ||
			    /* x16/x32 in x16 mode */
			    (info->chipwidth == FLASH_CFI_BY16 &&
				info->interface == FLASH_CFI_X16X32)) {
				info->addr_unlock1 = 0xaaa;
				info->addr_unlock2 = 0x555;
			}

			info->name = "CFI conformant";
			return 1;
		}
	}
#endif
	return 0;
}
