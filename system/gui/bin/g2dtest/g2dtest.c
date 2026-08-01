#include <g2d/g2d.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	int shm_id;
	uint32_t* pixels;
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
} shm_image_t;

static int shm_image_create(shm_image_t* img, key_t key, uint32_t width, uint32_t height) {
	if(img == NULL || width == 0 || height == 0)
		return -1;

	memset(img, 0, sizeof(*img));
	img->stride = width * 4;
	img->size = img->stride * height;
	img->width = width;
	img->height = height;
	img->shm_id = shmget(key, img->size, 0666 | IPC_CREAT);
	if(img->shm_id < 0)
		return -1;
	img->pixels = (uint32_t*)shmat(img->shm_id, 0, 0);
	if(img->pixels == NULL) {
		img->shm_id = -1;
		return -1;
	}
	return 0;
}

static void shm_image_destroy(shm_image_t* img) {
	if(img == NULL)
		return;
	if(img->pixels != NULL)
		shmdt(img->pixels);
	memset(img, 0, sizeof(*img));
	img->shm_id = -1;
}

static uint32_t make_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void fill_checker(shm_image_t* img) {
	uint32_t x;
	uint32_t y;

	if(img == NULL || img->pixels == NULL)
		return;

	for(y = 0; y < img->height; y++) {
		for(x = 0; x < img->width; x++) {
			uint8_t r = (uint8_t)((x * 255) / img->width);
			uint8_t g = (uint8_t)((y * 255) / img->height);
			uint8_t b = ((x / 16 + y / 16) & 1) ? 0xd0 : 0x30;
			img->pixels[y * img->width + x] = make_color(0xff, r, g, b);
		}
	}
}

static void fill_alpha_circle(shm_image_t* img) {
	int32_t x;
	int32_t y;
	int32_t cx;
	int32_t cy;
	int32_t radius;
	int32_t radius2;

	if(img == NULL || img->pixels == NULL)
		return;

	cx = (int32_t)img->width / 2;
	cy = (int32_t)img->height / 2;
	radius = (int32_t)((img->width < img->height ? img->width : img->height) / 2) - 2;
	radius2 = radius * radius;

	for(y = 0; y < (int32_t)img->height; y++) {
		for(x = 0; x < (int32_t)img->width; x++) {
			int32_t dx = x - cx;
			int32_t dy = y - cy;
			int32_t d2 = dx * dx + dy * dy;
			uint8_t alpha = 0;
			if(d2 < radius2) {
				alpha = (uint8_t)(255 - ((d2 * 255) / radius2));
			}
			img->pixels[y * img->width + x] = make_color(alpha, 0xff, 0xe0, 0x20);
		}
	}
}

int main(int argc, char** argv) {
	g2d_t g2d;
	g2d_info_t info;
	g2d_fill_req_t fill;
	g2d_blit_req_t blit;
	shm_image_t opaque_img;
	shm_image_t alpha_img;
	g2d_rect_t src_rect;
	int ret;

	(void)argc;
	(void)argv;

	if(g2d_open("/dev/g2d", &g2d) != 0) {
		printf("open /dev/g2d failed\n");
		return -1;
	}

	ret = g2d_info(&g2d, &info);
	if(ret != 0) {
		printf("g2d_info failed\n");
		return -1;
	}

	printf("g2d: %ux%u depth=%u backend=%u\n",
			info.width, info.height, info.depth, info.backend);

	if(shm_image_create(&opaque_img, 0x47324410, 160, 120) != 0) {
		printf("create opaque shm failed\n");
		return -1;
	}
	if(shm_image_create(&alpha_img, 0x47324411, 128, 128) != 0) {
		printf("create alpha shm failed\n");
		shm_image_destroy(&opaque_img);
		return -1;
	}

	fill_checker(&opaque_img);
	fill_alpha_circle(&alpha_img);

	g2d_clear(&g2d, 0xff101820);

	g2d_fill_req_init(&fill, g2d_rect(24, 24, 220, 120), 0xff204060);
	g2d_fill_rect(&g2d, &fill);

	g2d_fill_req_init(&fill, g2d_rect((int32_t)info.width - 180, 40, 140, 96), 0xff503040);
	g2d_fill_rect(&g2d, &fill);

	src_rect = g2d_rect(0, 0, (int32_t)opaque_img.width, (int32_t)opaque_img.height);
	g2d_blit_req_init(&blit,
			opaque_img.shm_id,
			opaque_img.size,
			opaque_img.width,
			opaque_img.height,
			opaque_img.stride,
			src_rect,
			g2d_rect(48, 72, (int32_t)opaque_img.width * 2, (int32_t)opaque_img.height * 2),
			0xff);
	g2d_blit_shm(&g2d, &blit);

	src_rect = g2d_rect(0, 0, (int32_t)alpha_img.width, (int32_t)alpha_img.height);
	g2d_blit_req_init(&blit,
			alpha_img.shm_id,
			alpha_img.size,
			alpha_img.width,
			alpha_img.height,
			alpha_img.stride,
			src_rect,
			g2d_rect((int32_t)info.width / 2, (int32_t)info.height / 2 - 32,
					(int32_t)alpha_img.width, (int32_t)alpha_img.height),
			0xc0);
	g2d_blit_alpha_shm(&g2d, &blit);

	g2d_fill_req_init(&fill, g2d_rect(0, (int32_t)info.height - 36, (int32_t)info.width, 36), 0xff000000);
	g2d_fill_rect(&g2d, &fill);

	ret = g2d_present(&g2d);
	printf("g2d_present: %d\n", ret);
	usleep(50000);

	shm_image_destroy(&alpha_img);
	shm_image_destroy(&opaque_img);
	g2d_close(&g2d);
	return ret;
}
