/* file - determine file type.
   Command-line and output-format compatible with the Linux/BSD file(1)
   utility (Ian Darwin / Christos Zoulas).

   The detection engine mirrors the upstream behaviour that matters for a
   POSIX-ish rootfs:
     * a full ELF reader (class, endianness, type, machine + arch flags,
       dynamic/static linking, PT_INTERP path, GNU BuildID, ABI-tag note and
       the stripped / with-debug_info verdict),
     * the upstream text_chars[] classification table so ASCII / UTF-8 /
       ISO-8859 / extended-ASCII / binary are decided exactly like file(1),
     * the same ", with very long lines (N)" / ", with CRLF, CR line
       terminators" / ", with escape sequences" qualifier ordering,
     * shebang script recognition with the friendly interpreter names,
     * a magic-signature table for the common container/image/archive/audio
       formats, with per-format detail (gzip flags, PNG IHDR, ZIP entries,
       RIFF form, ISO-media brand, PE machine/subsystem, ...),
     * special files (directory, char/block device, fifo, socket, symlink)
       and the "empty" / "data" / "cannot open" fallbacks.

   Output goes through -b/--brief, -F/--separator, -0/--print0 and the
   --mime / --mime-type / --mime-encoding modes, matching GNU file.

   EwokOS notes: the VFS has no symbolic links and lstat() is an alias of
   stat(), so -h/-L are accepted for compatibility but never change the
   result; readlink() returns ENOSYS.  st_rdev packs major in bits 8..15 and
   minor in bits 0..7, the same layout ls(1) already decodes. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifndef O_RDONLY
#define O_RDONLY 0
#endif
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

/* Bytes sampled from the head of a file for content sniffing.  ELF detail
   (program/section headers, notes) is fetched separately with read_at(). */
#define READ_BUF    (64 * 1024)
#define DESC_SZ     1024
#define MIME_SZ     128
#define CHARSET_SZ  64

/* file(1) ascmagic.c: "longest sane line length". */
#define MAXLINELEN  300

/* ── options ───────────────────────────────────────────────────────────── */
enum { OUT_TEXT = 0, OUT_MIME, OUT_MIME_TYPE, OUT_MIME_ENCODING };

static int         g_out_mode   = OUT_TEXT;
static int         g_brief      = 0;   /* -b  : no filename prefix          */
static int         g_deref      = 0;   /* -L  : follow symlinks             */
static int         g_special    = 0;   /* -s  : read block/char devices     */
static int         g_print0     = 0;   /* -0  : NUL after filename          */
static int         g_uncompress = 0;   /* -z  : peek inside containers      */
static int         g_nobuffer   = 0;   /* -n  : flush after each file       */
static const char* g_sep        = ":"; /* -F  : filename/result separator   */

/* Long-only option sentinels; getopt_long returns val unchanged. */
enum {
    OPT_MIME_TYPE = 256,
    OPT_MIME_ENCODING,
    OPT_EXTENSION,
    OPT_HELP
};

/* ── result of analysing one file ─────────────────────────────────────── */
typedef struct {
    char desc[DESC_SZ];          /* human-readable description            */
    char mime_type[MIME_SZ];     /* e.g. "text/plain"                     */
    char mime_charset[CHARSET_SZ];/* e.g. "us-ascii"                      */
} result_t;

/* printf-style append onto a NUL-terminated buffer (never overflows). */
static void dcat(char* d, size_t dsz, const char* fmt, ...) {
    size_t len = strlen(d);
    va_list ap;
    if(len + 1 >= dsz)
        return;
    va_start(ap, fmt);
    vsnprintf(d + len, dsz - len, fmt, ap);
    va_end(ap);
}

/* ── ELF name tables ──────────────────────────────────────────────────── */
static const char* elf_machine_name(uint16_t m) {
    switch(m) {
    case 0:   return "no machine";
    case 1:   return "AT&T WE 32100";
    case 2:   return "SPARC";
    case 3:   return "Intel 80386";
    case 4:   return "Motorola 68000";
    case 5:   return "Motorola 88000";
    case 6:   return "Intel 80486";
    case 7:   return "Intel 80860";
    case 8:   return "MIPS, RS3000 Big-endian";
    case 9:   return "IBM System/370";
    case 10:  return "MIPS RS3000 Little-endian";
    case 15:  return "HP/PA";
    case 17:  return "Fujitsu VPP500";
    case 18:  return "SPARC with enhanced instruction set";
    case 19:  return "Intel 80960";
    case 20:  return "PowerPC";
    case 21:  return "PowerPC 64-bit";
    case 22:  return "IBM S/390";
    case 36:  return "NEC V800";
    case 40:  return "ARM";
    case 41:  return "Digital Alpha";
    case 42:  return "Hitachi SH";
    case 43:  return "SPARC V9";
    case 44:  return "Siemens Tricore";
    case 45:  return "Argonaut RISC Core";
    case 46:  return "Hitachi H8/300";
    case 47:  return "Hitachi H8/300H";
    case 48:  return "Hitachi H8S";
    case 49:  return "Hitachi H8/500";
    case 50:  return "Intel IA-64";
    case 51:  return "Stanford MIPS-X";
    case 52:  return "Motorola Coldfire";
    case 53:  return "Motorola M68HC12";
    case 62:  return "x86-64";
    case 63:  return "Sony DSP Processor";
    case 75:  return "Digital VAX";
    case 76:  return "Axis Communications 32-bit embedded processor";
    case 77:  return "Infineon Technologies 32-bit embedded cpu";
    case 83:  return "Atmel AVR 8-bit microcontroller";
    case 84:  return "Fujitsu FR30";
    case 85:  return "Mitsubishi D10V";
    case 86:  return "Mitsubishi D30V";
    case 87:  return "NEC v850";
    case 88:  return "Renesas M32R";
    case 89:  return "Matsushita MN10300";
    case 90:  return "Matsushita MN10200";
    case 91:  return "picoJava";
    case 92:  return "OpenRISC 32-bit embedded processor";
    case 94:  return "Tensilica Xtensa Architecture";
    case 140: return "Infineon S/390";
    case 183: return "AArch64";
    case 188: return "Tilera TILE64";
    case 220: return "Zilog Z80";
    case 243: return "RISC-V";
    case 247: return "Linux BPF - in-kernel virtual machine";
    case 257: return "WDC 65C816";
    default:  return NULL;
    }
}

static const char* elf_osabi_name(uint8_t a) {
    switch(a) {
    case 0:   return "SYSV";
    case 1:   return "HP-UX";
    case 2:   return "NetBSD";
    case 3:   return "GNU/Linux";
    case 4:   return "Hurd";
    case 5:   return "86Open";
    case 6:   return "Solaris";
    case 7:   return "AIX";
    case 8:   return "IRIX";
    case 9:   return "FreeBSD";
    case 10:  return "TRU64";
    case 11:  return "Novell Modesto";
    case 12:  return "OpenBSD";
    case 13:  return "OpenVMS";
    case 14:  return "NonStop Kernel";
    case 15:  return "AROS";
    case 16:  return "Fenix OS";
    case 17:  return "CloudABI";
    case 97:  return "ARM";
    case 255: return "Standalone App";
    default:  return "unknown OS ABI";
    }
}

/* Endian-aware unsigned load of 1..8 bytes. */
static uint64_t elf_get(const unsigned char* p, int size, int be) {
    uint64_t v = 0;
    int i;
    if(be)
        for(i = 0; i < size; i++) v = (v << 8) | p[i];
    else
        for(i = size - 1; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}

/* ── text classification (verbatim from file(1) encoding.c) ───────────── */
#define F 0   /* character never appears in text                  */
#define T 1   /* character appears in plain ASCII text            */
#define I 2   /* character appears in ISO-8859 text               */
#define X 3   /* character appears in non-ISO extended ASCII      */

static const char text_chars[256] = {
    /*                  BEL BS HT LF VT FF CR SO SI */
    F, F, F, F, F, F, F, T, T, T, T, T, T, T, T, T,  /* 0x0X */
    /*                           SUB ESC          */
    F, F, F, F, F, F, F, F, F, F, T, T, F, F, F, F,  /* 0x1X */
    T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
    T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
    T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
    T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
    T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
    T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
    /*            NEL                            */
    X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
    X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
};

/* Encoding verdict, using the released file(1) wording ("UTF-8 Unicode
   text", not the unreleased master-branch "Unicode text, UTF-8"). */
typedef enum {
    ENC_ASCII = 0,
    ENC_UTF8_BOM,
    ENC_UTF8,
    ENC_UTF16LE,
    ENC_UTF16BE,
    ENC_LATIN1,
    ENC_EXTENDED,
    ENC_BINARY
} enc_kind_t;

static const char* enc_code(enc_kind_t k) {
    switch(k) {
    case ENC_ASCII:    return "ASCII";
    case ENC_UTF8_BOM: return "UTF-8 Unicode (with BOM)";
    case ENC_UTF8:     return "UTF-8 Unicode";
    case ENC_UTF16LE:  return "Little-endian UTF-16 Unicode";
    case ENC_UTF16BE:  return "Big-endian UTF-16 Unicode";
    case ENC_LATIN1:   return "ISO-8859";
    case ENC_EXTENDED: return "Non-ISO extended-ASCII";
    default:           return "binary";
    }
}

static const char* enc_charset(enc_kind_t k) {
    switch(k) {
    case ENC_ASCII:    return "us-ascii";
    case ENC_UTF8_BOM: return "utf-8";
    case ENC_UTF8:     return "utf-8";
    case ENC_UTF16LE:  return "utf-16le";
    case ENC_UTF16BE:  return "utf-16be";
    case ENC_LATIN1:   return "iso-8859-1";
    case ENC_EXTENDED: return "unknown-8bit";
    default:           return "binary";
    }
}

static int looks_all(const unsigned char* buf, size_t len, int allow_I, int allow_X) {
    size_t i;
    for(i = 0; i < len; i++) {
        int t = (unsigned char)text_chars[buf[i]];
        if(t == T) continue;
        if(allow_I && t == I) continue;
        if(allow_X && t == X) continue;
        return 0;
    }
    return 1;
}

/* Returns 1 = pure 7-bit, 2 = valid multi-byte UTF-8, 0 = not UTF-8.
   A sequence truncated by the end of the sampled buffer is accepted. */
static int looks_utf8(const unsigned char* buf, size_t len, int bom) {
    size_t i = bom ? 3 : 0;
    int has_multi = 0;
    while(i < len) {
        unsigned char c = buf[i];
        int n; unsigned int cp;
        if(c < 0x80) { i++; continue; }
        if((c & 0xe0) == 0xc0 && c >= 0xc2)      { n = 1; cp = c & 0x1f; }
        else if((c & 0xf0) == 0xe0)               { n = 2; cp = c & 0x0f; }
        else if((c & 0xf8) == 0xf0 && c <= 0xf4)  { n = 3; cp = c & 0x07; }
        else return 0;
        if((size_t)n > len - i - 1)
            return has_multi ? 2 : 1;   /* truncated at buffer end */
        int k;
        for(k = 1; k <= n; k++) {
            if((buf[i + k] & 0xc0) != 0x80) return 0;
            cp = (cp << 6) | (buf[i + k] & 0x3f);
        }
        if(n == 1 && cp < 0x80) return 0;
        if(n == 2) {
            if(cp < 0x800) return 0;
            if(cp >= 0xd800 && cp <= 0xdfff) return 0;
        }
        if(n == 3 && (cp < 0x10000 || cp > 0x10ffff)) return 0;
        i += (size_t)n + 1;
        has_multi = 1;
    }
    return has_multi ? 2 : 1;
}

/* Cheap UTF-16 heuristic: a BOM, or a steady stream of ASCII bytes padded
   with NULs on one side.  Returns ENC_UTF16LE / ENC_UTF16BE / -1. */
static int looks_utf16(const unsigned char* buf, size_t len) {
    size_t i, zeros_even = 0, zeros_odd = 0, pairs;
    if(len >= 2) {
        if(buf[0] == 0xff && buf[1] == 0xfe) return ENC_UTF16LE;
        if(buf[0] == 0xfe && buf[1] == 0xff) return ENC_UTF16BE;
    }
    if(len < 8 || (len & 1))
        return -1;
    pairs = len / 2;
    for(i = 0; i + 1 < len; i += 2) {
        if(buf[i] == 0x00 && buf[i + 1] != 0x00)     zeros_even++;
        else if(buf[i + 1] == 0x00 && buf[i] != 0x00) zeros_odd++;
    }
    if(zeros_odd  > pairs * 3 / 4 && zeros_odd  > zeros_even) return ENC_UTF16LE;
    if(zeros_even > pairs * 3 / 4 && zeros_even > zeros_odd)  return ENC_UTF16BE;
    return -1;
}

static enc_kind_t detect_encoding(const unsigned char* buf, size_t len) {
    int u;
    if(len == 0)
        return ENC_BINARY;
    if(looks_all(buf, len, 0, 0))
        return ENC_ASCII;
    if(len >= 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf) {
        if(looks_utf8(buf, len, 1) > 0)
            return ENC_UTF8_BOM;
    }
    u = looks_utf8(buf, len, 0);
    if(u > 1)
        return ENC_UTF8;
    {
        int u16 = looks_utf16(buf, len);
        if(u16 == ENC_UTF16LE) return ENC_UTF16LE;
        if(u16 == ENC_UTF16BE) return ENC_UTF16BE;
    }
    if(looks_all(buf, len, 1, 0))
        return ENC_LATIN1;
    if(looks_all(buf, len, 1, 1))
        return ENC_EXTENDED;
    return ENC_BINARY;
}

/* Append the ", with ..." qualifiers file(1) adds after "<encoding> text".
   Ordering and wording follow src/ascmagic.c: very long lines first, then
   the line-terminator clause, then escape sequences, then overstriking. */
static void text_qualifiers(char* desc, size_t dsz, const unsigned char* buf, size_t len) {
    size_t i, last_line_end = (size_t)-1, has_long = 0;
    size_t n_crlf = 0, n_lf = 0, n_cr = 0, n_nel = 0;
    int seen_cr = 0, has_esc = 0, has_bs = 0;

    for(i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if(c == '\n') {
            if(seen_cr) n_crlf++; else n_lf++;
            last_line_end = i;
        } else if(seen_cr) {
            n_cr++;
            last_line_end = i - 1;
        }
        seen_cr = (c == '\r');
        if(seen_cr) last_line_end = i;
        if(c == 0x85) { n_nel++; last_line_end = i; }
        if(i > last_line_end + MAXLINELEN) {
            size_t ll = i - last_line_end;
            if(ll > has_long) has_long = ll;
        }
        if(c == 0x1b) has_esc = 1;
        if(c == 0x08) has_bs = 1;
    }
    if(seen_cr && n_cr == 0 && n_crlf == 0)
        n_cr++;

    if(has_long)
        dcat(desc, dsz, ", with very long lines (%lu)", (unsigned long)has_long);

    /* Report terminators only if there are none at all, or a non-LF one. */
    if((n_crlf == 0 && n_cr == 0 && n_nel == 0 && n_lf == 0) ||
       (n_crlf != 0 || n_cr != 0 || n_nel != 0)) {
        dcat(desc, dsz, ", with");
        if(n_crlf == 0 && n_cr == 0 && n_nel == 0 && n_lf == 0) {
            dcat(desc, dsz, " no");
        } else {
            if(n_crlf) {
                dcat(desc, dsz, " CRLF");
                if(n_cr || n_lf || n_nel) dcat(desc, dsz, ",");
            }
            if(n_cr) {
                dcat(desc, dsz, " CR");
                if(n_lf || n_nel) dcat(desc, dsz, ",");
            }
            if(n_lf) {
                dcat(desc, dsz, " LF");
                if(n_nel) dcat(desc, dsz, ",");
            }
            if(n_nel) dcat(desc, dsz, " NEL");
        }
        dcat(desc, dsz, " line terminators");
    }
    if(has_esc) dcat(desc, dsz, ", with escape sequences");
    if(has_bs)  dcat(desc, dsz, ", with overstriking");
}

/* Strip trailing NUL bytes (file(1) trim_nuls) but keep at least one byte. */
static size_t trim_nuls(const unsigned char* buf, size_t len) {
    while(len > 1 && buf[len - 1] == '\0')
        len--;
    return len;
}


/* ── random-access helper (regular files are seekable; stdin is not) ──── */
static int read_at(int fd, uint64_t off, unsigned char* dst, size_t n) {
    size_t got = 0;
    if(fd < 0 || n == 0)
        return 0;
    if(lseek(fd, (off_t)off, SEEK_SET) < 0)
        return -1;
    while(got < n) {
        ssize_t k = read(fd, dst + got, n - got);
        if(k > 0) { got += (size_t)k; continue; }
        if(k < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        break;
    }
    return (int)got;
}

/* Return a pointer to n bytes at file offset off: straight out of the
   already-sampled head buffer when possible, else read into scratch. */
static const unsigned char* elf_fetch(int fd, const unsigned char* buf, size_t len,
                                      uint64_t off, size_t n, unsigned char* scratch) {
    if(off + n <= len)
        return buf + off;
    if(read_at(fd, off, scratch, n) == (int)n)
        return scratch;
    return NULL;
}

/* Parse one ELF note blob, extracting the GNU BuildID and ABI tag. */
static void parse_notes(const unsigned char* p, size_t sz, int be,
                        char* buildid, size_t bidsz,
                        char* abi, size_t abisz) {
    size_t o = 0;
    buildid[0] = 0;
    abi[0] = 0;
    while(o + 12 <= sz) {
        uint32_t namesz = (uint32_t)elf_get(p + o, 4, be);
        uint32_t descsz = (uint32_t)elf_get(p + o + 4, 4, be);
        uint32_t type   = (uint32_t)elf_get(p + o + 8, 4, be);
        size_t name_off = o + 12;
        size_t desc_off = name_off + ((namesz + 3) & ~(size_t)3);
        size_t next     = desc_off + ((descsz + 3) & ~(size_t)3);
        if(namesz > 64 || desc_off > sz || next > sz || next <= o)
            break;
        if(namesz == 4 && memcmp(p + name_off, "GNU", 4) == 0) {
            if(type == 3 && descsz > 0 && descsz <= 64 && buildid[0] == 0) {
                /* NT_GNU_BUILD_ID */
                static const char hex[] = "0123456789abcdef";
                size_t k, cap = (bidsz - 1) / 2;
                if(descsz < cap) cap = descsz;
                for(k = 0; k < cap; k++) {
                    buildid[k * 2]     = hex[p[desc_off + k] >> 4];
                    buildid[k * 2 + 1] = hex[p[desc_off + k] & 0xf];
                }
                buildid[cap * 2] = 0;
            } else if(type == 1 && descsz >= 16 && abi[0] == 0) {
                /* NT_GNU_ABI_TAG: os, major, minor, patch */
                uint32_t os    = (uint32_t)elf_get(p + desc_off, 4, be);
                uint32_t major = (uint32_t)elf_get(p + desc_off + 4, 4, be);
                uint32_t minor = (uint32_t)elf_get(p + desc_off + 8, 4, be);
                uint32_t patch = (uint32_t)elf_get(p + desc_off + 12, 4, be);
                const char* osn = (os == 0) ? "GNU/Linux" :
                                  (os == 1) ? "GNU" :
                                  (os == 2) ? "Solaris" :
                                  (os == 3) ? "FreeBSD" :
                                  (os == 4) ? "NetBSD" : "OS";
                snprintf(abi, abisz, "for %s %u.%u.%u", osn, major, minor, patch);
            }
        }
        o = next;
    }
}

/* Full ELF description, matching readelf.c field order. */
static void describe_elf(int fd, const unsigned char* buf, size_t len,
                         const struct stat* st, result_t* r) {
    int cls, be, have_interp = 0, have_dynamic = 0, stripped = 1, has_debug = 0;
    uint16_t etype, emach;
    uint32_t eversion, eflags = 0;
    uint64_t phoff = 0, shoff = 0;
    int phentsize = 0, phnum = 0, shentsize = 0, shnum = 0, shstrndx = 0;
    const char* typeword;
    char interp[256] = "", buildid[160] = "", abi[96] = "";
    unsigned char scratch[512];
    int i;

    if(len < 20) {
        snprintf(r->desc, DESC_SZ, "ELF (truncated)");
        snprintf(r->mime_type, MIME_SZ, "application/x-executable");
        snprintf(r->mime_charset, CHARSET_SZ, "binary");
        return;
    }

    cls = buf[4];                    /* 1 = 32-bit, 2 = 64-bit */
    be  = (buf[5] == 2);             /* EI_DATA: 1 = LSB, 2 = MSB */
    etype    = (uint16_t)elf_get(buf + 16, 2, be);
    emach    = (uint16_t)elf_get(buf + 18, 2, be);
    eversion = (uint32_t)elf_get(buf + 20, 4, be);

    if(cls == 1 && len >= 52) {
        eflags    = (uint32_t)elf_get(buf + 36, 4, be);
        phoff     = elf_get(buf + 28, 4, be);
        shoff     = elf_get(buf + 32, 4, be);
        phentsize = (int)elf_get(buf + 42, 2, be);
        phnum     = (int)elf_get(buf + 44, 2, be);
        shentsize = (int)elf_get(buf + 46, 2, be);
        shnum     = (int)elf_get(buf + 48, 2, be);
        shstrndx  = (int)elf_get(buf + 50, 2, be);
    } else if(cls == 2 && len >= 64) {
        eflags    = (uint32_t)elf_get(buf + 48, 4, be);
        phoff     = elf_get(buf + 32, 8, be);
        shoff     = elf_get(buf + 40, 8, be);
        phentsize = (int)elf_get(buf + 54, 2, be);
        phnum     = (int)elf_get(buf + 56, 2, be);
        shentsize = (int)elf_get(buf + 58, 2, be);
        shnum     = (int)elf_get(buf + 60, 2, be);
        shstrndx  = (int)elf_get(buf + 62, 2, be);
    }

    /* Program headers: PT_INTERP (3) gives the loader path, PT_DYNAMIC (2)
       means the object participates in dynamic linking. */
    for(i = 0; i < phnum && phentsize >= (cls == 1 ? 32 : 56); i++) {
        const unsigned char* p = elf_fetch(fd, buf, len,
                phoff + (uint64_t)i * phentsize, (size_t)phentsize, scratch);
        uint32_t p_type;
        uint64_t p_off, p_filesz;
        if(!p) break;
        p_type = (uint32_t)elf_get(p, 4, be);
        if(cls == 1) { p_off = elf_get(p + 4, 4, be);  p_filesz = elf_get(p + 16, 4, be); }
        else         { p_off = elf_get(p + 8, 8, be);  p_filesz = elf_get(p + 32, 8, be); }
        if(p_type == 2) have_dynamic = 1;
        else if(p_type == 3) {
            have_interp = 1;
            if(p_filesz > 0 && p_filesz < sizeof(interp)) {
                int n = read_at(fd, p_off, (unsigned char*)interp, (size_t)p_filesz);
                if(n > 0) {
                    while(n > 0 && (interp[n - 1] == '\0' || interp[n - 1] == '\n')) n--;
                    interp[n] = 0;
                } else interp[0] = 0;
            }
        }
    }

    switch(etype) {
    case 1: typeword = "relocatable"; break;
    case 2: typeword = "executable";  break;
    case 3: typeword = have_interp ? "pie executable" : "shared object"; break;
    case 4: typeword = "core file";   break;
    default: typeword = "unknown type"; break;
    }

    /* Head of the description: ELF <class>-bit <endian> <type>, <machine> */
    r->desc[0] = 0;
    dcat(r->desc, DESC_SZ, "ELF %d-bit %s %s, ",
         cls == 2 ? 64 : 32, be ? "MSB" : "LSB", typeword);
    {
        const char* mn = elf_machine_name(emach);
        if(mn) dcat(r->desc, DESC_SZ, "%s", mn);
        else   dcat(r->desc, DESC_SZ, "unknown arch 0x%x", emach);
    }

    /* ARM carries EABI version + float ABI in e_flags. */
    if(emach == 40) {
        unsigned eabi = (eflags >> 24) & 0xff;
        if(eabi) dcat(r->desc, DESC_SZ, ", EABI%d", eabi);
        if(eflags & 0x00000400)      dcat(r->desc, DESC_SZ, ", hard-float");
        else if(eflags & 0x00000200) dcat(r->desc, DESC_SZ, ", soft-float");
    } else if(emach == 8 || emach == 10) {
        /* MIPS ABI lives in the low nibble of e_flags. */
        static const char* mipsabi[] = { "", " [abi 1]", " [abi 2]", "", "", "", "", "" };
        unsigned abi_n = eflags & 0x7;
        if(abi_n < 8 && mipsabi[abi_n][0]) dcat(r->desc, DESC_SZ, ",%s", mipsabi[abi_n]);
        if(eflags & 0x00000020) dcat(r->desc, DESC_SZ, ", 64-bit registers");
    }

    dcat(r->desc, DESC_SZ, ", version %u (%s)", eversion, elf_osabi_name(buf[7]));

    if(have_dynamic || have_interp) dcat(r->desc, DESC_SZ, ", dynamically linked");
    else if(etype == 2 || etype == 3) dcat(r->desc, DESC_SZ, ", statically linked");

    if(have_interp && interp[0])
        dcat(r->desc, DESC_SZ, ", interpreter %s", interp);

    /* Section headers: symbol table => not stripped; .debug* => debug_info;
       SHT_NOTE (7) sections carry BuildID / ABI-tag. */
    if(shnum > 0 && shentsize >= (cls == 1 ? 40 : 64) && shoff) {
        uint64_t shstr_off = 0, shstr_sz = 0;
        unsigned char* shstr = NULL;
        if(shstrndx < shnum) {
            const unsigned char* s = elf_fetch(fd, buf, len,
                    shoff + (uint64_t)shstrndx * shentsize, (size_t)shentsize, scratch);
            if(s) {
                if(cls == 1) { shstr_off = elf_get(s + 16, 4, be); shstr_sz = elf_get(s + 20, 4, be); }
                else         { shstr_off = elf_get(s + 24, 8, be); shstr_sz = elf_get(s + 32, 8, be); }
            }
        }
        if(shstr_sz > 0 && shstr_sz < (1u << 20)) {
            shstr = (unsigned char*)malloc((size_t)shstr_sz + 1);
            if(shstr && read_at(fd, shstr_off, shstr, (size_t)shstr_sz) == (int)shstr_sz)
                shstr[shstr_sz] = 0;
            else { free(shstr); shstr = NULL; }
        }
        for(i = 0; i < shnum; i++) {
            const unsigned char* s = elf_fetch(fd, buf, len,
                    shoff + (uint64_t)i * shentsize, (size_t)shentsize, scratch);
            uint32_t sh_type, sh_name;
            uint64_t sh_offset, sh_size;
            if(!s) break;
            sh_type = (uint32_t)elf_get(s + 4, 4, be);
            sh_name = (uint32_t)elf_get(s, 4, be);
            if(cls == 1) { sh_offset = elf_get(s + 16, 4, be); sh_size = elf_get(s + 20, 4, be); }
            else         { sh_offset = elf_get(s + 24, 8, be); sh_size = elf_get(s + 32, 8, be); }
            if(sh_type == 2) stripped = 0;                    /* SHT_SYMTAB */
            if(sh_type == 7 && sh_size > 0 && sh_size < (1u << 20)) { /* SHT_NOTE */
                unsigned char* nb = (unsigned char*)malloc((size_t)sh_size);
                if(nb && read_at(fd, sh_offset, nb, (size_t)sh_size) == (int)sh_size) {
                    char bid[160], ab[96];
                    parse_notes(nb, (size_t)sh_size, be, bid, sizeof(bid), ab, sizeof(ab));
                    if(bid[0] && !buildid[0]) snprintf(buildid, sizeof(buildid), "%s", bid);
                    if(ab[0]  && !abi[0])     snprintf(abi, sizeof(abi), "%s", ab);
                }
                free(nb);
            }
            if(shstr && sh_name < shstr_sz) {
                const char* nm = (const char*)shstr + sh_name;
                if(strncmp(nm, ".debug", 6) == 0) has_debug = 1;
            }
        }
        free(shstr);
    }

    if(buildid[0]) dcat(r->desc, DESC_SZ, ", BuildID[sha1]=%s", buildid);
    if(abi[0])     dcat(r->desc, DESC_SZ, ", %s", abi);
    if(has_debug)  dcat(r->desc, DESC_SZ, ", with debug_info");
    dcat(r->desc, DESC_SZ, ", %s", stripped ? "stripped" : "not stripped");

    /* MIME type follows the ELF type. */
    switch(etype) {
    case 1: snprintf(r->mime_type, MIME_SZ, "application/x-object"); break;
    case 2: snprintf(r->mime_type, MIME_SZ, "application/x-executable"); break;
    case 3: snprintf(r->mime_type, MIME_SZ,
                     have_interp ? "application/x-pie-executable" : "application/x-sharedlib");
            break;
    case 4: snprintf(r->mime_type, MIME_SZ, "application/x-coredump"); break;
    default: snprintf(r->mime_type, MIME_SZ, "application/x-executable"); break;
    }
    snprintf(r->mime_charset, CHARSET_SZ, "binary");
    (void)st;
}


/* ── little/big-endian raw readers ────────────────────────────────────── */
static uint16_t rd16le(const unsigned char* p){ return (uint16_t)(p[0] | (p[1] << 8)); }
static uint16_t rd16be(const unsigned char* p){ return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t rd32le(const unsigned char* p){
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint32_t rd32be(const unsigned char* p){
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void set_mime(result_t* r, const char* type) {
    snprintf(r->mime_type, MIME_SZ, "%s", type);
    snprintf(r->mime_charset, CHARSET_SZ, "binary");
}

/* ── gzip ─────────────────────────────────────────────────────────────── */
static void gzip_detail(const unsigned char* b, size_t len, result_t* r) {
    static const char* osn[] = { "FAT filesystem (MS-DOS, OS/2, NT)", "Amiga",
        "VMS (or OpenVMS)", "Unix", "VM/CMS", "Atari TOS", "HPFS filesystem (OS/2, NT)",
        "Macintosh", "Z-System", "CP/M", "TOPS/20", "NTFS filesystem (NT)",
        "QDOS", "Acorn RISCOS" };
    unsigned char flg = b[3], xfl = b[6], os = b[7];
    uint32_t mtime = rd32le(b + 4);
    size_t p = 10;
    r->desc[0] = 0;
    dcat(r->desc, DESC_SZ, "gzip compressed data");
    if(flg & 0x04) {                                  /* FEXTRA */
        if(p + 2 <= len) p += 2 + rd16le(b + p);
    }
    if(flg & 0x08) {                                  /* FNAME */
        char nm[256]; size_t k = 0;
        while(p < len && b[p] != 0 && k < sizeof(nm) - 1) nm[k++] = (char)b[p++];
        if(p < len) p++;
        nm[k] = 0;
        if(k) dcat(r->desc, DESC_SZ, ", was \"%s\"", nm);
    }
    if(flg & 0x10) { while(p < len && b[p] != 0) p++; if(p < len) p++; } /* FCOMMENT */
    if(mtime) {
        time_t t = (time_t)mtime; struct tm tm; char tb[64];
        localtime_r(&t, &tm);
        if(strftime(tb, sizeof(tb), "%a %b %e %H:%M:%S %Y", &tm))
            dcat(r->desc, DESC_SZ, ", last modified: %s", tb);
    }
    if(xfl == 2)      dcat(r->desc, DESC_SZ, ", max compression");
    else if(xfl == 4) dcat(r->desc, DESC_SZ, ", fastest compression");
    dcat(r->desc, DESC_SZ, ", from %s", os == 255 ? "unknown" : (os <= 13 ? osn[os] : "unknown"));
    set_mime(r, "application/gzip");
}

/* ── PNG ──────────────────────────────────────────────────────────────── */
static void png_detail(const unsigned char* b, size_t len, result_t* r) {
    uint32_t w = rd32be(b + 16), h = rd32be(b + 20);
    unsigned depth = b[24], color = b[25], inter = b[28];
    const char* cs;
    r->desc[0] = 0;
    dcat(r->desc, DESC_SZ, "PNG image data, %u x %u, ", w, h);
    switch(color) {
    case 0: cs = "grayscale";    dcat(r->desc, DESC_SZ, "%u-bit %s", depth, cs); break;
    case 2: cs = "RGB";          dcat(r->desc, DESC_SZ, "%u-bit/color %s", depth, cs); break;
    case 3: cs = "colormap";     dcat(r->desc, DESC_SZ, "%u-bit %s", depth, cs); break;
    case 4: cs = "gray+alpha";   dcat(r->desc, DESC_SZ, "%u-bit %s", depth, cs); break;
    case 6: cs = "RGBA";         dcat(r->desc, DESC_SZ, "%u-bit/color %s", depth, cs); break;
    default:                     dcat(r->desc, DESC_SZ, "%u-bit/color unknown", depth); break;
    }
    dcat(r->desc, DESC_SZ, ", %s", inter ? "interlaced" : "non-interlaced");
    (void)len;
    set_mime(r, "image/png");
}

/* ── JPEG ─────────────────────────────────────────────────────────────── */
static void jpeg_detail(const unsigned char* b, size_t len, result_t* r) {
    size_t i = 2;
    r->desc[0] = 0;
    dcat(r->desc, DESC_SZ, "JPEG image data");
    if(len >= 11 && memcmp(b + 6, "JFIF", 4) == 0)
        dcat(r->desc, DESC_SZ, ", JFIF standard %d.%02d", b[11], b[12]);
    else if(len >= 11 && memcmp(b + 6, "Exif", 4) == 0)
        dcat(r->desc, DESC_SZ, ", Exif standard: 'Exif', %s",
             (len >= 12 && b[12] == 'I' && b[13] == 'I') ? "little-endian" : "big-endian");
    /* Scan markers for a Start-Of-Frame to report dimensions and mode. */
    while(i + 9 < len) {
        if(b[i] != 0xff) { i++; continue; }
        while(i < len && b[i] == 0xff) i++;
        if(i >= len) break;
        {
            unsigned char m = b[i++];
            unsigned seglen;
            if(m == 0xd8 || m == 0x01 || (m >= 0xd0 && m <= 0xd9)) continue;
            if(i + 1 >= len) break;
            seglen = rd16be(b + i);
            if((m >= 0xc0 && m <= 0xc3) || (m >= 0xc5 && m <= 0xc7) ||
               (m >= 0xc9 && m <= 0xcb) || (m >= 0xcd && m <= 0xcf)) {
                if(i + 7 < len) {
                    unsigned prec = b[i + 2];
                    uint16_t hh = rd16be(b + i + 3), ww = rd16be(b + i + 5);
                    dcat(r->desc, DESC_SZ, ", %s, precision %u, %ux%u",
                         (m == 0xc0 || m == 0xc1) ? "baseline" :
                         (m == 0xc2) ? "progressive" : "SOF", prec, ww, hh);
                }
                break;
            }
            if(seglen < 2) break;
            i += seglen;
        }
    }
    set_mime(r, "image/jpeg");
}

/* ── BMP ──────────────────────────────────────────────────────────────── */
static void bmp_detail(const unsigned char* b, size_t len, result_t* r) {
    r->desc[0] = 0;
    if(len >= 30) {
        uint32_t hsz = rd32le(b + 14);
        int32_t w = (int32_t)rd32le(b + 18), h = (int32_t)rd32le(b + 22);
        uint16_t bpp = rd16le(b + 28);
        const char* fmt = (hsz == 40) ? "Windows 3.x format" :
                          (hsz == 12) ? "OS/2 1.x format" :
                          (hsz == 108 || hsz == 124) ? "Windows NT/2000 format" : "format";
        dcat(r->desc, DESC_SZ, "PC bitmap, %s, %d x %d x %u", fmt, w, h < 0 ? -h : h, bpp);
    } else {
        dcat(r->desc, DESC_SZ, "PC bitmap");
    }
    set_mime(r, "image/x-ms-bmp");
}

/* ── RIFF (WAVE / AVI / WEBP) ─────────────────────────────────────────── */
static void riff_detail(const unsigned char* b, size_t len, result_t* r) {
    int be = (memcmp(b, "RIFX", 4) == 0);
    r->desc[0] = 0;
    dcat(r->desc, DESC_SZ, "RIFF (%s-endian) data", be ? "big" : "little");
    if(len >= 12) {
        if(memcmp(b + 8, "WAVE", 4) == 0)      { dcat(r->desc, DESC_SZ, ", WAVE audio"); set_mime(r, "audio/x-wav"); }
        else if(memcmp(b + 8, "AVI ", 4) == 0) { dcat(r->desc, DESC_SZ, ", AVI video");  set_mime(r, "video/x-msvideo"); }
        else if(memcmp(b + 8, "WEBP", 4) == 0) { dcat(r->desc, DESC_SZ, ", Web/P image"); set_mime(r, "image/webp"); }
        else { char f[5]; memcpy(f, b + 8, 4); f[4] = 0; dcat(r->desc, DESC_SZ, ", %s", f); set_mime(r, "application/octet-stream"); }
    } else {
        set_mime(r, "application/octet-stream");
    }
}

/* ── ZIP ──────────────────────────────────────────────────────────────── */
static void zip_detail(const unsigned char* b, size_t len, result_t* r) {
    r->desc[0] = 0;
    if(len >= 30 && memcmp(b, "PK\x03\x04", 4) == 0) {
        unsigned ver = rd16le(b + 4), method = rd16le(b + 8);
        const char* mth = method == 0 ? "store" : method == 8 ? "deflate" :
                          method == 12 ? "bzip2" : method == 14 ? "LZMA" : "unknown";
        dcat(r->desc, DESC_SZ, "Zip archive data, at least v%u.%u to extract",
             ver / 10, ver % 10);
        dcat(r->desc, DESC_SZ, ", compression method=%s", mth);
    } else if(len >= 4 && memcmp(b, "PK\x05\x06", 4) == 0) {
        dcat(r->desc, DESC_SZ, "Zip archive data (empty)");
    } else if(len >= 4 && memcmp(b, "PK\x07\x08", 4) == 0) {
        dcat(r->desc, DESC_SZ, "Zip multi-volume archive data (spanned)");
    } else {
        dcat(r->desc, DESC_SZ, "Zip archive data");
    }
    set_mime(r, "application/zip");
}

/* ── ISO base media (MP4 / MOV / HEIF) ────────────────────────────────── */
static void isomedia_detail(const unsigned char* b, size_t len, result_t* r) {
    char brand[5];
    r->desc[0] = 0;
    memcpy(brand, b + 8, 4); brand[4] = 0;
    if(memcmp(brand, "qt  ", 4) == 0) {
        dcat(r->desc, DESC_SZ, "ISO Media, Apple QuickTime movie");
        set_mime(r, "video/quicktime");
    } else if(memcmp(brand, "mif1", 4) == 0 || memcmp(brand, "msf1", 4) == 0 ||
              memcmp(brand, "heic", 4) == 0 || memcmp(brand, "heix", 4) == 0) {
        dcat(r->desc, DESC_SZ, "ISO Media, HEIF image");
        set_mime(r, "image/heif");
    } else if(memcmp(brand, "dash", 4) == 0) {
        dcat(r->desc, DESC_SZ, "ISO Media, MPEG-DASH segment");
        set_mime(r, "video/mp4");
    } else {
        dcat(r->desc, DESC_SZ, "ISO Media, MP4 base media v1 [%s]", brand);
        set_mime(r, "video/mp4");
    }
    (void)len;
}

/* ── PE / MZ (Windows) ────────────────────────────────────────────────── */
static void pe_detail(int fd, const unsigned char* b, size_t len, result_t* r) {
    unsigned char scratch[1024];
    const unsigned char* pe;
    uint32_t peoff;
    uint16_t machine, subsys, chars, opthdr_magic;
    const char* mach;
    r->desc[0] = 0;
    if(len < 0x40) { dcat(r->desc, DESC_SZ, "MS-DOS executable (built-in)"); set_mime(r, "application/x-dosexec"); return; }
    peoff = rd32le(b + 0x3c);
    pe = elf_fetch(fd, b, len, peoff, 24 + 2, scratch);
    if(!pe || memcmp(pe, "PE\0\0", 4) != 0) {
        dcat(r->desc, DESC_SZ, "MS-DOS executable (built-in)");
        set_mime(r, "application/x-dosexec");
        return;
    }
    machine = rd16le(pe + 4);
    chars   = rd16le(pe + 4 + 18);
    opthdr_magic = rd16le(pe + 24);
    subsys  = rd16le(pe + 24 + 68);
    mach = machine == 0x14c ? "Intel 80386" : machine == 0x8664 ? "x86-64" :
           machine == 0x1c0 ? "ARM" : machine == 0xaa64 ? "ARM64" :
           machine == 0x200 ? "Intel Itanium" : machine == 0x166 ? "MIPS R4000" : "unknown";
    dcat(r->desc, DESC_SZ, "PE32%s executable (%s) %s",
         opthdr_magic == 0x20b ? "+" : "",
         (chars & 0x2000) ? "DLL" : subsys == 2 ? "GUI" : subsys == 3 ? "console" : "subsystem",
         mach);
    dcat(r->desc, DESC_SZ, ", for MS Windows");
    set_mime(r, "application/x-dosexec");
}

/* ── POSIX/GNU tar ────────────────────────────────────────────────────── */
static void tar_detail(const unsigned char* b, size_t len, result_t* r) {
    r->desc[0] = 0;
    if(len >= 265 && b[263] == '0' && b[264] == '0')
        dcat(r->desc, DESC_SZ, "POSIX tar archive");
    else
        dcat(r->desc, DESC_SZ, "POSIX tar archive (GNU)");
    set_mime(r, "application/x-tar");
}

/* ── ar archive (and Debian packages) ─────────────────────────────────── */
static void ar_detail(const unsigned char* b, size_t len, result_t* r) {
    r->desc[0] = 0;
    if(len >= 21 && memcmp(b + 8, "debian-binary", 13) == 0) {
        dcat(r->desc, DESC_SZ, "Debian binary package");
        set_mime(r, "application/vnd.debian.binary-package");
    } else {
        dcat(r->desc, DESC_SZ, "current ar archive");
        set_mime(r, "application/x-archive");
    }
}

/* ── 0xCAFEBABE: Java class vs Mach-O fat binary ──────────────────────── */
static void cafebabe_detail(const unsigned char* b, size_t len, result_t* r) {
    r->desc[0] = 0;
    /* Java class: cafebabe, minor(u16), major(u16); minor is ~always 0.
       A Mach-O fat binary stores nfat_arch (>=1) in that same u32. */
    if(len >= 8 && rd16be(b + 4) == 0) {
        dcat(r->desc, DESC_SZ, "compiled Java class data, version %u.%u",
             rd16be(b + 6), rd16be(b + 4));
        set_mime(r, "application/x-java-applet");
    } else {
        dcat(r->desc, DESC_SZ, "Mach-O universal binary");
        set_mime(r, "application/x-mach-binary");
    }
}

/* ── PDF ──────────────────────────────────────────────────────────────── */
static void pdf_detail(const unsigned char* b, size_t len, result_t* r) {
    r->desc[0] = 0;
    if(len >= 8 && b[4] == '-' && b[5] >= '0' && b[5] <= '9')
        dcat(r->desc, DESC_SZ, "PDF document, version %c.%c", b[5], b[6]);
    else
        dcat(r->desc, DESC_SZ, "PDF document");
    set_mime(r, "application/pdf");
}

/* ── simple signature table ───────────────────────────────────────────── */
typedef struct { size_t off; const char* sig; size_t siglen; const char* desc; const char* mime; } magic_t;
#define M(off, s, d, mi) { (off), (s), sizeof(s) - 1, (d), (mi) }

static const magic_t magic_tab[] = {
    M(0, "BZh",                     "bzip2 compressed data",              "application/x-bzip2"),
    M(0, "\xfd" "7zXZ" "\x00",      "XZ compressed data",                 "application/x-xz"),
    M(0, "\x28\xb5\x2f\xfd",        "Zstandard compressed data",          "application/zstd"),
    M(0, "\x04\x22\x4d\x18",        "LZ4 compressed data",                "application/x-lz4"),
    M(0, "\x1f\x9d",                "compress'd data 16-bit",             "application/x-compress"),
    M(0, "7z\xbc\xaf\x27\x1c",      "7-zip archive data",                 "application/x-7z-compressed"),
    M(0, "MSCF\x00\x00\x00\x00",    "Microsoft Cabinet archive data",     "application/vnd.ms-cab-compressed"),
    M(0, "OggS\x00",                "Ogg data",                           "application/ogg"),
    M(0, "fLaC\x00\x00\x00\x22",    "FLAC audio bitstream data",          "audio/x-flac"),
    M(0, "ID3",                     "Audio file with ID3 tag",            "audio/mpeg"),
    M(0, "GIF87a",                  "GIF image data, version 87a",        "image/gif"),
    M(0, "GIF89a",                  "GIF image data, version 89a",        "image/gif"),
    M(0, "SQLite format 3\x00",     "SQLite 3.x database",                "application/x-sqlite3"),
    M(0, "\xed\xab\xee\xdb",        "RPM v3.0 bin",                       "application/x-rpm"),
    M(0, "%!PS-Adobe-",             "PostScript document text",           "application/postscript"),
    M(0, "%!",                      "PostScript document text",           "application/postscript"),
    M(0, "\x00\x00\x01\x00",        "MS Windows icon resource",           "image/vnd.microsoft.icon"),
    M(0, "\xcf\xfa\xed\xfe",        "Mach-O 64-bit x86_64 executable",    "application/x-mach-binary"),
    M(0, "\xce\xfa\xed\xfe",        "Mach-O i386 executable",             "application/x-mach-binary"),
    M(0, "\xfe\xed\xfa\xcf",        "Mach-O 64-bit big-endian",           "application/x-mach-binary"),
    M(0, "\xfe\xed\xfa\xce",        "Mach-O big-endian",                  "application/x-mach-binary"),
    M(0, "BC\xc0\xde",              "LLVM IR bitcode",                    "application/octet-stream"),
    M(0, "\x00asm\x01\x00\x00\x00", "WebAssembly (wasm) binary module",   "application/wasm"),
    M(0, "070701",                  "ASCII cpio archive (newc)",          "application/x-cpio"),
    M(0, "070707",                  "ASCII cpio archive (pre-SVR4 or odc)","application/x-cpio"),
};
#define MAGIC_TAB_N ((int)(sizeof(magic_tab) / sizeof(magic_tab[0])))


/* ── shebang scripts ──────────────────────────────────────────────────── */
typedef struct { const char* base; const char* desc; const char* mime; int prefix; } interp_t;

static const interp_t interp_tab[] = {
    { "sh",      "POSIX shell script",        "text/x-shellscript", 0 },
    { "bash",    "Bourne-Again shell script", "text/x-shellscript", 0 },
    { "dash",    "Debian shell script",       "text/x-shellscript", 0 },
    { "ksh",     "Korn shell script",         "text/x-shellscript", 0 },
    { "csh",     "c shell script",            "text/x-shellscript", 0 },
    { "tcsh",    "c shell script",            "text/x-shellscript", 0 },
    { "zsh",     "Z shell script",            "text/x-shellscript", 0 },
    { "fish",    "fish shell script",         "text/x-shellscript", 0 },
    { "perl",    "Perl script",               "text/x-perl",        1 },
    { "python",  "Python script",             "text/x-python",      1 },
    { "ruby",    "Ruby script",               "text/x-ruby",        1 },
    { "lua",     "Lua script",                "text/x-lua",         1 },
    { "php",     "PHP script",                "text/x-php",         1 },
    { "node",    "Node.js script",            "text/javascript",    1 },
    { "tclsh",   "Tcl/Tk script",             "text/x-tcl",         0 },
    { "wish",    "Tcl/Tk script",             "text/x-tcl",         0 },
    { "awk",     "awk script",                "text/x-awk",         0 },
    { "gawk",    "awk script",                "text/x-awk",         0 },
    { "sed",     "sed script",                "text/x-sed",         0 },
    { "expect",  "Expect script",             "text/x-expect",      0 },
    { "Rscript", "R script",                  "text/x-R",           0 },
    { NULL, NULL, NULL, 0 }
};

static const interp_t* interp_lookup(const char* base) {
    int i;
    for(i = 0; interp_tab[i].base; i++)
        if(strcmp(base, interp_tab[i].base) == 0)
            return &interp_tab[i];
    for(i = 0; interp_tab[i].base; i++)
        if(interp_tab[i].prefix && strncmp(base, interp_tab[i].base, strlen(interp_tab[i].base)) == 0)
            return &interp_tab[i];
    return NULL;
}

static const char* base_name(const char* p) {
    const char* s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* Returns 1 when the buffer begins with a shebang and was described. */
static int describe_script(const unsigned char* b, size_t len, result_t* r) {
    char line[256], interp[256], arg[128];
    size_t i = 2, k = 0, p, q;
    const interp_t* fr;
    enc_kind_t enc;

    if(len < 2 || b[0] != '#' || b[1] != '!')
        return 0;
    while(i < len && (b[i] == ' ' || b[i] == '\t')) i++;
    while(i < len && b[i] != '\n' && b[i] != '\r' && k < sizeof(line) - 1)
        line[k++] = (char)b[i++];
    while(k > 0 && (line[k - 1] == ' ' || line[k - 1] == '\t')) k--;
    line[k] = 0;
    if(k == 0)
        return 0;

    /* first token = interpreter, second token = first argument */
    p = 0; q = 0;
    while(p < k && line[p] != ' ' && line[p] != '\t') interp[q++] = line[p++];
    interp[q] = 0;
    while(p < k && (line[p] == ' ' || line[p] == '\t')) p++;
    q = 0;
    while(p < k && line[p] != ' ' && line[p] != '\t') arg[q++] = line[p++];
    arg[q] = 0;

    /* env(1) wrapper: look through to the real program for a friendly name */
    {
        const char* eff = interp;
        if(strcmp(base_name(interp), "env") == 0 && arg[0])
            eff = arg;
        fr = interp_lookup(base_name(eff));
    }

    enc = detect_encoding(b, len);
    r->desc[0] = 0;
    if(fr) dcat(r->desc, DESC_SZ, "%s", fr->desc);
    else   dcat(r->desc, DESC_SZ, "%s script", line);
    dcat(r->desc, DESC_SZ, ", ");
    if(enc == ENC_BINARY) {
        dcat(r->desc, DESC_SZ, "executable");
        snprintf(r->mime_type, MIME_SZ, "%s", fr ? fr->mime : "text/x-script");
        snprintf(r->mime_charset, CHARSET_SZ, "binary");
    } else {
        dcat(r->desc, DESC_SZ, "%s text executable", enc_code(enc));
        text_qualifiers(r->desc, DESC_SZ, b, len);
        snprintf(r->mime_type, MIME_SZ, "%s", fr ? fr->mime : "text/x-script");
        snprintf(r->mime_charset, CHARSET_SZ, "%s", enc_charset(enc));
    }
    return 1;
}

/* ── text vs. data fallback ───────────────────────────────────────────── */
static void describe_text(const unsigned char* b, size_t len, result_t* r) {
    enc_kind_t enc = detect_encoding(b, len);
    r->desc[0] = 0;
    if(enc == ENC_BINARY) {
        dcat(r->desc, DESC_SZ, "data");
        set_mime(r, "application/octet-stream");
        return;
    }
    dcat(r->desc, DESC_SZ, "%s text", enc_code(enc));
    text_qualifiers(r->desc, DESC_SZ, b, len);
    snprintf(r->mime_type, MIME_SZ, "text/plain");
    snprintf(r->mime_charset, CHARSET_SZ, "%s", enc_charset(enc));
}

/* ── content dispatcher ───────────────────────────────────────────────── */
static void describe_content(int fd, const unsigned char* b, size_t len, result_t* r) {
    int i;
    len = trim_nuls(b, len);

    if(len >= 4 && b[0] == 0x7f && b[1] == 'E' && b[2] == 'L' && b[3] == 'F') {
        struct stat dummy; memset(&dummy, 0, sizeof(dummy));
        describe_elf(fd, b, len, &dummy, r);
        return;
    }
    if(describe_script(b, len, r)) return;
    if(len >= 10 && b[0] == 0x1f && b[1] == 0x8b && b[2] == 0x08) { gzip_detail(b, len, r); return; }
    if(len >= 8 && memcmp(b, "\x89PNG\r\n\x1a\n", 8) == 0) {
        if(len >= 29 && memcmp(b + 12, "IHDR", 4) == 0) png_detail(b, len, r);
        else { snprintf(r->desc, DESC_SZ, "PNG image data"); set_mime(r, "image/png"); }
        return;
    }
    if(len >= 3 && b[0] == 0xff && b[1] == 0xd8 && b[2] == 0xff) { jpeg_detail(b, len, r); return; }
    if(len >= 2 && b[0] == 'B' && b[1] == 'M') { bmp_detail(b, len, r); return; }
    if(len >= 4 && (memcmp(b, "PK\x03\x04", 4) == 0 || memcmp(b, "PK\x05\x06", 4) == 0 ||
                    memcmp(b, "PK\x07\x08", 4) == 0)) { zip_detail(b, len, r); return; }
    if(len >= 12 && (memcmp(b, "RIFF", 4) == 0 || memcmp(b, "RIFX", 4) == 0)) { riff_detail(b, len, r); return; }
    if(len >= 12 && memcmp(b + 4, "ftyp", 4) == 0) { isomedia_detail(b, len, r); return; }
    if(len >= 2 && b[0] == 'M' && b[1] == 'Z') { pe_detail(fd, b, len, r); return; }
    if(len >= 263 && memcmp(b + 257, "ustar", 5) == 0) { tar_detail(b, len, r); return; }
    if(len >= 8 && memcmp(b, "!<arch>\n", 8) == 0) { ar_detail(b, len, r); return; }
    if(len >= 4 && b[0] == 0xca && b[1] == 0xfe && b[2] == 0xba && b[3] == 0xbe) { cafebabe_detail(b, len, r); return; }
    if(len >= 4 && memcmp(b, "%PDF", 4) == 0) { pdf_detail(b, len, r); return; }

    for(i = 0; i < MAGIC_TAB_N; i++) {
        const magic_t* m = &magic_tab[i];
        if(len >= m->off + m->siglen && memcmp(b + m->off, m->sig, m->siglen) == 0) {
            snprintf(r->desc, DESC_SZ, "%s", m->desc);
            set_mime(r, m->mime);
            return;
        }
    }

    /* MPEG audio frame sync (checked last: 0xFF Ex is a loose signature) */
    if(len >= 2 && b[0] == 0xff && (b[1] & 0xe0) == 0xe0) {
        static const char* layer[] = { "", "III", "II", "I" };
        int l = (b[1] >> 1) & 0x3;
        r->desc[0] = 0;
        dcat(r->desc, DESC_SZ, "MPEG ADTS, layer %s", l >= 1 && l <= 3 ? layer[l] : "unknown");
        set_mime(r, "audio/mpeg");
        return;
    }

    describe_text(b, len, r);
}

/* ── special (non-regular) files ──────────────────────────────────────── */
static void describe_special(const char* name, const struct stat* st, result_t* r) {
    unsigned maj = (unsigned)((st->st_rdev >> 8) & 0xff);
    unsigned min = (unsigned)(st->st_rdev & 0xff);
    r->desc[0] = 0;
    if(S_ISDIR(st->st_mode))       { dcat(r->desc, DESC_SZ, "directory"); set_mime(r, "inode/directory"); }
    else if(S_ISCHR(st->st_mode))  { dcat(r->desc, DESC_SZ, "character special (%u/%u)", maj, min); set_mime(r, "inode/chardevice"); }
    else if(S_ISBLK(st->st_mode))  { dcat(r->desc, DESC_SZ, "block special (%u/%u)", maj, min); set_mime(r, "inode/blockdevice"); }
    else if(S_ISFIFO(st->st_mode)) { dcat(r->desc, DESC_SZ, "fifo (named pipe)"); set_mime(r, "inode/fifo"); }
    else if(S_ISSOCK(st->st_mode)) { dcat(r->desc, DESC_SZ, "socket"); set_mime(r, "inode/socket"); }
    else if(S_ISLNK(st->st_mode)) {
        char lb[512]; ssize_t n = readlink(name, lb, sizeof(lb) - 1);
        if(n > 0) { lb[n] = 0; dcat(r->desc, DESC_SZ, "symbolic link to %s", lb); }
        else      { dcat(r->desc, DESC_SZ, "symbolic link"); }
        set_mime(r, "inode/symlink");
    } else {
        dcat(r->desc, DESC_SZ, "unknown file mode 0%o", (unsigned)st->st_mode);
        set_mime(r, "inode/x-unknown");
    }
}

/* ── read up to max bytes, retrying on EAGAIN/EINTR ───────────────────── */
static ssize_t read_all(int fd, unsigned char* buf, size_t max) {
    size_t got = 0;
    while(got < max) {
        ssize_t k = read(fd, buf + got, max - got);
        if(k > 0) { got += (size_t)k; continue; }
        if(k < 0 && (errno == EAGAIN || errno == EINTR)) { errno = 0; continue; }
        break;
    }
    return (ssize_t)got;
}

static void cannot_open(result_t* r, const char* disp) {
    snprintf(r->desc, DESC_SZ, "cannot open `%s' (%s)", disp, strerror(errno));
    set_mime(r, "inode/x-invalid");
}

/* Format and emit one result line, honouring -b/-F/-0/-n and the mime modes. */
static void print_result(const char* disp, const result_t* r) {
    const char* out;
    char mbuf[MIME_SZ + CHARSET_SZ + 16];
    switch(g_out_mode) {
    case OUT_MIME:
        snprintf(mbuf, sizeof(mbuf), "%s; charset=%s", r->mime_type, r->mime_charset);
        out = mbuf;
        break;
    case OUT_MIME_TYPE:     out = r->mime_type;    break;
    case OUT_MIME_ENCODING: out = r->mime_charset; break;
    default:                out = r->desc;         break;
    }
    if(g_brief) {
        fputs(out, stdout);
    } else {
        fputs(disp, stdout);
        if(g_print0) fputc('\0', stdout);
        fputs(g_sep, stdout);
        fputc(' ', stdout);
        fputs(out, stdout);
    }
    fputc('\n', stdout);
    if(g_nobuffer) fflush(stdout);
}

/* ── analyse and print one operand ────────────────────────────────────── */
static void process(const char* name) {
    result_t r;
    struct stat st;
    const char* disp = name;
    int is_stdin = (name[0] == '-' && name[1] == 0);
    int fd;
    unsigned char* buf;
    ssize_t total;

    if(is_stdin) disp = "/dev/stdin";
    r.desc[0] = 0; r.mime_type[0] = 0; r.mime_charset[0] = 0;

    if(!is_stdin) {
        if((g_deref ? stat(name, &st) : lstat(name, &st)) != 0) {
            cannot_open(&r, disp);
            print_result(disp, &r);
            return;
        }
        if(!S_ISREG(st.st_mode)) {
            /* char/block devices are only read when -s is given */
            if(!(g_special && (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)))) {
                describe_special(name, &st, &r);
                print_result(disp, &r);
                return;
            }
        } else if(st.st_size == 0) {
            snprintf(r.desc, DESC_SZ, "empty");
            set_mime(&r, "inode/x-empty");
            print_result(disp, &r);
            return;
        }
    } else {
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFREG | 0644;
    }

    if(is_stdin) fd = 0;
    else {
        fd = open(name, O_RDONLY);
        if(fd < 0) { cannot_open(&r, disp); print_result(disp, &r); return; }
    }

    buf = (unsigned char*)malloc(READ_BUF);
    if(!buf) {
        errno = ENOMEM;
        cannot_open(&r, disp);
        if(!is_stdin) close(fd);
        print_result(disp, &r);
        return;
    }
    total = read_all(fd, buf, READ_BUF);
    if(total < 0) total = 0;
    describe_content(fd, buf, (size_t)total, &r);
    free(buf);
    if(!is_stdin) close(fd);
    print_result(disp, &r);
}

/* ── -f/--files-from: one filename per line ───────────────────────────── */
static void process_files_from(const char* path) {
    int fd = (path[0] == '-' && path[1] == 0) ? 0 : open(path, O_RDONLY);
    char* buf;
    ssize_t n;
    if(fd < 0) {
        fprintf(stderr, "file: cannot open `%s' (%s)\n", path, strerror(errno));
        return;
    }
    buf = (char*)malloc(READ_BUF);
    if(!buf) { if(fd != 0) close(fd); return; }
    n = read_all(fd, (unsigned char*)buf, READ_BUF - 1);
    if(n > 0) {
        char* p;
        buf[n] = 0;
        p = buf;
        while(p && *p) {
            char* nl = strchr(p, '\n');
            size_t l;
            if(nl) *nl = 0;
            l = strlen(p);
            while(l > 0 && p[l - 1] == '\r') p[--l] = 0;
            if(l > 0) process(p);
            if(!nl) break;
            p = nl + 1;
        }
    }
    free(buf);
    if(fd != 0) close(fd);
}

/* ── usage / version ──────────────────────────────────────────────────── */
static void usage(int status) {
    FILE* f = status ? stderr : stdout;
    if(status) {
        fputs("Try 'file --help' for more information.\n", f);
        exit(status);
    }
    fputs(
"Usage: file [OPTION...] [FILE...]\n"
"Determine type of FILE.\n"
"\n"
"      --help                 display this help and exit\n"
"  -v, --version              output version information and exit\n"
"  -b, --brief                do not prepend filenames to output lines\n"
"  -c, --checking-printout    print the parsed form of the magic file\n"
"  -C, --compile              write a magic.mgc output file\n"
"  -d, --debug                apply default system tests\n"
"  -e, --exclude=testname     exclude named test (apptype, ascii, cdf,\n"
"                             compress, csv, elf, encoding, json, soft,\n"
"                             tar, text, tokens)\n"
"  -f, --files-from=FILE      read the filenames to be examined from FILE\n"
"  -F, --separator=string     use string as separator instead of `:'\n"
"  -h, --no-dereference       do not follow symlinks\n"
"  -i, --mime                 output mime type strings\n"
"  -k, --keep-going           don't stop at the first match\n"
"  -l, --list                 list magic strength\n"
"  -L, --dereference          follow symlinks\n"
"  -m, --magic-file=list      use list of magic number files\n"
"  -n, --no-buffer            do not buffer output\n"
"  -N, --no-pad               do not pad output\n"
"  -P, --parameter=name=value set file parameter\n"
"  -p, --preserve-date        preserve access times on files examined\n"
"  -r, --raw                  don't translate unprintable chars to \\ooo\n"
"  -s, --special-files        read special (block/char devices) files\n"
"  -S, --no-special-files     don't read special files (default)\n"
"  -z, --uncompress           attempt to look inside compressed files\n"
"  -0, --print0               output \\0 after the filename\n"
"      --mime-type            output only the mime type\n"
"      --mime-encoding        output only the mime encoding\n", f);
    exit(0);
}

int main(int argc, char** argv) {
    int c, i;
    char* files_from = NULL;
    static struct option longopts[] = {
        { "brief",             no_argument,       NULL, 'b' },
        { "checking-printout", no_argument,       NULL, 'c' },
        { "compile",           no_argument,       NULL, 'C' },
        { "debug",             no_argument,       NULL, 'd' },
        { "exclude",           required_argument, NULL, 'e' },
        { "files-from",        required_argument, NULL, 'f' },
        { "separator",         required_argument, NULL, 'F' },
        { "no-dereference",    no_argument,       NULL, 'h' },
        { "mime",              no_argument,       NULL, 'i' },
        { "keep-going",        no_argument,       NULL, 'k' },
        { "list",              no_argument,       NULL, 'l' },
        { "dereference",       no_argument,       NULL, 'L' },
        { "magic-file",        required_argument, NULL, 'm' },
        { "no-buffer",         no_argument,       NULL, 'n' },
        { "no-pad",            no_argument,       NULL, 'N' },
        { "preserve-date",     no_argument,       NULL, 'p' },
        { "parameter",         required_argument, NULL, 'P' },
        { "raw",               no_argument,       NULL, 'r' },
        { "special-files",     no_argument,       NULL, 's' },
        { "no-special-files",  no_argument,       NULL, 'S' },
        { "version",           no_argument,       NULL, 'v' },
        { "uncompress",        no_argument,       NULL, 'z' },
        { "print0",            no_argument,       NULL, '0' },
        { "mime-type",         no_argument,       NULL, OPT_MIME_TYPE },
        { "mime-encoding",     no_argument,       NULL, OPT_MIME_ENCODING },
        { "extension",         no_argument,       NULL, OPT_EXTENSION },
        { "help",              no_argument,       NULL, OPT_HELP },
        { NULL, 0, NULL, 0 }
    };

    while((c = getopt_long(argc, argv, "bcCde:f:F:hikLlm:nNpP:rsSvz0", longopts, NULL)) != -1) {
        switch(c) {
        case 'b': g_brief = 1; break;
        case 'f': files_from = optarg; break;
        case 'F': g_sep = optarg; break;
        case 'h': g_deref = 0; break;
        case 'L': g_deref = 1; break;
        case 'i': g_out_mode = OUT_MIME; break;
        case 'n': g_nobuffer = 1; break;
        case 's': g_special = 1; break;
        case 'z': g_uncompress = 1; break;
        case '0': g_print0 = 1; break;
        case OPT_MIME_TYPE:     g_out_mode = OUT_MIME_TYPE; break;
        case OPT_MIME_ENCODING: g_out_mode = OUT_MIME_ENCODING; break;
        case 'v':
            fputs("file-5.45\nmagic file from built-in database\n", stdout);
            return 0;
        case OPT_HELP: usage(0); break;
        /* Accepted for compatibility; no effect in this implementation. */
        case 'c': case 'C': case 'd': case 'e': case 'k': case 'l':
        case 'm': case 'N': case 'p': case 'P': case 'r': case 'S':
        case OPT_EXTENSION:
            break;
        case '?': usage(1); break;
        default:  usage(1); break;
        }
    }

    if(files_from)
        process_files_from(files_from);

    if(optind < argc) {
        for(i = optind; i < argc; i++)
            process(argv[i]);
    } else if(!files_from) {
        process("-");   /* no operands: read standard input */
    }

    return 0;
}
