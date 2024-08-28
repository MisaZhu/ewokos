#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <ewoksys/klog.h>
#include <sys/errno.h>

#include "pcm_lib.h"

#define KLOG 		klog
#define UNUSED(v)	((void)v)
//define EAGAIN		1 /* defined in errno.h */
#define ENOMEM		12
#define	EINVAL		22
#define ETIMEDOUT	23
#define	EBADF		9
#define	EPIPE		32
#define	EBUSY		16

#define WRITE_LONG(a, v)	(*(volatile int32_t *)(a) = (v))
#define READ_LONG(a)		(*(volatile int32_t *)(a))

#define TIME_USEC_TO_FRAMES(usec, rate) (uint32_t)((uint64_t)((usec) * (rate)) / (uint64_t)1000000)

int snd_card_new(struct snd_card **snd_card, const char *name)
{
	struct snd_card *card;
	card = (struct snd_card *)malloc(sizeof(*card));
	if (card == NULL) {
		return -ENOMEM;
	}

	memset(card, 0, sizeof(struct snd_card));
	list_init(&card->dev_list);
	list_init(&card->pcm_list);
	strncpy(card->name, name, 32);
	*snd_card = card;
	return 0;
}

int snd_card_free(struct snd_card *card)
{
	if (card == NULL) {
		return -EBADF;
	}

	struct listnode *node;
	struct listnode *node_next;
	list_for_each_safe(node, node_next, &card->pcm_list) {
		struct snd_pcm *pcm = node_to_item(node, struct snd_pcm, list);
		snd_pcm_free(pcm);
	}
	return 0;
}

int snd_card_register(struct snd_card *card)
{
	struct listnode *node;
	list_for_each(node, &card->dev_list) {
		struct snd_device *cur_dev = node_to_item(node, struct snd_device, list);
		cur_dev->dev_new(cur_dev);
	}
	return 0;
}

int snd_card_unregister(struct snd_card *card)
{
	struct listnode *node;
	list_for_each(node, &card->dev_list) {
		struct snd_device *cur_dev = node_to_item(node, struct snd_device, list);
		cur_dev->dev_free(cur_dev);
	}
	return 0;
}

int snd_card_info_print(struct snd_card *card)
{
	if (card == NULL) {
		return 0;
	}
	KLOG("-Card Info:\n%s\n", card->name);
	KLOG("\tName:%s\n", card->name);
	KLOG("\tPcm Num:%d\n", card->num_pcm);
	KLOG("\t-PCM Info:\n");
	if (card->num_pcm == 0) {
		KLOG("\t\tNo PCM!");
	} else {
		struct listnode *node;
		int index = 0;
		list_for_each(node, &card->pcm_list) {
			struct snd_pcm_substream *substream = NULL;
			struct snd_pcm *pcm = node_to_item(node, struct snd_pcm, list);
			substream = pcm->substream;
			KLOG("\t\tid:%d\n", index);
			KLOG("\t\tname:%s\n", pcm->name);
			if (substream != NULL) {
				KLOG("\t\tsubstream:%s, status:%s\n", substream->name,
							pcm_state_str(substream->runtime->status.state));
			}
		}
	}
	return 0;
}

static int snd_pcm_substream_new(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	substream = malloc(sizeof(*substream));
	if (substream == NULL) {
		return -ENOMEM;
	}

	memset(substream, 0, sizeof(*substream));
	pcm->substream = substream;
	substream->pcm = pcm;
	strncpy(substream->name, "substream-0", 32);

	struct snd_pcm_runtime *runtime;
	runtime = malloc(sizeof(*runtime));
	if (runtime == NULL) {
		free(substream);
		return -ENOMEM;
	}

	pthread_mutex_init(&substream->lock, NULL);
	memset(runtime, 0, sizeof(*runtime));
	return 0;
}

static int snd_pcm_substream_free(struct snd_pcm *pcm)
{
	if (pcm->substream == NULL) {
		return 0;
	}

	free(pcm->substream->runtime);
	pcm->substream->runtime = NULL;
	free(pcm->substream);
	pcm->substream = NULL;
	return 0;
}

int snd_pcm_lock(struct snd_pcm_substream *substream)
{
	pthread_mutex_lock(&substream->lock);
	return 0;
}

int snd_pcm_unlock(struct snd_pcm_substream *substream)
{
	pthread_mutex_unlock(&substream->lock);
	return 0;
}


static int snd_pcm_device_create(struct snd_device *device);
static int snd_pcm_device_free(struct snd_device *device);
static struct file_operation vdev_ops;

int snd_pcm_new(struct snd_card *card, int type, int id, struct snd_pcm **rpcm)
{
	struct snd_pcm *pcm;
	pcm = (struct snd_pcm*)malloc(sizeof(*pcm));
	if (pcm == NULL) {
		return -ENOMEM;
	}

	memset(pcm, 0, sizeof(*pcm));

	struct snd_device *device = malloc(sizeof(struct snd_device));
	if (device == NULL) {
		free(pcm);
		return -ENOMEM;
	}
	device->owner = pcm;
	device->param = (void*)&vdev_ops;
	device->type = SND_DEV_TYPE_PCM;
	device->dev_new = snd_pcm_device_create;
	device->dev_free = snd_pcm_device_free;

	int err = snd_pcm_substream_new(pcm);
	if (err != 0) {
		free(device);
		free(pcm);
		return -EINVAL;
	}

	pcm->type = type;
	pcm->id = id;
	snprintf(pcm->name, 32, "%s%c%d", "pcm", PCM_TYPE_TO_TAG(pcm->type), pcm->id);
	pcm->card = card;
	card->num_pcm++;
	list_init(&pcm->list);
	list_init(&pcm->dai_list);
	list_add_tail(&card->pcm_list, &pcm->list);
	list_add_tail(&card->dev_list, &device->list);
	*rpcm = pcm;
	return 0;
}

int snd_pcm_free(struct snd_pcm *pcm)
{
	if (pcm == NULL) {
		return 0;
	}

	snd_pcm_substream_free(pcm);
	list_remove(&pcm->list);
	pcm->card->num_pcm--;
	free(pcm);
	return 0;
}

int snd_set_pcm_ops(struct snd_pcm *pcm, struct snd_pcm_ops *ops)
{
	pcm->substream->ops = ops;
	return 0;
}

int snd_add_dai(struct snd_pcm *pcm, struct snd_soc_dai *dai, int dai_type)
{
	if (dai_type == DAI_TYPE_MEMIF) {
		list_add_head(&pcm->dai_list, &dai->list);
		pcm->mem_dai = dai;
	} else {
		list_add_tail(&pcm->dai_list, &dai->list);
	}
	return 0;
}

/***************** DAI operation ******************/
static int snd_dai_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai;
	struct listnode *node;
	list_for_each(node, &pcm->dai_list) {
		dai = node_to_item(node, struct snd_soc_dai, list);
		if (dai->ops->open != NULL) {
			dai->ops->open(dai, substream);
		}
	}
	return 0;
}

static int snd_dai_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai;
	struct listnode *node;
	list_for_each(node, &pcm->dai_list) {
		dai = node_to_item(node, struct snd_soc_dai, list);
		if (dai->ops->close != NULL) {
			dai->ops->close(dai, substream);
		}
	}
	return 0;
}

static int snd_dai_pcm_hw_params(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai;
	struct listnode *node;
	list_for_each(node, &pcm->dai_list) {
		dai = node_to_item(node, struct snd_soc_dai, list);
		if (dai->ops->hw_params != NULL) {
			dai->ops->hw_params(dai, substream);
		}
	}
	return 0;
}

static int snd_dai_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai;
	struct listnode *node;
	list_for_each(node, &pcm->dai_list) {
		dai = node_to_item(node, struct snd_soc_dai, list);
		if (dai->ops->hw_free != NULL) {
			dai->ops->hw_free(dai, substream);
		}
	}
	return 0;
}

static int snd_dai_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai;
	struct listnode *node;
	list_for_each(node, &pcm->dai_list) {
		dai = node_to_item(node, struct snd_soc_dai, list);
		if (dai->ops->prepare != NULL) {
			dai->ops->prepare(dai, substream);
		}
	}
	return 0;
}

static int snd_dai_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai;
	struct listnode *node;
	list_for_each(node, &pcm->dai_list) {
		dai = node_to_item(node, struct snd_soc_dai, list);
		if (dai->ops->trigger != NULL) {
			dai->ops->trigger(dai, substream, cmd);
		}
	}
	return 0;
}

static int snd_dai_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai = pcm->mem_dai;
	int pos;
	if (dai->ops->pointer) {
		pos = dai->ops->pointer(dai, substream);
	} else {
		pos = 0;
		KLOG("%s() Error! The DAI hasn't pointer() ops!\n", __func__);
	}
	return pos;
}

static int snd_dai_pcm_ack(struct snd_pcm_substream *substream)
{
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_dai *dai = pcm->mem_dai;
	int ret = 0;
	if (dai->ops->ack) {
		ret = dai->ops->ack(dai, substream);
	}
	return ret;
}

struct snd_pcm_ops soc_dai_pcm_ops = {
	.open = snd_dai_pcm_open,
	.close = snd_dai_pcm_close,
	.hw_params = snd_dai_pcm_hw_params,
	.hw_free = snd_dai_pcm_hw_free,
	.prepare = snd_dai_pcm_prepare,
	.trigger = snd_dai_pcm_trigger,
	.pointer = snd_dai_pcm_pointer,
	.ack = snd_dai_pcm_ack,
};

/**************** substream operations ******************/
static inline void delay(int32_t count)
{
	while (count > 0) count--;
}

int wait_avail(struct snd_pcm_substream *substream, int *ravail)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int32_t avail = 0;
	int try_count = 0;
	int err = 0;
	const int max_try_count = 3;

	while (1) {
		avail = play_avail(runtime);
		if (avail >= runtime->period_size) {
			break;
		}

		if (try_count++ >= max_try_count) {
			err  = -EAGAIN;
			break;
		}

		delay(100);

		//check pcm state again
		switch (runtime->status.state) {
		case PCM_STATE_XRUN: {
			err = -EPIPE;
			} break;
		case PCM_STATE_OPEN:
		case PCM_STATE_SETUP: {
			err = -EBADF;
			} break;
		case PCM_STATE_STOPED:{
			continue;
			}
		default:
			break;
		}
	}

	*ravail = avail;
	return err;
}

void snd_dump_substream(struct snd_pcm_substream *substream, int is_interrupt)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	KLOG("-%s() %s appl:%d hw_ptr:%d avail:%d hw_base:%d\n", __func__,
		(is_interrupt? "[IRQ]" : "[NM]"),
       	runtime->status.appl_ptr,
       	runtime->status.hw_ptr,
       	play_avail(runtime),
       	runtime->hw_ptr_base);
}


int update_hw_ptr(struct snd_pcm_substream *substream, int is_interrupt)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int pos = 0;
	int old_hw_ptr, new_hw_ptr, hw_base;
	int delta = 0;
	int avail = 0;
	int cross_boundary = 0;

#if 0 //TODO
	uint64_t now_usec;
	uint32_t d_Frames;
	kernel_tic(NULL, &now_usec);
	d_Frames = TIME_USEC_TO_FRAMES(now_usec - runtime->irq_usec, runtime->rate);
#endif

	old_hw_ptr = runtime->status.hw_ptr;
	if (is_interrupt) {
		pos = substream->ops->pointer(substream);
	}

	if (pos >= runtime->buffer_size) {
		KLOG("[DBG] Invalid position: pos=%d\n", pos);
		pos = 0;
	}

	//KLOG("[DBG] Invalid position: pos=%d delay_frames:%u\n", pos, d_Frames);

	hw_base = runtime->hw_ptr_base;
	new_hw_ptr = hw_base + pos;
    	avail = play_avail(runtime);

	if (is_interrupt) {
		if (new_hw_ptr < old_hw_ptr) {
			hw_base += runtime->buffer_size;
			if (hw_base >= runtime->boundary) {
				hw_base = 0;
				cross_boundary++;
			}
			new_hw_ptr = hw_base + pos;
		}
	} else {
		if ((new_hw_ptr < old_hw_ptr) &&
			(hw_base + runtime->buffer_size < runtime->status.appl_ptr)) {
			hw_base += runtime->buffer_size;
			if (hw_base >= runtime->boundary) {
				hw_base = 0;
				cross_boundary++;
			}
			new_hw_ptr = hw_base + pos;
		}
	}

	delta = new_hw_ptr - old_hw_ptr;
	if (delta < 0) {
		delta += runtime->boundary;
	}

	if (delta >= runtime->buffer_size) {
		KLOG("%s() [IRQ Delay] Interrupt:%d delta:%d > bufsize:%d new_hw_ptr=%d, old_hw_ptr=%d\n",
			__func__, is_interrupt, delta, runtime->buffer_size, new_hw_ptr, old_hw_ptr);
	}

	if (is_interrupt) {
		WRITE_LONG(&runtime->hw_ptr_base, hw_base);
	} else {
		if (hw_base != runtime->hw_ptr_base) {
			KLOG("---%s() [Warning] User want increase hw_base!\n",__func__);
		}
	}
	WRITE_LONG(&(runtime->status.hw_ptr), new_hw_ptr);

	avail = play_avail(runtime);
	runtime->irq_count++;

#if 0
	KLOG("%s() irq_count:%d avail:%d appl:%d hw_ptr:%d hw_base:%d old_hw:%d pos:%d\n",
		__func__,runtime->irq_count, avail,
		 runtime->status.appl_ptr, runtime->status.hw_ptr, runtime->hw_ptr_base, old_hw_ptr, pos);
#endif

	if ((avail >= runtime->stop_threshold) && (runtime->status.state == PCM_STATE_RUNNING)) {
		substream->ops->trigger(substream, PCM_TRIGER_STOP);
		runtime->status.state = PCM_STATE_XRUN;

        	KLOG("%s() [Warning] XRun happen! appl:%d hw_ptr:%d\n",
             		__func__, runtime->status.appl_ptr, runtime->status.hw_ptr);
	}

	return 0;
}

static int do_transfer(struct snd_pcm_runtime *runtime,
						int appl_off,
						const void *src_base,
						int off,
						int frames)
{
	char *hw_app_ptr = runtime->dma_area + frame_to_bytes(runtime, appl_off);
	char *user_ptr = (char *)src_base + frame_to_bytes(runtime, off);
	memcpy(hw_app_ptr, user_ptr, frame_to_bytes(runtime, frames));
	return 0;
}

int snd_pcm_substeam_write(struct snd_pcm_substream *substream, const void *source, int size)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int offset = 0;
	int avail = 0;
	int written = 0;
	int err = 0;

	snd_pcm_lock(substream);
	switch (runtime->status.state) {
	case PCM_STATE_PREPARE:
	case PCM_STATE_RUNNING:
		break;
	case PCM_STATE_XRUN:
		snd_pcm_unlock(substream);
		return -EPIPE;
	default:
		snd_pcm_unlock(substream);
		return -EBADF;
	}

#if 0
	if (runtime->status.state == PCM_STATE_RUNNING) {
		//KLOG(">>>>%s()\n",__func__);
		update_hw_ptr(substream, 0);
	}
#endif
	snd_pcm_unlock(substream);

	avail = play_avail(runtime);

	while(size > 0) {
		int copy_frames = 0;
		int to_end = 0;
		int appl_ptr = 0;
		int app_offset = 0;
		//int old_appl = 0;

		if (avail == 0) {
			err = wait_avail(substream, &avail);
			if (err < 0) {
				KLOG("%s() No avail timeout, ret=%d\n", __func__, err);
				break;
			}
		}

		copy_frames = size > avail ? avail : size;
		to_end = runtime->buffer_size - runtime->status.appl_ptr % runtime->buffer_size;
		if (copy_frames > to_end) {
			copy_frames = to_end;
		}

		app_offset = runtime->status.appl_ptr % runtime->buffer_size;
		//old_appl = runtime->status.appl_ptr;

		do_transfer(runtime, app_offset, source, offset, copy_frames);
		appl_ptr = runtime->status.appl_ptr + copy_frames;
		if (appl_ptr > runtime->boundary) {
			appl_ptr -= runtime->boundary;
		}
		offset += copy_frames;
		size -= copy_frames;
		written += copy_frames;
		avail -= copy_frames;

		//Acquire lock
		snd_pcm_lock(substream);

		WRITE_LONG(&(runtime->status.appl_ptr), appl_ptr);

		if ((runtime->status.state == PCM_STATE_PREPARE) && (frames_ready(runtime) >= runtime->start_threshold)) {
			int temp = substream->ops->trigger(substream, PCM_TRIGER_START);
			if (temp == 0) {
				set_pcm_state(substream, PCM_STATE_RUNNING);
			}
		}

		if ((get_pcm_state(substream) == PCM_STATE_RUNNING) && substream->ops->ack) {
			substream->ops->ack(substream);
		}
		//Release lock
		snd_pcm_unlock(substream);
	}
	//update pcm state TODO

	return (err < 0 ? err : written);
}

int snd_pcm_buf_avail(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int avail_bytes = 0;

	snd_pcm_lock(substream);
	switch (runtime->status.state) {
	case PCM_STATE_PREPARE:
	case PCM_STATE_RUNNING:
	case PCM_STATE_SETUP:
		break;
	case PCM_STATE_XRUN:
		snd_pcm_unlock(substream);
		return -EPIPE;
	default:
		snd_pcm_unlock(substream);
		return -EBADF;
	}

	avail_bytes = frame_to_bytes(runtime, play_avail(runtime));
    	snd_pcm_unlock(substream);

	return avail_bytes;
}

static int snd_pcm_open(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream = pcm->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;
	if (substream->open_count != 0) {
		KLOG("%s() device is busy! open_count=%d\n",
			__func__, substream->open_count);
		return -EBUSY;
	}

	substream->open_count = 1;

	/* TODO: attach substream to PCM dynamicly */
	memset(runtime, 0, sizeof(struct snd_pcm_runtime));

	runtime->status.state = PCM_STATE_OPEN;
	substream->private_data = substream->pcm->private_data;
	if (substream->ops->open != NULL) {
		err = substream->ops->open(substream);
	}

	if (err != 0) {
		set_pcm_state(substream, PCM_STATE_UNKOWN);
	}
	return 0;
}

static int snd_pcm_write1(struct snd_pcm *pcm,
		const void *data,
		int size,/* in bytes */
		int offset)
{
	struct snd_pcm_substream *substream  = pcm->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	if (!runtime->dma_area || !runtime->dma_addr) {
		KLOG("%s() ERROR, substream not allocate dma buffer!\n");
		return -EBADF;
	}

	if (size == 0 || offset != 0) {
		return 0;
	}

	if (runtime->status.state == PCM_STATE_PREPARE ||
		runtime->status.state == PCM_STATE_RUNNING) {
		ret = snd_pcm_substeam_write(substream, data, size / runtime->frame_size);
		if (ret > 0) {
			ret = ret * runtime->frame_size;
		}
	}
	return ret;
}


static void dump_pcm_runtime(struct snd_pcm_runtime *runtime)
{
	if (runtime != NULL) {
		KLOG("runtime bit:%d rate:%d channels:%d period_size:%d period_cnt:%d start_thres:%d stop_thres:%d\n",
			runtime->bit_depth,
			runtime->rate,
			runtime->channels,
			runtime->period_size,
			runtime->periods,
			runtime->start_threshold,
			runtime->stop_threshold);
	}
}

static int snd_pcm_hw_sw_parms(struct snd_pcm_substream* substream, struct pcm_config *config)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	switch(runtime->status.state) {
	case PCM_STATE_OPEN:
	case PCM_STATE_SETUP:
	case PCM_STATE_PREPARE:
		break;
	default:
		KLOG("%s() ERROR return! beacuse of state=%s\n",
			__func__, pcm_state_str(runtime->status.state));
		return -EBADF;
	}

	runtime->bit_depth = config->bit_depth;
	runtime->rate = config->rate;
	runtime->channels = config->channels;
	runtime->period_size = config->period_size;
	runtime->periods = config->period_count;
	runtime->buffer_size = runtime->period_size * runtime->periods;
	runtime->start_threshold = config->start_threshold;
	runtime->stop_threshold = config->stop_threshold;
	runtime->frame_bits = runtime->channels * runtime->bit_depth;
	if (runtime->stop_threshold == 0) {
		runtime->stop_threshold = runtime->buffer_size;
	}
	runtime->frame_size = runtime->channels * runtime->bit_depth / 8;
	runtime->boundary = runtime->buffer_size;
	if (runtime->boundary == 0) {
		KLOG("%s() ERROR! Invalid config!\n", __func__);
		return -EINVAL;
	}

	uint32_t temp = (uint32_t)runtime->buffer_size;
	while (temp * 2 <= (uint32_t)(0x7FFFFFFF - runtime->buffer_size)) {
		temp *= 2;
	}
	runtime->boundary = (int32_t)temp;

	if (substream->ops->hw_params) {
		err = substream->ops->hw_params(substream);
		if (err == 0) {
			set_pcm_state(substream, PCM_STATE_SETUP);
		}
	}

	if (err != 0) {
		if (substream->ops->hw_free) {
			substream->ops->hw_free(substream);
		}
		set_pcm_state(substream, PCM_STATE_OPEN);
	}

	dump_pcm_runtime(runtime);

	KLOG("%s() ret:%d state:%s\n", __func__, err, pcm_state_str(runtime->status.state));
	return err;
}

static int snd_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	switch(runtime->status.state) {
	case PCM_STATE_SETUP:
	case PCM_STATE_PREPARE:
	case PCM_STATE_STOPED:
		break;
	default:
		return -EBADF;
	}

	if (substream->ops->hw_free) {
		err = substream->ops->hw_free(substream);
	}

	set_pcm_state(substream, PCM_STATE_OPEN);
	return err;
}

static int snd_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	switch(runtime->status.state) {
	case PCM_STATE_SETUP:
	case PCM_STATE_PREPARE:
	case PCM_STATE_STOPED:
	case PCM_STATE_XRUN:
		break;
	case PCM_STATE_OPEN:
		return -EBADF;
	case PCM_STATE_RUNNING:
		return -EBUSY;
	default:
		return -EBADF;
	}

	if (!substream->ops->prepare) {
		KLOG("%s() ERROR! Driver hasn't prepare()\n", __func__);
		return -EBADF;
	}

	err = substream->ops->prepare(substream);
	if (err == 0) {
		runtime->status.appl_ptr = runtime->status.hw_ptr;
		set_pcm_state(substream, PCM_STATE_PREPARE);
	} else {
		runtime->hw_ptr_base = 0;
		runtime->status.appl_ptr = runtime->status.hw_ptr = 0;
		set_pcm_state(substream, PCM_STATE_SETUP);
	}
	//KLOG("%s() ret:%d state:%s\n", __func__, err, pcm_state_str(runtime->status.state));
	return err;
}

static int snd_pcm_release_substream(struct snd_pcm_substream *substream)
{
	int state;

	state = get_pcm_state(substream);
	if (state == PCM_STATE_RUNNING ||
		state == PCM_STATE_PREPARE) {
		if (substream->ops->trigger) {
			substream->ops->trigger(substream, PCM_TRIGER_STOP);
		}

		set_pcm_state(substream, PCM_STATE_STOPED);
		state = get_pcm_state(substream);
	}

	if (state == PCM_STATE_XRUN) {
		set_pcm_state(substream, PCM_STATE_STOPED);
		state = get_pcm_state(substream);
		KLOG("%s() change state:XRUN to STOPED!\n", __func__);
	}

	if (state == PCM_STATE_SETUP ||
		state == PCM_STATE_XRUN ||
		state ==  PCM_STATE_STOPED) {
		if (substream->ops->hw_free) {
			substream->ops->hw_free(substream);
		}

		set_pcm_state(substream, PCM_STATE_OPEN);
		state = get_pcm_state(substream);
	}

	if (state == PCM_STATE_OPEN) {
		if (substream->ops->close) {
			substream->ops->close(substream);
		}
		set_pcm_state(substream, PCM_STATE_UNKOWN);
	}

	return 0;
}

static int snd_pcm_release(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream = pcm->substream;

	snd_pcm_release_substream(substream);
	substream->open_count = 0;
	KLOG("%s()\n", __func__);
	return 0;
}


/***** File operations: Make vdevice conectted with PCM ***/
int fdev_open(int fd, int from_pid, fsinfo_t* info, int oflag, void* p)
{
	KLOG("fdev_open() fd:%d from:%d\n", fd, from_pid);
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(info);
	UNUSED(oflag);

	struct snd_pcm *pcm = (struct snd_pcm *)p;
	int err = snd_pcm_open(pcm);
	return err;
}


int fdev_close(int fd, int from_pid, fsinfo_t* info, void* p)
{
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(info);

	struct snd_pcm *pcm = (struct snd_pcm *)p;
	int err = 0;
	err = snd_pcm_release(pcm);
	KLOG("fdev_close() fd:%d from:%d\n", fd, from_pid);
	return err;
}


int fdev_write(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p)
{
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(info);

	struct snd_pcm *pcm = (struct snd_pcm *)p;
	int ret = snd_pcm_write1(pcm, buf, size, offset);
	if (ret < 0) {
		//KLOG("%s() wait_avail() break! err:%d bytes:%d\n", __func__, ret, size);
		return ret;
	} else if (ret != size) { /* written < size || written = 0 */
		KLOG("%s() WARNING written:%d != bytes:%d\n", __func__, ret, size);
		ret = size;
	}

	return ret;
}

int fdev_ctrl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p)
{
	UNUSED(from_pid);
	struct snd_pcm *pcm = (struct snd_pcm *)p;
	struct snd_pcm_substream *substream = pcm->substream;
	int result = 0;

	switch (cmd) {
	case CTRL_PCM_DEV_HW: {
		struct pcm_config config;
		memset(&config, 0, sizeof(struct pcm_config));
		proto_read_to(in, &config, sizeof(struct pcm_config));
		result = snd_pcm_hw_sw_parms(substream, &config);
		break;
	}
	case CTRL_PCM_DEV_HW_FREE:
		result = snd_pcm_hw_free(substream);
		break;
	case CTRL_PCM_DEV_PRPARE:
		result = snd_pcm_prepare(substream);
		break;
	case CTRL_PCM_BUF_AVAIL:
		result = snd_pcm_buf_avail(substream);
		break;
	default:
		KLOG("fdev_ctrl() error! unkown cmd:%d\n", cmd);
		return -EINVAL;
	}

	PF->addi(ret, result);
	return 0;
}

static struct file_operation vdev_ops = {
	.open = fdev_open,
	.close = fdev_close,
	.write = fdev_write,
	.dev_cntl = fdev_ctrl,
};

static int snd_pcm_device_create(struct snd_device *device)
{
	struct snd_pcm *pcm = (struct snd_pcm *)device->owner;
	struct file_operation *fops = (struct file_operation *)device->param;
	vdevice_t *vdev = malloc(sizeof(vdevice_t));
	if (vdev == NULL) {
		return -ENOMEM;
	}

	memset(vdev, 0, sizeof(vdevice_t));

	strcpy(vdev->name, "sound");
	vdev->open = fops->open;
	vdev->close = fops->close;
	vdev->dev_cntl = fops->dev_cntl;
	vdev->write = fops->write;

	vdev->extra_data = pcm;
	pcm->private_data = vdev;
	KLOG(">>>>>>%s() pcm-device register done!\n", __func__);
	return 0;
}

static int snd_pcm_device_free(struct snd_device *device)
{
	if (!device) {
		return 0;
	}

	struct snd_pcm *pcm = (struct snd_pcm *)device->owner;
	vdevice_t *vdev = pcm->private_data;
	if (vdev) {
		free(vdev);
	}
	return 0;
}