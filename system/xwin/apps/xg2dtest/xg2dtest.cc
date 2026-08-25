#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <g2dclient/g2dclient.h>
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
			if(d2 < radius2)
				alpha = (uint8_t)(255 - ((d2 * 255) / radius2));
			img->pixels[y * img->width + x] = make_color(alpha, 0xff, 0xe0, 0x20);
		}
	}
}

class G2DTestWidget: public Widget {
	/*测试在后台线程循环执行：一轮 = 一组命令级检查（返回码 +
	  g2d_info 尺寸验证）。所有中间结果先写本地 TestCtx（不持锁），
	  跑完短暂加锁发布给 UI，避免长时间持锁饿死窗口重绘。
	  设备没有像素回读接口，预览只是把同一组命令在本地 graph 上
	  镜像一遍，让画面可见。*/
	struct TestCtx {
		g2d_t g2d;
		g2d_info_t info;
		uint32_t w0;      /*初始目标面尺寸*/
		uint32_t h0;
		graph_t* preview;
		std::vector<std::string> logs;
		std::string firstFailure;
		int failures;
		bool pass;

		TestCtx() : preview(NULL), failures(0), pass(false) {
			memset(&g2d, 0, sizeof(g2d));
			memset(&info, 0, sizeof(info));
			w0 = 0;
			h0 = 0;
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

	void checkRet(TestCtx& ctx, const char* label, int ret, bool expect_ok) {
		bool ok = expect_ok ? (ret == 0) : (ret != 0);
		if(!ok) {
			appendLog(ctx, "FAIL %-18s ret=%d (expect %s)", label, ret, expect_ok ? "ok" : "error");
			markFailure(ctx, "%s ret=%d", label, ret);
			return;
		}
		appendLog(ctx, "PASS %-18s ret=%d", label, ret);
	}

	/*目标面尺寸只能通过 g2d_info 观察，用来验证 rotate/scale_to 生效*/
	void checkInfo(TestCtx& ctx, const char* label, uint32_t exp_w, uint32_t exp_h) {
		g2d_info_t info;

		if(g2d_info(&ctx.g2d, &info) != 0) {
			appendLog(ctx, "FAIL %-18s g2d_info failed", label);
			markFailure(ctx, "%s g2d_info failed", label);
			return;
		}
		if(info.width != exp_w || info.height != exp_h) {
			appendLog(ctx, "FAIL %-18s %ux%u (expect %ux%u)",
					label, info.width, info.height, exp_w, exp_h);
			markFailure(ctx, "%s size %ux%u", label, info.width, info.height);
			return;
		}
		appendLog(ctx, "PASS %-18s %ux%u", label, info.width, info.height);
	}

	/*预览镜像：没有像素回读，只保证预览与目标面同尺寸，
	  内容按命令重画。*/
	void syncPreview(TestCtx& ctx) {
		g2d_info_t info;

		if(g2d_info(&ctx.g2d, &info) != 0)
			return;
		if(info.width == 0 || info.height == 0)
			return;
		if(ctx.preview != NULL &&
				ctx.preview->w == (int32_t)info.width &&
				ctx.preview->h == (int32_t)info.height)
			return;
		if(ctx.preview != NULL) {
			graph_free(ctx.preview);
			ctx.preview = NULL;
		}
		ctx.preview = graph_new(NULL, (int32_t)info.width, (int32_t)info.height);
		if(ctx.preview != NULL && ctx.preview->buffer == NULL) {
			graph_free(ctx.preview);
			ctx.preview = NULL;
			return;
		}
		if(ctx.preview != NULL)
			graph_clear(ctx.preview, 0xff101820);
	}

	void fillPreview(TestCtx& ctx, const g2d_fill_req_t* req) {
		if(ctx.preview == NULL || req == NULL)
			return;
		graph_fill_rect(ctx.preview, req->rect.x, req->rect.y, req->rect.w, req->rect.h, req->color);
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

		if(src == NULL || req == NULL)
			return NULL;
		cropped = cropPreviewSource(src, req);
		if(cropped == NULL)
			return NULL;
		if(req->rotate == G2D_ROTATE_0)
			return cropped;

		/*graph_rotate 的参数是顺时针 90 度步数，与 G2D_ROTATE_* 编码一致*/
		rotated = graph_rotate(cropped, (int)(req->rotate & 3));
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

	/*rotate/scale_to 之后目标面内容未知，预览只同步尺寸*/
	void rotatePreview(TestCtx& ctx, uint8_t rotate) {
		graph_t* rotated;

		if(ctx.preview == NULL)
			return;
		if(rotate == G2D_ROTATE_0)
			return;
		rotated = graph_rotate(ctx.preview, (int)(rotate & 3));
		graph_free(ctx.preview);
		ctx.preview = rotated;
	}

	void scalePreviewTo(TestCtx& ctx, uint32_t w, uint32_t h) {
		graph_t* scaled;

		if(ctx.preview == NULL || w == 0 || h == 0)
			return;
		if((uint32_t)ctx.preview->w == w && (uint32_t)ctx.preview->h == h)
			return;
		scaled = graph_new(NULL, (int32_t)w, (int32_t)h);
		if(scaled == NULL || scaled->buffer == NULL) {
			if(scaled != NULL)
				graph_free(scaled);
			return;
		}
		graph_blt_fit(ctx.preview, 0, 0, ctx.preview->w, ctx.preview->h,
				scaled, 0, 0, (int32_t)w, (int32_t)h);
		graph_free(ctx.preview);
		ctx.preview = scaled;
	}

	void runTest(TestCtx& ctx) {
		shm_image_t opaque_img;
		shm_image_t alpha_img;
		g2d_fill_req_t fill;
		g2d_blit_req_t blit;
		g2d_rotate_req_t rotate_req;
		g2d_scale_to_req_t scale_req;
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
		ctx.w0 = ctx.info.width;
		ctx.h0 = ctx.info.height;
		appendLog(ctx, "g2d: %ux%u depth=%u backend=%u",
				ctx.info.width, ctx.info.height, ctx.info.depth, ctx.info.backend);

		syncPreview(ctx);

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
		checkRet(ctx, "clear", ret, true);
		if(ctx.preview != NULL)
			graph_clear(ctx.preview, bg_color);

		g2d_fill_req_init(&fill, g2d_rect(24, 24, 220, 120), 0xff204060);
		ret = g2d_fill_rect(&ctx.g2d, &fill);
		checkRet(ctx, "fill_rect #1", ret, true);
		fillPreview(ctx, &fill);

		g2d_fill_req_init(&fill, g2d_rect((int32_t)ctx.w0 - 180, 40, 140, 96), 0xff503040);
		ret = g2d_fill_rect(&ctx.g2d, &fill);
		checkRet(ctx, "fill_rect #2", ret, true);
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
		checkRet(ctx, "blit_opaque", ret, true);
		blitPreview(ctx, &blit, &opaque_graph, 0);

		blit2_x = (int32_t)ctx.w0 - 280;
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
		checkRet(ctx, "blit_scale_rot90", ret, true);
		blitPreview(ctx, &blit, &opaque_graph, 0);

		src_rect = g2d_rect(0, 0, (int32_t)alpha_img.width, (int32_t)alpha_img.height);
		g2d_blit_req_init(&blit,
				alpha_img.shm_id,
				alpha_img.size,
				alpha_img.width,
				alpha_img.height,
				alpha_img.stride,
				src_rect,
				g2d_rect((int32_t)ctx.w0 / 2, (int32_t)ctx.h0 / 2 - 32,
						(int32_t)alpha_img.width, (int32_t)alpha_img.height),
				0xff);
		ret = g2d_blit_alpha_shm(&ctx.g2d, &blit);
		checkRet(ctx, "blit_alpha", ret, true);
		blitPreview(ctx, &blit, &alpha_graph, 1);

		alpha2_x = (int32_t)ctx.w0 / 2 - 220;
		alpha2_y = (int32_t)ctx.h0 / 2 + 40;
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
		checkRet(ctx, "blit_alpha_rot270", ret, true);
		blitPreview(ctx, &blit, &alpha_graph, 1);

		g2d_fill_req_init(&fill, g2d_rect(0, (int32_t)ctx.h0 - 36, (int32_t)ctx.w0, 36), 0xff000000);
		ret = g2d_fill_rect(&ctx.g2d, &fill);
		checkRet(ctx, "fill_rect footer", ret, true);
		fillPreview(ctx, &fill);

		/*目标面 rotate：90/270 交换宽高，180 不变*/
		g2d_rotate_req_init(&rotate_req, G2D_ROTATE_90);
		ret = g2d_rotate(&ctx.g2d, &rotate_req);
		checkRet(ctx, "rotate_90", ret, true);
		checkInfo(ctx, "rotate_90_size", ctx.h0, ctx.w0);
		rotatePreview(ctx, G2D_ROTATE_90);

		g2d_rotate_req_init(&rotate_req, G2D_ROTATE_180);
		ret = g2d_rotate(&ctx.g2d, &rotate_req);
		checkRet(ctx, "rotate_180", ret, true);
		checkInfo(ctx, "rotate_180_size", ctx.h0, ctx.w0);
		rotatePreview(ctx, G2D_ROTATE_180);

		g2d_rotate_req_init(&rotate_req, G2D_ROTATE_270);
		ret = g2d_rotate(&ctx.g2d, &rotate_req);
		checkRet(ctx, "rotate_270", ret, true);
		checkInfo(ctx, "rotate_270_size", ctx.w0, ctx.h0);
		rotatePreview(ctx, G2D_ROTATE_270);

		/*目标面 scale_to：缩小、恢复、非法尺寸*/
		g2d_scale_to_req_init(&scale_req, 320, 240);
		ret = g2d_scale_to(&ctx.g2d, &scale_req);
		checkRet(ctx, "scale_to_320x240", ret, true);
		checkInfo(ctx, "scale_to_size", 320, 240);
		scalePreviewTo(ctx, 320, 240);

		g2d_scale_to_req_init(&scale_req, ctx.w0, ctx.h0);
		ret = g2d_scale_to(&ctx.g2d, &scale_req);
		checkRet(ctx, "scale_to_restore", ret, true);
		checkInfo(ctx, "scale_restore_size", ctx.w0, ctx.h0);
		scalePreviewTo(ctx, ctx.w0, ctx.h0);

		g2d_scale_to_req_init(&scale_req, 0, 0);
		ret = g2d_scale_to(&ctx.g2d, &scale_req);
		checkRet(ctx, "scale_to_invalid", ret, false);
		checkInfo(ctx, "scale_invalid_size", ctx.w0, ctx.h0);

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
