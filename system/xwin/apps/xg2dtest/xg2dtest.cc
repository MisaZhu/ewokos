#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <g2dclient/g2dclient.h>
#include <graph/graph.h>
#include <font/font.h>
#include <ewoksys/kernel_tic.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <string>
#include <vector>

using namespace Ewok;

/*dwell time (ms) for each rotate/scale stage so transitions are visible*/
#define STAGE_DELAY_MS 160

#define CANVAS_W 560u
#define CANVAS_H 320u

/*throughput bench geometry, kept in sync with g2dtest so the reported
  fps is directly comparable: one bench frame = 16 random 64x64 fills
  + a full-canvas opaque blit + a full-canvas alpha blit*/
#define BENCH_CANVAS_W 800u
#define BENCH_CANVAS_H 600u
#define BENCH_SRC_W 640u
#define BENCH_SRC_H 480u
#define BENCH_USEC 1000000u /* ~1 second per measurement */
#define BENCH_MAX_FRAMES 5000u

/*stateless g2d: the canvases are shm segments owned by this app and
  carried in the requests by shm id. the driver attaches, operates in
  place and detaches, so the shm pixels ARE the result and the preview
  is just a graph_t wrapped over them, a true readback instead of a
  cpu mirror. rotation is clockwise degrees; 90/270 swap dimensions and
  the dst canvas must be created at the rotated size, so the rotate
  stages hop between canvases of matching size.*/

typedef struct {
	graph_t* g;
	uint32_t* pixels;
	uint32_t size;
	uint32_t width;
	uint32_t height;
} shm_image_t;

static int shm_image_create(shm_image_t* img, uint32_t width, uint32_t height) {
	if(img == NULL || width == 0 || height == 0)
		return -1;

	memset(img, 0, sizeof(*img));
	/* graph_new_shm canvas: prefers a physically contiguous shm segment
	   (IPC_CONTIG) and falls back to a scattered one; the contig flag
	   travels with the graph so img_canvas can report it */
	img->g = graph_new_shm((int32_t)width, (int32_t)height);
	if(img->g == NULL || img->g->buffer == NULL)
		return -1;
	img->pixels = img->g->buffer;
	img->size = width * height * 4;
	img->width = width;
	img->height = height;
	return 0;
}

static void shm_image_destroy(shm_image_t* img) {
	if(img == NULL)
		return;
	if(img->g != NULL)
		graph_free(img->g);
	memset(img, 0, sizeof(*img));
}

static g2d_canvas_t img_canvas(const shm_image_t* img) {
	return g2d_canvas(img->g->shm_id, img->size, img->width, img->height,
			img->g->shm_contig ? 1 : 0);
}

static uint32_t make_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void img_clear(shm_image_t* img, uint32_t color) {
	uint32_t i;
	uint32_t n;

	if(img == NULL || img->pixels == NULL)
		return;
	n = img->width * img->height;
	for(i = 0; i < n; i++)
		img->pixels[i] = color;
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

/*deterministic corner markers so rotations can be verified: clockwise
  90 moves TL->TR, TR->BR, BR->BL, BL->TL*/
static void set_corner_markers(shm_image_t* img) {
	uint32_t w;
	uint32_t h;

	if(img == NULL || img->pixels == NULL)
		return;
	w = img->width;
	h = img->height;
	img->pixels[0] = 0xff000001u;                 /* TL */
	img->pixels[w - 1] = 0xff000002u;             /* TR */
	img->pixels[(h - 1) * w + w - 1] = 0xff000003u; /* BR */
	img->pixels[(h - 1) * w] = 0xff000004u;       /* BL */
}

class G2DTestWidget: public Widget {
	/*the test loops in a background thread: one round = base scene +
	  staged rotate/scale checks (return codes + shm pixel validation).
	  the canvases ARE shm, the driver writes them in place and the
	  preview wraps the canvas pixels directly, a true readback instead
	  of a cpu mirror. all intermediate results go into the local
	  TestCtx first (no lock held), then a brief lock hands them to the
	  UI so long lock holds never starve window repaints.*/
	struct TestCtx {
		shm_image_t* canvas;   /*currently active canvas (A/B ping-pong)*/
		graph_t previewWrap;   /*preview wrapped over the active canvas pixels*/
		std::vector<std::string> logs;
		std::string firstFailure;
		int failures;
		bool pass;

		TestCtx() : canvas(NULL), failures(0), pass(false) {
			memset(&previewWrap, 0, sizeof(previewWrap));
		}
	};

	//state published to the UI, read/written only under brief lock holds.
	graph_t* preview;
	std::vector<std::string> logs;
	bool testDone;
	bool testPass;
	int failures;
	uint32_t fps;        /*measured g2d throughput: bench frames per second*/
	uint32_t usPerFrame; /*avg microseconds per bench frame*/
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

	void checkPixel(TestCtx& ctx, const char* label, const shm_image_t* img,
			uint32_t x, uint32_t y, uint32_t expect) {
		uint32_t got;

		if(img == NULL || img->pixels == NULL ||
				x >= img->width || y >= img->height) {
			appendLog(ctx, "FAIL %-18s bad coords", label);
			markFailure(ctx, "%s bad coords", label);
			return;
		}
		got = img->pixels[y * img->width + x];
		if(got != expect) {
			appendLog(ctx, "FAIL %-18s (%u,%u) %08x want %08x", label, x, y, got, expect);
			markFailure(ctx, "%s pixel", label);
			return;
		}
		appendLog(ctx, "PASS %-18s (%u,%u)", label, x, y);
	}

	void checkSize(TestCtx& ctx, const char* label, const shm_image_t* img,
			uint32_t exp_w, uint32_t exp_h) {
		if(img == NULL || img->width != exp_w || img->height != exp_h) {
			appendLog(ctx, "FAIL %-18s size", label);
			markFailure(ctx, "%s size", label);
			return;
		}
		appendLog(ctx, "PASS %-18s %ux%u", label, exp_w, exp_h);
	}

	void runTest(TestCtx& ctx) {
		shm_image_t canvasA;
		shm_image_t canvasB;
		shm_image_t canvasC;
		shm_image_t scaled;
		shm_image_t opaque_img;
		shm_image_t alpha_img;
		g2d_fill_req_t fill;
		g2d_blit_req_t blit;
		g2d_rotate_req_t rotate_req;
		g2d_scale_to_req_t scale_req;
		uint32_t bg_color = 0xff101820;
		int ret;

		memset(&canvasA, 0, sizeof(canvasA));
		memset(&canvasB, 0, sizeof(canvasB));
		memset(&canvasC, 0, sizeof(canvasC));
		memset(&scaled, 0, sizeof(scaled));
		memset(&opaque_img, 0, sizeof(opaque_img));
		memset(&alpha_img, 0, sizeof(alpha_img));
		ctx.canvas = NULL;
		ctx.failures = 0;
		ctx.pass = false;
		ctx.firstFailure.clear();
		ctx.logs.clear();

		if(has_g2d() != 0) {
			appendLog(ctx, "FAIL g2d device not found");
			markFailure(ctx, "g2d device not found");
			return;
		}
		appendLog(ctx, "g2d: stateless shm canvas %ux%u", CANVAS_W, CANVAS_H);

		if(shm_image_create(&canvasA, CANVAS_W, CANVAS_H) != 0 ||
				shm_image_create(&canvasB, CANVAS_H, CANVAS_W) != 0 ||
				shm_image_create(&canvasC, CANVAS_H, CANVAS_W) != 0 ||
				shm_image_create(&opaque_img, 160, 120) != 0 ||
				shm_image_create(&alpha_img, 128, 128) != 0) {
			appendLog(ctx, "FAIL create shm canvases");
			markFailure(ctx, "create shm canvases failed");
			goto cleanup;
		}

		fill_checker(&opaque_img);
		fill_alpha_circle(&alpha_img);
		ctx.canvas = &canvasA;

		/*stage 1: base scene on canvasA, verified against the shm pixels
		  the driver wrote in place*/
		img_clear(&canvasA, bg_color);
		g2d_fill_req_init(&fill, img_canvas(&canvasA), g2d_rect(24, 24, 220, 120), 0xff204060);
		ret = g2d_fill_rect(&fill);
		checkRet(ctx, "fill_rect", ret, true);
		checkPixel(ctx, "fill_inside", &canvasA, 100, 60, 0xff204060);
		checkPixel(ctx, "fill_outside", &canvasA, 10, 10, bg_color);

		g2d_fill_req_init(&fill, img_canvas(&canvasA), g2d_rect(380, 40, 140, 96), 0xff503040);
		ret = g2d_fill_rect(&fill);
		checkRet(ctx, "fill_rect #2", ret, true);

		g2d_blit_req_init(&blit,
				img_canvas(&canvasA), img_canvas(&opaque_img),
				g2d_rect(0, 0, (int32_t)opaque_img.width, (int32_t)opaque_img.height),
				g2d_rect(48, 172, (int32_t)opaque_img.width, (int32_t)opaque_img.height),
				0xff);
		ret = g2d_blit(&blit);
		checkRet(ctx, "blit_opaque", ret, true);
		checkPixel(ctx, "blit_pixel", &canvasA, 48, 172, opaque_img.pixels[0]);

		g2d_blit_req_init(&blit,
				img_canvas(&canvasA), img_canvas(&opaque_img),
				g2d_rect(0, 0, (int32_t)opaque_img.width, (int32_t)opaque_img.height),
				g2d_rect(280, 160, 200, 130),
				0xff);
		ret = g2d_blit(&blit);
		checkRet(ctx, "blit_scale", ret, true);
		checkPixel(ctx, "blit_scale_tl", &canvasA, 280, 160, opaque_img.pixels[0]);

		/*rotated blit: crop rotated clockwise 90 then scaled into dst;
		  the dst top-left gets the crop bottom-left pixel*/
		g2d_blit_req_init_ex(&blit,
				img_canvas(&canvasA), img_canvas(&opaque_img),
				g2d_rect(20, 16, 80, 60),
				g2d_rect(232, 24, 120, 100),
				0xff, G2D_ROTATE_90);
		ret = g2d_blit(&blit);
		checkRet(ctx, "blit_rot90", ret, true);
		checkPixel(ctx, "blit_rot90_tl", &canvasA, 232, 24,
				opaque_img.pixels[75 * opaque_img.width + 20]);

		g2d_blit_req_init(&blit,
				img_canvas(&canvasA), img_canvas(&alpha_img),
				g2d_rect(0, 0, (int32_t)alpha_img.width, (int32_t)alpha_img.height),
				g2d_rect(220, 150, (int32_t)alpha_img.width, (int32_t)alpha_img.height),
				0xff);
		ret = g2d_blit_alpha(&blit);
		checkRet(ctx, "blit_alpha", ret, true);
		checkPixel(ctx, "blit_alpha_corner", &canvasA, 220, 150, bg_color);
		checkPixel(ctx, "blit_alpha_center", &canvasA, 220 + 64, 150 + 64,
				alpha_img.pixels[64 * alpha_img.width + 64]);

		g2d_fill_req_init(&fill, img_canvas(&canvasA),
				g2d_rect(0, (int32_t)canvasA.height - 32, (int32_t)canvasA.width, 32), 0xff000000);
		ret = g2d_fill_rect(&fill);
		checkRet(ctx, "fill_footer", ret, true);

		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		/*stage 2: rotate ping-pong. corners travel clockwise and 90/270
		  swap the dimensions, so the dst canvas must be the other size;
		  180 keeps the dimensions, so it needs the same-size canvasC.*/
		img_clear(&canvasA, bg_color);
		set_corner_markers(&canvasA);

		g2d_rotate_req_init(&rotate_req, img_canvas(&canvasA), img_canvas(&canvasB), 90);
		ret = g2d_rotate(&rotate_req);
		checkRet(ctx, "rotate_90", ret, true);
		ctx.canvas = &canvasB;
		checkSize(ctx, "rot90_size", &canvasB, CANVAS_H, CANVAS_W);
		checkPixel(ctx, "rot90_TL_from_BL", &canvasB, 0, 0, 0xff000004u);
		checkPixel(ctx, "rot90_TR_from_TL", &canvasB, canvasB.width - 1, 0, 0xff000001u);
		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		g2d_rotate_req_init(&rotate_req, img_canvas(&canvasB), img_canvas(&canvasC), 180);
		ret = g2d_rotate(&rotate_req);
		checkRet(ctx, "rotate_180", ret, true);
		ctx.canvas = &canvasC;
		checkSize(ctx, "rot180_size", &canvasC, CANVAS_H, CANVAS_W);
		checkPixel(ctx, "rot180_TL_from_BR", &canvasC, 0, 0, 0xff000002u);
		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		g2d_rotate_req_init(&rotate_req, img_canvas(&canvasC), img_canvas(&canvasA), 270);
		ret = g2d_rotate(&rotate_req);
		checkRet(ctx, "rotate_270", ret, true);
		ctx.canvas = &canvasA;
		checkSize(ctx, "rot270_size", &canvasA, CANVAS_W, CANVAS_H);
		checkPixel(ctx, "rot270_TL_from_TR", &canvasA, 0, 0, 0xff000003u);
		checkPixel(ctx, "rot270_BR_from_BL", &canvasA,
				canvasA.width - 1, canvasA.height - 1, 0xff000001u);
		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		/*rejects: dst canvas size must match the rotated size*/
		g2d_rotate_req_init(&rotate_req, img_canvas(&canvasA), img_canvas(&canvasA), 90);
		ret = g2d_rotate(&rotate_req);
		checkRet(ctx, "rotate_90_badsize", ret, false);
		g2d_rotate_req_init(&rotate_req, img_canvas(&canvasA), img_canvas(&canvasB), 0);
		ret = g2d_rotate(&rotate_req);
		checkRet(ctx, "rotate_0_reject", ret, false);
		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		/*stage 3: scale_to into a dedicated dst canvas (nearest neighbor
		  keeps the corners), then a rotated blit over the pattern*/
		fill_checker(&canvasA);
		if(shm_image_create(&scaled, 320, 240) == 0) {
			g2d_scale_to_req_init(&scale_req, img_canvas(&canvasA), img_canvas(&scaled));
			ret = g2d_scale_to(&scale_req);
			checkRet(ctx, "scale_to_320x240", ret, true);
			checkPixel(ctx, "scale_tl", &scaled, 0, 0, canvasA.pixels[0]);
			checkPixel(ctx, "scale_br", &scaled, 319, 239,
					canvasA.pixels[(canvasA.height - 1) * canvasA.width + canvasA.width - 1]);
			shm_image_destroy(&scaled);
		}
		else {
			appendLog(ctx, "FAIL scale shm");
			markFailure(ctx, "create scale shm failed");
		}
		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		img_clear(&canvasA, bg_color);
		g2d_blit_req_init_ex(&blit,
				img_canvas(&canvasA), img_canvas(&opaque_img),
				g2d_rect(0, 0, (int32_t)opaque_img.width, (int32_t)opaque_img.height),
				g2d_rect(170, 50, 220, 220),
				0xff, 45);
		ret = g2d_blit(&blit);
		checkRet(ctx, "blit_rot45", ret, true);
		publishPreview(ctx);
		usleep(STAGE_DELAY_MS * 1000);

		ctx.pass = (ctx.failures == 0);
		appendLog(ctx, "summary: %s (%d failure)", ctx.pass ? "PASS" : "FAIL", ctx.failures);

	cleanup:
		shm_image_destroy(&scaled);
		shm_image_destroy(&alpha_img);
		shm_image_destroy(&opaque_img);
		shm_image_destroy(&canvasC);
		shm_image_destroy(&canvasB);
		shm_image_destroy(&canvasA);
		ctx.canvas = NULL;
	}

	void publishResult(TestCtx& ctx) {
		/*only briefly holds the lock while handing results to the UI,
		  to avoid starving UI repaints. the last publishPreview already
		  stored the final canvas frame: ctx.previewWrap now wraps a
		  DETACHED shm canvas (runTest cleanup destroyed the images), so
		  it must not be read again.*/
		pthread_mutex_lock(&stateLock);
		logs.swap(ctx.logs);
		firstFailure.swap(ctx.firstFailure);
		failures = ctx.failures;
		testPass = ctx.pass;
		testDone = true;
		pthread_mutex_unlock(&stateLock);
	}

	/*publish an intermediate preview frame: the preview is a graph_t
	  wrapped directly over the active shm canvas, so this is the real
	  device output, not a cpu mirror.*/
	void publishPreview(TestCtx& ctx) {
		graph_t* copy;

		if(ctx.canvas == NULL || ctx.canvas->pixels == NULL)
			return;
		graph_init(&ctx.previewWrap, ctx.canvas->pixels,
				(int32_t)ctx.canvas->width, (int32_t)ctx.canvas->height);
		copy = graph_dup(&ctx.previewWrap);
		if(copy == NULL)
			return;
		pthread_mutex_lock(&stateLock);
		if(preview != NULL)
			graph_free(preview);
		preview = copy;
		pthread_mutex_unlock(&stateLock);
	}

	static uint32_t benchNowUsec(void) {
		uint32_t low = 0;
		kernel_tic32(NULL, NULL, &low);
		return low;
	}

	/*one bench frame = 1 full-canvas fill + full-canvas opaque blit
	  + full-canvas alpha blit, all synchronous IPC to /dev/g2d, same
	  mix as g2dtest's mixed_frame*/
	static int benchFrame(shm_image_t* canvas, shm_image_t* opaque,
			shm_image_t* alpha, uint32_t seq) {
		g2d_fill_req_t fill;
		g2d_blit_req_t blit;

		g2d_fill_req_init(&fill, img_canvas(canvas),
				g2d_rect(0, 0, (int32_t)canvas->width, (int32_t)canvas->height),
				0xff000000u | ((seq * 31u) & 0xffffffu));
		if(g2d_fill_rect(&fill) != 0)
			return -1;
		g2d_blit_req_init(&blit,
				img_canvas(canvas), img_canvas(opaque),
				g2d_rect(0, 0, (int32_t)opaque->width, (int32_t)opaque->height),
				g2d_rect(0, 0, (int32_t)canvas->width, (int32_t)canvas->height),
				0xff);
		if(g2d_blit(&blit) != 0)
			return -1;
		g2d_blit_req_init(&blit,
				img_canvas(canvas), img_canvas(alpha),
				g2d_rect(0, 0, (int32_t)alpha->width, (int32_t)alpha->height),
				g2d_rect(0, 0, (int32_t)canvas->width, (int32_t)canvas->height),
				0xff);
		return g2d_blit_alpha(&blit);
	}

	/*~1s of back-to-back bench frames on dedicated canvases, so the
	  displayed fps measures real g2d throughput instead of the visual
	  stage dwell time*/
	void runPerfBench() {
		shm_image_t canvas;
		shm_image_t opaque;
		shm_image_t alpha;
		uint32_t frames = 0;
		uint32_t elapsed = 0;
		uint32_t t0;

		memset(&canvas, 0, sizeof(canvas));
		memset(&opaque, 0, sizeof(opaque));
		memset(&alpha, 0, sizeof(alpha));
		if(shm_image_create(&canvas, BENCH_CANVAS_W, BENCH_CANVAS_H) == 0 &&
				shm_image_create(&opaque, BENCH_SRC_W, BENCH_SRC_H) == 0 &&
				shm_image_create(&alpha, BENCH_SRC_W, BENCH_SRC_H) == 0) {
			fill_checker(&opaque);
			fill_alpha_circle(&alpha);

			t0 = benchNowUsec();
			while(elapsed < BENCH_USEC && frames < BENCH_MAX_FRAMES && benchRunning) {
				if(benchFrame(&canvas, &opaque, &alpha, frames) != 0)
					break;
				frames++;
				elapsed = benchNowUsec() - t0;
			}
			if(frames > 0 && elapsed > 0) {
				pthread_mutex_lock(&stateLock);
				fps = (uint32_t)(((uint64_t)frames * 1000000u) / elapsed);
				usPerFrame = elapsed / frames;
				pthread_mutex_unlock(&stateLock);
			}
		}
		shm_image_destroy(&alpha);
		shm_image_destroy(&opaque);
		shm_image_destroy(&canvas);
	}

	static void* benchThreadEntry(void* p) {
		G2DTestWidget* self = (G2DTestWidget*)p;

		while(self->benchRunning) {
			TestCtx ctx;
			/*the test itself holds no lock; publishing briefly locks only.*/
			self->runTest(ctx);
			self->publishResult(ctx);
			/*throughput burst after each visual round*/
			self->runPerfBench();
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
		/*debug info on the right, 1/3 of the window width; the preview
		  takes the remaining left area.*/
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
			snprintf(status_text, sizeof(status_text), "xg2dtest RUNNING  g2d %u fps (%u us/frame)", fps, usPerFrame);
			snprintf(detail_text, sizeof(detail_text), "running g2d API checks...");
		}
		else if(testPass) {
			status_color = 0xff1d7f3b;
			snprintf(status_text, sizeof(status_text), "xg2dtest TEST PASSED  g2d %u fps (%u us/frame)", fps, usPerFrame);
			snprintf(detail_text, sizeof(detail_text),
					"bench frame %ux%u: fill %ux%u + blit %ux%u + alpha %ux%u",
					BENCH_CANVAS_W, BENCH_CANVAS_H, BENCH_CANVAS_W, BENCH_CANVAS_H,
					BENCH_SRC_W, BENCH_SRC_H,
					BENCH_SRC_W, BENCH_SRC_H);
		}
		else {
			status_color = 0xff8f2d2d;
			snprintf(status_text, sizeof(status_text), "xg2dtest TEST FAILED  failures=%d  g2d %u fps", failures, fps);
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
		usPerFrame = 0;
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
