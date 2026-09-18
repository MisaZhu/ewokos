# 18 EwokOS's Common Libraries (libs)

> Language: **English** | [中文](18-libs.zh.md)
>
> Goal: take stock of the "wheels" EwokOS has prepared above libc —
> filesystems, image codecs, fonts, 2D drawing, window widgets, network
> protocols, C++ support. After reading you'll know: when you want to do
> something, which directory to look in for which library, and which
> `-lxxx` to link. This is the third (final) article of the "ewokos
> series".

[16](16-libs-overview.md) gave the map; [17](17-libc.md) covered the
bottommost libc. This chapter introduces the common libraries layer by
layer, by "library settlement". All libraries follow the uniform directory
structure from §16.3.

## 18.1 The Basic Libraries (system/basic/libs)

Low-level libraries dealing with hardware, filesystems, and data formats
([system/basic/libs/](../../system/basic/libs/)). The build list is in
[libs/Makefile](../../system/basic/libs/Makefile)'s `DIRS`.

### Storage and Filesystems

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [sd](../../system/basic/libs/sd/) | `-lsd` | `sd/sd.h`, `sd/gpt.h`, `sd/partition.h` | SD card / block device read-write, GPT and MBR partition-table parsing (Ch. 10) |
| [ext2](../../system/basic/libs/ext2/) | `-lext2` | `ext2/ext2fs.h`, `ext2/ext2head.h` | ext2 filesystem read-write (the rootfs is ext2) |
| [ext3](../../system/basic/libs/ext3/) | `-lext3` | `ext3/...` | the ext3 filesystem (ext2 with journaling) |
| [fat32](../../system/basic/libs/fat32/) | `-lfat32` | `fat32/fat32fs.h`, `fat32/fat32head.h` | the FAT32 filesystem (USB sticks, SD card FAT partitions) |
| [elf](../../system/basic/libs/elf/) | `-lelf` | `elf/elf.h` | parsing and loading the ELF executable format (used by Ch. 07's exec) |

> Dependencies: `ext2` and `ext3` are both built on the block-device access
> provided by `sd`, hence the Makefile line `ext2 ext3: sd`.

### Data and Compression

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [tinyjson](../../system/basic/libs/tinyjson/) | `-ltinyjson` | `tinyjson/tinyjson.h` | lightweight JSON parsing/generation. All of the system's `.json` configs rely on it (chapters 10, 13) |
| [zlib](../../system/basic/libs/zlib/) | `-lz` | `zlib.h` | the classic compression library: deflate/inflate, gzip, the underlying dependency of PNG |
| [openlibm](../../system/basic/libs/openlibm/) | `-lopenlibm` | `math.h` | the math library `libm` (introduced in Ch. [17](17-libc.md)) |

### Hardware Access

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [gpio](../../system/basic/libs/gpio/) | `-lgpio` | `gpio/gpio.h` | GPIO pin read-write (light LEDs, read buttons, bit-bang protocols) |
| [usb](../../system/basic/libs/usb/) | `-lusb` | `usb/usb_defs.h`, `usb/bsp_usb.h`, `usb/usbhid.h`, `usb/usbhidsrv.h` | the USB host stack and HID (keyboard/mouse) device support |

### C++ Support

EwokOS's GUI and quite a few applications are written in C++, and the C++
runtime also lives in the basic libraries
([system/basic/libs/c++/](../../system/basic/libs/c++/)):

| Library | Link name | Purpose |
|----|--------|------|
| `c++/c++` | (the C++ runtime) | C++ language runtime support (`cxx.cc`: construction, exception tables, RTTI, and other infrastructure) |
| `c++/stl` | `-lewokstl` | EwokOS's trimmed STL (containers, algorithms) |
| `c++/object++` | (UniObject) | `UniObject` — bidirectional object↔JSON binding/reflection, depends on tinyjson |

> Dependency chain: `c++/stl` and `c++/object++` both depend on `c++/c++`;
> `object++` additionally depends on `tinyjson`.

## 18.2 The Graphics Libraries (system/gui/libs)

2D drawing, fonts, image codecs, display, and input
([system/gui/libs/](../../system/gui/libs/)). The build list is in
[gui/libs/Makefile](../../system/gui/libs/Makefile). This whole layer is
linked as a bundle via `EWOK_LIB_GRAPH`.

### The Drawing Core

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [graph](../../system/gui/libs/graph/) | `-lgraph` | `graph/graph.h`, `graph_image.h`, `graph_ex.h`, `rgb24.h`, `rgb15.h`, `uv12.h` | **the 2D graphics core**: points/lines/rectangles, bit blits (blt), alpha blending, color-space conversion, and loading PNG/JPEG/GIF/TGA/SVG (Ch. 13) |
| [g2dclient](../../system/gui/libs/g2dclient/) | `-lg2dclient` | `graph/graph_g2d.h` | the 2D graphics **hardware acceleration** client (offloads blt/fill to the g2d driver) |
| [libiconbuf](../../system/gui/libs/libiconbuf/) | `-liconbuf` | `iconbuf/iconbuf.h` | the icon buffer: load/cache icon resources |

### Fonts

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [font](../../system/gui/libs/font/) | `-lfont` | `font/font.h` | text rendering: draw strings into bitmaps, manage fonts/sizes/colors |
| [freetype](../../system/gui/libs/freetype/) | `-lfreetype` | (FreeType) | the industry-standard TrueType/OpenType font rasterization engine, `font`'s foundation |

### Image Codecs

| Library | Link name | Purpose |
|----|--------|------|
| [libpng](../../system/gui/libs/libpng/) | `-lpng` | PNG encode/decode (depends on zlib) |
| [libjpeg](../../system/gui/libs/libjpeg/) | `-ljpeg` | JPEG decoding |
| [libgif](../../system/gui/libs/libgif/) | `-lgif` | GIF decoding |
| [libtga](../../system/gui/libs/libtga/) | `-ltga` | TGA (Targa) images |
| [libsvg](../../system/gui/libs/libsvg/) | `-lsvg` | SVG vector rendering (based on plutovg) |

### Display and Terminals

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [display](../../system/gui/libs/display/) | `-ldisplay` | `display/display.h` | the display-device abstraction (resolution, color depth, flipping) |
| [displayman](../../system/gui/libs/displayman/) | `-ldisplayman` | `displayman/displayman.h` | display management (multiple outputs / dynamic timings) |
| [displayd](../../system/gui/libs/displayd/) | (driver) | — | the display-daemon-side library |
| [textgrid](../../system/gui/libs/textgrid/) | `-ltextgrid` | `textgrid/textgrid.h`, `text_content.h` | the character grid: a terminal's "rows/columns + characters" model |
| [gterminal](../../system/gui/libs/gterminal/) | `-lgterminal` | `gterminal/gterminal.h` | the graphical terminal: renders a textgrid with font/graph |

### Input and Audio

| Library | Link name | Purpose |
|----|--------|------|
| [keyb](../../system/gui/libs/keyb/) | `-lkeyb` | keyboard input |
| [mouse](../../system/gui/libs/mouse/) | `-lmouse` | mouse input |
| [minimp3](../../system/gui/libs/minimp3/) | — | MP3 decoding (single header) |
| [libogg](../../system/gui/libs/libogg/) / [libvorbis](../../system/gui/libs/libvorbis/) | `-logg`/`-lvorbis` | the OGG container and Vorbis audio decoding |

> Dependencies (from the Makefile): `graph` depends on the image codec
> libraries and `g2dclient`; `font` depends on `freetype`;
> `display`/`libiconbuf` depend on `graph`; `textgrid`/`gterminal` depend
> on `font`+`graph`, and `gterminal` depends on `textgrid`.

## 18.3 The Window Libraries (system/xwin/libs)

The X-style window system's client libraries and the C++ widget toolkit
([system/xwin/libs/](../../system/xwin/libs/)). The build chain: `x` →
`x++` → `widget++`, linked as a bundle via `EWOK_LIB_X` (Ch. 13).

| Library | Link name | Headers | Purpose |
|----|--------|--------|------|
| [x](../../system/xwin/libs/x/) | `-lx` | `x/x.h`, `xwin.h`, `xevent.h`, `xwm.h`, `xtheme.h`, `xcntl.h` | the window system's C interface: create windows, send/receive events, interact with the window manager/themes |
| [x++](../../system/xwin/libs/x++/) | `-lx++` | `x++/X.h`, `XWin.h`, `XWM.h`, `XTheme.h` | a C++ wrapper of `x`, operating windows object-orientedly |
| [widget++](../../system/xwin/libs/widget++/) | `-lwidget++` | `Widget/*.h` | **the C++ widget toolkit**: ready-made UI components |

The widgets `widget++` provides (`include/Widget/`) are quite complete:

```
Widget (base)  Container  WidgetWin / RootWidget / WidgetX
Button  RoundButton  LabelButton  RoundLabelButton  Label  Image
Text  EditLine  List  ListBase  Scroller  Scrollable  Slider
Grid  Columns  Split  Splitter  Stage  Blank  SpriteAnim  SpriteWin
```

To write a graphical app with buttons, lists, and input fields, just
assemble these widgets — no need to start from drawing pixels.

## 18.4 The Network Libraries (system/network/libs)

Network protocols and upper-level application protocols
([system/network/libs/](../../system/network/libs/)). The build list is in
[network/libs/Makefile](../../system/network/libs/Makefile).

| Library | Link name | Purpose |
|----|--------|------|
| [socket](../../system/network/libs/socket/) | `-lsocket` | the BSD socket API: `socket`/`bind`/`connect`/`listen`/`accept`/`send`/`recv`, plus `inet_ntop`/`inet_pton`/`inet_ntoa`. Underneath is the userland netd protocol stack (Ch. 09 IPC + the network daemon) |
| [ntpc](../../system/network/libs/ntpc/) | `-lntpc` | an NTP client: synchronize time with a time server |
| [wolfssl](../../system/network/libs/wolfssl/) | `-lwolfssl` | an embedded TLS/SSL library (wolfSSL), providing HTTPS/encryption capability |
| [libtinyhttpsc](../../system/network/libs/libtinyhttpsc/) | `-ltinyhttpsc` | a lightweight HTTP server/client; can run on top of wolfSSL for HTTPS |
| [libwebsockets](../../system/network/libs/libwebsockets/) | `-lwebsockets` | a WebSocket library: full-duplex real-time communication |

> Dependencies: `ntpc`/`wolfssl`/`libwebsockets` all need `socket`'s
> headers installed first; `libtinyhttpsc` depends on both `socket` and
> `wolfssl`.

## 18.5 Library Quick Reference (Look Up by "I Want to…")

| I want to… | Use this library | Link |
|--------|----------|------|
| read/write the SD card / parse partitions | sd | `-lsd` |
| read/write ext2/ext3/FAT32 files | ext2 / ext3 / fat32 | `-lext2` etc. |
| parse/load ELF executables | elf | `-lelf` |
| parse JSON configs | tinyjson | `-ltinyjson` |
| compress/decompress (gzip, zip, PNG's foundation) | zlib | `-lz` |
| do math (sin/cos/sqrt) | openlibm | `-lopenlibm` |
| light LEDs / read GPIO | gpio | `-lgpio` |
| attach USB keyboards/mice/HID | usb | `-lusb` |
| draw points/lines/shapes, blit, alpha blend | graph | `-lgraph` (`EWOK_LIB_GRAPH`) |
| hardware-accelerated 2D blt/fill | g2dclient | `-lg2dclient` |
| render TrueType fonts/text | font + freetype | `-lfont -lfreetype` |
| decode PNG/JPEG/GIF/TGA/SVG | libpng/libjpeg/libgif/libtga/libsvg | `-lpng` etc. |
| operate the display/framebuffer | display / displayman / fb | `-ldisplay` etc. |
| build a character terminal | textgrid + gterminal | `-lgterminal` |
| read keyboard/mouse | keyb / mouse | `-lkeyb -lmouse` |
| play MP3/OGG audio | minimp3 / libogg+libvorbis | — |
| open windows, send/receive window events | x / x++ | `-lx -lx++` (`EWOK_LIB_X`) |
| use ready-made UI widgets (buttons/lists/input fields) | widget++ | `-lwidget++` |
| send TCP/UDP packets | socket | `-lsocket` |
| do HTTPS / encryption | wolfssl + libtinyhttpsc | `-lwolfssl -ltinyhttpsc` |
| do WebSocket real-time communication | libwebsockets | `-lwebsockets` |
| synchronize time over NTP | ntpc | `-lntpc` |
| use C++ / STL / object-JSON binding | c++ / stl / object++ | `-lewokstl` etc. |

## 18.6 And More: Third-Party Suites in sw.extra

What this chapter lists are the libraries **shipped with the core system**.
The repo also has a separate submodule
[sw.extra/](../../sw.extra/), collecting larger third-party suites and
applications, for example:

- **SDL2** ([sw.extra/libs/SDL2/](../../sw.extra/libs/SDL2/)) — a
  cross-platform multimedia/game development layer;
- **curses** ([sw.extra/libs/curses/](../../sw.extra/libs/curses/)) — text
  terminal UI (`libcurses` + `libterminfo`);
- **vim**, a calculator, a calendar, and other applications (the `widget++`
  extension widgets are in `projects`, see [19](19-extra-libs.md)).

They are likewise vendored as source and cross-compiled with customized
Makefiles, but they count as "extended software" — not part of the core
`system/` build chain, enabled on demand.

## 18.7 Summary

- **Basic libraries** (`basic/libs`): filesystems (sd/ext2/ext3/fat32/elf),
  data (tinyjson/zlib), math (openlibm), hardware (gpio/usb), C++
  (c++/stl/object++);
- **Graphics libraries** (`gui/libs`): the drawing core graph + g2dclient,
  fonts font+freetype, image codecs png/jpeg/gif/tga/svg, display
  display/fb, terminals textgrid/gterminal, input keyb/mouse, audio
  minimp3/ogg/vorbis — uniformly linked via `EWOK_LIB_GRAPH`;
- **Window libraries** (`xwin/libs`): x → x++ → widget++ (rich C++
  widgets), linked via `EWOK_LIB_X`;
- **Network libraries** (`network/libs`): socket, ntpc, wolfssl,
  libtinyhttpsc, libwebsockets;
- Bigger third-party suites (SDL2, curses, vim…) are in the separate
  submodule `sw.extra/`.

That concludes the three articles of the "ewokos series": you now hold a
complete map from libc to the common libraries. What next? Go back to
[15 Debugging and the Road Ahead](15-debug.md), or pick a library and read
its source — they're all small with clear structures, perfect as advanced
exercises.
