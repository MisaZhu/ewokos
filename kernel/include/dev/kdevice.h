#ifndef BASIC_DEVICE_H
#define BASIC_DEVICE_H

#include <charbuf.h>
#include <dev/device.h>

typedef struct st_dev {
	int32_t type;
	uint32_t state;

	int32_t (*ready_read)(struct st_dev* p);
	int32_t (*ready_write)(struct st_dev* p);
	int32_t (*op)(struct st_dev* dev, int32_t opcode, int32_t arg);

	struct {
		struct {		
			charbuf_t buffer;
			int32_t (*inputch)(struct st_dev* dev, int32_t loop);
			int32_t (*outputch)(struct st_dev* dev, int32_t c);
			int32_t (*read)(struct st_dev* dev, void* buf, uint32_t size);
			int32_t (*write)(struct st_dev* dev, const void* buf, uint32_t size);
		} ch;

		struct {
			void* data;
			uint32_t block_size;
			int32_t (*read_done)(struct st_dev* p, void* buf);
			int32_t (*write_done)(struct st_dev* p);
			int32_t (*read)(struct st_dev* dev, int32_t bid);
			int32_t (*write)(struct st_dev* dev, int32_t bid, const void* buf);
		} block;
	} io;
} dev_t;

extern void    dev_init(void);
extern dev_t*  get_dev(uint32_t type);
extern int32_t dev_ready_read(dev_t* dev);
extern int32_t dev_ready_write(dev_t* dev);
extern int32_t dev_op(dev_t* dev, int32_t opcode, int32_t arg);

/*return : -1 for error/closed, 0 for retry*/
extern int32_t dev_ch_read(dev_t* dev, void* data, uint32_t size);
/*return : -1 for error/closed, 0 for retry*/
extern int32_t dev_ch_write(dev_t* dev, void* data, uint32_t size);

extern int32_t dev_block_read(dev_t* dev, int32_t bid);
extern int32_t dev_block_read_done(dev_t* dev, char* buf);
extern int32_t dev_block_write(dev_t* dev, int32_t bid, const char* buf);
extern int32_t dev_block_write_done(dev_t* dev);

#endif
