# Q8B offscreen GPU regression

`make -C tools/q8b_gpu_bench -j4` builds AArch64 `bench`, `clock`,
`input-latency`, `shm-pool`, `shm-map`, `g2d-ipc` and a texture shader in `/tmp/q8b-gpu-bench`.
It uses the Q8B SDK and current GLSL frontend/Adreno backend. `make ... check` runs the native
clock-anchor and physical memory pool regressions with ASan/UBSan.

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
bounded runs. After the SD update below, the user reported reproducing a
reset with glmatrix; the serial cache confirms a PSHOLD warm reset followed
by Sahara/EDL, without a preceding GPU fault or kernel exception message.
The later instrumented reproduction located an unsafe physical memory pool,
described below. The original reset excerpt is in `evidence/glmatrix-reset.log`.

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

The actual board SD-card rootfs was also updated: 129 existing files were
backed up and replaced, and 22 missing resources were added. All 446 manifest
files matched content, size and permissions both before and after an MCP
reboot. `/` remains mounted by `/sbin/sdfsd`, and the normal desktop services
started successfully. The old files remain in
`/sd-rootfs-update-20261026/backup` on the SD card. The post-reboot clock check
passed 100,000 samples with no regressions. A new 60-second mouse test was
interrupted by the glmatrix reset and is not counted as a passing run.
See `evidence/sd-rootfs.json` and `evidence/sd-rootfs.log`.

## glmatrix + fullscreen xapps reset

The generic boot layout reserved 256MB DMA and 512MB contiguous SHM directly
above kmalloc, ignoring the final UEFI memory map. The normal page allocator
already filtered that map, but these two pools did not. Instrumented SHM
allocation stopped at `memset(P2V(0xba790000), 0, 2025 * 16384)`: its end
`0xbc734000` crosses the first physical RAM bank end `0xbb700000`. The old
SHM pool also covered firmware-owned memory beginning at `0xaeb00000`.
The last GPU submissions had retired; no kernel zero-completion message
appeared, and a warm reset into Sahara followed about 44 seconds later.

Q8B now chooses both pools from reclaimable final UEFI descriptors, joining
physically adjacent descriptors regardless of allocation type/order, excluding
firmware reservations, GOP framebuffer, ramdisk and GPU ZAP. Pools stay below
4GB and use full 16KB CPU pages. An invalid map or unavailable safe range stops
boot instead of reverting to the unsafe carve. The ordinary page allocator
excludes both pools; the kernel direct map also skips them so it cannot
replace SHM's uncached mapping or create a cached DMA alias. Other BSPs retain
their existing layout through a weak hook.

The board selected DMA `0xc8600000..0xd8600000` and contiguous SHM
`0xd8600000..0xf8600000`. Both glmatrix and xapps ran at 3840×2160 with about
460MB SHM in use. A 90-second run including xapps startup completed 746 mouse
IPCs with no errors/reset; two startup requests exceeded 200ms (maximum
239.339ms). This startup latency remains a limitation. In the reverse launch
order, a steady 60-second run completed 833 requests, maximum 64.006ms,
with no requests at/above 200ms or IPC errors. These IPC tests do not measure
physical USB or scanout latency.

`shm-pool` uses exclusive per-process keys (EwokOS IPC_PRIVATE discards
IPC_CONTIG), verifies the contiguous backing flag and physical bounds, fills
available memory in 8MB chunks, checks initial zero and one word per 16KB page,
then checks retained patterns and frees/reuses the allocations for three rounds.
The board allocated 400MB each round with zero errors. Run it separately from
latency measurements: allocation/zeroing deliberately adds synchronous work.
Native tests cover the actual reserved ranges, unsorted adjacent descriptors,
physical gaps, alignment, overflow and 20,000 cases against a brute-force oracle.

Temporary per-allocation kernel tracing was removed after diagnosis. Optional
GPU tracing remains disabled at boot: `devcmd /dev/g2d trace 4` records the next
four submissions (0 disables; maximum 256), including fence retirement and
fault-check completion, without additional GX register reads. See
`evidence/glmatrix-memory.log` and `evidence/glmatrix-memory.json` for the
instrumented failure and final validation/deployment details.

The final kernel was deployed to PXE and booted through MCP, followed by two
complete app open/ESC-close cycles in opposite launch orders. Both processes
were present with 459–460MB SHM, no unexpected reset followed, and the three
400MB pool rounds passed again on that boot. The final 100,000-sample clock
check had no regressions/failures. The actual SD rootfs and rebuilt 320MB image
each matched all 446 staged files; SD verification was repeated after the final
MCP reboot. `e2fsck -fn` returned 0. The normal desktop remains running with
test apps closed. Final kernel SHA-256:
`218b8d6633140c8801209a603572fb47c69a5349b3e917a33f4f8ccb3a8cbde9`.
The installed/image g2dd (optional bounded trace, default off) SHA-256 is
`4cb73731009b42701adbbd38ccef93cd47f7f644163c40a2c9ef328e689f6a0d`.
The Q8B BSP/driver changes are in machines commit `00ca0c57`.

## Shared canvas mapping, 2026-10-26

The running desktop's attach cache had 4,434 hits, 3,944 misses and 3,874
evictions, with 130,400KB pinned against its 128MB budget. A 4K canvas spans
2,025 16KB pages. Each attach/detach previously issued a broadcast TLBI plus
DSB/ISB barriers for **every page**, while holding the global SHM lock.

SHM attach/detach now changes all PTEs before calling `flush_tlb_range()`.
AArch64 publishes the stores with one DSB, issues the same all-level,
all-ASID, inner-shareable VAAE1IS invalidation for each page, and completes
with one DSB/ISB pair. This uses baseline ARMv8 instructions and the VA >> 12
operand even with 16KB pages. Invalidation completes before reference counts
drop or physical pages can be recycled. Other architectures retain their
existing per-page cache/TLB maintenance through a weak default implementation.
Partial attach failure now removes the successfully installed mappings and
clears the attachment slot before returning failure; page-table OOM was not
injected in the board tests.

Two new tools exercise the common path without becoming another GPU owner:

- `shm-map`: 64KB, 8MB and 32MB segments, ordinary/contiguous backing, one/four
  child processes with independent address spaces. A parent retains each
  segment while children repeatedly attach, check/write page markers and
  detach. Allocation, zeroing and fork are outside the timed samples.
- `g2d-ipc 1` / `g2d-ipc 5`: 100 synchronous 4K fills through the **running
  g2dd**, using one or five canvases. Five exceed the mapping cache budget.
  Every output pixel and all 64 guard words are checked after the timed work.
  Terminal GPU errors retain all buffers and stop the benchmark until reset.

Upload these executables from `/tmp/q8b-gpu-bench` to the board and run them
by absolute path. Keep the desktop workload fixed for comparisons. Do not
confuse them with `bench`, which still requires exclusive GPU ownership.

An empty desktop was used on both kernels, with the same SD userland/driver.
The original kernel was `218b8d66...`, the new kernel `7148b049...`. Two
`shm-map` runs per kernel completed 2,400 attaches and 2,400 detaches, with
zero marker/result errors. Means below average the per-run means:

| Operation | Before | After | Reduction |
| --- | ---: | ---: | ---: |
| 32MB contiguous attach, one worker | 410.029µs | 46.934µs | 88.6% |
| 32MB contiguous detach, one worker | 377.862µs | 24.229µs | 93.6% |
| Five 4K canvases, fill IPC round trip | 3.146ms | 1.966ms | 37.5% |

The IPC result uses two successful baseline runs and three new-kernel runs,
100 requests per run, after warming each canvas. The third baseline five-canvas
run failed to allocate before issuing a request; it remains visible in the
raw log and is excluded from timing averages. Single-canvas median latency
stayed near 1.96ms: tick-based parking in the GPU wait remains a separate
bottleneck. Four-worker runs still contain scheduling outliers around 30ms;
these measurements do not establish that every input stall is fixed.

On the new kernel, glmatrix plus fullscreen xapps ran for 60 seconds with 533
mouse IPCs: mean 62.755ms, maximum 162.937ms, zero >=200ms samples and zero IPC
errors. This is a bounded integration check, not an input-latency A/B test.
After normal ESC closes, three pool exhaustion/reuse rounds each allocated
376MB with zero initial-zero, physical-range or retained-pattern errors.
No GPU fault, kernel panic or unexpected reboot appeared after desktop boot.
The native clock/pool ASan/UBSan checks passed. Disassembly confirmed the
16KB TLBI operand stride of four with barriers outside the loop.

Raw samples and validation logs: `evidence/common-path-{before,after,validation}.log`;
parsed timings: `evidence/common-path.json`. Kernel built and deployed to the
existing PXE path, then booted through MCP; SD GPU driver was kept fixed
(read-back SHA-256 `496d59c35cc9e56da91228a2f13f3e16d5dd9a6ed70602ec099db35b899eb218`).
No production rootfs binary changes are required by this kernel-only change.
New kernel SHA-256:
`7148b0493f3f9e1345452cc8af3ae857f05d408bb75045e4377cbf9b583bdc8e`.

## Q8B completion-wait parking (2026-10-26)

The synchronous GPU wait previously spent up to 32 precise 10µs polls, then
called `usleep(1000)`. With the board's timer scheduling, a warmed 4K fill
usually returned about 1.96ms later. The new policy keeps that poll budget,
uses **direct `SYS_USLEEP(1)` for up to eight early parks**, then restores
1ms sleeps for later wakes. This lets a completed job be observed at the next timer
tick. The one-second wall deadline, authoritative completion fence, final
RPTR sample, fault check and terminal wedge handling remain in force.

Do not substitute libc `usleep(1)`: `usleep` values <=200µs use precise busy
waiting. That was tried during development and did not improve latency;
those attempts are excluded from the results below. The final policy has
no history table and no extra busy-poll budget. A first-park-only variant
helped fills but regressed 4K copies and some mixed workloads: its following
1ms sleep could skip their completion tick. Eight early wakes cover the
measured blit workloads before returning to the historical long-job backoff.

Same-boot A/B on an empty desktop, with the mapping cache enabled and
flushed before each allocation, used modes `0,1,1,0,0,1`, each with one
then five canvases. Each run warmed its canvases, measured 1,024 requests,
and checked every output pixel and 64 guard words per canvas. Profiling
was enabled for these unpaced runs. All 12 runs passed with zero errors.
The means below average three run means per mode:

| Fill IPC round trip | Historical park | First park at next tick | Reduction |
| --- | ---: | ---: | ---: |
| One 4K canvas | 1.966ms | 0.984ms | 50.0% |
| Five 4K canvases | 1.965ms | 0.984ms | 49.9% |

With profiling off, a separate `0,1,1,0` sequence issued 2,048 requests per
run at 200Hz. Historical/new mean latency was 1.966/0.987ms. The daemon
plus its direct worker threads averaged 7.763/7.715% of one CPU core,
about 388/385µs estimated CPU time per request. These are samples of the
kernel's two-second process-accounting windows, not exact cumulative CPU
times. Unpaced new-mode runs finish before the two-second CPU sampling
warm-up; their zero CPU samples must not be interpreted as zero CPU use.

Diagnostics through the running g2dd:

```
/bin/devcmd /dev/g2d submit profile 1
/bin/devcmd /dev/g2d submit reset
/bin/devcmd /dev/g2d submit
/bin/devcmd /dev/g2d submit profile 0
/bin/devcmd /dev/g2d submit wait 0
/bin/devcmd /dev/g2d submit wait 1
```

`submit` reports profiled submitted jobs, errors, average queue/stage/wait/
fault-check wall times, maximum wait, precise polls, parks, early short
parks and failed precise polls. Counters include background desktop work.
Profile collection defaults off; mode 1 defaults on. `stat` also includes
the backend counters; `stat reset` clears them. Mode 0 selects the old
parking policy for comparisons. All commands serialize with GPU submission.

`g2d-ipc [canvases=5] [samples=100] [rate=0]` now supports longer runs and
fixed request rates (up to 1,000Hz). `bench 60 /path/texture.bin ab` runs
both policies over the existing render cases and then stresses mode 1;
it still requires exclusive GPU ownership and runs only once per boot.
For an exclusive boot, retain **sessiond** as well as the network/SSH
services: sshd requires it for authentication. Restore the original init
script before starting the GPU test.

Raw IPC and submit samples: `evidence/wait-ipc.log`,
`evidence/wait-submit.log`; parsed runs and binary hashes:
`evidence/wait.json`. Kernel remains `7148b049...`; the staged rootfs
image passes `e2fsck -fn` and its extracted g2dd matches the new build
(`cf003bf0...`). The final driver was also read back from the actual SD
rootfs and matched this hash. The original PXE kernel and SD init script
were restored before the final runs.

Exclusive rendering used the same original kernel and one GPU owner, with
one full pass per mode over 37 size/operation combinations. Both passes
checked 175,698,766 pixels in total, plus guard words, with zero failures.
All 74 performance cases completed; the largest increase in a per-case mean
was 0.103%. Representative direct-call means (µs):

| Operation | Historical park | Eight early wakes |
| --- | ---: | ---: |
| 64x64 fill / copy / blend / texture | 14 | 14 |
| 1080p alpha fill / blend / scale / rotate / texture | 1,959 | 980 |
| 1080p mixed | 1,959 | 1,960 |
| 4K fill | 1,959 | 980 |
| 4K copy | 1,959 | 1,961 |
| 4K alpha fill / blend / scale / texture | 3,918 | 2,941 |
| 4K rotate90 | 5,877 | 5,882 |
| 4K mixed | 7,836 | 5,882 |

The final 60-second mixed-operation stress completed 13,450 frames and
reached fence 360,958 with zero failures and no wedge. Descriptor reuse
also passed 32 reload cycles and 256 draws, including 128 short uniform
uploads. `evidence/wait-render.log` contains the full results;
`evidence/wait-first-park-render.log` preserves the rejected one-short-park
variant's regression. A cached v4 texture blob was rejected before this
final run; `make shaders` regenerated the matching v6 blob after correcting
the benchmark's compiler include path to `projects/opengl`.

On the same restored desktop boot, one 60-second glmatrix + fullscreen
xapps run per mode completed with these synthetic mouse IPC results:

| Input IPC | Historical park | Eight early wakes |
| --- | ---: | ---: |
| Samples | 528 | 571 |
| Mean | 63.609ms | 55.091ms |
| Maximum | 178.289ms | 159.520ms |
| >=200ms / IPC errors | 0 / 0 | 0 / 0 |

The apps were normally closed between runs; g2dd recorded PGL draw calls
in both. This is one ordered pair of integration runs, not a characterization
of variance or physical USB-to-scanout latency. No GPU timeout/fault, kernel
panic or unexpected boot marker appeared during the final desktop tests.
Afterward, both test apps were closed, attach-cache pins were drained and
the cache re-enabled, submit statistics reset, and mode 1/profile 0 verified.
The board remains on its normal desktop. Logs: `evidence/wait-input.log`
and `evidence/wait-validation.log`; all parsed results: `evidence/wait.json`.
