
#include <stdio.h>
#include <sys/types.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>

#include "list.h"
#include "pcm_lib.h"
#include "miyoo-dais.h"

#define UNUSED(p) ((void)p)
#define KLOG klog

static struct snd_card *sound_card = 0;

static void enter_pcm_device_loop(struct snd_pcm *pcm, const char* dev_name)
{
	if (pcm == NULL) {
		return;
	}

	vdevice_t *vdev = (vdevice_t*)pcm->private_data;
	char mount_point[33] = {0};
	strncpy(mount_point, dev_name, 32);
	device_run(vdev, mount_point, FS_TYPE_CHAR, 0666);
}

int main(int argc, char *argv[])
{
	const char* dev_name = argc < 2 ? "/dev/sound":argv[1];
	int ret = 0;
	ret = snd_card_new(&sound_card, "ewokos sound card");
	if (ret != 0 || !sound_card) {
		KLOG("%s() snd_card_new() fail!, ret = %d\n",__func__, ret);
		return 0;
	}

	struct snd_pcm *pcm_playback;
	ret = snd_pcm_new(sound_card, PCM_TYPE_PLAYBACK, 0, &pcm_playback);
	if (ret != 0) {
		KLOG("%s() snd_pcm_new() fail, ret =%d\n",__func__, ret);
		return -1;
	}

	/* Add msc313 sound card dais on PCM */
	msc313_add_dais(pcm_playback);
	/* This is Soc sound card, contain DAIs */
	snd_set_pcm_ops(pcm_playback, &soc_dai_pcm_ops);

	ret = snd_card_register(sound_card);
	if (ret != 0) {
		KLOG("%s() snd_card_register() fail, ret=%d\n",__func__, ret);
		return -1;
	}

	snd_card_info_print(sound_card);

	enter_pcm_device_loop(pcm_playback, dev_name);

	return 0;
}