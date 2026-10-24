#ifndef BSP_BSP_CL_H
#define BSP_BSP_CL_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

/* OpenCL compute (V3D CSD) hardware offload.  Only the raspi5 back end
   implements these against the V3D GPU (via libvideocore's vc_cl layer);
   every other machine declines every op with the sticky
   G2D_ERR_NOT_SUPPORTED (-2) so libOpenCL latches its GPU device off and
   keeps the always-present CPU device.  This mirrors the bsp_g3d split
   exactly: one canonical header copied byte-identical into all machines
   that build the shared g2dd (which references these symbols), a real
   raspi5 implementation, and decline-only stubs elsewhere, so a signature
   change surfaces as a link error rather than silent divergence.

   A kernel reaches the GPU as a mesa-compiled compute blob (the same
   g2d_pgl_program_blob_t wire format portablegl uses, compute stage only),
   uploaded once via bsp_cl_program and dispatched by handle through the
   proven V3D CSD path (v3d_g2d_run_csd).  All ops return the g2d result
   codes: G2D_OK (0), G2D_ERR_FAILED (-1, transient) or
   G2D_ERR_NOT_SUPPORTED (-2, sticky). */

/* Maximum __global buffer SSBO arguments one dispatch carries.  Mirrors
   G2D_CL_MAX_ARGS in g2dclient.h; kept as its own constant so this header
   stays free of any g2dclient dependency and can be copied byte-identical
   across machines whose bsp build does not add the g2dclient include path. */
#define BSP_CL_MAX_ARGS 16

/* Resolved dispatch parameters g2dd hands the back end.  The __global buffer
   arguments arrive as already-resolved contig physical bases in SSBO binding
   order (the GPU addresses them directly, zero copy), arg_size[i] giving each
   buffer's byte length (a 0 phy/size marks an unused trailing slot); `unif`
   points to num_uniforms u32 packed scalar + __constant argument values the
   back end's QUNIFORM interpreter reads (NULL when num_uniforms == 0).  The
   work-group shape itself is baked into the compiled kernel, so only the grid
   (wg_count) and the local_size product (wg_size, 1..256) travel here. */
typedef struct {
	uint32_t wg_count[3];
	uint32_t wg_size;
	uint32_t num_args;
	ewokos_addr_t arg_phy[BSP_CL_MAX_ARGS];
	uint32_t arg_size[BSP_CL_MAX_ARGS];
	const void* unif;
	uint32_t num_uniforms;
} bsp_cl_dispatch_t;

/* Bring up the compute path before first use (idempotent, on top of the
   2D engine's V3D init).  Returns 0 when the compute path is usable,
   non-zero when it is not.  The raspi5 back end also re-checks this lazily
   inside bsp_cl_program / bsp_cl_dispatch, so an unsupported platform simply
   declines every op. */
int32_t bsp_cl_init(void);

/* Upload a compiled compute program: `blob` points to a serialized
   g2d_pgl_program_blob_t header immediately followed by its compute-stage
   QPU/uniform payload, `size` bytes total (a CPU-side buffer - no GPU
   visibility needed; the back end copies the QPU into GPU dma and caches the
   program).  Returns a non-negative program handle (referenced by a later
   bsp_cl_dispatch) or a negative g2d result code: G2D_ERR_FAILED (-1,
   transient) or G2D_ERR_NOT_SUPPORTED (-2, sticky - no compute path, or the
   blob is out of range so libOpenCL keeps the kernel on its CPU device).
   Only the raspi5 back end implements this; every other machine declines
   with -2. */
int32_t bsp_cl_program(const void* blob, uint32_t size);

/* Run one NDRange launch of the program `handle` over the grid and arguments
   described by `d`.  Returns G2D_OK, a transient G2D_ERR_FAILED, or the
   sticky G2D_ERR_NOT_SUPPORTED (unknown handle, no compute path, or a kernel
   the CSD cannot run - e.g. one needing shared memory / barriers beyond CSD
   support - so libOpenCL replays it on the CPU device). */
int32_t bsp_cl_dispatch(int32_t handle, const bsp_cl_dispatch_t* d);

/* Release a program handle returned by bsp_cl_program: its cache slot and GPU
   dma go back for reuse.  g2dd calls this for every program whose owning
   client has exited (the back end caches only a handful of programs).
   Returns 0, or -2 for an unknown / already-free handle and on every machine
   without a compute back end. */
int32_t bsp_cl_program_free(int32_t handle);

#endif
