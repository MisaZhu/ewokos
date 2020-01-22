#ifndef PARTITION_H
#define PARTITION_H

#include <types.h>

typedef struct {
	uint8_t drive; /* 0x80 - active */
	uint8_t head; /* starting head */
	uint8_t sector; /* starting sector */
	uint8_t cylinder;
	/* starting cylinder */
	uint8_t sys_type; /* partition type */
	uint8_t end_head; /* end head */
	uint8_t end_sector; /* end sector */
	uint8_t end_cylinder; /* end cylinder */
	uint32_t start_sector;
	uint32_t nr_sectors;
	/* starting sector counting from 0 */
	/* nr of sectors in partition */
} __attribute__((packed)) partition_t;

typedef struct {
	char      jmp[3];
	char      oem[8];
	uint8_t   bps0;
	uint8_t   bps1;
	uint8_t   spc;
	uint16_t  rsc;
	uint8_t   nf;
	uint8_t   nr0;
	uint8_t   nr1;
	uint16_t  ts16;
	uint8_t   media;
	uint16_t  spf16;
	uint16_t  spt;
	uint16_t  nh;
	uint32_t  hs;
	uint32_t  ts32;
	uint32_t  spf32;
	uint32_t  flg;
	uint32_t  rc;
	char      vol[6];
	char      fst[8];
	char      dmy[20];
	char      fst2[8];
} __attribute__((packed)) bpb_t;

#endif
