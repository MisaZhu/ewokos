#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <ewoksys/timer.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/kernel_tic.h>

#include "pcm_lib.h"
#include "miyoo-dais.h"
#include "reg_ctrl.h"
#include "infinity_reg.h"

#define POLLING_DMA_WITH_SYS_TIMER0	1

typedef unsigned char   U8;
typedef signed char     S8;
typedef unsigned short  U16;
typedef short           S16;

#define KLOG		klog
#define UNUSED(v)	((void)v)
#define ENOMEM		(12)
#define	EINVAL		(22)
#define ETIMEDOUT	(116)
#define	EBADF		(9)
#define	EPIPE		(32)
#define	EBUSY		(16)

#define ARRAY_SIZE(A) ((sizeof(A)/sizeof(*A)))

#define WRITE_BYTE(_reg, _val)      (*((volatile U8*)(_reg)))  = (U8)(_val)
#define WRITE_WORD(_reg, _val)      (*((volatile U16*)(_reg))) = (U16)(_val)
#define WRITE_LONG(_reg, _val)      (*((volatile U32*)(_reg))) = (U32)(_val)
#define READ_BYTE(_reg)             (*(volatile U8*)(_reg))
#define READ_WORD(_reg)             (*(volatile U16*)(_reg))
#define READ_LONG(_reg)             (*(volatile U32*)(_reg))

#define TO_MIUSIZE(_x)			(_x >> 3)
#define FROM_MIUSIZE(_x) 		(_x << 3)

#define MSC313_BACH_DMA_SUB_CHANNEL_EN			0
#define MSC313_BACH_DMA_SUB_CHANNEL_ADDR		0x4
#define MSC313_BACH_DMA_SUB_CHANNEL_SIZE		0x8
#define MSC313_BACH_DMA_SUB_CHANNEL_TRIGGER		0xc
#define MSC313_BACH_DMA_SUB_CHANNEL_OVERRUNTHRESHOLD	0x10
#define MSC313_BACH_DMA_SUB_CHANNEL_UNDERRUNTHRESHOLD	0x14
#define MSC313_BACK_DMA_SUB_CHANNEL_LEVEL		0x18

#define MSC313_BACH_DMA_CHANNEL_CTRL0	0x0
#define MSC313_BACH_DMA_CHANNEL_CTRL8	0x20
#define MSC313_SUB_CHANNEL_READER	0
#define MSC313_SUB_CHANNEL_WRITER	1

#define MSC313_BACH_SR0_SEL		0x4
#define MSC313_BACH_DMA_TEST_CTRL7	0x1dc

/* Bank 1 */
#define REG_MUX0SEL	0xc
#define REG_SINEGEN	0x1d4
/* Bank 2 */
#define REG_DMA_INT	0x21c

/* Audio top */
#define REG_ATOP_OFFSET		0x1000
#define REG_ATOP_ANALOG_CTRL0	(REG_ATOP_OFFSET + 0)
#define REG_ATOP_ANALOG_CTRL1	(REG_ATOP_OFFSET + 0x4)
#define REG_ATOP_ANALOG_CTRL3	(REG_ATOP_OFFSET + 0xc)

#define PERIOD_BYTES_MIN 0x100
#define PRE_ALLOCATED_PCM_BUF_MAX_SIZE (16 * 1024)

#define MSC313_BACH		0x002a0400
#define MSC313_BACH_TOP		0x00206800
#define MSC313_BACH_CLK		0x028400

#define DELAY_INTERVAL_MS	(5 * 1000)
#define US_TO_MS(US)		(US / 1000)

static const int msc313_bach_src_rates[] = {
	8000,
	11025,
	12000, /* unsupported by alsa? */
	16000,
	22050,
	24000, /* unsupported by alsa? */
	32000,
	44100,
	48000,
};

struct msc313_bach_dma_channel;

struct msc313_bach_dma_sub_channel {
	struct msc313_bach_dma_channel *dma_channel;

	struct reg_field *count;
	struct reg_field *trigger;
	struct reg_field *init;
	struct reg_field *en;
	struct reg_field *addr_hi, *addr_lo;
	struct reg_field *size;
	struct reg_field *trigger_level;
	struct reg_field *overrunthreshold;
	struct reg_field *underrunthreshold;
	struct reg_field *level;

	struct snd_pcm_substream *substream;
};

struct msc313_bach_dma_channel {
	/*
	 * Enabling the channel might cause an interrupt
	 * and bust everything, this lock must be taken
	 * when doing something that might result in an
	 * interrupt and when handling interrupts.
	 */
	//spinlock_t lock;

	struct reg_field *rst;
	struct reg_field *en;
	struct reg_field *live_count_en;
	struct reg_field *rd_int_clear;
	struct reg_field *rd_empty_int_en;
	struct reg_field *rd_overrun_int_en;
	struct reg_field *rd_underrun_int_en;

	struct reg_field *wr_underrun_flag;
	struct reg_field *wr_overrun_flag;
	struct reg_field *rd_underrun_flag;
	struct reg_field *rd_overrun_flag;
	struct reg_field *rd_empty_flag;
	struct reg_field *wr_full_flag;
	struct reg_field *wr_localbuf_full_flag;
	struct reg_field *rd_localbuf_empty_flag;

	struct reg_field *dma_rd_mono;
	struct reg_field *dma_wr_mono;
	struct reg_field *dma_rd_mono_copy;

	struct msc313_bach_dma_sub_channel reader_writer[2];
};

struct msc313_bach {
	unsigned int clk;
	unsigned int  audiotop;
	unsigned int bach;

	/* Digital controls */
	struct reg_field *src2_sel;
	struct reg_field *src1_sel;

	/* DMA */
	struct reg_field *dma_int_en;
	struct msc313_bach_dma_channel dma_channels[1];

	/* Analog controls? */
	struct reg_field *codec_sel;

	/* Pre-allocated DMA memory when probe */
	unsigned int dma_areas;
};

struct msc313_bach_substream_runtime {
	struct msc313_bach_dma_sub_channel *sub_channel;
	bool running;

	int last_appl_ptr;

	/* Filled by prepare */
	ssize_t period_bytes;
	ssize_t max_inflight;
	unsigned max_level;

	/* Updated by queuing */
	ssize_t pending_bytes;
	ssize_t total_bytes;

	/* Updated by irq */
	unsigned irqs;
	unsigned empties;
	unsigned underruns;
	ssize_t processed_bytes;
};


static inline void delay(int32_t count)
{
	while (count > 0) count--;
}


static int msc313_bach_get_level(struct msc313_bach_dma_sub_channel *sub_channel)
{
	unsigned level;
	regmap_field_write(sub_channel->count, 1);
	delay(100);
	regmap_field_read(sub_channel->level, &level);
	delay(100);
	regmap_field_read(sub_channel->level, &level);
	regmap_field_write(sub_channel->count, 0);
	return level;
}

#if 0
static void msc313_bach_dump_dmactrl(struct msc313_bach *bach)
{
	unsigned ctrl0;
	int i;

	for(i = 0; i < 0x9; i++){
		regmap_read(bach->bach, 0x100 + (i * 4), &ctrl0);
		KLOG("ctrl%d: %04x\n", i, ctrl0);
	}
}
#endif

static void msc313_bach_pcm_dumpruntime(struct msc313_bach_substream_runtime *bach_runtime, int tag)
{
	struct msc313_bach_dma_sub_channel *sub_channel = bach_runtime->sub_channel;
	struct snd_pcm_runtime *runtime = sub_channel->substream->runtime;
	int level;
	uint64_t now_usec;

	kernel_tic(NULL, &now_usec);
	level = msc313_bach_get_level(sub_channel);

	klog("-->>bach_runtime_dump() IRQ:%d, irqs %d, empties %d, underruns %d, pending: %u, processed: %u, total: %u level:%d usec: %u\n",
		tag,
		bach_runtime->irqs,
		bach_runtime->empties,
		bach_runtime->underruns,
		(unsigned) bach_runtime->pending_bytes/4,
		(unsigned) bach_runtime->processed_bytes/4,
		(unsigned) bach_runtime->total_bytes/4,
		level,
		now_usec);

	klog("\t appl %d hw_ptr%d avai %d ready %d hw_base %d\n",
			runtime->status.appl_ptr,
			runtime->status.hw_ptr,
			play_avail(runtime),
			frames_ready(runtime),
			runtime->hw_ptr_base);
}


static struct snd_pcm_substream *tSubstream;
struct msc313_bach *tBach;
static int tStart;

static int msc313_bach_irq(int irq, void *data, int push_pending);
static int msc313_bach_pcm_ack(struct snd_soc_dai *dai,
			       struct snd_pcm_substream *substream);
static int register_irq_handle(struct msc313_bach *bach);
static int unregister_irq_handle(struct msc313_bach *bach);

#ifndef POLLING_DMA_WITH_SYS_TIMER0
static uint32_t uTimerId;

static void timer_handle(void)
{
	if (tStart == 1) {
		snd_pcm_lock(tSubstream);
		msc313_bach_irq(0, tBach, 1);
		snd_pcm_unlock(tSubstream);
	}
}
#endif

static void bach_irq_handle(uint32_t interrupt, uint32_t data)
{
	(void)interrupt;
	struct msc313_bach *tBach = (struct msc313_bach *)data;
	struct snd_pcm_runtime *runtime;
	uint64_t now_usec;
	uint64_t diff_1 = 0;
	uint64_t diff_2 = 0;
	uint64_t diff_3 = 0;

	kernel_tic(NULL, &now_usec);

	if (!tSubstream || tStart != 1) {
		goto TIMER_INT_END;
	}

	runtime = tSubstream->runtime;

	//now - last_call_end == interval between two calls
	diff_1 = now_usec - runtime->time_after;
	//now - last_call_start
	diff_2 = now_usec - runtime->time_pre;
	runtime->time_pre = now_usec;

	if (diff_1 > DELAY_INTERVAL_MS || diff_2 > 2 * DELAY_INTERVAL_MS) {
		if (tStart == 1) {
			snd_pcm_lock(tSubstream);
			msc313_bach_irq(0, tBach, 0);
			snd_pcm_unlock(tSubstream);
		}
	}

	kernel_tic(NULL, &now_usec);
	//irq_handle process cost time
	diff_3 = now_usec - runtime->time_pre;
	runtime->time_after = now_usec;

	if (US_TO_MS(diff_3) > 20) {
		KLOG("%s() [WARNING] tStart:%d diff_1:%u diff_2:%u diff_3:%u\n", __func__,
			tStart, US_TO_MS(diff_1), US_TO_MS(diff_2),  US_TO_MS(diff_3));
	}

TIMER_INT_END:
	return;
}


int mi_cpu_dai_open(struct snd_soc_dai *dai, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msc313_bach *bach = dai->private_data;
	int state = substream->runtime->status.state;
	struct msc313_bach_substream_runtime *bach_runtime;
	struct msc313_bach_dma_channel *dma_channel = &bach->dma_channels[0];

	bach_runtime = calloc(1, sizeof(*bach_runtime));
	if (!bach_runtime) {
		KLOG("%s() ERROR! No memory!\n", __func__);
		return -1;
	}

	runtime->private_data = bach_runtime;
	bach_runtime->sub_channel = &bach->dma_channels[0].reader_writer[0];
	bach->dma_channels[0].reader_writer[0].substream = substream;

	regmap_field_write(dma_channel->rst, 1);
	delay(1000);
	regmap_field_write(dma_channel->rst, 0);
	delay(1000);

	/* Setup default register config */
	regmap_field_write(dma_channel->live_count_en, 1);

	tBach = NULL;
	tStart = 0;
	tSubstream = substream;
	tBach = bach;

#ifdef POLLING_DMA_WITH_SYS_TIMER0
	/* register Timer0 IRQ handler for polling audio dma status */
	register_irq_handle(bach);
#else
	uTimerId = timer_set(10*1000, timer_handle);
	delay(100);
#endif
	KLOG("%s() state:%s\n", __func__, pcm_state_str(state));
	return 0;
}


int mi_cpu_dai_hw_params(struct snd_soc_dai *dai, struct snd_pcm_substream *substream)
{
	struct msc313_bach *bach = dai->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (runtime->dma_area != NULL) {
		return 0;
	}

	runtime->dma_area = (char *)bach->dma_areas;
	runtime->dma_addr = (unsigned int)syscall1(SYS_V2P, (int32_t)runtime->dma_area);
	runtime->dma_bytes = frame_to_bytes(runtime, runtime->buffer_size); //TODO
	/* clear allocated buffer */
	memset(runtime->dma_area, 0, runtime->dma_bytes);

	KLOG("%s() dma_addr:0x%x dma_area:0x%x dma_bytes:%d\n",__func__,
		runtime->dma_addr, runtime->dma_area, runtime->dma_bytes);
	return 0;
}

int mi_cpu_dai_hw_free(struct snd_soc_dai *dai, struct snd_pcm_substream *substream)
{
	UNUSED(dai);
	int state = substream->runtime->status.state;
	struct snd_pcm_runtime *runtime = substream->runtime;
	if (runtime->dma_area == NULL) {
		return 0;
	}

	runtime->dma_addr = 0;
	runtime->dma_area = NULL;
	runtime->dma_bytes = 0;
	KLOG("%s() state:%s\n", __func__, pcm_state_str(state));
	return 0;
}

int mi_cpu_dai_close(struct snd_soc_dai *dai, struct snd_pcm_substream *substream)
{
	struct msc313_bach *bach = dai->private_data;
	struct msc313_bach_dma_channel *dma_channel = &bach->dma_channels[0];
	int state = substream->runtime->status.state;

	bach->dma_channels[0].reader_writer[0].substream = NULL;
	regmap_field_write(dma_channel->rst, 1);

#ifdef POLLING_DMA_WITH_SYS_TIMER0
	/* detach Timer0 handle from the process */
	unregister_irq_handle(bach);
#else
	timer_remove(uTimerId);
#endif

	KLOG("%s() state:%s\n",__func__, pcm_state_str(state));
	return 0;
}

static int msc313_bach_queue_bytes(/*struct msc313_bach *bach,*/
				   struct snd_pcm_substream *substream,
				   ssize_t new_bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msc313_bach_substream_runtime *bach_runtime = runtime->private_data;
	struct msc313_bach_dma_sub_channel *sub_channel = bach_runtime->sub_channel;
	struct msc313_bach_dma_channel *dma_channel = sub_channel->dma_channel;
	unsigned trigbit;
	unsigned miu_trigger_level = TO_MIUSIZE(new_bytes);
	int target_level, old_level, new_level; // delay_level;

	old_level = msc313_bach_get_level(sub_channel);

	target_level = old_level + miu_trigger_level;
	if (target_level > (int)bach_runtime->max_level) {
		KLOG("%s() WARNING! target level, %d, is max %d\n", target_level, bach_runtime->max_level);
		return -EINVAL;
	}

	regmap_field_write(sub_channel->trigger_level, miu_trigger_level);
	regmap_field_read(sub_channel->trigger, &trigbit);
	regmap_field_write(sub_channel->trigger, ~trigbit);

	bach_runtime->total_bytes += new_bytes;
	bach_runtime->pending_bytes -= new_bytes;

	new_level = msc313_bach_get_level(sub_channel);
	/* frequently CPU cost may make underrun */
	//delay(10 * 1000);
	//delay_level = msc313_bach_get_level(sub_channel);

#if 0
	if (delay_level == new_level)
		KLOG("level didn't change after waiting, dma is probably stuck\n");
#endif

	/* should be safe to turn the underrun and empty irq back on */
	regmap_field_write(dma_channel->rd_underrun_int_en, 1);
	regmap_field_write(dma_channel->rd_empty_int_en, 1);

#if 0
	//msc313_bach_pcm_dumpruntime(bach_runtime);
	KLOG("++>>%s() who=%s total %d processed %d pending %d level %d\n", __func__,
		runtime->ack_count == 1 ? "APP" : "IRQ",
		bach_runtime->total_bytes/4,
		bach_runtime->processed_bytes/4,
		bach_runtime->pending_bytes/4,
		new_level);
#endif
	runtime->ack_count = 0;
	return 0;
}

static void msc313_bach_queue_pending(struct msc313_bach *bach,
				     struct snd_pcm_substream *substream,
				     struct msc313_bach_substream_runtime *bach_runtime) {
	UNUSED(bach);
	ssize_t new_queue_bytes = 0;
	ssize_t hw_cache_bytes = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	/*
	 * Trying to queue before the channel is running results in either
	 * the data not being queued or the dma locking up, so don't do that.
	 */
	if (!bach_runtime->running) {
		return;
	}

	/* Need at least a full period buffered */
	if (bach_runtime->pending_bytes <= 0) {
#if 0
		KLOG("%s() [WARN] who:%s pending_bytes:%d avail:%d\n",
			__func__,
			(runtime->ack_count == 1 ? "APP" : "IRQ"),
			bach_runtime->pending_bytes,
			play_avail(runtime));
#endif
		runtime->ack_count = 0;
		return;
	}

	if (bach_runtime->pending_bytes < bach_runtime->period_bytes) {
		return;
	}

	hw_cache_bytes = bach_runtime->total_bytes - bach_runtime->processed_bytes;

	if (hw_cache_bytes < 0) {
		KLOG("%s() [WARNING] Underrun! hw consumed bytes:%d > user queued bytes:%d\n", __func__,
			bach_runtime->processed_bytes, bach_runtime->total_bytes);
	}
	/* First queue, want two periods really */
	else if (hw_cache_bytes == 0) {
		new_queue_bytes = bach_runtime->period_bytes;
		/* Don't actually have enough */
		if (bach_runtime->pending_bytes < new_queue_bytes) {
			new_queue_bytes = bach_runtime->period_bytes;
		}
	}
	/* One period in the tank, queue one more */
	else if (hw_cache_bytes <= (int)bach_runtime->period_bytes) {
		new_queue_bytes = bach_runtime->period_bytes;
	} else {
		KLOG("%s() Not queue hw_cache_bytes:%d > period_bytes:%d\n", __func__,
			hw_cache_bytes, bach_runtime->period_bytes);
		new_queue_bytes = 0;
	}

	if (new_queue_bytes) {
		msc313_bach_queue_bytes(substream, new_queue_bytes);
	}
}

int mi_cpu_dai_prepare(struct snd_soc_dai *dai, struct snd_pcm_substream *substream)
{
	struct msc313_bach *bach = dai->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msc313_bach_substream_runtime *bach_runtime = runtime->private_data;
	struct msc313_bach_dma_sub_channel *sub_channel = bach_runtime->sub_channel;
	struct msc313_bach_dma_channel *dma_channel = sub_channel->dma_channel;
	unsigned stride = runtime->frame_bits / 8;
	unsigned miu_underrun_size, miu_buffer_size, miu_addr;
	unsigned mono = runtime->channels == 1 ? 1 : 0;
	unsigned int i;
	int ret;

	bach_runtime->last_appl_ptr = 0;
	bach_runtime->irqs = 0;
	bach_runtime->empties = 0;
	bach_runtime->underruns = 0;
	bach_runtime->pending_bytes = 0;
	bach_runtime->processed_bytes = 0;
	bach_runtime->total_bytes = 0;
	bach_runtime->period_bytes = frame_to_bytes(runtime, runtime->period_size);
	bach_runtime->max_inflight = bach_runtime->period_bytes * 2;

	miu_underrun_size = TO_MIUSIZE((bach_runtime->period_bytes + stride));
	miu_buffer_size = TO_MIUSIZE(runtime->dma_bytes);
	miu_addr = TO_MIUSIZE(runtime->dma_addr); //TODO
	bach_runtime->max_level = TO_MIUSIZE(bach_runtime->period_bytes * 2);

	/* This is needed to reset the buffer level */
	regmap_field_write(sub_channel->trigger, 0);
	regmap_field_write(sub_channel->init, 1);
	regmap_field_write(sub_channel->init, 0);

	KLOG("%s() dma addr %08x\n", __func__, (unsigned) runtime->dma_addr);

	regmap_field_write(sub_channel->addr_hi, miu_addr >> 12);
	regmap_field_write(sub_channel->addr_lo, miu_addr);
	regmap_field_write(sub_channel->size, miu_buffer_size);

	/* We want an interrupt underrun when we hit the last frame of the second period */
	regmap_field_write(sub_channel->underrunthreshold, miu_underrun_size);
	/* This shouldn't really matter,.. */
	regmap_field_write(sub_channel->overrunthreshold, 0);

	//
	regmap_field_write(dma_channel->dma_rd_mono, mono);
	regmap_field_write(dma_channel->dma_rd_mono_copy, mono);

	ret = -EINVAL;
	for (i = 0; i < ARRAY_SIZE(msc313_bach_src_rates); i++) {
		if (msc313_bach_src_rates[i] == (int)substream->runtime->rate) {
			regmap_field_write(bach->src1_sel, i);
			ret = 0;
			break;
		}
	}

	KLOG("%s() bach_runtime: period_size:%d period_bytes:%d strid:%d max_inflight:%d max_level:%d\n", __func__,
		runtime->period_size, bach_runtime->period_bytes, stride, bach_runtime->max_inflight, bach_runtime->max_level);
	return ret;
}

int mi_cpu_dai_trigger(struct snd_soc_dai *dai, struct snd_pcm_substream *substream, int cmd)
{
	UNUSED(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msc313_bach_substream_runtime *bach_runtime = runtime->private_data;
	struct msc313_bach_dma_sub_channel *sub_channel = bach_runtime->sub_channel;
	struct msc313_bach_dma_channel *dma_channel = sub_channel->dma_channel;

	KLOG(">>>>>>>%s():%d cmd:%s\n", __func__, __LINE__,
		(cmd == PCM_TRIGER_START ? "START" : "STOP"));

	switch (cmd) {
	case PCM_TRIGER_START:
		/*
		 * Enabling the channel can cause interrupts before we are ready,
		 * take the lock to force an irq to wait until we are finished.
		 */

		/* Clear any pending interrupts */
		regmap_field_write(dma_channel->rd_int_clear, 1);
		regmap_field_write(dma_channel->rd_int_clear, 0);

		/* Unmask interrupts */
		regmap_field_write(dma_channel->rd_overrun_int_en, 0);
		//regmap_field_write(dma_channel->rd_underrun_int_en, 1); //TODO
		//regmap_field_write(dma_channel->rd_empty_int_en, 1); //1

		/*
		 * Note: it seems like enabling the DMA channel must happen right
		 * before enabling the reader or the reader locks up.
		 */
		regmap_field_write(dma_channel->en, 1);
		delay(1000);

		/* Start playback */
		regmap_field_write(sub_channel->en, 1);
		delay(1000);
		bach_runtime->running = true;
		//msc313_bach_queue_pending(bach, substream, bach_runtime);

		/* start polling dma status */
		tStart = 1;
		break;
	case PCM_TRIGER_STOP:
		regmap_field_write(sub_channel->en, 0);
		delay(1000);
		regmap_field_write(dma_channel->en, 0);
		/* Mask interrupts */
		regmap_field_write(dma_channel->rd_underrun_int_en, 0);
		regmap_field_write(dma_channel->rd_empty_int_en, 0);
		bach_runtime->running = false;

		/* stop polling dma status */
		tStart = 0;
		break;
	default:
		KLOG("%s() Unkown cmd:%d\n", __func__, cmd);
	}

	return 0;
}

int mi_cpu_dai_pointer(struct snd_soc_dai *dai, struct snd_pcm_substream *substream)
{
	UNUSED(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msc313_bach_substream_runtime *bach_runtime = runtime->private_data;
	struct msc313_bach_dma_sub_channel *sub_channel = bach_runtime->sub_channel;
	//struct msc313_bach_dma_channel *dma_channel = sub_channel->dma_channel;
	//ssize_t inflight, done;
	int pos;
	int level;

	/*
	 * The amount of bytes the channel is currently munching through is the difference
	 * between the bytes queued and the number of bytes that have been processed
	 * according to an IRQ coming.
	 */
	//inflight = bach_runtime->total_bytes - bach_runtime->processed_bytes;
	/*
	 * The number of bytes that have been processed before the next IRQ comes will
	 * be roughly the number of bytes that are waiting to be confirmed by an IRQ
	 * minus current number of bytes the hardware says it still hasn't processed.
	 */
	level = msc313_bach_get_level(sub_channel);
	//done = inflight - FROM_MIUSIZE(level);

	bach_runtime->processed_bytes = bach_runtime->total_bytes - FROM_MIUSIZE(level);

	//TODO
	pos = bytes_to_frames(runtime, (bach_runtime->processed_bytes) % runtime->dma_bytes);

	return pos;
}

static int msc313_bach_pcm_ack(struct snd_soc_dai *dai,
			       struct snd_pcm_substream *substream)
{
	struct msc313_bach *bach = dai->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msc313_bach_substream_runtime *bach_runtime = runtime->private_data;
	int new_bytes = 0;

	new_bytes = frame_to_bytes(runtime, runtime->status.appl_ptr - bach_runtime->last_appl_ptr);
	bach_runtime->last_appl_ptr = runtime->status.appl_ptr;
	bach_runtime->pending_bytes += new_bytes;

	if (bach_runtime->pending_bytes >= frame_to_bytes(runtime, runtime->period_size)) {
		runtime->ack_count = 1;
		msc313_bach_queue_pending(bach, substream, bach_runtime);
	}

	return 0;
}

static int msc313_bach_irq(int irq, void *data, int push_pending)
{
	UNUSED(irq);
	UNUSED(push_pending);
	struct msc313_bach *bach = data;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(bach->dma_channels); i++) {
		struct msc313_bach_dma_channel *dma_channel = &bach->dma_channels[i];
		struct msc313_bach_substream_runtime *bach_runtime;
		struct snd_pcm_substream *substream;
		struct snd_pcm_runtime *runtime;
		unsigned empty, underrun; //overrun, irqflags;
		int level;

		level = msc313_bach_get_level(&bach->dma_channels[i].reader_writer[0]);
		/* Handle reader flags */
		substream = dma_channel->reader_writer[0].substream;

		runtime = substream->runtime;
		bach_runtime = runtime->private_data;
		bach_runtime->irqs++;

		if (!bach_runtime->running) {
			KLOG("%s() WARNING! bach is not RUNING!\n", __func__);
			return 0;
		}

		if (level > runtime->period_size / 2) {
			break;
		}

		regmap_field_read(bach->dma_channels[i].rd_empty_flag, &empty);
		regmap_field_read(bach->dma_channels[i].rd_underrun_flag, &underrun);

		if (!underrun || empty) {
			update_hw_ptr(substream, 1);
			msc313_bach_queue_pending(bach, substream, bach_runtime);
			//msc313_bach_pcm_dumpruntime(bach_runtime, 0);
			return 0;
		}

		/*
		 * It looks like the interrupt has to be ack'd by setting clear high
		 * it then needs to cleared to get it to trigger again.
		 */
		regmap_field_write(bach->dma_channels[i].rd_int_clear, 1);
		regmap_field_write(bach->dma_channels[i].rd_int_clear, 0);

		/*
		 * Sometimes interrupts happen at odd times before expected.
		 * Just ignore them.
		 */
		if (empty && (level == 0)) {
			bach_runtime->empties++;
			regmap_field_write(dma_channel->rd_empty_int_en, 0);
			bach_runtime->processed_bytes = bach_runtime->total_bytes;
			msc313_bach_pcm_dumpruntime(bach_runtime, 1);
			//snd_pcm_stop_xrun(substream); TODO
			KLOG("%s() WARNING X-Run happen level:%d\n", __func__, level);
			break;
		}

		if (underrun && empty == 0) {
			bach_runtime->underruns++;
			regmap_field_write(dma_channel->rd_underrun_int_en, 0);

			update_hw_ptr(substream, 1);
			//hw have consumed half buffer (but alsa not know)
			msc313_bach_queue_pending(bach, substream, bach_runtime);
			//msc313_bach_pcm_dumpruntime(bach_runtime, 1);
		}
	}

	return 0;
}

static struct snd_soc_dai_ops msc313_cpu_dai_ops = {
	.open = mi_cpu_dai_open,
	.close = mi_cpu_dai_close,
	.hw_params = mi_cpu_dai_hw_params,
	.hw_free = mi_cpu_dai_hw_free,
	.prepare = mi_cpu_dai_prepare,
	.trigger = mi_cpu_dai_trigger,
	.pointer = mi_cpu_dai_pointer,
	.ack =  msc313_bach_pcm_ack,
};

static void msc313_bach_the_horror(struct msc313_bach *bach)
{
	unsigned int ret_val = 0;
	regmap_write(bach->audiotop, 0x00, 0x00000A14);
	regmap_write(bach->audiotop, 0x04, 0x00000030);
	regmap_write(bach->audiotop, 0x08, 0x00000080);
	// power downs?
	regmap_write(bach->audiotop, 0x0C, 0x000001A5);
	// dac/adc resets
	regmap_write(bach->audiotop, 0x10, 0x00000000);
	regmap_write(bach->audiotop, 0x14, 0x00000000);
	regmap_write(bach->audiotop, 0x18, 0x00000000);
	regmap_write(bach->audiotop, 0x1C, 0x00000000);
	regmap_write(bach->audiotop, 0x20, 0x00003000);
	regmap_write(bach->audiotop, 0x24, 0x00000000);
	regmap_write(bach->audiotop, 0x28, 0x00000000);
	regmap_write(bach->audiotop, 0x2C, 0x00000000);
	regmap_write(bach->audiotop, 0x30, 0x00000000);
	regmap_write(bach->audiotop, 0x34, 0x00000000);
	regmap_write(bach->audiotop, 0x38, 0x00000000);
	regmap_write(bach->audiotop, 0x3C, 0x00000000);
	regmap_write(bach->audiotop, 0x40, 0x00000000);
	regmap_write(bach->audiotop, 0x44, 0x00000000);
	regmap_write(bach->audiotop, 0x48, 0x00000000);
	regmap_write(bach->audiotop, 0x4C, 0x00000000);
	regmap_write(bach->audiotop, 0x50, 0x00000000);
	regmap_write(bach->audiotop, 0x54, 0x00000000);
	regmap_write(bach->audiotop, 0x58, 0x00000000);
	regmap_write(bach->audiotop, 0x5C, 0x00000000);
	regmap_write(bach->audiotop, 0x60, 0x00000000);
	regmap_write(bach->audiotop, 0x64, 0x00000000);
	regmap_write(bach->audiotop, 0x68, 0x00000000);
	regmap_write(bach->audiotop, 0x6C, 0x00000000);
	regmap_write(bach->audiotop, 0x70, 0x00000000);
	regmap_write(bach->audiotop, 0x74, 0x00000000);
	regmap_write(bach->audiotop, 0x78, 0x00000000);
	regmap_write(bach->audiotop, 0x7C, 0x00000000);
	regmap_write(bach->audiotop, 0x80, 0x00000000);
	regmap_write(bach->audiotop, 0x84, 0x00003C1E);
	regmap_write(bach->audiotop, 0x88, 0x00000000);
	regmap_write(bach->audiotop, 0x8C, 0x00000000);
	regmap_write(bach->audiotop, 0x90, 0x00000000);
	regmap_write(bach->audiotop, 0x94, 0x00000000);
	regmap_write(bach->audiotop, 0x98, 0x00000000);
	regmap_write(bach->audiotop, 0x9C, 0x00000000);
	regmap_write(bach->audiotop, 0xA0, 0x00000000);
	regmap_write(bach->audiotop, 0xA4, 0x00000000);
	regmap_write(bach->audiotop, 0xA8, 0x00000000);
	regmap_write(bach->audiotop, 0xAC, 0x00000000);
	regmap_write(bach->audiotop, 0xB0, 0x00000000);
	regmap_write(bach->audiotop, 0xB4, 0x00000000);
	regmap_write(bach->audiotop, 0xB8, 0x00000000);
	regmap_write(bach->audiotop, 0xBC, 0x00000000);
	regmap_write(bach->audiotop, 0xC0, 0x00000000);
	regmap_write(bach->audiotop, 0xC4, 0x00000000);
	regmap_write(bach->audiotop, 0xC8, 0x00000000);
	regmap_write(bach->audiotop, 0xCC, 0x00000000);
	regmap_write(bach->audiotop, 0xD0, 0x00000000);
	regmap_write(bach->audiotop, 0xD4, 0x00000000);
	regmap_write(bach->audiotop, 0xD8, 0x00000000);
	regmap_write(bach->audiotop, 0xDC, 0x00000000);
	regmap_write(bach->audiotop, 0xE0, 0x00000000);
	regmap_write(bach->audiotop, 0xE4, 0x00000000);
	regmap_write(bach->audiotop, 0xE8, 0x00000000);
	regmap_write(bach->audiotop, 0xEC, 0x00000000);
	regmap_write(bach->audiotop, 0xF0, 0x00000000);
	regmap_write(bach->audiotop, 0xF4, 0x00000000);
	regmap_write(bach->audiotop, 0xF8, 0x00000000);
	regmap_write(bach->audiotop, 0xFC, 0x00000000);

	regmap_write(bach->bach, 0x00, 0x000089FF);
	regmap_write(bach->bach, 0x04, 0x0000FF88);
	regmap_write(bach->bach, 0x08, 0x00000003);
	regmap_write(bach->bach, 0x0C, 0x000019B4);
	regmap_write(bach->bach, 0x10, 0x0000F000);
	regmap_write(bach->bach, 0x14, 0x00008000);
	regmap_write(bach->bach, 0x18, 0x0000C09A);
	// MIX config?
	regmap_write(bach->bach, 0x1C, 0x0000555A);
	regmap_write(bach->bach, 0x20, 0x00000000);
	regmap_write(bach->bach, 0x24, 0x00000209);
	regmap_write(bach->bach, 0x28, 0x00000000);
	regmap_write(bach->bach, 0x2C, 0x0000007D);
	regmap_write(bach->bach, 0x30, 0x00000000);
	regmap_write(bach->bach, 0x34, 0x00000000);
	regmap_write(bach->bach, 0x38, 0x00003017);
	regmap_write(bach->bach, 0x3C, 0x00000002);
	regmap_write(bach->bach, 0x40, 0x00009400);
	regmap_write(bach->bach, 0x44, 0x00009400);
	regmap_write(bach->bach, 0x48, 0x00009400);
	regmap_write(bach->bach, 0x4C, 0x0000D400);
	regmap_write(bach->bach, 0x50, 0x00008400);
	regmap_write(bach->bach, 0x54, 0x0000D000);
	regmap_write(bach->bach, 0x58, 0x00009400);
	regmap_write(bach->bach, 0x5C, 0x00009400);
	regmap_write(bach->bach, 0x60, 0x00008400);
	regmap_write(bach->bach, 0x64, 0x00000000);
	regmap_write(bach->bach, 0x68, 0x00000000);
	regmap_write(bach->bach, 0x6C, 0x00000000);
	regmap_write(bach->bach, 0x70, 0x00000000);
	regmap_write(bach->bach, 0x74, 0x00000000);
	regmap_write(bach->bach, 0x78, 0x00000000);
	regmap_write(bach->bach, 0x7C, 0x00000000);
	regmap_write(bach->bach, 0x80, 0x00000005);
	//regmap_write(bach->bach, 0x84, 0x0000ECEC;
	regmap_write(bach->bach, 0x88, 0x00000007);
	regmap_write(bach->bach, 0x8C, 0x00000000);
	regmap_write(bach->bach, 0x90, 0x00000037);
	regmap_write(bach->bach, 0x94, 0x00000000);
	regmap_write(bach->bach, 0x98, 0x00000007);
	regmap_write(bach->bach, 0x9C, 0x00000000);
	regmap_write(bach->bach, 0xA0, 0x00000037);
	regmap_write(bach->bach, 0xA4, 0x00000000);
	regmap_write(bach->bach, 0xA8, 0x00000007);
	regmap_write(bach->bach, 0xAC, 0x00000000);
	regmap_write(bach->bach, 0xB0, 0x00000007);
	regmap_write(bach->bach, 0xB4, 0x00000000);
	regmap_write(bach->bach, 0xB8, 0x00000007);
	regmap_write(bach->bach, 0xBC, 0x00000000);
	regmap_write(bach->bach, 0xC0, 0x00000037);
	regmap_write(bach->bach, 0xC4, 0x00000000);
	regmap_write(bach->bach, 0xC8, 0x00000007);
	regmap_write(bach->bach, 0xCC, 0x00000000);
	regmap_write(bach->bach, 0xD0, 0x00000000);
	regmap_write(bach->bach, 0xD4, 0x00000000);
	regmap_write(bach->bach, 0xD8, 0x00000000);
	regmap_write(bach->bach, 0xDC, 0x00000000);
	regmap_write(bach->bach, 0xE0, 0x00000000);
	regmap_write(bach->bach, 0xE4, 0x00000000);
	regmap_write(bach->bach, 0xE8, 0x00000000);
	regmap_write(bach->bach, 0xEC, 0x00000000);
	regmap_write(bach->bach, 0xF0, 0x00000000);
	regmap_write(bach->bach, 0xF4, 0x00000000);
	regmap_write(bach->bach, 0xF8, 0x00000000);
	regmap_write(bach->bach, 0xFC, 0x00000000);
	//#regmap_write(bach->bach, 0x100, 0x00000496;
	//#regmap_write(bach->bach, 0x104, 0x00008000);
	regmap_write(bach->bach, 0x108, 0x00000FE8);
	regmap_write(bach->bach, 0x10C, 0x00002000);
	regmap_write(bach->bach, 0x110, 0x00000800);
	regmap_write(bach->bach, 0x114, 0x00000000);
	regmap_write(bach->bach, 0x118, 0x00001FE0);
	regmap_write(bach->bach, 0x11C, 0x00000F88);
	regmap_write(bach->bach, 0x120, 0x0000000F);
	regmap_write(bach->bach, 0x124, 0x00000000);
	regmap_write(bach->bach, 0x128, 0x00000000);
	regmap_write(bach->bach, 0x12C, 0x00000000);
	regmap_write(bach->bach, 0x130, 0x00000000);
	regmap_write(bach->bach, 0x134, 0x00000000);
	regmap_write(bach->bach, 0x138, 0x00000000);
	regmap_write(bach->bach, 0x13C, 0x00000000);
	regmap_write(bach->bach, 0x140, 0x00000000);
	regmap_write(bach->bach, 0x144, 0x00000000);
	regmap_write(bach->bach, 0x148, 0x00000000);
	regmap_write(bach->bach, 0x14C, 0x00000000);
	regmap_write(bach->bach, 0x150, 0x00000000);
	regmap_write(bach->bach, 0x154, 0x00000000);
	regmap_write(bach->bach, 0x158, 0x00000000);
	regmap_write(bach->bach, 0x15C, 0x00000000);
	regmap_write(bach->bach, 0x160, 0x00000000);
	regmap_write(bach->bach, 0x164, 0x00000000);
	regmap_write(bach->bach, 0x168, 0x00000000);
	regmap_write(bach->bach, 0x16C, 0x00000000);
	regmap_write(bach->bach, 0x170, 0x00000000);
	regmap_write(bach->bach, 0x174, 0x00000000);
	regmap_write(bach->bach, 0x178, 0x00000000);
	regmap_write(bach->bach, 0x17C, 0x00000000);
	regmap_write(bach->bach, 0x180, 0x00000000);
	regmap_write(bach->bach, 0x184, 0x00000000);
	regmap_write(bach->bach, 0x188, 0x00000000);
	regmap_write(bach->bach, 0x18C, 0x00000000);
	regmap_write(bach->bach, 0x190, 0x00000000);
	regmap_write(bach->bach, 0x194, 0x00000000);
	regmap_write(bach->bach, 0x198, 0x00000000);
	regmap_write(bach->bach, 0x19C, 0x00000000);
	regmap_write(bach->bach, 0x1A0, 0x00000000);
	regmap_write(bach->bach, 0x1A4, 0x00000000);
	regmap_write(bach->bach, 0x1A8, 0x00000000);
	regmap_write(bach->bach, 0x1AC, 0x00000000);
	regmap_write(bach->bach, 0x1B0, 0x00000000);
	regmap_write(bach->bach, 0x1B4, 0x00000000);
	regmap_write(bach->bach, 0x1B8, 0x00000000);
	regmap_write(bach->bach, 0x1BC, 0x00000000);
	regmap_write(bach->bach, 0x1C0, 0x00000000);
	regmap_write(bach->bach, 0x1C4, 0x00000000);
	regmap_write(bach->bach, 0x1C8, 0x00000000);
	regmap_write(bach->bach, 0x1CC, 0x000000E3);
	regmap_write(bach->bach, 0x1D0, 0x00000097);
	regmap_write(bach->bach, 0x1D4, 0x00000000);
	regmap_write(bach->bach, 0x1D8, 0x00000000);
	regmap_write(bach->bach, 0x1DC, 0x00000400);
	regmap_write(bach->bach, 0x1E0, 0x00000000);
	regmap_write(bach->bach, 0x1E4, 0x00000000);
	regmap_write(bach->bach, 0x1E8, 0x00000000);
	regmap_write(bach->bach, 0x1EC, 0x00000000);
	regmap_write(bach->bach, 0x1F0, 0x00000000);
	regmap_write(bach->bach, 0x1F4, 0x00000000);
	regmap_write(bach->bach, 0x1F8, 0x00000000);
	regmap_write(bach->bach, 0x1FC, 0x00000000);
	regmap_write(bach->bach, 0x200, 0x00000000);
	regmap_write(bach->bach, 0x204, 0x00000000);
	regmap_write(bach->bach, 0x208, 0x00000000);
	regmap_write(bach->bach, 0x20C, 0x00000000);
	regmap_write(bach->bach, 0x210, 0x00004000);
	regmap_write(bach->bach, 0x214, 0x00000100);
	regmap_write(bach->bach, 0x218, 0x000003E8);
	//#regmap_write(bach->bach, 0x21C, 0x00000002;
	regmap_write(bach->bach, 0x220, 0x00000000);
	regmap_write(bach->bach, 0x224, 0x00000000);
	regmap_write(bach->bach, 0x228, 0x00000000);
	regmap_write(bach->bach, 0x22C, 0x00000000);
	regmap_write(bach->bach, 0x230, 0x00000000);
	regmap_write(bach->bach, 0x234, 0x00000000);
	regmap_write(bach->bach, 0x238, 0x00000003);
	regmap_write(bach->bach, 0x23C, 0x00000000);
	regmap_write(bach->bach, 0x240, 0x000038C0);
	regmap_write(bach->bach, 0x244, 0x00003838);
	regmap_write(bach->bach, 0x248, 0x00000C04);
	regmap_write(bach->bach, 0x24C, 0x00001C14);
	regmap_write(bach->bach, 0x250, 0x00000001);
	regmap_write(bach->bach, 0x254, 0x00000000);
	regmap_write(bach->bach, 0x258, 0x00000003);
	regmap_write(bach->bach, 0x25C, 0x00000000);
	regmap_write(bach->bach, 0x260, 0x00000000);
	regmap_write(bach->bach, 0x264, 0x00000000);
	regmap_write(bach->bach, 0x268, 0x00000000);
	regmap_write(bach->bach, 0x26C, 0x00000202);
	regmap_write(bach->bach, 0x270, 0x00000000);
	regmap_write(bach->bach, 0x274, 0x00000000);
	regmap_write(bach->bach, 0x278, 0x00000000);
	regmap_write(bach->bach, 0x27C, 0x00000000);
	regmap_write(bach->bach, 0x280, 0x00000000);
	regmap_write(bach->bach, 0x284, 0x00000000);
	regmap_write(bach->bach, 0x288, 0x00000000);
	regmap_write(bach->bach, 0x28C, 0x00000000);
	regmap_write(bach->bach, 0x290, 0x00000000);
	regmap_write(bach->bach, 0x294, 0x00001234);
	regmap_write(bach->bach, 0x298, 0x00005678);
	regmap_write(bach->bach, 0x29C, 0x00000000);
	regmap_write(bach->bach, 0x2A0, 0x00000000);
	regmap_write(bach->bach, 0x2A4, 0x00000000);
	regmap_write(bach->bach, 0x2A8, 0x00000000);
	regmap_write(bach->bach, 0x2AC, 0x00000000);
	regmap_write(bach->bach, 0x2B0, 0x00000000);
	regmap_write(bach->bach, 0x2B4, 0x00000000);
	regmap_write(bach->bach, 0x2B8, 0x00000000);
	regmap_write(bach->bach, 0x2BC, 0x00000000);
	regmap_write(bach->bach, 0x2C0, 0x00000000);
	regmap_write(bach->bach, 0x2C4, 0x00000000);
	regmap_write(bach->bach, 0x2C8, 0x00000000);
	regmap_write(bach->bach, 0x2CC, 0x00000000);
	regmap_write(bach->bach, 0x2D0, 0x00000000);
	regmap_write(bach->bach, 0x2D4, 0x00000000);
	regmap_write(bach->bach, 0x2D8, 0x00000000);
	regmap_write(bach->bach, 0x2DC, 0x00000000);
	regmap_write(bach->bach, 0x2E0, 0x00000000);
	regmap_write(bach->bach, 0x2E4, 0x00000000);
	regmap_write(bach->bach, 0x2E8, 0x00000000);
	regmap_write(bach->bach, 0x2EC, 0x00000000);
	regmap_write(bach->bach, 0x2F0, 0x00000000);
	regmap_write(bach->bach, 0x2F4, 0x00000000);
	regmap_write(bach->bach, 0x2F8, 0x00000000);
	regmap_write(bach->bach, 0x2FC, 0x00000000);
	regmap_write(bach->bach, 0x300, 0x00000000);
	regmap_write(bach->bach, 0x304, 0x00000000);
	regmap_write(bach->bach, 0x308, 0x00000000);
	regmap_write(bach->bach, 0x30C, 0x00000000);
	regmap_write(bach->bach, 0x310, 0x00000000);
	regmap_write(bach->bach, 0x314, 0x00000000);
	regmap_write(bach->bach, 0x318, 0x00000000);
	regmap_write(bach->bach, 0x31C, 0x00000000);
	regmap_write(bach->bach, 0x320, 0x00000000);
	regmap_write(bach->bach, 0x324, 0x00000000);
	regmap_write(bach->bach, 0x328, 0x00000000);
	regmap_write(bach->bach, 0x32C, 0x00000001);
	regmap_write(bach->bach, 0x330, 0x00000000);
	regmap_write(bach->bach, 0x334, 0x00000000);
	regmap_write(bach->bach, 0x338, 0x00000000);
	regmap_write(bach->bach, 0x33C, 0x00000000);
	regmap_write(bach->bach, 0x340, 0x00000000);
	regmap_write(bach->bach, 0x344, 0x00000000);
	regmap_write(bach->bach, 0x348, 0x00000000);
	regmap_write(bach->bach, 0x34C, 0x00000000);
	regmap_write(bach->bach, 0x350, 0x00000000);
	regmap_write(bach->bach, 0x354, 0x00000000);
	regmap_write(bach->bach, 0x358, 0x00000000);
	regmap_write(bach->bach, 0x35C, 0x00000000);
	regmap_write(bach->bach, 0x360, 0x00000000);
	regmap_write(bach->bach, 0x364, 0x00000000);
	regmap_write(bach->bach, 0x368, 0x00000000);
	regmap_write(bach->bach, 0x36C, 0x00000000);
	regmap_write(bach->bach, 0x370, 0x00000000);
	regmap_write(bach->bach, 0x374, 0x00000000);
	regmap_write(bach->bach, 0x378, 0x00000000);
	regmap_write(bach->bach, 0x37C, 0x00000080);
	regmap_write(bach->bach, 0x380, 0x00000000);
	regmap_write(bach->bach, 0x384, 0x00000000);
	regmap_write(bach->bach, 0x388, 0x0000FF34);
	regmap_write(bach->bach, 0x38C, 0x00000000);
	regmap_write(bach->bach, 0x390, 0x00007FFF);
	regmap_write(bach->bach, 0x394, 0x00007FE9);
	regmap_write(bach->bach, 0x398, 0x00000000);
	regmap_write(bach->bach, 0x39C, 0x00000000);
	regmap_write(bach->bach, 0x3A0, 0x00000000);
	regmap_write(bach->bach, 0x3A4, 0x00000000);
	regmap_write(bach->bach, 0x3A8, 0x00000000);
	regmap_write(bach->bach, 0x3AC, 0x0000FEA6);
	regmap_write(bach->bach, 0x3B0, 0x0000019D);
	regmap_write(bach->bach, 0x3B4, 0x00000000);
	regmap_write(bach->bach, 0x3B8, 0x00000000);
	regmap_write(bach->bach, 0x3BC, 0x000078F4);
	regmap_write(bach->bach, 0x3C0, 0x00000000);
	regmap_write(bach->bach, 0x3C4, 0x00000000);
	regmap_write(bach->bach, 0x3C8, 0x000010D3);
	regmap_write(bach->bach, 0x3CC, 0x00000942);
	regmap_write(bach->bach, 0x3D0, 0x00000000);
	regmap_write(bach->bach, 0x3D4, 0x00000000);
	regmap_write(bach->bach, 0x3D8, 0x0000FDB6);
	regmap_write(bach->bach, 0x3DC, 0x0000F291);
	regmap_write(bach->bach, 0x3E0, 0x000078F4);
	regmap_write(bach->bach, 0x3E4, 0x00000000);
	regmap_write(bach->bach, 0x3E8, 0x00000000);
	regmap_write(bach->bach, 0x3EC, 0x00000000);
	regmap_write(bach->bach, 0x3F0, 0x00007FFF);
	regmap_write(bach->bach, 0x3F4, 0x00000000);
	regmap_write(bach->bach, 0x3F8, 0x00000001);
	regmap_write(bach->bach, 0x3FC, 0x00000000);
	regmap_write(bach->bach, 0x400, 0x00000000);
	regmap_write(bach->bach, 0x404, 0x00000021);
	regmap_write(bach->bach, 0x408, 0x00000000);
	regmap_write(bach->bach, 0x40C, 0x00000000);
	regmap_write(bach->bach, 0x410, 0x0000000A);
	regmap_write(bach->bach, 0x414, 0x00008000);
	regmap_write(bach->bach, 0x418, 0x0000011F);
	regmap_write(bach->bach, 0x41C, 0x00000000);
	regmap_write(bach->bach, 0x420, 0x00000000);
	regmap_write(bach->bach, 0x424, 0x00000000);
	regmap_write(bach->bach, 0x428, 0x00000000);
	regmap_write(bach->bach, 0x42C, 0x00000000);
	regmap_write(bach->bach, 0x430, 0x00000000);
	regmap_write(bach->bach, 0x434, 0x00000000);
	regmap_write(bach->bach, 0x438, 0x00000000);
	regmap_write(bach->bach, 0x43C, 0x0000FFFF);
	regmap_write(bach->bach, 0x440, 0x00000000);
	regmap_write(bach->bach, 0x444, 0x00000001);
	regmap_write(bach->bach, 0x448, 0x00008000);
	regmap_write(bach->bach, 0x44C, 0x00000001);
	regmap_write(bach->bach, 0x450, 0x00008000);
	regmap_write(bach->bach, 0x454, 0x00000000);
	regmap_write(bach->bach, 0x458, 0x00000000);
	regmap_write(bach->bach, 0x45C, 0x00000000);
	regmap_write(bach->bach, 0x460, 0x00000000);
	regmap_write(bach->bach, 0x464, 0x00000000);
	regmap_write(bach->bach, 0x468, 0x00000000);
	regmap_write(bach->bach, 0x46C, 0x00000000);
	regmap_write(bach->bach, 0x470, 0x00000000);
	regmap_write(bach->bach, 0x474, 0x00000000);
	regmap_write(bach->bach, 0x478, 0x00000000);
	regmap_write(bach->bach, 0x47C, 0x00000000);
	regmap_write(bach->bach, 0x480, 0x00000001);
	regmap_write(bach->bach, 0x484, 0x00000000);
	regmap_write(bach->bach, 0x488, 0x00000000);
	regmap_write(bach->bach, 0x48C, 0x00000000);
	regmap_write(bach->bach, 0x490, 0x00000000);
	regmap_write(bach->bach, 0x494, 0x00000000);
	regmap_write(bach->bach, 0x498, 0x00000000);
	regmap_write(bach->bach, 0x49C, 0x00000000);
	regmap_write(bach->bach, 0x4A0, 0x00000000);
	regmap_write(bach->bach, 0x4A4, 0x00000000);
	regmap_write(bach->bach, 0x4A8, 0x00000000);
	regmap_write(bach->bach, 0x4AC, 0x00000000);
	regmap_write(bach->bach, 0x4B0, 0x00000000);
	regmap_write(bach->bach, 0x4B4, 0x00000000);
	regmap_write(bach->bach, 0x4B8, 0x00000000);
	regmap_write(bach->bach, 0x4BC, 0x00000000);
	regmap_write(bach->bach, 0x4C0, 0x00000000);
	regmap_write(bach->bach, 0x4C4, 0x00000000);
	regmap_write(bach->bach, 0x4C8, 0x00000000);
	regmap_write(bach->bach, 0x4CC, 0x00000000);
	regmap_write(bach->bach, 0x4D0, 0x00000000);
	regmap_write(bach->bach, 0x4D4, 0x00000000);
	regmap_write(bach->bach, 0x4D8, 0x00000000);
	regmap_write(bach->bach, 0x4DC, 0x00000000);
	regmap_write(bach->bach, 0x4E0, 0x00000000);
	regmap_write(bach->bach, 0x4E4, 0x00000000);
	regmap_write(bach->bach, 0x4E8, 0x00000000);
	regmap_write(bach->bach, 0x4EC, 0x00000000);
	regmap_write(bach->bach, 0x4F0, 0x00000000);
	regmap_write(bach->bach, 0x4F4, 0x00000000);
	regmap_write(bach->bach, 0x4F8, 0x00000000);
	regmap_write(bach->bach, 0x4FC, 0x00000000);
	regmap_write(bach->bach, 0x500, 0x00000080);
	regmap_write(bach->bach, 0x504, 0x00000078);
	regmap_write(bach->bach, 0x508, 0x00000000);
	regmap_write(bach->bach, 0x50C, 0x00000000);
	regmap_write(bach->bach, 0x510, 0x00000000);
	regmap_write(bach->bach, 0x514, 0x00000000);
	regmap_write(bach->bach, 0x518, 0x00000000);
	regmap_write(bach->bach, 0x51C, 0x00000000);
	regmap_write(bach->bach, 0x520, 0x00000000);
	regmap_write(bach->bach, 0x524, 0x00000000);
	regmap_write(bach->bach, 0x528, 0x00000000);
	regmap_write(bach->bach, 0x52C, 0x00000000);
	regmap_write(bach->bach, 0x530, 0x00000000);
	regmap_write(bach->bach, 0x534, 0x00000000);
	regmap_write(bach->bach, 0x538, 0x00000000);
	regmap_write(bach->bach, 0x53C, 0x00000000);
	regmap_write(bach->bach, 0x540, 0x00000000);
	regmap_write(bach->bach, 0x544, 0x00000000);
	regmap_write(bach->bach, 0x548, 0x00000000);
	regmap_write(bach->bach, 0x54C, 0x00000000);
	regmap_write(bach->bach, 0x550, 0x00000000);
	regmap_write(bach->bach, 0x554, 0x00000000);
	regmap_write(bach->bach, 0x558, 0x00000000);
	regmap_write(bach->bach, 0x55C, 0x00000000);
	regmap_write(bach->bach, 0x560, 0x00000000);
	regmap_write(bach->bach, 0x564, 0x00000000);
	regmap_write(bach->bach, 0x568, 0x00000000);
	regmap_write(bach->bach, 0x56C, 0x00000000);
	regmap_write(bach->bach, 0x570, 0x00000000);
	regmap_write(bach->bach, 0x574, 0x00000000);
	regmap_write(bach->bach, 0x578, 0x00000000);
	regmap_write(bach->bach, 0x57C, 0x00000000);
	regmap_write(bach->bach, 0x580, 0x00000000);
	regmap_write(bach->bach, 0x584, 0x00000000);
	regmap_write(bach->bach, 0x588, 0x00000000);
	regmap_write(bach->bach, 0x58C, 0x00000000);
	regmap_write(bach->bach, 0x590, 0x00000000);
	regmap_write(bach->bach, 0x594, 0x00000000);
	regmap_write(bach->bach, 0x598, 0x00000000);
	regmap_write(bach->bach, 0x59C, 0x00000000);
	regmap_write(bach->bach, 0x5A0, 0x00000000);
	regmap_write(bach->bach, 0x5A4, 0x00000000);
	regmap_write(bach->bach, 0x5A8, 0x00000000);
	regmap_write(bach->bach, 0x5AC, 0x00000000);
	regmap_write(bach->bach, 0x5B0, 0x00000000);
	regmap_write(bach->bach, 0x5B4, 0x00000000);
	regmap_write(bach->bach, 0x5B8, 0x00000000);
	regmap_write(bach->bach, 0x5BC, 0x00000000);
	regmap_write(bach->bach, 0x5C0, 0x00000000);
	regmap_write(bach->bach, 0x5C4, 0x00000B0B);
	regmap_write(bach->bach, 0x5C8, 0x00000000);
	regmap_write(bach->bach, 0x5CC, 0x00004A4A);
	regmap_read(bach->bach, 0x5CC, &ret_val);
	KLOG("%s() bach_off:0x5CC val:%x\n",__func__, ret_val);
	regmap_read(bach->bach, 0x5CC, &ret_val);
	KLOG("%s() bach_off:0x5CC val:%x\n",__func__, ret_val);
	regmap_write(bach->bach, 0x5D0, 0x00004A4A);
	regmap_write(bach->bach, 0x5D4, 0x00000000);
	regmap_write(bach->bach, 0x5D8, 0x00000000);
	regmap_write(bach->bach, 0x5DC, 0x00004949);
	regmap_read(bach->bach, 0x5DC, &ret_val);
	KLOG("%s() bach_off:0x5DC val:%x\n",__func__, ret_val);
	regmap_write(bach->bach, 0x5E0, 0x00004949);
	regmap_read(bach->bach, 0x5E0, &ret_val);
	KLOG("%s() bach_off:0x5E0 val:%x\n",__func__, ret_val);
	regmap_write(bach->bach, 0x5E4, 0x00000000);
	regmap_write(bach->bach, 0x5E8, 0x00000000);
	regmap_write(bach->bach, 0x5EC, 0x00000000);
	regmap_write(bach->bach, 0x5F0, 0x00000000);
	regmap_write(bach->bach, 0x5F4, 0x00000000);
	regmap_write(bach->bach, 0x5F8, 0x00000000);
	regmap_write(bach->bach, 0x5FC, 0x00000000);
}


static int io_mmap(struct msc313_bach *bach)
{
	_mmio_base = mmio_map();
	bach->clk = _mmio_base + MSC313_BACH_CLK;
	bach->bach = _mmio_base + MSC313_BACH;
	bach->audiotop = _mmio_base + MSC313_BACH_TOP;
	KLOG("%s() _mmio_base:%x bach:%x audiotop:%x clk:%x\n", __func__,
		_mmio_base, bach->bach, bach->audiotop, bach->clk);
	return 0;
}

static int enable_clk(unsigned int clk_base)
{
	WRITE_WORD(clk_base + 0x0, 0x00c0);
	WRITE_BYTE(clk_base + 0x1c, 0x01);
	return 0;
}

static int pre_allocate_dma_buffer(struct msc313_bach *bach)
{
	//unsigned int vaddr = dma_phy_addr(dma_map(PRE_ALLOCATED_PCM_BUF_MAX_SIZE));
	unsigned int vaddr = (dma_map(PRE_ALLOCATED_PCM_BUF_MAX_SIZE));
	if (vaddr != 0) {
		bach->dma_areas = vaddr;
	} else {
		KLOG("%s() Error! pre_allocate_dma()\n", __func__);
		return -1;
	}
	return 0;
}

static int register_irq_handle(struct msc313_bach *bach)
{
	tStart = 0;

	static interrupt_handler_t handler;
	handler.data = 0;
	handler.handler = bach_irq_handle;
	sys_interrupt_setup(IRQ_TIMER0, &handler);
	/* Enable audio dma irq */
	//msc313_unmask_irq(42);
	//msc313_unmask_irq_polarity(42);
	KLOG("%s() irq_id:%d\n", __func__, IRQ_TIMER0);
	return 0;
}

static int unregister_irq_handle(struct msc313_bach *bach)
{
	tStart = 0;
	sys_interrupt_setup(IRQ_TIMER0, NULL);
	KLOG("%s() irq_id:%d\n", __func__, IRQ_TIMER0);
	return 0;
}

static int msc313_mem_dai_init(struct snd_soc_dai *dai)
{
	KLOG("+%s()\n", __func__);
	struct msc313_bach *bach;
	unsigned int i, j;
	struct reg_field src2_sel_field = REG_FIELD(MSC313_BACH_SR0_SEL, 0, 3);
	struct reg_field src1_sel_field = REG_FIELD(MSC313_BACH_SR0_SEL, 4, 7);
	struct reg_field dma1_rd_mono_field = REG_FIELD(MSC313_BACH_DMA_TEST_CTRL7, 15, 15);
	struct reg_field dma1_wr_mono_field = REG_FIELD(MSC313_BACH_DMA_TEST_CTRL7, 14, 14);
	struct reg_field dma1_rd_mono_copy_field = REG_FIELD(MSC313_BACH_DMA_TEST_CTRL7, 13, 13);
	struct reg_field dma_int_en_field = REG_FIELD(REG_DMA_INT, 1, 1);

	bach = calloc(1, sizeof(*bach));
	if(bach == NULL) {
		KLOG("%s() ERROR, No memory!\n", __func__);
		return -ENOMEM;
	}

	io_mmap(bach);
	enable_clk(bach->clk);
	/* pre allocated dma memory */
	pre_allocate_dma_buffer(bach);

	bach->src1_sel = regmap_field_alloc(bach->bach, src1_sel_field);
	bach->src2_sel = regmap_field_alloc(bach->bach, src2_sel_field);
	bach->dma_int_en = regmap_field_alloc(bach->bach, dma_int_en_field);

	for (i = 0; i < ARRAY_SIZE(bach->dma_channels); i++) {
		struct msc313_bach_dma_channel *chan = &bach->dma_channels[i];
		unsigned int chan_offset = 0x100 + (0x40 * i);
		struct reg_field chan_rst_field = REG_FIELD(chan_offset + MSC313_BACH_DMA_CHANNEL_CTRL0, 0, 0);
		struct reg_field chan_en_field = REG_FIELD(chan_offset + MSC313_BACH_DMA_CHANNEL_CTRL0, 1, 1);
		struct reg_field live_count_en_field = REG_FIELD(chan_offset + MSC313_BACH_DMA_CHANNEL_CTRL0, 2, 2);

		/* interrupt controls */
		struct reg_field chan_rd_int_clear_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL0, 8, 8);
		struct reg_field chan_rd_empty_int_en_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL0, 10, 10);
		struct reg_field chan_rd_overrun_int_en_field = REG_FIELD(chan_offset +
						MSC313_BACH_DMA_CHANNEL_CTRL0, 12, 12);
		struct reg_field chan_rd_underrun_int_en_field = REG_FIELD(chan_offset +
						MSC313_BACH_DMA_CHANNEL_CTRL0, 13, 13);

		/* flags */
		struct reg_field chan_wd_underrun_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 0, 0);
		struct reg_field chan_wd_overrun_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 1, 1);
		struct reg_field chan_rd_underrun_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 2, 2);
		struct reg_field chan_rd_overrun_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 3, 3);
		struct reg_field chan_rd_empty_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 4, 4);
		struct reg_field chan_wr_full_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 5, 5);
		struct reg_field chan_wr_localbuf_full_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 6, 6);
		struct reg_field chan_rd_localbuf_empty_flag_field = REG_FIELD(chan_offset +
				MSC313_BACH_DMA_CHANNEL_CTRL8, 7, 7);

		//spin_lock_init(&chan->lock);

		chan->rst = regmap_field_alloc(bach->bach, chan_rst_field);
		chan->en = regmap_field_alloc(bach->bach, chan_en_field);
		chan->live_count_en = regmap_field_alloc(bach->bach, live_count_en_field);
		chan->rd_int_clear = regmap_field_alloc(bach->bach, chan_rd_int_clear_field);
		chan->rd_empty_int_en = regmap_field_alloc(bach->bach, chan_rd_empty_int_en_field);
		chan->rd_overrun_int_en = regmap_field_alloc(bach->bach, chan_rd_overrun_int_en_field);
		chan->rd_underrun_int_en = regmap_field_alloc(bach->bach, chan_rd_underrun_int_en_field);

		chan->wr_underrun_flag = regmap_field_alloc(bach->bach, chan_wd_underrun_flag_field);
		chan->wr_overrun_flag = regmap_field_alloc(bach->bach, chan_wd_overrun_flag_field);
		chan->rd_underrun_flag = regmap_field_alloc(bach->bach, chan_rd_underrun_flag_field);
		chan->rd_overrun_flag = regmap_field_alloc(bach->bach, chan_rd_overrun_flag_field);
		chan->rd_empty_flag = regmap_field_alloc(bach->bach, chan_rd_empty_flag_field);
		chan->wr_full_flag = regmap_field_alloc(bach->bach, chan_wr_full_flag_field);
		chan->wr_localbuf_full_flag = regmap_field_alloc(bach->bach, chan_wr_localbuf_full_flag_field);
		chan->rd_localbuf_empty_flag = regmap_field_alloc(bach->bach, chan_rd_localbuf_empty_flag_field);

		if (i == 0) {
			chan->dma_rd_mono = regmap_field_alloc(bach->bach, dma1_rd_mono_field);
			chan->dma_wr_mono = regmap_field_alloc(bach->bach, dma1_wr_mono_field);
			chan->dma_rd_mono_copy = regmap_field_alloc(bach->bach, dma1_rd_mono_copy_field);
		}

		for (j = 0; j < ARRAY_SIZE(chan->reader_writer); j++){
			struct msc313_bach_dma_sub_channel *sub = &chan->reader_writer[j];
			unsigned int sub_chan_offset = chan_offset + 4 + (0x20 * j);

			sub->dma_channel = chan;

			/* Sub channel ctrl  fields */
			struct reg_field sub_chan_count_field = REG_FIELD(sub_chan_offset +
								MSC313_BACH_DMA_SUB_CHANNEL_EN, 12, 12);
			struct reg_field sub_chan_trigger_field = REG_FIELD(sub_chan_offset +
					MSC313_BACH_DMA_SUB_CHANNEL_EN, 13, 13);
			struct reg_field sub_chan_init_field = REG_FIELD(sub_chan_offset +
								MSC313_BACH_DMA_SUB_CHANNEL_EN, 14, 14);
			struct reg_field sub_chan_en_field = REG_FIELD(sub_chan_offset +
					MSC313_BACH_DMA_SUB_CHANNEL_EN, 15, 15);
			/* Buffer address */
			struct reg_field sub_chan_addr_lo_field = REG_FIELD(sub_chan_offset +
								MSC313_BACH_DMA_SUB_CHANNEL_EN, 0, 11);
			struct reg_field sub_chan_addr_hi_field = REG_FIELD(sub_chan_offset +
								MSC313_BACH_DMA_SUB_CHANNEL_ADDR, 0, 14);
			/* The rest .. */
			struct reg_field sub_chan_size_field = REG_FIELD(sub_chan_offset +
					MSC313_BACH_DMA_SUB_CHANNEL_SIZE, 0, 15);
			struct reg_field sub_chan_trigger_level_field = REG_FIELD(sub_chan_offset +
					MSC313_BACH_DMA_SUB_CHANNEL_TRIGGER, 0, 15);
			struct reg_field sub_chan_overrunthreshold_field = REG_FIELD(sub_chan_offset + 0x10, 0, 15);
			struct reg_field sub_chan_underrunthreshold_field = REG_FIELD(sub_chan_offset + 0x14, 0, 15);
			struct reg_field sub_chan_level_field = REG_FIELD(sub_chan_offset +
					MSC313_BACK_DMA_SUB_CHANNEL_LEVEL, 0, 15);

			sub->count = regmap_field_alloc(bach->bach, sub_chan_count_field);
			sub->trigger = regmap_field_alloc(bach->bach, sub_chan_trigger_field);
			sub->init = regmap_field_alloc(bach->bach, sub_chan_init_field);
			sub->en = regmap_field_alloc(bach->bach, sub_chan_en_field);
			sub->addr_hi = regmap_field_alloc(bach->bach, sub_chan_addr_hi_field);
			sub->addr_lo = regmap_field_alloc(bach->bach, sub_chan_addr_lo_field);
			sub->size = regmap_field_alloc(bach->bach, sub_chan_size_field);
			sub->trigger_level = regmap_field_alloc(bach->bach, sub_chan_trigger_level_field);
			sub->overrunthreshold = regmap_field_alloc(bach->bach, sub_chan_overrunthreshold_field);
			sub->underrunthreshold = regmap_field_alloc(bach->bach, sub_chan_underrunthreshold_field);
			sub->level = regmap_field_alloc(bach->bach, sub_chan_level_field);

			regmap_field_write(sub->en, 0);
		}

		regmap_field_write(chan->en, 0);
		regmap_field_write(chan->rst, 1);
	}

	dai->private_data = bach;

	regmap_field_write(bach->dma_int_en, 1);

	msc313_bach_the_horror(bach);

	KLOG("-%s() line:%d\n", __func__, __LINE__);
	return 0;
}


struct snd_soc_dai msc_dais[] = {
	[0] = {
		.name = "msc313-bach-cpu-dai",
		.type = DAI_TYPE_MEMIF,
		.ops = &msc313_cpu_dai_ops,
	}
};

static int msc313_add_dais_internal(struct snd_pcm *pcm, struct snd_soc_dai *dais, int dai_num)
{
	int index = 0;
	struct snd_soc_dai *mem_dai = NULL;

	if (dai_num == 0) {
		return 0;
	}

	for (index = 0; index < dai_num; index++) {
		struct snd_soc_dai *dai = &dais[index];
		list_init(&dai->list);
		snd_add_dai(pcm, dai, dai->type);

		if (dai->type == DAI_TYPE_MEMIF) {
			mem_dai = dai;
		}
	}

	if (mem_dai != NULL) {
		msc313_mem_dai_init(mem_dai);
	}
	return 0;
}

int msc313_add_dais(struct snd_pcm *pcm)
{
	return msc313_add_dais_internal(pcm, msc_dais, ARRAY_SIZE(msc_dais));
}