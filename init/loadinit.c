#include <proc.h>
#include <kalloc.h>
#include <lib/string.h>

extern const char *init_img[];

/* We don't have file system yet, so the first elf-format process "init" will be encoded to a base64 in memory. 
after booted, the kernel will decode the elf and run it*/
static void b16decode(const char *input, int inputLen, char *output, int *outputLen)
{
	int i;
	for (i = 0; i < inputLen / 2; i++) {
		int low = input[2 * i] - 'A';
		int high = input[2 * i + 1] - 'A';
		output[i] = low | (high << 4);
	}

	*outputLen = inputLen / 2;
}

static void decodeImage(const char **program, char ***image,
		int *imagePageCount)
{
	int i = 0;
	(*image) = kalloc();

	for (i = 0; program[i]; i++) {
		int len = 0;
		(*image)[i] = kalloc();
		b16decode(program[i], strlen(program[i]),
				(*image)[i], &len);

		(*imagePageCount) = i + 1;
	}
}

static void freeImage(char **image, int imagePageCount)
{
	int i = 0;
	for (i = 0; i < imagePageCount; i++)
		kfree(image[i]);
	kfree(image);
}

bool loadInit(ProcessT *proc)
{
	bool loaded = false;
	char **processImage = NULL;
	int processImagePageCount = 0;

	decodeImage(init_img, &processImage,
			&processImagePageCount);
	loaded = procLoad(proc, processImage, processImagePageCount);
	freeImage(processImage, processImagePageCount);

	return loaded;
}

