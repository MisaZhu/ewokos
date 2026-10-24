/*
 * bsp_cl.c - OpenCL compute (V3D CSD) offload stub.  This platform has no
 * compute back end, so decline every op with the sticky G2D_ERR_NOT_SUPPORTED
 * (-2): libOpenCL latches its GPU device off and keeps its always-present CPU
 * device.  Nothing is submitted.  bsp_cl_init() is a benign no-op.
 *
 * The literal -2 mirrors G2D_ERR_NOT_SUPPORTED (this file does not pull in
 * g2dclient.h); g2dd passes the code back to the client verbatim.  This file is
 * byte-identical across every machine that carries bsp_g3d.h but no real compute
 * back end, so the shared g2dd links everywhere; only raspi5 ships a real
 * bsp_cl (src/cl/bsp_cl.c -> libvideocore's vc_cl).
 */

#include <bsp/bsp_cl.h>

int32_t bsp_cl_init(void) {
	return 0;
}

int32_t bsp_cl_program(const void* blob, uint32_t size) {
	(void)blob; (void)size;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}

int32_t bsp_cl_dispatch(int32_t handle, const bsp_cl_dispatch_t* d) {
	(void)handle; (void)d;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}

int32_t bsp_cl_program_free(int32_t handle) {
	(void)handle;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}
