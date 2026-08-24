#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <g2d/g2d.h>
#include <graph/graph.h>
#include <font/font.h>
#include <ewoksys/kernel_tic.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <string>
#include <vector>

using namespace Ewok;

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
	if(img->pixels == (void*)-1) {
		img->pixels = NULL;
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

static uint32_t blend_over(uint32_t dst, uint32_t src) {
	uint8_t sa = (src >> 24) & 0xff;
	uint8_t sr = (src >> 16) & 0xff;
	uint8_t sg = (src >> 8) & 0xff;
	uint8_t sb = src & 0xff;
	uint8_t da = (dst >> 24) & 0xff;
	uint8_t dr = (dst >> 16) & 0xff;
	uint8_t dg = (dst >> 8) & 0xff;
	uint8_t db = dst & 0xff;
	uint32_t inv_a = 255 - sa;

	if(sa == 0)
		return dst;
	if(sa == 0xff)
		return src;

	da = da + (uint8_t)((255 - da) * sa / 255);
	dr = (uint8_t)((sr * sa + dr * inv_a) / 255);
	dg = (uint8_t)((sg * sa + dg * inv_a) / 255);
	db = (uint8_t)((sb * sa + db * inv_a) / 255);
	return make_color(da, dr, dg, db);
}

static uint32_t checker_color(uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
	uint8_t r = (uint8_t)((x * 255) / width);
	uint8_t g = (uint8_t)((y * 255) / height);
	uint8_t b = ((x / 16 + y / 16) & 1) ? 0xd0 : 0x30;
	return make_color(0xff, r, g, b);
}

static uint32_t alpha_circle_color(uint32_t width, uint32_t height, int32_t x, int32_t y) {
	int32_t cx = (int32_t)width / 2;
	int32_t cy = (int32_t)height / 2;
	int32_t radius = (int32_t)((width < height ? width : height) / 2) - 2;
	int32_t radius2 = radius * radius;
	int32_t dx = x - cx;
	int32_t dy = y - cy;
	int32_t d2 = dx * dx + dy * dy;
	uint8_t alpha = 0;

	if(d2 < radius2)
		alpha = (uint8_t)(255 - ((d2 * 255) / radius2));
	return make_color(alpha, 0xff, 0xe0, 0x20);
}

static void fill_checker(shm_image_t* img) {
	uint32_t x;
	uint32_t y;

	if(img == NULL || img->pixels == NULL)
		return;

	for(y = 0; y < img->height; y++) {
		for(x = 0; x < img->width; x++)
			img->pixels[y * img->width + x] = checker_color(img->width, img->height, x, y);
	}
}

static void fill_alpha_circle(shm_image_t* img) {
	int32_t x;
	int32_t y;

	if(img == NULL || img->pixels == NULL)
		return;

	for(y = 0; y < (int32_t)img->height; y++) {
		for(x = 0; x < (int32_t)img->width; x++)
			img->pixels[y * img->width + x] = alpha_circle_color(img->width, img->height, x, y);
	}
}

class G2DTestWidget: public Widget {
	/*测试在后台线程执行。所有中间结果先写入本地 TestCtx（不持锁），
	  跑完后短暂加锁发布给 UI，避免长时间持锁饿死窗口重绘。*/
	struct TestCtx {
		g2d_t g2d;
		g2d_info_t info;
		g2d_stats_t stats_before;
		g2d_stats_t stats_after;
		graph_t* preview;
		std::vector<std::string> logs;
		std::string firstFailure;
		int failures;
		bool pass;

		TestCtx() : preview(NULL), failures(0), pass(false) {
			memset(&g2d, 0, sizeof(g2d));
			memset(&info, 0, sizeof(info));
			memset(&stats_before, 0, sizeof(stats_before));
			memset(&stats_after, 0, sizeof(stats_after));
		}
	};

	//发布给 UI 的状态，仅在短暂持锁时读写。
	graph_t* preview;
	std::vector<std::string> logs;
	bool testDone;
	bool testPass;
	int failures;
	uint32_t fps;
	uint64_t fpsTick;
	std::string firstFailure;
	pthread_t benchThread;
	pthread_mutex_t stateLock;
	bool benchRunning;
	bool benchThreadStarted;

	void appendLog(TestCtx& ctx, const char* fmt, ...) {
		char buf[256];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		ctx.logs.push_back(buf);
	}

	void markFailure(TestCtx& ctx, const char* fmt, ...) {
		char buf[192];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
		if(ctx.firstFailure.empty())
			ctx.firstFailure = buf;
		ctx.failures++;
	}

	void clearPreview(TestCtx& ctx, uint32_t color) {
		if(ctx.preview != NULL)
			graph_clear(ctx.preview, color);
	}

	void fillPreview(TestCtx& ctx, const g2d_fill_req_t* req) {
		if(ctx.preview == NULL || req == NULL)
			return;
		graph_fill_rect(ctx.preview, req->rect.x, req->rect.y, req->rect.w, req->rect.h, req->color);
	}

	bool rotateValid(uint8_t rotate) {
		switch(rotate) {
		case G2D_ROTATE_0:
		case G2D_ROTATE_90:
		case G2D_ROTATE_180:
		case G2D_ROTATE_270:
			return true;
		default:
			return false;
		}
	}

	graph_t* cropPreviewSource(graph_t* src, const g2d_blit_req_t* req) {
		graph_t* cropped;

		if(src == NULL || req == NULL)
			return NULL;
		if(req->sx < 0 || req->sy < 0 || req->sw <= 0 || req->sh <= 0)
			return NULL;
		if(req->sx + req->sw > src->w || req->sy + req->sh > src->h)
			return NULL;

		cropped = graph_new(NULL, req->sw, req->sh);
		if(cropped == NULL || cropped->buffer == NULL) {
			if(cropped != NULL)
				graph_free(cropped);
			return NULL;
		}

		graph_blt(src,
				req->sx, req->sy, req->sw, req->sh,
				cropped,
				0, 0, req->sw, req->sh);
		return cropped;
	}

	graph_t* preparePreviewSource(graph_t* src, const g2d_blit_req_t* req) {
		graph_t* cropped;
		graph_t* rotated;

		if(src == NULL || req == NULL || !rotateValid(req->rotate))
			return NULL;
		cropped = cropPreviewSource(src, req);
		if(cropped == NULL)
			return NULL;
		if(req->rotate == G2D_ROTATE_0)
			return cropped;

		rotated = graph_rotate(cropped, req->rotate);
		graph_free(cropped);
		return rotated;
	}

	void blitPreview(TestCtx& ctx, const g2d_blit_req_t* req, graph_t* src, uint8_t alpha) {
		graph_t* prepared;

		if(ctx.preview == NULL || req == NULL || src == NULL)
			return;
		prepared = preparePreviewSource(src, req);
		if(prepared == NULL)
			return;
		if(alpha != 0) {
			if(prepared->w == req->dw && prepared->h == req->dh) {
				graph_blt_alpha(prepared,
						0, 0, prepared->w, prepared->h,
						ctx.preview,
						req->dx, req->dy, req->dw, req->dh,
						req->alpha);
			}
			else {
				graph_blt_fit_alpha(prepared,
						0, 0, prepared->w, prepared->h,
						ctx.preview,
						req->dx, req->dy, req->dw, req->dh,
						req->alpha);
			}
		}
		else {
			if(prepared->w == req->dw && prepared->h == req->dh) {
				graph_blt(prepared,
						0, 0, prepared->w, prepared->h,
						ctx.preview,
						req->dx, req->dy, req->dw, req->dh);
			}
			else {
				graph_blt_fit(prepared,
						0, 0, prepared->w, prepared->h,
						ctx.preview,
						req->dx, req->dy, req->dw, req->dh);
			}
		}
		graph_free(prepared);
	}

	bool initPreview(TestCtx& ctx) {
		if(ctx.info.width == 0 || ctx.info.height == 0)
			return false;
		if(ctx.preview != NULL && ctx.preview->w == (int32_t)ctx.info.width && ctx.preview->h == (int32_t)ctx.info.height)
			return true;
		if(ctx.preview != NULL) {
			graph_free(ctx.preview);
			ctx.preview = NULL;
		}
		ctx.preview = graph_new(NULL, ctx.info.width, ctx.info.height);
		return ctx.preview != NULL && ctx.preview->buffer != NULL;
	}

	bool verifyPixel(TestCtx& ctx, const char* label, int32_t x, int32_t y, uint32_t expected) {
		uint32_t actual = 0;
		int ret = g2d_get_pixel(&ctx.g2d, x, y, &actual);
		if(ret != 0) {
			appendLog(ctx, "FAIL %-18s readback (%d,%d) ret=%d", label, x, y, ret);
			markFailure(ctx, "%s readback error", label);
			return false;
		}
		if(actual != expected) {
			appendLog(ctx, "FAIL %-18s (%d,%d) act=0x%08x exp=0x%08x", label, x, y, actual, expected);
			markFailure(ctx, "%s pixel mismatch", label);
			return false;
		}
		appendLog(ctx, "PASS %-18s (%d,%d) 0x%08x", label, x, y, actual);
		return true;
	}

	bool verifyPreviewPixel(TestCtx& ctx, const char* label, int32_t x, int32_t y) {
		if(ctx.preview == NULL || x < 0 || y < 0 || x >= ctx.preview->w || y >= ctx.preview->h) {
			appendLog(ctx, "FAIL %-18s invalid ref point (%d,%d)", label, x, y);
			markFailure(ctx, "%s invalid ref", label);
			return false;
		}
		return verifyPixel(ctx, label, x, y, graph_get_pixel(ctx.preview, x, y));
	}

	bool verifyStats(TestCtx& ctx) {
		uint32_t clear_ops;
		uint32_t fill_ops;
		uint32_t blit_ops;
		uint32_t alpha_ops;
		uint32_t vc_present_ops;
		uint32_t soft_present_ops;

		if(g2d_get_stats(&ctx.g2d, &ctx.stats_after) != 0) {
			appendLog(ctx, "FAIL g2d_get_stats(after)");
			markFailure(ctx, "g2d_get_stats(after) failed");
			return false;
		}

		clear_ops = (ctx.stats_after.vc_clear_ops - ctx.stats_before.vc_clear_ops) +
				(ctx.stats_after.soft_clear_ops - ctx.stats_before.soft_clear_ops);
		fill_ops = (ctx.stats_after.vc_fill_ops - ctx.stats_before.vc_fill_ops) +
				(ctx.stats_after.soft_fill_ops - ctx.stats_before.soft_fill_ops);
		blit_ops = (ctx.stats_after.vc_blit_ops - ctx.stats_before.vc_blit_ops) +
				(ctx.stats_after.soft_blit_ops - ctx.stats_before.soft_blit_ops);
		alpha_ops = (ctx.stats_after.vc_alpha_blit_ops - ctx.stats_before.vc_alpha_blit_ops) +
				(ctx.stats_after.soft_alpha_blit_ops - ctx.stats_before.soft_alpha_blit_ops);
		vc_present_ops = ctx.stats_after.vc_present_ops - ctx.stats_before.vc_present_ops;
		soft_present_ops = ctx.stats_after.soft_present_ops - ctx.stats_before.soft_present_ops;

		appendLog(ctx, "stats total clear=%u fill=%u blit=%u alpha=%u present=%u",
				clear_ops, fill_ops, blit_ops, alpha_ops, vc_present_ops + soft_present_ops);
		if(clear_ops != 1 || fill_ops != 3 || blit_ops != 2 || alpha_ops != 2) {
			appendLog(ctx, "FAIL unexpected stats total");
			markFailure(ctx, "stats total mismatch");
			return false;
		}
		if(vc_present_ops != 0 || soft_present_ops != 0) {
			appendLog(ctx, "FAIL present counters changed vc_present=%u soft_present=%u",
					vc_present_ops, soft_present_ops);
			markFailure(ctx, "present counters changed");
			return false;
		}
		appendLog(ctx, "PASS stats totals");
		return true;
	}

	void runTest(TestCtx& ctx) {
		shm_image_t opaque_img;
		shm_image_t alpha_img;
		g2d_fill_req_t fill;
		g2d_blit_req_t blit;
		g2d_rect_t src_rect;
		graph_t opaque_graph;
		graph_t alpha_graph;
		uint32_t bg_color = 0xff101820;
		int32_t blit2_x;
		int32_t blit2_y;
		int32_t alpha2_x;
		int32_t alpha2_y;
		int ret;

		memset(&opaque_img, 0, sizeof(opaque_img));
		memset(&alpha_img, 0, sizeof(alpha_img));
		memset(&opaque_graph, 0, sizeof(opaque_graph));
		memset(&alpha_graph, 0, sizeof(alpha_graph));
		ctx.failures = 0;
		ctx.pass = false;
		ctx.firstFailure.clear();
		ctx.logs.clear();

		if(g2d_open("/dev/g2d", &ctx.g2d) != 0) {
			appendLog(ctx, "FAIL open /dev/g2d");
			markFailure(ctx, "open /dev/g2d failed");
			return;
		}

		ret = g2d_info(&ctx.g2d, &ctx.info);
		if(ret != 0) {
			appendLog(ctx, "FAIL g2d_info ret=%d", ret);
			markFailure(ctx, "g2d_info failed");
			g2d_close(&ctx.g2d);
			return;
		}
		appendLog(ctx, "g2d: %ux%u depth=%u backend=%u(%s)",
				ctx.info.width, ctx.info.height, ctx.info.depth,
				ctx.info.backend, g2d_backend_name(ctx.info.backend));

		if(!initPreview(ctx)) {
			appendLog(ctx, "FAIL preview alloc");
			markFailure(ctx, "preview alloc failed");
			g2d_close(&ctx.g2d);
			return;
		}

		if(g2d_get_stats(&ctx.g2d, &ctx.stats_before) != 0) {
			appendLog(ctx, "FAIL g2d_get_stats(before)");
			markFailure(ctx, "g2d_get_stats(before) failed");
			g2d_close(&ctx.g2d);
			return;
		}

		if(shm_image_create(&opaque_img, 0x47324420, 160, 120) != 0 ||
				shm_image_create(&alpha_img, 0x47324421, 128, 128) != 0) {
			appendLog(ctx, "FAIL create shm images");
			markFailure(ctx, "create shm images failed");
			shm_image_destroy(&alpha_img);
			shm_image_destroy(&opaque_img);
			g2d_close(&ctx.g2d);
			return;
		}

		fill_checker(&opaque_img);
		fill_alpha_circle(&alpha_img);
		graph_init(&opaque_graph, opaque_img.pixels, opaque_img.width, opaque_img.height);
		graph_init(&alpha_graph, alpha_img.pixels, alpha_img.width, alpha_img.height);

		ret = g2d_clear(&ctx.g2d, bg_color);
		appendLog(ctx, "g2d_clear: %d", ret);
		if(ret != 0)
			markFailure(ctx, "g2d_clear failed");
		clearPreview(ctx, bg_color);

		g2d_fill_req_init(&fill, g2d_rect(24, 24, 220, 120), 0xff204060);
		ret = g2d_fill_rect(&ctx.g2d, &fill);
		appendLog(ctx, "fill_rect #1: %d", ret);
		if(ret != 0)
			markFailure(ctx, "fill_rect #1 failed");
		fillPreview(ctx, &fill);

		g2d_fill_req_init(&fill, g2d_rect((int32_t)ctx.info.width - 180, 40, 140, 96), 0xff503040);
		ret = g2d_fill_rect(&ctx.g2d, &fill);
		appendLog(ctx, "fill_rect #2: %d", ret);
		if(ret != 0)
			markFailure(ctx, "fill_rect #2 failed");
		fillPreview(ctx, &fill);

		src_rect = g2d_rect(0, 0, (int32_t)opaque_img.width, (int32_t)opaque_img.height);
		g2d_blit_req_init(&blit,
				opaque_img.shm_id,
				opaque_img.size,
				opaque_img.width,
				opaque_img.height,
				opaque_img.stride,
				src_rect,
				g2d_rect(48, 72, (int32_t)opaque_img.width, (int32_t)opaque_img.height),
				0xff);
		ret = g2d_blit_shm(&ctx.g2d, &blit);
		appendLog(ctx, "blit_opaque: %d", ret);
		if(ret != 0)
			markFailure(ctx, "blit_opaque failed");
		blitPreview(ctx, &blit, &opaque_graph, 0);

		blit2_x = (int32_t)ctx.info.width - 280;
		blit2_y = 96;
		src_rect = g2d_rect(20, 16, 80, 60);
		g2d_blit_req_init_ex(&blit,
				opaque_img.shm_id,
				opaque_img.size,
				opaque_img.width,
				opaque_img.height,
				opaque_img.stride,
				src_rect,
				g2d_rect(blit2_x, blit2_y, 180, 140),
				0xff,
				G2D_ROTATE_90);
		ret = g2d_blit_shm(&ctx.g2d, &blit);
		appendLog(ctx, "blit_scale_rotate: %d", ret);
		if(ret != 0)
			markFailure(ctx, "blit_scale_rotate failed");
		blitPreview(ctx, &blit, &opaque_graph, 0);

		src_rect = g2d_rect(0, 0, (int32_t)alpha_img.width, (int32_t)alpha_img.height);
		g2d_blit_req_init(&blit,
				alpha_img.shm_id,
				alpha_img.size,
				alpha_img.width,
				alpha_img.height,
				alpha_img.stride,
				src_rect,
				g2d_rect((int32_t)ctx.info.width / 2, (int32_t)ctx.info.height / 2 - 32,
						(int32_t)alpha_img.width, (int32_t)alpha_img.height),
				0xff);
		ret = g2d_blit_alpha_shm(&ctx.g2d, &blit);
		appendLog(ctx, "blit_alpha: %d", ret);
		if(ret != 0)
			markFailure(ctx, "blit_alpha failed");
		blitPreview(ctx, &blit, &alpha_graph, 1);

		alpha2_x = (int32_t)ctx.info.width / 2 - 220;
		alpha2_y = (int32_t)ctx.info.height / 2 + 40;
		src_rect = g2d_rect(16, 16, 96, 80);
		g2d_blit_req_init_ex(&blit,
				alpha_img.shm_id,
				alpha_img.size,
				alpha_img.width,
				alpha_img.height,
				alpha_img.stride,
				src_rect,
				g2d_rect(alpha2_x, alpha2_y, 200, 120),
				0xff,
				G2D_ROTATE_270);
		ret = g2d_blit_alpha_shm(&ctx.g2d, &blit);
		appendLog(ctx, "blit_alpha_scale_rotate: %d", ret);
		if(ret != 0)
			markFailure(ctx, "blit_alpha_scale_rotate failed");
		blitPreview(ctx, &blit, &alpha_graph, 1);

		g2d_fill_req_init(&fill, g2d_rect(0, (int32_t)ctx.info.height - 36, (int32_t)ctx.info.width, 36), 0xff000000);
		ret = g2d_fill_rect(&ctx.g2d, &fill);
		appendLog(ctx, "fill_rect footer: %d", ret);
		if(ret != 0)
			markFailure(ctx, "fill_rect footer failed");
		fillPreview(ctx, &fill);

		verifyPixel(ctx, "clear_bg", 0, 0, bg_color);
		verifyPixel(ctx, "fill1_inside", 24, 24, 0xff204060);
		verifyPixel(ctx, "fill1_outside", 23, 24, bg_color);
		verifyPixel(ctx, "fill2_inside", (int32_t)ctx.info.width - 180, 40, 0xff503040);
		verifyPixel(ctx, "blit_opaque_tl", 48, 72, checker_color(opaque_img.width, opaque_img.height, 0, 0));
		verifyPixel(ctx, "blit_opaque_mid", 58, 92, checker_color(opaque_img.width, opaque_img.height, 10, 20));
		verifyPixel(ctx, "blit_opaque_br",
				48 + (int32_t)opaque_img.width - 1,
				72 + (int32_t)opaque_img.height - 1,
				checker_color(opaque_img.width, opaque_img.height, opaque_img.width - 1, opaque_img.height - 1));

		verifyPreviewPixel(ctx, "alpha_base_tl",
				(int32_t)ctx.info.width / 2,
				(int32_t)ctx.info.height / 2 - 32);
		verifyPreviewPixel(ctx, "alpha_base_mid",
				(int32_t)ctx.info.width / 2 + 64,
				(int32_t)ctx.info.height / 2 + 32);
		verifyPreviewPixel(ctx, "alpha_base_partial",
				(int32_t)ctx.info.width / 2 + 96,
				(int32_t)ctx.info.height / 2 + 32);
		verifyPreviewPixel(ctx, "blit_scale_rot_tl", blit2_x, blit2_y);
		verifyPreviewPixel(ctx, "blit_scale_rot_mid", blit2_x + 90, blit2_y + 70);
		verifyPreviewPixel(ctx, "blit_scale_rot_br", blit2_x + 179, blit2_y + 139);
		verifyPreviewPixel(ctx, "alpha_rot_tl", alpha2_x, alpha2_y);
		verifyPreviewPixel(ctx, "alpha_rot_mid", alpha2_x + 100, alpha2_y + 60);
		verifyPreviewPixel(ctx, "alpha_rot_br", alpha2_x + 199, alpha2_y + 119);
		verifyPreviewPixel(ctx, "footer_fill", 0, (int32_t)ctx.info.height - 1);
		verifyStats(ctx);

		ctx.pass = (ctx.failures == 0);
		appendLog(ctx, "summary: %s (%d failure)", ctx.pass ? "PASS" : "FAIL", ctx.failures);

		shm_image_destroy(&alpha_img);
		shm_image_destroy(&opaque_img);
		g2d_close(&ctx.g2d);
	}

	void publishResult(TestCtx& ctx) {
		/*仅在把后台结果交给 UI 时短暂持锁，避免长时间占用 stateLock。*/
		pthread_mutex_lock(&stateLock);
		if(preview != NULL)
			graph_free(preview);
		preview = ctx.preview;
		ctx.preview = NULL;
		logs.swap(ctx.logs);
		firstFailure.swap(ctx.firstFailure);
		failures = ctx.failures;
		testPass = ctx.pass;
		testDone = true;
		pthread_mutex_unlock(&stateLock);
	}

	static void* benchThreadEntry(void* p) {
		G2DTestWidget* self = (G2DTestWidget*)p;
		uint32_t loop_count = 0;
		uint64_t tick = kernel_tic_ms(0);

		while(self->benchRunning) {
			uint64_t now;
			TestCtx ctx;
			/*测试全程不持锁，跑完才短暂加锁发布，UI 重绘不会被饿死。*/
			self->runTest(ctx);
			self->publishResult(ctx);

			loop_count++;
			now = kernel_tic_ms(0);
			if(now - tick >= 1000) {
				uint64_t elapsed = now - tick;
				pthread_mutex_lock(&self->stateLock);
				if(elapsed != 0)
					self->fps = (uint32_t)((loop_count * 1000ULL) / elapsed);
				pthread_mutex_unlock(&self->stateLock);
				loop_count = 0;
				tick = now;
			}
		}
		return NULL;
	}

protected:
	void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
		(void)timerFPS;
		(void)timerSteps;
		update();
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		int32_t padding = 10;
		int32_t header_h = theme->basic.fontSize * 2 + 14;
		int32_t line_h = theme->basic.fontSize + 2;
		uint32_t status_color = 0xff555566;
		char status_text[128];
		char detail_text[224];
		/*调试信息移到右侧，占窗口宽度 1/3，预览占左侧剩余区域。*/
		int32_t right_w = r.w / 3;
		int32_t left_w = r.w - right_w;
		int32_t body_y = r.y + header_h;
		int32_t body_h = r.h - header_h;
		int32_t preview_x = r.x + padding;
		int32_t preview_y = body_y + padding;
		int32_t preview_w = left_w - padding * 2;
		int32_t preview_h = body_h - padding * 2;
		int32_t logs_x = r.x + left_w + padding;
		int32_t logs_y = body_y + padding;

		pthread_mutex_lock(&stateLock);

		if(!testDone) {
			status_color = 0xff3a5878;
			snprintf(status_text, sizeof(status_text), "xg2dtest RUNNING  fps=%u", fps);
			snprintf(detail_text, sizeof(detail_text), "running g2d API checks...");
		}
		else if(testPass) {
			status_color = 0xff1d7f3b;
			snprintf(status_text, sizeof(status_text), "xg2dtest TEST PASSED  fps=%u", fps);
			snprintf(detail_text, sizeof(detail_text), "all g2d API checks passed");
		}
		else {
			status_color = 0xff8f2d2d;
			snprintf(status_text, sizeof(status_text), "xg2dtest TEST FAILED  failures=%d  fps=%u", failures, fps);
			snprintf(detail_text, sizeof(detail_text), "first failure: %s",
					firstFailure.empty() ? "unknown" : firstFailure.c_str());
		}

		graph_fill_rect(g, r.x, r.y, r.w, r.h, 0xff1e1e24);
		graph_fill_rect(g, r.x, r.y, r.w, header_h, status_color);
		graph_draw_text_font(g, r.x + padding, r.y + 4, status_text,
				theme->getFont(), theme->basic.fontSize, 0xffffffff);
		graph_draw_text_font(g, r.x + padding, r.y + theme->basic.fontSize + 6, detail_text,
				theme->getFont(), theme->basic.fontSize, 0xffffffff);

		if(preview_w > 0 && preview_h > 0) {
			graph_fill_rect(g, preview_x - 1, preview_y - 1, preview_w + 2, preview_h + 2, 0xff555566);
			graph_fill_rect(g, preview_x, preview_y, preview_w, preview_h, 0xff0f0f14);
			if(preview != NULL)
				graph_blt_fit(preview, 0, 0, preview->w, preview->h, g,
						preview_x, preview_y, preview_w, preview_h);
		}

		graph_fill_rect(g, r.x + left_w, body_y, right_w, body_h, 0xff16161c);
		graph_set_clip(g, r.x + left_w, body_y, right_w, body_h);
		if(logs.empty()) {
			graph_draw_text_font(g, logs_x, logs_y, "testing...",
					theme->getFont(), theme->basic.fontSize, 0xff9aa0b0);
		}
		else {
			for(size_t i = 0; i < logs.size(); ++i) {
				int32_t y = logs_y + (int32_t)i * line_h;
				if(y + line_h > r.y + r.h)
					break;
				graph_draw_text_font(g, logs_x, y, logs[i].c_str(),
						theme->getFont(), theme->basic.fontSize, 0xffe8e8e8);
			}
		}
		graph_unset_clip(g);
		pthread_mutex_unlock(&stateLock);
	}

public:
	G2DTestWidget() {
		preview = NULL;
		testDone = false;
		testPass = false;
		failures = 0;
		fps = 0;
		fpsTick = kernel_tic_ms(0);
		firstFailure.clear();
		benchRunning = true;
		benchThreadStarted = false;
		pthread_mutex_init(&stateLock, NULL);
		if(pthread_create(&benchThread, NULL, benchThreadEntry, this) == 0)
			benchThreadStarted = true;
	}

	~G2DTestWidget() {
		benchRunning = false;
		if(benchThreadStarted)
			pthread_join(benchThread, NULL);
		pthread_mutex_destroy(&stateLock);
		if(preview != NULL)
			graph_free(preview);
	}
};

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	root->setType(Container::VERTICAL);
	root->setAlpha(false);
	win.setRoot(root);

	G2DTestWidget* tester = new G2DTestWidget();
	root->add(tester);

	win.open(&x, -1, -1, -1, 0, 0, "xg2dtest", XWIN_STYLE_NORMAL);
	win.max();
	win.setTimer(8);
	widgetXRun(&x, &win);
	return 0;
}
