# 19 EwokOS's Extra Libraries (sw.extra and projects)

> Language: **English** | [中文](19-extra-libs.zh.md)
>
> Goal: take stock of the libraries shipped in the two "extension"
> submodules outside the core system — `sw.extra` (the extra software
> suite) and `projects` (more applications): SDL2, curses, an HTML engine,
> 3D, physics, a multimedia framework… This is the fourth article of the
> "ewokos series", following [18](18-libs.md)'s core libraries.

The end of [18](18-libs.md) mentioned `sw.extra`. This chapter explains it
together with `projects`: both are **independent git submodules**, not part
of the core `system/` build chain, enabled on demand. They vendor a batch
of "heavyweight" third-party libraries for advanced applications (a
browser, a video player, 3D, emulators).

## 19.1 The Two Extension Submodules

| Submodule | Directory | Positioning | Built by default? |
|--------|------|------|--------------|
| **ewokos_extra** | [sw.extra/](../../sw.extra/) | the extra software suite: SDL2 multimedia, curses text UI, plus apps like vim / calculator / calendar | triggered separately by `make extra` |
| **evokes.apps** | [projects/](../../projects/) | more applications: browser, video, screensaver, software 3D, plus a bunch of emulators and games | whatever `projects/Makefile`'s `DIRS` lists gets built |

Both follow the "manual, source-level dependency management" from Ch.
[16](16-libs-overview.md): third-party libraries are vendored into the
directories as source, cross-compiled into static `.a` against the EwokOS
SDK with customized Makefiles, then linked into their respective
applications. They **depend at compile time on the SDK already installed by
the core system** (`system/build_<arch>/<hw>/{include,lib}`), so you must
build `system/` first before building the extensions.

> The key difference from the core libraries: core libraries
> (`system/*/libs`) are built with a full `make` and go into the rootfs;
> extension libraries are "optional equipment" — you only compile them when
> you want to run the corresponding applications.

## 19.2 sw.extra's Libraries

sw.extra's libraries are all in [sw.extra/libs/](../../sw.extra/libs/);
the build list is in its [Makefile](../../sw.extra/libs/Makefile) (`DIRS =
curses SDL2`).

### curses — Text Terminal UI

[sw.extra/libs/curses/](../../sw.extra/libs/curses/) is a port of NetBSD
curses, letting you draw "character interfaces" in a terminal (menus,
panels, colored text) — the foundation of fullscreen text programs like
vim. It consists of two static libraries:

| Library | Purpose |
|----|------|
| **libcurses** | screen drawing: windows, cursor, attributes, key input |
| **libterminfo** | terminal capability handling: parses the terminfo database, outputs escape sequences via `tparm`/`tputs`; also provides a termcap compatibility layer (`tgetent`/`tgetstr`/`tgoto`, etc.) |

> Companion apps: [sw.extra/bin/curse_test/](../../sw.extra/bin/curse_test/)
> is curses's test program, while
> [sw.extra/bin/vim/](../../sw.extra/bin/vim/) is the real "user".

### The SDL2 Family — a Cross-Platform Multimedia Layer

[sw.extra/libs/SDL2/](../../sw.extra/libs/SDL2/) comes from the "Raspberry
Pi Baremetal" port — a complete set of SDL2 and its official extension
libraries (sources in the `libs/` subdirectory, plus `apps/` and `bin/`):

| Library | Link name | Purpose |
|----|--------|------|
| **SDL2** | `-lSDL2` | Simple DirectMedia Layer: a unified abstraction of windows/input/audio/timers/threads — the foundation of games and multimedia programs |
| **SDL2_image** | `-lSDL2_image` | image loading: BMP/GIF/JPG/PNG/TGA/TIF/WEBP |
| **SDL2_mixer** | `-lSDL2_mixer` | audio mixing: multi-channel playback, supporting MP3/OGG/AIFF/VOC etc. |
| **SDL2_ttf** | `-lSDL2_ttf` | TrueType font rendering (based on FreeType) |
| **SDL2_gfx** | `-lSDL2_gfx` | graphics primitives (lines/circles/polygons), rotozoom scaling/rotation, frame-rate control |

> On EwokOS, SDL2 maps "windows/input/audio" onto the system's xwin,
> keyboard/mouse drivers, and audio services, letting SDL programs
> originally written for Linux/Windows be ported and run.
> `sw.extra/libs/SDL2/` is its SDK/installation area.

### Others (Applications and Submodules)

`sw.extra` also ships applications and tools — not "libraries" but worth
knowing about: [apps/](../../sw.extra/apps/) (the calculator, the
calendar), [bin/](../../sw.extra/bin/) (vim, doubao, curse_test),
[x/xwm](../../sw.extra/x/) (a window manager). Additionally, `.gitmodules`
declares **mario_vm** (a JavaScript VM submodule), fetched on demand.

## 19.3 projects' Libraries

In `projects` ([projects/](../../projects/)), each application usually
carries its own `libs/` with its dedicated third-party libraries. These
libraries are **not in the core build chain** — they compile together with
their own applications.

> Note: `projects/Makefile`'s `DIRS = macemu nesemu soft3d doom cards mine
> previous` — i.e. emulators and games are built by default; `browser`,
> `saver`, and `video` are **optional/experimental** applications, built by
> entering their directories separately when needed.

### browser — Web Browsing

[projects/browser/libs/](../../projects/browser/libs/):

| Library | Purpose |
|----|------|
| **litehtml** | a lightweight HTML/CSS rendering engine: parses and lays out web pages, outputting a drawable document tree |
| **gumbo** | Google's HTML5 parser (ships with litehtml; parses HTML text into a tree) |
| **widget++** | the browser project's local widget variant (works with litehtml for the web view) |

The corresponding app:
[projects/browser/apps/xBrowser](../../projects/browser/apps/).

### saver — Screensaver (2D Physics + GL Math)

[projects/saver/libs/](../../projects/saver/libs/):

| Library | Purpose |
|----|------|
| **cglm** | the OpenGL math library (C edition): vectors, matrices, quaternions, affine transforms, projections, etc. — the foundation of 3D/2D graphics computation |
| **ferox** | a 2D rigid-body physics engine (the `FR_`-prefixed API): worlds, collision detection (broadphase), dynamics, friction |

The corresponding app:
[projects/saver/apps/saver](../../projects/saver/apps/).

### soft3d — Software 3D Rendering

[projects/soft3d/libs/](../../projects/soft3d/libs/):

| Library | Purpose |
|----|------|
| **portablegl** | a software renderer implementing the OpenGL 3.x core profile in pure C — 3D without a GPU |
| **imgui** | Dear ImGui: an immediate-mode GUI, often used for debug panels/tool interfaces of 3D programs |
| **glcommon** | general GL helpers to go with portablegl (math, meshes, texture loading, etc.) |

The corresponding apps
([projects/soft3d/apps/](../../projects/soft3d/apps/)): classic 3D demos
like `3ddemo`, `gears`, `matrix`, `Fireworks`, `imgui_demo`.

### video — Video Playback

[projects/video/libs/](../../projects/video/libs/):

| Library | Purpose |
|----|------|
| **ffmpeg** | the industry-standard multimedia framework: demuxing and decoding audio/video (`lib/` is the complete ffmpeg source + the EwokOS customized build `ewok.mk`) |
| **widget++** | the video project's local widget variant: on top of the core `widget++` (see [18](18-libs.md).3) it adds two "heavyweight" widgets — **WidgetWebview** (a web view, embedding a browser engine to render pages) and **WidgetVideo** (video playback) |

The corresponding app:
[projects/video/apps/VideoPlayer](../../projects/video/apps/), plus the
library's own [bin/mp4player.c](../../projects/video/libs/ffmpeg/bin/).

### Emulators and Games (No Dedicated libs)

The **macemu / nesemu / minivmac / previous** (emulators) and **doom /
cards / mine** (games) in `projects` **don't carry their own `libs/`
directories**: they link the core libraries directly (`EWOK_LIBC` /
`EWOK_LIB_GRAPH` / `EWOK_LIB_X`); the few needing multimedia capabilities
(e.g. `previous` links sw.extra's SDL2 family) additionally link the
extension libraries. Their source code itself is a fine example of "porting
+ adapting to EwokOS", suitable for advanced reading.

## 19.4 Extension-Library Quick Reference

| I want to… | Use this library | Where |
|--------|----------|------|
| build fullscreen text/character UIs (menus, panels) | curses (libcurses + libterminfo) | `sw.extra/libs/curses` |
| write games/multimedia with SDL (cross-platform) | SDL2 + image/mixer/ttf/gfx | `sw.extra/libs/SDL2` |
| embed web/video widgets in a window | widget++ (WidgetWebview/WidgetVideo) | `projects/video/libs/widget++` |
| render HTML/CSS pages | litehtml + gumbo | `projects/browser/libs` |
| do 3D/2D graphics math | cglm | `projects/saver/libs` |
| add 2D physics (collisions, rigid bodies) | ferox | `projects/saver/libs` |
| run 3D without a GPU (software OpenGL) | portablegl + glcommon | `projects/soft3d/libs` |
| add a debug UI to a 3D program | imgui (Dear ImGui) | `projects/soft3d/libs` |
| decode/play audio and video | ffmpeg | `projects/video/libs` |

## 19.5 How to Use the Extension Libraries

Extension libraries don't go into the core rootfs; the usage is
"cross-compile against the SDK + link into your application":

1. **Build the core system first**, ensuring the SDK
   (`system/build_<arch>/<hw>/{include,lib}`) is ready;
2. **Enter the corresponding library/app directory and `make`**: the
   extension libraries' Makefiles `include` the platform's `make.rule`,
   finding core headers with `-isystem $(SDK_DIR)/include` and core
   libraries with `-L $(SDK_DIR)/lib`;
3. **Add the extension libraries' `.a` at link time** — e.g. an SDL program
   links `-lSDL2 -lSDL2_image -lSDL2_mixer ...` in addition to
   `EWOK_LIB_GRAPH`/`EWOK_LIB_X`.

> At the top level you can use `make extra` (in `sw.extra`) to trigger the
> extension-suite build; `projects` builds per its `Makefile`'s `DIRS`, or
> by entering subdirectories individually.

## 19.6 Summary

- **sw.extra** (the ewokos_extra submodule): `curses` (text UI), the
  `SDL2` family (SDL2/image/mixer/ttf/gfx multimedia), plus apps like
  vim/calculator/calendar and the mario_vm (JS VM) submodule;
- **projects** (the evokes.apps submodule): libraries travel with their
  apps — browser uses `litehtml`+`gumbo`, saver uses `cglm`+`ferox`, soft3d
  uses `portablegl`+`imgui`+`glcommon`, video uses `ffmpeg` and the
  `widget++` extension widgets (WidgetWebview/WidgetVideo); emulators and
  games link the core libraries directly;
- Extension libraries are **optional equipment**: not in the core build
  chain, cross-compiled against the installed SDK, linked on demand;
- At this point the "ewokos series" has covered the complete library
  landscape from libc through the core libraries to the extensions.

Want hands-on practice? Pick an extension app (say `soft3d`'s `gears`) and
read its Makefile and source — see how it wires portablegl onto EwokOS's
graph/xwin. That's the best example for understanding "how a library
lands".
