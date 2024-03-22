// SPDX-License-Identifier: ISC
/*
 * Copyright (c) 2013 Broadcom Corporation
 */
#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/log.h>
#include <netinet/if_ether.h>

#include "firmware_bin.h"
#include "chip.h"


#define BRCMF_FW_MAX_NVRAM_SIZE			64000
#define BRCMF_FW_NVRAM_DEVPATH_LEN		19	/* devpath0=pcie/1/4/ */
#define BRCMF_FW_NVRAM_PCIEDEV_LEN		10	/* pcie/1/4/ + \0 */
#define BRCMF_FW_DEFAULT_BOARDREV		"boardrev=0xff"
#define BRCMF_FW_MACADDR_FMT			"macaddr=%pM"
#define BRCMF_FW_MACADDR_LEN			(7 + ETH_ALEN * 3)

enum nvram_parser_state {
	IDLE,
	KEY,
	VALUE,
	COMMENT,
	END
};

char saved_ccode[2] = {};

/**
 * struct nvram_parser - internal info for parser.
 *
 * @state: current parser state.
 * @data: input buffer being parsed.
 * @nvram: output buffer with parse result.
 * @nvram_len: length of parse result.
 * @line: current line.
 * @column: current column in line.
 * @pos: byte offset in input buffer.
 * @entry: start position of key,value entry.
 * @multi_dev_v1: detect pcie multi device v1 (compressed).
 * @multi_dev_v2: detect pcie multi device v2.
 * @boardrev_found: nvram contains boardrev information.
 * @strip_mac: strip the MAC address.
 */
struct nvram_parser {
	enum nvram_parser_state state;
	const char *data;
	uint8_t *nvram;
	uint32_t nvram_len;
	uint32_t line;
	uint32_t column;
	uint32_t pos;
	uint32_t entry;
	bool multi_dev_v1;
	bool multi_dev_v2;
	bool boardrev_found;
	bool strip_mac;
};

/*
 * is_nvram_char() - check if char is a valid one for NVRAM entry
 *
 * It accepts all printable ASCII chars except for '#' which opens a comment.
 * Please note that ' ' (space) while accepted is not a valid key name char.
 */
static bool is_nvram_char(char c)
{
	/* comment marker excluded */
	if (c == '#')
		return false;

	/* key and value may have any other readable character */
	return (c >= 0x20 && c < 0x7f);
}

static bool is_whitespace(char c)
{
	return (c == ' ' || c == '\r' || c == '\n' || c == '\t');
}

static enum nvram_parser_state brcmf_nvram_handle_idle(struct nvram_parser *nvp)
{
	char c;

	c = nvp->data[nvp->pos];
	if (c == '\n')
		return COMMENT;
	if (is_whitespace(c) || c == '\0')
		goto proceed;
	if (c == '#')
		return COMMENT;
	if (is_nvram_char(c)) {
		nvp->entry = nvp->pos;
		return KEY;
	}
	brcm_log("warning: ln=%d:col=%d: ignoring invalid character\n",
		  nvp->line, nvp->column);
proceed:
	nvp->column++;
	nvp->pos++;
	return IDLE;
}

static enum nvram_parser_state brcmf_nvram_handle_key(struct nvram_parser *nvp)
{
	enum nvram_parser_state st = nvp->state;
	char c;

	c = nvp->data[nvp->pos];
	if (c == '=') {
		/* ignore RAW1 by treating as comment */
		if (strncmp(&nvp->data[nvp->entry], "RAW1", 4) == 0)
			st = COMMENT;
		else
			st = VALUE;
		if (strncmp(&nvp->data[nvp->entry], "devpath", 7) == 0)
			nvp->multi_dev_v1 = true;
		if (strncmp(&nvp->data[nvp->entry], "pcie/", 5) == 0)
			nvp->multi_dev_v2 = true;
		if (strncmp(&nvp->data[nvp->entry], "boardrev", 8) == 0)
			nvp->boardrev_found = true;
		/* strip macaddr if platform MAC overrides */
		if (nvp->strip_mac &&
		    strncmp(&nvp->data[nvp->entry], "macaddr", 7) == 0)
			st = COMMENT;
	} else if (!is_nvram_char(c) || c == ' ') {
		brcm_log("warning: ln=%d:col=%d: '=' expected, skip invalid key entry\n",
			  nvp->line, nvp->column);
		return COMMENT;
	}

	nvp->column++;
	nvp->pos++;
	return st;
}

static enum nvram_parser_state
brcmf_nvram_handle_value(struct nvram_parser *nvp)
{
	char c;
	char *skv;
	char *ekv;
	uint32_t cplen;

	c = nvp->data[nvp->pos];
	if (!is_nvram_char(c)) {
		/* key,value pair complete */
		ekv = (char *)&nvp->data[nvp->pos];
		skv = (char *)&nvp->data[nvp->entry];
		cplen = ekv - skv;
		if (nvp->nvram_len + cplen + 1 >= BRCMF_FW_MAX_NVRAM_SIZE)
			return END;
		/* copy to output buffer */
		memcpy(&nvp->nvram[nvp->nvram_len], skv, cplen);
		nvp->nvram_len += cplen;
		nvp->nvram[nvp->nvram_len] = '\0';
		nvp->nvram_len++;
		return IDLE;
	}
	nvp->pos++;
	nvp->column++;
	return VALUE;
}

static enum nvram_parser_state
brcmf_nvram_handle_comment(struct nvram_parser *nvp)
{
	char *eoc, *sol;

	sol = (char *)&nvp->data[nvp->pos];
	eoc = strchr(sol, '\n');
	if (!eoc) {
		eoc = strchr(sol, '\0');
		if (!eoc)
			return END;
	}

	/* eat all moving to next line */
	nvp->line++;
	nvp->column = 1;
	nvp->pos += (eoc - sol) + 1;
	return IDLE;
}

static enum nvram_parser_state brcmf_nvram_handle_end(struct nvram_parser *nvp)
{
	/* final state */
	return END;
}


static enum nvram_parser_state
(*nv_parser_states[])(struct nvram_parser *nvp) = {
	brcmf_nvram_handle_idle,
	brcmf_nvram_handle_key,
	brcmf_nvram_handle_value,
	brcmf_nvram_handle_comment,
	brcmf_nvram_handle_end
};

static int brcmf_init_nvram_parser(struct nvram_parser *nvp,
				   const char *data, size_t data_len)
{
	size_t size;

	memset(nvp, 0, sizeof(*nvp));
	nvp->data = data;
	/* Limit size to MAX_NVRAM_SIZE, some files contain lot of comment */
	if (data_len > BRCMF_FW_MAX_NVRAM_SIZE)
		size = BRCMF_FW_MAX_NVRAM_SIZE;
	else
		size = data_len;
	/* Add space for properties we may add */
	size += strlen(BRCMF_FW_DEFAULT_BOARDREV) + 1;
	size += BRCMF_FW_MACADDR_LEN + 1;
	/* Alloc for extra 0 byte + roundup by 4 + length field */
	size += 1 + 3 + sizeof(uint32_t);
	nvp->nvram = malloc(size);
	if (!nvp->nvram)
		return -ENOMEM;

	nvp->line = 1;
	nvp->column = 1;
	return 0;
}

/* brcmf_fw_strip_multi_v1 :Some nvram files contain settings for multiple
 * devices. Strip it down for one device, use domain_nr/bus_nr to determine
 * which data is to be returned. v1 is the version where nvram is stored
 * compressed and "devpath" maps to index for valid entries.
 */
static void brcmf_fw_strip_multi_v1(struct nvram_parser *nvp, uint16_t domain_nr,
				    uint16_t bus_nr)
{
	/* Device path with a leading '=' key-value separator */
	char pci_path[] = "=pci/?/?";
	size_t pci_len;
	char pcie_path[] = "=pcie/?/?";
	size_t pcie_len;

	uint32_t i, j;
	bool found;
	uint8_t *nvram;
	uint8_t id;

	nvram = malloc(nvp->nvram_len + 1 + 3 + sizeof(uint32_t));
	if (!nvram)
		goto fail;

	/* min length: devpath0=pcie/1/4/ + 0:x=y */
	if (nvp->nvram_len < BRCMF_FW_NVRAM_DEVPATH_LEN + 6)
		goto fail;

	/* First search for the devpathX and see if it is the configuration
	 * for domain_nr/bus_nr. Search complete nvp
	 */
	snprintf(pci_path, sizeof(pci_path), "=pci/%d/%d", domain_nr,
		 bus_nr);
	pci_len = strlen(pci_path);
	snprintf(pcie_path, sizeof(pcie_path), "=pcie/%d/%d", domain_nr,
		 bus_nr);
	pcie_len = strlen(pcie_path);
	found = false;
	i = 0;
	while (i < nvp->nvram_len - BRCMF_FW_NVRAM_DEVPATH_LEN) {
		/* Format: devpathX=pcie/Y/Z/
		 * Y = domain_nr, Z = bus_nr, X = virtual ID
		 */
		if (strncmp((const char*)&nvp->nvram[i], "devpath", 7) == 0 &&
		    (!strncmp((const char*)&nvp->nvram[i + 8], pci_path, pci_len) ||
		     !strncmp((const char*)&nvp->nvram[i + 8], pcie_path, pcie_len))) {
			id = nvp->nvram[i + 7] - '0';
			found = true;
			break;
		}
		while (nvp->nvram[i] != 0)
			i++;
		i++;
	}
	if (!found)
		goto fail;

	/* Now copy all valid entries, release old nvram and assign new one */
	i = 0;
	j = 0;
	while (i < nvp->nvram_len) {
		if ((nvp->nvram[i] - '0' == id) && (nvp->nvram[i + 1] == ':')) {
			i += 2;
			if (strncmp((const char*)&nvp->nvram[i], "boardrev", 8) == 0)
				nvp->boardrev_found = true;
			while (nvp->nvram[i] != 0) {
				nvram[j] = nvp->nvram[i];
				i++;
				j++;
			}
			nvram[j] = 0;
			j++;
		}
		while (nvp->nvram[i] != 0)
			i++;
		i++;
	}
	free(nvp->nvram);
	nvp->nvram = nvram;
	nvp->nvram_len = j;
	return;

fail:
	free(nvram);
	nvp->nvram_len = 0;
}

/* brcmf_fw_strip_multi_v2 :Some nvram files contain settings for multiple
 * devices. Strip it down for one device, use domain_nr/bus_nr to determine
 * which data is to be returned. v2 is the version where nvram is stored
 * uncompressed, all relevant valid entries are identified by
 * pcie/domain_nr/bus_nr:
 */
static void brcmf_fw_strip_multi_v2(struct nvram_parser *nvp, uint16_t domain_nr,
				    uint16_t bus_nr)
{
	char prefix[BRCMF_FW_NVRAM_PCIEDEV_LEN];
	size_t len;
	uint32_t i, j;
	uint8_t *nvram;

	nvram = malloc(nvp->nvram_len + 1 + 3 + sizeof(uint32_t));
	if (!nvram) {
		nvp->nvram_len = 0;
		return;
	}

	/* Copy all valid entries, release old nvram and assign new one.
	 * Valid entries are of type pcie/X/Y/ where X = domain_nr and
	 * Y = bus_nr.
	 */
	snprintf(prefix, sizeof(prefix), "pcie/%d/%d/", domain_nr, bus_nr);
	len = strlen(prefix);
	i = 0;
	j = 0;
	while (i < nvp->nvram_len - len) {
		if (strncmp((const char*)&nvp->nvram[i], prefix, len) == 0) {
			i += len;
			if (strncmp((const char*)&nvp->nvram[i], "boardrev", 8) == 0)
				nvp->boardrev_found = true;
			while (nvp->nvram[i] != 0) {
				nvram[j] = nvp->nvram[i];
				i++;
				j++;
			}
			nvram[j] = 0;
			j++;
		}
		while (nvp->nvram[i] != 0)
			i++;
		i++;
	}
	free(nvp->nvram);
	nvp->nvram = nvram;
	nvp->nvram_len = j;
}

static void brcmf_fw_add_defaults(struct nvram_parser *nvp)
{
	if (nvp->boardrev_found)
		return;

	memcpy(&nvp->nvram[nvp->nvram_len], &BRCMF_FW_DEFAULT_BOARDREV,
	       strlen(BRCMF_FW_DEFAULT_BOARDREV));
	nvp->nvram_len += strlen(BRCMF_FW_DEFAULT_BOARDREV);
	nvp->nvram[nvp->nvram_len] = '\0';
	nvp->nvram_len++;
}

static void brcmf_fw_add_macaddr(struct nvram_parser *nvp, uint8_t *mac)
{
	int len;

	len = snprintf((char*)&nvp->nvram[nvp->nvram_len], BRCMF_FW_MACADDR_LEN + 1,
			BRCMF_FW_MACADDR_FMT, mac);
	WARN_ON(len != BRCMF_FW_MACADDR_LEN);
	nvp->nvram_len += len + 1;
}

/* brcmf_nvram_strip :Takes a buffer of "<var>=<value>\n" lines read from a fil
 * and ending in a NUL. Removes carriage returns, empty lines, comment lines,
 * and converts newlines to NULs. Shortens buffer as needed and pads with NULs.
 * End of buffer is completed with token identifying length of buffer.
 */
static void *brcmf_fw_nvram_strip(const char *data, size_t data_len,
				  uint32_t *new_length)
{
	struct nvram_parser nvp;
	uint32_t pad;
	uint32_t token;
	uint32_t token_le;
	uint8_t mac[ETH_ALEN];

	if (brcmf_init_nvram_parser(&nvp, data, data_len) < 0)
		return NULL;

	nvp.strip_mac = false;

	while (nvp.pos < data_len) {
		nvp.state = nv_parser_states[nvp.state](&nvp);
		if (nvp.state == END)
			break;
	}
	if (nvp.multi_dev_v1) {
		nvp.boardrev_found = false;
		brcmf_fw_strip_multi_v1(&nvp, 0, 0);
	} else if (nvp.multi_dev_v2) {
		nvp.boardrev_found = false;
		brcmf_fw_strip_multi_v2(&nvp, 0, 0);
	}

	if (nvp.nvram_len == 0) {
		free(nvp.nvram);
		return NULL;
	}

	brcmf_fw_add_defaults(&nvp);

	if (nvp.strip_mac)
		brcmf_fw_add_macaddr(&nvp, mac);

	pad = nvp.nvram_len;
	*new_length = roundup(nvp.nvram_len + 1, 4);
	while (pad != *new_length) {
		nvp.nvram[pad] = 0;
		pad++;
	}

	token = *new_length / 4;
	token = (~token << 16) | (token & 0x0000FFFF);
	token_le = cpu_to_le32(token);

	memcpy(&nvp.nvram[*new_length], &token_le, sizeof(token_le));
	*new_length += sizeof(token_le);

	return nvp.nvram;
}

uint8_t* brcmf_fw_get_firmware(uint32_t* len){
    *len = cyfmac43455_sdio_bin_len;
    return cyfmac43455_sdio_bin;
}

uint8_t* brcmf_fw_get_nvram(uint32_t* len){
    return brcmf_fw_nvram_strip(brcmfmac43455_sdio_txt, brcmfmac43455_sdio_txt_len, len);
}

uint8_t* brcmf_fw_get_clm(uint32_t* len){
	*len = cyfmac43455_sdio_clm_blob_len;
	return cyfmac43455_sdio_clm_blob;
}