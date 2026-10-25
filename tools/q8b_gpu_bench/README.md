# Q8B offscreen GPU regression

`make -C tools/q8b_gpu_bench -j4` builds AArch64 `bench`, `clock`,
`input-latency` and a texture shader in `/tmp/q8b-gpu-bench`. It uses the Q8B
SDK and current GLSL frontend/Adreno backend. `make ... check` runs the native
clock-anchor regression with ASan/UBSan.

The offscreen benchmark must be the **only GPU owner**: boot without g2dd,
fbdisplayd or X, and run it once after a board reset. Do not launch a second
GPU process on that boot. Upload `texture.bin` to `/gpu-bench/texture.bin` and
run `/gpu-bench/bench 300`. The argument is the stress duration in seconds.
It deliberately parks after completion, keeping ring/GMU/DMA memory alive
because the driver has no validated GPU teardown protocol. Reset through the
MCP interface before returning to the desktop. Follow the machine's
[kernel test workflow](../../machines/q8b/skills/q8b-kernel-test/SKILL.md)
for PXE/reset and read only the monitored serial cache.

The suite directly calls libadreno, with no CPU fallback. It checks every
output pixel and a 64-word destination guard for fill, copy, alpha fill,
blend, scale, 90-degree rotation, sampled textures and A2D/compute transitions.
Sizes range from 64×64 to 3840×2160, including packed 801×603 compute surfaces.
It skips A2D cases whose pitch is unsupported. It also alternates full/short
uniform uploads and releases/reloads the texture program 32 times. Stress
alternates VGA and 1080p with short idle gaps.

`clock` checks 100,000 successive nanosecond timestamps and eight 10µs precise
waits against CNTVCT. `input-latency 60` requires a normal desktop and sends
relative mouse movement through the real X server IPC, reporting requests
blocked for at least 200ms. This measures input handling and server blocking;
it does not measure USB polling or time to physical display scanout.

## Fixes and evidence, 2026-10-26

The initial vsyscall page contained an odd seqlock (`f4eb55dd`) and nonzero
reserved RAM. Since the writer increments the sequence by two, it remained
odd forever. `proc_nsleep_precise(10000)` returned -1, and the GPU completion
loop quickly fell back to tick-based sleeping. Initialize this reserved page
once after installing the boot address space, outside per-process mapping.

Further live checks found occasional clock regressions. Synchronize AArch64
CNTVCT reads with ISB, allow short seqlock publications to finish, and align
the kernel clock pair to the exact 5µs/96-count period of Q8B's 19.2MHz clock.
Integer-MHz clocks retain their 1µs boundary. The shared coarse timestamp can
be rounded down by at most 4µs on Q8B; interpolation recovers that interval.
Unsupported periods and BSP clocks with different origins keep their sampled
base. Native tests check 300,000 samples against an independent 128-bit oracle
at 19.2MHz, 24MHz and 1MHz, plus unavailable and mismatched clocks.

The compute path now retains one descriptor/UBO BO and CPU command buffer per
kernel, protected by its existing caller lock through synchronous fence
completion. Short UBO uploads clear the remaining bytes. Normal program
release frees the pool; a wedged device retains allocations and rejects
subsequent dispatch/release, preserving in-flight GPU references.

Q8B's counter-access setup now preserves CNTKCTL event-stream controls and
executes ISB, matching the common AArch64 setup.

Measurements and serial excerpts are in `evidence/`. Old-kernel percentile
samples are tick-quantized; compare elapsed-time averages for that baseline.

| Average latency (µs) | Original kernel | Initialized clock | Descriptor pool | Final clock fixes |
| --- | ---: | ---: | ---: | ---: |
| 64×64 blend | 20 | 16 | 13 | 14 |
| 640×480 scale | 1918 | 110 | 107 | 106 |
| 1080p fill | 1973 | 194 | 194 | 188 |
| 1080p mixed fill/copy/blend | 5775 | 3953 | 3948 | 2156 |

The initialized-clock baseline completed 180,677 submissions with 60 seconds
of stress. The pool run completed 466,760 submissions with 300 seconds of
stress. The final build completed 386,754 submissions with 180 seconds of
stress, 27,500 mixed-size frames, zero pixel failures and `wedged=0`.
Every completed run had exactly one benchmark start and no later boot/fault
markers before completion. A partial baseline run interrupted by a manual
reset is excluded. New kernel SHA-256:
`f7b3d64bc8008a6a7842dba64e287a1255ab7e8c4791ceecf56e7c1ac2d41548`.
Installed g2dd was read back and matched the local build, SHA-256:
`80e2a6086c0142c0e927195eb07bfa612988419d296551eae3f2c3088c6045a5`.

The hardware fault/restart reported earlier was not reproduced by these
runs; successful bounded stress testing cannot establish its cause.

After restoring the original desktop startup files and installing g2dd,
the mouse IPC baseline completed 393 requests over 20 seconds (mean 4µs,
maximum 19µs). A 60-second run covering xapps startup and transparent
fullscreen display completed 1,096 requests: mean 3.811ms, maximum 122.678ms,
zero requests at or above 200ms and zero IPC errors. After loading, the
five-second interval maxima were about 49–51ms. The earlier diagnostic
capture had 19/101 requests at or above 200ms and a 263.524ms maximum;
its instrumentation and duration differed, so it is historical evidence
rather than an isolated A/B measurement of this change. No GPU fault or
boot marker appeared during the new GUI run. The remaining tens-of-ms
server/compositor blocking is visible in the logs.

## Rootfs update

Rebuilt the complete X userland with `make -C machines/q8b/system x -j8`,
then regenerated `machines/q8b/root_aarch64.img` with `make ... sd`.
The staged files total about 250MB; the previous 112MB image was nearly
full and lacked `/apps`. The default image capacity is now 320MB.
`e2fsck -fn` passes, and all 446 staged files match their image readback.
The normal desktop startup files contain no temporary GPU benchmark or
GUI tracing configuration. The image's g2dd matches the hardware-tested
build. See `evidence/rootfs.json` for image and key-file hashes.
The generated image remains an ignored local build artifact.
