#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <graph/graph.h>
#include "ttf/truety.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------- */
/* Constants */
/* --------- */
#define TTY_SCALAR_VERSION         40
#define TTY_NUM_PHANTOM_POINTS     4
#define TTY_ACTIVE_EDGES_PER_CHUNK 10
#define TTY_SUBDIVIDE_SQRD_ERROR   0x1  /* 26.6 */
#define TTY_PIXELS_PER_SCANLINE    0x10 /* 26.6 */


/* --------- */
/* Debugging */
/* --------- */
// #define TTY_DEBUG

#ifdef TTY_DEBUG
    #include <assert.h>

    #define TTY_ASSERT(cond) assert(cond)
    
    #define TTY_FIX_TO_FLOAT(val, shift) tty_fix_to_float(val, shift)
    
    float tty_fix_to_float(TTY_S32 val, TTY_U8 shift) {
        float s = val >> shift;
        float power = 0.5f;
        TTY_S32 mask = 1 << (shift - 1);
        for (TTY_U8 i = 0; i < shift; i++) {
            if (val & mask) {
                s += power;
            }
            mask >>= 1;
            power /= 2.0f;
        }
        return s;
    }
#else
    #define TTY_ASSERT(cond)
    #define TTY_FIX_TO_FLOAT(val, shift)
#endif


/* ------- */
/* Logging */
/* ------- */
// #define TTY_LOGGING

#ifdef TTY_LOGGING
    static int tty_insCount = 0;

    #define TTY_LOG_PROGRAM(program)\
        printf("\n--- %s ---\n", program);\
        tty_insCount = 0

    #define TTY_LOG_UNKNOWN_INS(ins)\
        printf("Unknown instruction: %#X\n", (int)ins)

    #define TTY_LOG_INS()\
        printf("%d) %s\n", tty_insCount++, __func__ + 4)

    #define TTY_LOG_POINT(point)\
        printf("\t(%d, %d)\n", (int)(point).x, (int)(point).y)

    #define TTY_LOG_VALUE(val)\
        printf("\t%d\n", (int)val)

    #define TTY_LOG_INTERP_STACK_TOP(stack)\
        TTY_LOG_VALUE(stack.buff[stack.count - 1])

    #define TTY_LOGF(format, ...)\
        printf("\t"format"\n", __VA_ARGS__)

    #define TTY_LOG_ZONE1_POINTS(font)\
        printf("\n-- Results --\n");\
        for (TTY_U32 i = 0; i < font->hint.zone1.numPoints; i++) {\
            printf("%u) (%d, %d)\n", (unsigned int)i, (int)font->hint.zone1.cur[i].x, (int)font->hint.zone1.cur[i].y);\
        }\
        printf("\n");

#else
    #define TTY_LOG_UNKNOWN_INS(ins)
    #define TTY_LOG_PROGRAM(program)
    #define TTY_LOG_INS()
    #define TTY_LOG_POINT(point)
    #define TTY_LOG_VALUE(val)
    #define TTY_LOG_INTERP_STACK_TOP(stack)
    #define TTY_LOGF(format, ...)
    #define TTY_LOG_ZONE1_POINTS(font)
#endif


/* ---- */
/* Util */
/* ---- */
#define TTY_DEFINE_PADDING_FUNC(type, name)\
    size_t tty_add_padding_to_align_##name(size_t off) {\
        struct Align {\
            char  c;\
            type  t;\
        };\
        \
        size_t alignment = offsetof(struct Align, t);\
        \
        if (alignment == 1 || off % alignment == 0) {\
            return off;\
        }\
        return off + alignment - (off % alignment);\
    }

TTY_DEFINE_PADDING_FUNC(TTY_U8               , u8)
TTY_DEFINE_PADDING_FUNC(TTY_U8*              , u8_ptr)
TTY_DEFINE_PADDING_FUNC(TTY_U16              , u16)
TTY_DEFINE_PADDING_FUNC(TTY_U32              , u32)
TTY_DEFINE_PADDING_FUNC(TTY_S32              , s32)
TTY_DEFINE_PADDING_FUNC(TTY_V2               , v2)
TTY_DEFINE_PADDING_FUNC(TTY_V2               , edge)
TTY_DEFINE_PADDING_FUNC(TTY_V2               , active_edge)
TTY_DEFINE_PADDING_FUNC(TTY_V2               , f26dot6)

#define TTY_TAG_EQUALS(tag, val)\
    !memcmp(tag, val, 4)

static TTY_U16 tty_get_u16(TTY_U8* data) {
    return data[0] << 8 | data[1];
}

static TTY_U32 tty_get_u32(TTY_U8* data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static TTY_S16 tty_get_s16(TTY_U8* data) {
    return data[0] << 8 | data[1];
}

// static TTY_S32 tty_get_s32(TTY_U8* data) {
//     return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
// }


/* ---- */
/* Math */
/* ---- */
#define TTY_MAX(a, b) ((a) > (b) ? (a) : (b))

#define TTY_MIN(a, b) ((a) < (b) ? (a) : (b))

#define TTY_ROUNDED_DIV_POW2(a, addend, shift)\
    (TTY_S32)(((a) + (addend)) >> (shift))

#define TTY_FIX_DIV(a, b, numerShift, addend, shift)\
    TTY_ROUNDED_DIV_POW2(tty_rounded_div((TTY_S64)(a) << (numerShift), b), addend, shift)

#define TTY_F26DOT6_DIV(a, b)\
    TTY_FIX_DIV(a, b, 31, 0x1000000, 25)

#define TTY_F2DOT14_DIV(a, b)\
    TTY_FIX_DIV(a, b, 31, 0x10000, 17)

#define TTY_F16DOT16_DIV(a, b)\
    TTY_FIX_DIV(a, b, 31, 0x4000, 15)

#define TTY_F10DOT22_DIV(a, b)\
    TTY_FIX_DIV(a, b, 31, 0x100, 9)

#define TTY_FIX_MUL(a, b, addend, shift)\
    TTY_ROUNDED_DIV_POW2((TTY_S64)(a) * (TTY_S64)(b), addend, shift)

#define TTY_F26DOT6_MUL(a, b)\
    TTY_FIX_MUL(a, b, 0x20, 6)

#define TTY_F2DOT14_MUL(a, b)\
    TTY_FIX_MUL(a, b, 0x2000, 14)

#define TTY_F16DOT16_MUL(a, b)\
    TTY_FIX_MUL(a, b, 0x8000, 16)

#define TTY_F10DOT22_MUL(a, b)\
    TTY_FIX_MUL(a, b, 0x200000, 22)

#define TTY_F2DOT30_MUL(a, b)\
    TTY_FIX_MUL(a, b, 0x20000000, 30)

#define TTY_FIX_V2_ADD(a, b, result)\
    {\
        (result)->x = (a)->x + (b)->x;\
        (result)->y = (a)->y + (b)->y;\
    }

#define TTY_FIX_V2_SUB(a, b, result)\
    {\
        (result)->x = (a)->x - (b)->x;\
        (result)->y = (a)->y - (b)->y;\
    }

#define TTY_F26DOT6_V2_SCALE(v, scale, result)\
    {\
        (result)->x = TTY_F26DOT6_MUL((v)->x, scale);\
        (result)->y = TTY_F26DOT6_MUL((v)->y, scale);\
    }

static TTY_S64 tty_rounded_div(TTY_S64 a, TTY_S64 b) {
    return b == 0 ? 0 : (a < 0) ^ (b < 0) ? (a - b / 2) / b : (a + b / 2) / b;
}

static TTY_F26Dot6 tty_f26dot6_round(TTY_F26Dot6 val) {
    return ((val & 0x20) << 1) + (val & 0xFFFFFFC0);
}

static TTY_F26Dot6 tty_f26dot6_floor(TTY_F26Dot6 val) {
    return val & 0xFFFFFFC0;
}

static TTY_F26Dot6 tty_f26dot6_ceil(TTY_F26Dot6 val) {
    return (val & 0x3F) ? (val & 0xFFFFFFC0) + 0x40 : val;
}

static TTY_F26Dot6 tty_f26dot6_round_to_half_grid(TTY_F26Dot6 val) {
    if (val < 0) {
        val = -val;
        return -((val & 0xFFFFFFC0) | 0x20);
    }
    return (val & 0xFFFFFFC0) | 0x20;
}

static TTY_F26Dot6 tty_f26dot6_round_to_grid(TTY_F26Dot6 val) {
    if (val < 0) { 
        val = -val;
        return -(((val & 0x20) << 1) + (val & 0xFFFFFFC0));
    }
    return ((val & 0x20) << 1) + (val & 0xFFFFFFC0);
}

static TTY_F26Dot6 tty_f26dot6_round_to_double_grid(TTY_F26Dot6 val) {
    if ((val & 0x3F) <= 0xF) {
        return val & 0xFFFFFFE0;
    }
    return tty_f26dot6_round(val);
}

static TTY_F26Dot6 tty_f26dot6_round_down_to_grid(TTY_F26Dot6 val) {
    if (val < 0) {
        val = -val;
        return -(val & 0xFFFFFFC0);
    }
    return val & 0xFFFFFFC0;
}

static TTY_F26Dot6 tty_f26dot6_round_up_to_grid(TTY_F26Dot6 val) {
    if (val < 0) {
        val = -val;
        return -((val & 0x3F) ? (val & 0xFFFFFFC0) + 0x40 : val);
    }
    return (val & 0x3F) ? (val & 0xFFFFFFC0) + 0x40 : val;
}

static TTY_F2Dot14 tty_f2dot14_round_to_grid(TTY_F2Dot14 val) {
    if (val < 0) {
        val = -val;
        return -(((val & 0x2000) << 1) + (val & 0xFFFFC000));
    }
    return ((val & 0x2000) << 1) + (val & 0xFFFFC000);
}

static TTY_S32 tty_fix_v2_mag(TTY_V2* v) {
    // This is taken directly from the FreeType source code
    
    TTY_U32 lo = (TTY_U32)(v->x & 0xFFFF);
    TTY_S32 hi = v->x >> 16;

    TTY_U32 l = lo * lo;
    TTY_S32  m = hi * lo;
    
    hi = hi * hi;

    TTY_U32 lo1 = l  + (TTY_U32)(m << 17);
    TTY_S32 hi1 = hi + (m >> 15) + (lo1 < l);
    
    lo = (TTY_U32)(v->y & 0xFFFF);
    hi = v->y >> 16;

    l  = lo * lo;
    m  = hi * lo;
    hi = hi * hi;

    TTY_U32 lo2 = l  + (TTY_U32)(m << 17);
    TTY_S32 hi2 = hi + (m >> 15) + (lo2 < l);
    
    lo = lo1 + lo2;
    hi = hi1 + hi2 + (lo < lo1);
    
    TTY_U32 root  = 0;
    TTY_U32 rem   = 0;
    TTY_S32 count = 32;
    TTY_U32 testDiv;

    do {
        rem     =   (rem << 2) | ((TTY_U32)hi >> 30);
        hi      =   (hi  << 2) | (         lo >> 30);
        lo      <<= 2;
        root    <<= 1;
        testDiv =   (root << 1) + 1;

        if (rem >= testDiv) {
            rem  -= testDiv;
            root += 1;
        }
    }
    while (--count);

    return root;
}

static void tty_normalize_f26dot6_to_f2dot14(TTY_V2* v) {
    // This is taken directly from the FreeType source code
    
    if (labs(v->x) < 0x10000 && labs(v->y) < 0x10000) {
        v->x <<= 8;
        v->y <<= 8;

        TTY_F2Dot14 w = tty_fix_v2_mag(v);
        if (w != 0) {
            v->x = TTY_F2DOT14_DIV(v->x, w);
            v->y = TTY_F2DOT14_DIV(v->y, w);
        }

        return;
    }

    TTY_F26Dot6 w = tty_fix_v2_mag(v);
    v->x = TTY_F2DOT14_DIV(v->x, w);
    v->y = TTY_F2DOT14_DIV(v->y, w);
    w    = v->x * v->x + v->y * v->y;

    TTY_Bool xIsNeg, yIsNeg;

    if (v->x < 0) {
        v->x = -v->x;
        xIsNeg = TTY_TRUE;
    }
    else {
        xIsNeg = TTY_FALSE;
    }

    if (v->y < 0) {
        v->y = -v->y;
        yIsNeg = TTY_TRUE;
    }
    else {
        yIsNeg = TTY_FALSE;
    }

    while (w < 0x10000000) {
        if (v->x < v->y) {
            v->x++;
        }
        else {
            v->y++;
        }

        w = v->x * v->x + v->y * v->y;
    }

    while (w >= 0x10004000) {
        if (v->x < v->y) {
            v->x--;
        }
        else {
            v->y--;
        }

        w = v->x * v->x + v->y * v->y;
    }

    if (xIsNeg) {
        v->x = -v->x;
    }

    if (yIsNeg) {
        v->y = -v->y;
    }
}

static void tty_max_min(TTY_S32 a, TTY_S32 b, TTY_S32* max, TTY_S32* min) {
    if (a > b) {
        *max = a;
        *min = b;
    }
    else {
        *max = b;
        *min = a;
    }
}


/* ------- */
/* Hinting */
/* ------- */
enum {
    TTY_ROUND_TO_HALF_GRID  ,
    TTY_ROUND_TO_GRID       ,
    TTY_ROUND_TO_DOUBLE_GRID,
    TTY_ROUND_DOWN_TO_GRID  ,
    TTY_ROUND_UP_TO_GRID    ,
    TTY_ROUND_OFF           ,
};

enum {
    TTY_UNTOUCHED = 0x0,
    TTY_TOUCH_X   = 0x1,
    TTY_TOUCH_Y   = 0x2,
    TTY_TOUCH_XY  = 0x3,
};

enum {
    TTY_IUP_STATE_DEFAULT = 0x0,
    TTY_IUP_STATE_X       = 0x1,
    TTY_IUP_STATE_Y       = 0x2,
    TTY_IUP_STATE_XY      = 0x3,
};

enum {
    TTY_ABS        = 0x64,
    TTY_ADD        = 0x60,
    TTY_ALIGNRP    = 0x3C,
    TTY_AND        = 0x5A,
    TTY_CALL       = 0x2B,
    TTY_CINDEX     = 0x25,
    TTY_DELTAC1    = 0x73,
    TTY_DELTAC2    = 0x74,
    TTY_DELTAC3    = 0x75,
    TTY_DELTAP1    = 0x5D,
    TTY_DELTAP2    = 0x71,
    TTY_DELTAP3    = 0x72,
    TTY_DEPTH      = 0x24,
    TTY_DIV        = 0x62,
    TTY_DUP        = 0x20,
    TTY_EIF        = 0x59,
    TTY_ELSE       = 0x1B,
    TTY_ENDF       = 0x2D,
    TTY_EQ         = 0x54,
    TTY_FDEF       = 0x2C,
    TTY_FLOOR      = 0x66,
    TTY_GC         = 0x46,
    TTY_GC_MAX     = 0x47,
    TTY_GETINFO    = 0x88,
    TTY_GPV        = 0x0C,
    TTY_GT         = 0x52,
    TTY_GTEQ       = 0x53,
    TTY_IDEF       = 0x89,
    TTY_IF         = 0x58,
    TTY_IP         = 0x39,
    TTY_ISECT      = 0x0F,
    TTY_IUP        = 0x30,
    TTY_IUP_MAX    = 0x31,
    TTY_JROT       = 0x78,
    TTY_JMPR       = 0x1C,
    TTY_LOOPCALL   = 0x2A,
    TTY_LT         = 0x50,
    TTY_LTEQ       = 0x51,
    TTY_MAX        = 0x8B,
    TTY_MD         = 0x49,
    TTY_MD_MAX     = 0x4A,
    TTY_MDAP       = 0x2E,
    TTY_MDAP_MAX   = 0x2F,
    TTY_MDRP       = 0xC0,
    TTY_MDRP_MAX   = 0xDF,
    TTY_MIAP       = 0x3E,
    TTY_MIAP_MAX   = 0x3F,
    TTY_MIN        = 0x8C,
    TTY_MINDEX     = 0x26,
    TTY_MIRP       = 0xE0,
    TTY_MIRP_MAX   = 0xFF,
    TTY_MPPEM      = 0x4B,
    TTY_MUL        = 0x63,
    TTY_NEG        = 0x65,
    TTY_NEQ        = 0x55,
    TTY_NOT        = 0x5C,
    TTY_NPUSHB     = 0x40,
    TTY_NPUSHW     = 0x41,
    TTY_OR         = 0x5B,
    TTY_POP        = 0x21,
    TTY_PUSHB      = 0xB0,
    TTY_PUSHB_MAX  = 0xB7,
    TTY_PUSHW      = 0xB8,
    TTY_PUSHW_MAX  = 0xBF,
    TTY_RCVT       = 0x45,
    TTY_RDTG       = 0x7D,
    TTY_ROFF       = 0x7A,
    TTY_ROLL       = 0x8A,
    TTY_ROUND      = 0x68,
    TTY_ROUND_MAX  = 0x6B,
    TTY_RS         = 0x43,
    TTY_RTDG       = 0x3D,
    TTY_RTG        = 0x18,
    TTY_RTHG       = 0x19,
    TTY_RUTG       = 0x7C,
    TTY_SCANCTRL   = 0x85,
    TTY_SCANTYPE   = 0x8D,
    TTY_SCVTCI     = 0x1D,
    TTY_SDB        = 0x5E,
    TTY_SDPVTL     = 0x86,
    TTY_SDPVTL_MAX = 0x87,
    TTY_SDS        = 0x5F,
    TTY_SFVTCA     = 0x04,
    TTY_SFVTCA_MAX = 0x05,
    TTY_SFVTL      = 0x08,
    TTY_SFVTL_MAX  = 0x08,
    TTY_SFVTPV     = 0x0E,
    TTY_SHP        = 0x32,
    TTY_SHP_MAX    = 0x33,
    TTY_SHPIX      = 0x38,
    TTY_SLOOP      = 0x17,
    TTY_SMD        = 0x1A,
    TTY_SPVTCA     = 0x02,
    TTY_SPVTCA_MAX = 0x03,
    TTY_SRP0       = 0x10,
    TTY_SRP1       = 0x11,
    TTY_SRP2       = 0x12,
    TTY_SUB        = 0x61,
    TTY_SVTCA      = 0x00,
    TTY_SVTCA_MAX  = 0x01,
    TTY_SWAP       = 0x23,
    TTY_SZPS       = 0x16,
    TTY_SZP0       = 0x13,
    TTY_SZP1       = 0x14,
    TTY_SZP2       = 0x15,
    TTY_WCVTF      = 0x70,
    TTY_WCVTP      = 0x44,
    TTY_WS         = 0x42,
};

struct TTY_Program_Context;

typedef struct {
    void     (*execute_next_ins)(struct TTY_Program_Context*);
    TTY_U8*  buff;
    TTY_U32  off;
    TTY_U32  cap;
} TTY_Ins_Stream;

typedef struct TTY_Program_Context {
    TTY_Font*        font;
    TTY_Instance*    instance;
    TTY_Glyph*       glyph;
    TTY_Ins_Stream   stream;
    TTY_U8           iupState;
    TTY_Bool         foundUnknownIns; /* TODO: This can be removed once all instructions are implemented. */
} TTY_Program_Context;


static TTY_Bool tty_ins_stream_has_next(TTY_Ins_Stream* stream) {
    return stream->off < stream->cap;
}

static TTY_U8 tty_ins_stream_next(TTY_Ins_Stream* stream) {
    TTY_ASSERT(tty_ins_stream_has_next(stream));
    return stream->buff[stream->off++];
}

static TTY_U8* tty_ins_stream_next_ptr(TTY_Ins_Stream* stream) {
    TTY_ASSERT(tty_ins_stream_has_next(stream));
    return stream->buff + stream->off++;
}

static TTY_U8 tty_ins_stream_peek(TTY_Ins_Stream* stream) {
    TTY_ASSERT(tty_ins_stream_has_next(stream));
    return stream->buff[stream->off];
}

static void tty_ins_stream_consume(TTY_Ins_Stream* stream) {
    TTY_ASSERT(tty_ins_stream_has_next(stream));
    stream->off++;
}

#ifdef TTY_DEBUG
static TTY_Bool tty_ins_stream_can_jump(TTY_Ins_Stream* stream, TTY_S32 count) {
    return count < 0 ? -count <= stream->off : stream->off < stream->cap - count;
}
#endif

static void tty_ins_stream_jump(TTY_Ins_Stream* stream, TTY_S32 count) {
    TTY_ASSERT(tty_ins_stream_can_jump(stream, count));
    stream->off += count;
}

static TTY_U8 tty_ins_stream_jump_to_else_or_eif(TTY_Ins_Stream* stream) {
    TTY_U32 numNested = 0;

    while (TTY_TRUE) {
        TTY_U8 ins = tty_ins_stream_next(stream);

        if (ins >= TTY_PUSHB && ins <= TTY_PUSHB_MAX) {
            tty_ins_stream_jump(stream, 1 + (ins & 0x7));
        }
        else if (ins >= TTY_PUSHW && ins <= TTY_PUSHW_MAX) {
            tty_ins_stream_jump(stream, 2 * (1 + (ins & 0x7)));
        }
        else if (ins == TTY_NPUSHB) {
            tty_ins_stream_jump(stream, tty_ins_stream_next(stream));
        }
        else if (ins == TTY_NPUSHW) {
            tty_ins_stream_jump(stream, 2 * tty_ins_stream_next(stream));
        }
        else if (ins == TTY_IF) {
            numNested++;
        }
        else if (numNested == 0) {
            if (ins == TTY_EIF || ins == TTY_ELSE) {
                return ins;
            }
        }
        else if (ins == TTY_EIF) {
            numNested--;
        }
    }

    TTY_ASSERT(0);
    return 0;
}


static void tty_interp_stack_clear(TTY_Interp_Stack* stack) {
    stack->count = 0;
}

static void tty_interp_stack_push(TTY_Interp_Stack* stack, TTY_S32 val) {
    TTY_ASSERT(stack->count < stack->cap);
    stack->buff[stack->count++] = val;
}

static TTY_S32 tty_interp_stack_pop(TTY_Interp_Stack* stack) {
    TTY_ASSERT(stack->count > 0);
    return stack->buff[--stack->count];
}

static void tty_interp_stack_push_bytes_from_stream(TTY_Interp_Stack* stack, TTY_Ins_Stream* stream, TTY_U8 count) {
    do {
        TTY_U8 byte = tty_ins_stream_next(stream);
        tty_interp_stack_push(stack, byte);
    } while (--count != 0);
}

static void tty_interp_stack_push_words_from_stream(TTY_Interp_Stack* stack, TTY_Ins_Stream* stream, TTY_U8 count) {
    do {
        TTY_S8  ms  = tty_ins_stream_next(stream);
        TTY_U8  ls  = tty_ins_stream_next(stream);
        TTY_S32 val = (ms << 8) | ls;
        tty_interp_stack_push(stack, val);
    } while (--count != 0);
}


static void tty_call_func(TTY_Program_Context* ctx, TTY_U32 funcId, TTY_U32 count) {
    TTY_ASSERT(funcId < ctx->font->hint.funcs.cap);
    TTY_ASSERT(ctx->font->hint.funcs.insPtrs[funcId] != NULL);

    TTY_LOG_VALUE(funcId);

    TTY_Ins_Stream streamCpy = ctx->stream;
    ctx->stream.buff = ctx->font->hint.funcs.insPtrs[funcId];
    ctx->stream.cap  = ctx->font->hint.funcs.sizes  [funcId];

    while (count > 0) {
        ctx->stream.off = 0;

        while (tty_ins_stream_has_next(&ctx->stream)) {
            ctx->stream.execute_next_ins(ctx);
            if (ctx->foundUnknownIns) {
                break;
            }
        }

        count--;
    }

    ctx->stream = streamCpy;
}

static TTY_S32 tty_proj(TTY_Program_Context* ctx, TTY_V2* v) {
    return 
        TTY_F2DOT14_MUL(v->x, ctx->font->hint.gs.projVec.x) + 
        TTY_F2DOT14_MUL(v->y, ctx->font->hint.gs.projVec.y);
}

static TTY_S32 tty_dual_proj(TTY_Program_Context* ctx, TTY_V2* v) {
    return 
        TTY_F2DOT14_MUL(v->x, ctx->font->hint.gs.dualProjVec.x) + 
        TTY_F2DOT14_MUL(v->y, ctx->font->hint.gs.dualProjVec.y);
}

static TTY_S32 tty_sub_proj(TTY_Program_Context* ctx, TTY_V2* a, TTY_V2* b) {
    TTY_V2 diff;
    TTY_FIX_V2_SUB(a, b, &diff);
    return tty_proj(ctx, &diff);
}

static TTY_S32 tty_sub_dual_proj(TTY_Program_Context* ctx, TTY_V2* a, TTY_V2* b) {
    TTY_V2 diff;
    TTY_FIX_V2_SUB(a, b, &diff);
    return tty_dual_proj(ctx, &diff);
}

static void tty_update_proj_dot_free(TTY_Program_Context* ctx) {
    ctx->font->hint.gs.projDotFree =
        TTY_F2DOT30_MUL(ctx->font->hint.gs.projVec.x << 16, ctx->font->hint.gs.freedomVec.x << 16) +
        TTY_F2DOT30_MUL(ctx->font->hint.gs.projVec.y << 16, ctx->font->hint.gs.freedomVec.y << 16);

    if (labs(ctx->font->hint.gs.projDotFree) < 0x4000000) {
        ctx->font->hint.gs.projDotFree = 0x40000000;
    }
}

static TTY_F26Dot6 tty_mul_x_free_div_proj_dot_free(TTY_Program_Context* ctx, TTY_F26Dot6 val) {
    return tty_rounded_div((TTY_S64)val * (ctx->font->hint.gs.freedomVec.x << 16), ctx->font->hint.gs.projDotFree);
}

static TTY_F26Dot6 tty_mul_y_free_div_proj_dot_free(TTY_Program_Context* ctx, TTY_F26Dot6 val) {
    return tty_rounded_div((TTY_S64)val * (ctx->font->hint.gs.freedomVec.y << 16), ctx->font->hint.gs.projDotFree);
}

static void tty_move_point_x(TTY_Program_Context* ctx, TTY_Zone* zone, TTY_U32 idx, TTY_F26Dot6 dist) {
    // In accordance with the FreeType's v40 interpreter (with backward 
    // compatability enabled), movement along the x-axis is disabled 

    // zone->cur[idx].x      += dist;
    zone->touchFlags[idx] |= TTY_TOUCH_X;
}

static void tty_move_point_y(TTY_Program_Context* ctx, TTY_Zone* zone, TTY_U32 idx, TTY_F26Dot6 dist) {
    // In accordance with the FreeType's v40 interpreter (with backward 
    // compatability enabled), movement along the y-axis cannot occur post-IUP

    if (ctx->iupState != TTY_IUP_STATE_XY) {
        zone->cur[idx].y += dist;
    }
    zone->touchFlags[idx] |= TTY_TOUCH_Y;
}

static void tty_move_point(TTY_Program_Context* ctx, TTY_Zone* zone, TTY_U32 idx, TTY_F26Dot6 dist) {
    if (ctx->font->hint.gs.freedomVec.x != 0) {
        // In accordance with the FreeType's v40 interpreter (with backward 
        // compatability enabled), movement along the x-axis is disabled 

        // zone->cur[idx].x      += tty_mul_x_free_div_proj_dot_free(ctx, dist);
        zone->touchFlags[idx] |= TTY_TOUCH_X;
    }

    if (ctx->font->hint.gs.freedomVec.y != 0) {
        if (ctx->iupState != TTY_IUP_STATE_XY) {
            zone->cur[idx].y += tty_mul_y_free_div_proj_dot_free(ctx, dist);
        }

        zone->touchFlags[idx] |= TTY_TOUCH_Y;
    }
}

static void tty_move_point_zp2(TTY_Program_Context* ctx, TTY_U32 idx, TTY_F26Dot6_V2* dist, TTY_Bool applyTouch) {
    if (ctx->font->hint.gs.freedomVec.x != 0) {
        // In accordance with the FreeType's v40 interpreter (with backward 
        // compatability enabled), movement along the x-axis is disabled 

        // zone->cur[idx].x += dist->x;

        if (applyTouch) {
            ctx->font->hint.gs.zp2->touchFlags[idx] |= TTY_TOUCH_X;
        }
    }

    if (ctx->font->hint.gs.freedomVec.y != 0) {
        if (ctx->iupState != TTY_IUP_STATE_XY) {
            ctx->font->hint.gs.zp2->cur[idx].y += dist->y;
        }

        if (applyTouch) {
            ctx->font->hint.gs.zp2->touchFlags[idx] |= TTY_TOUCH_Y;
        }
    }
}

static void tty_update_move_point_func(TTY_Program_Context* ctx) {
    ctx->font->hint.gs.move_point = tty_move_point;

    if (ctx->font->hint.gs.projDotFree == 0x4000000) {
        if (ctx->font->hint.gs.freedomVec.x == 0) {
            ctx->font->hint.gs.move_point = tty_move_point_y;
        }
        else if (ctx->font->hint.gs.freedomVec.y == 0) {
            ctx->font->hint.gs.move_point = tty_move_point_x;
        }
    }
}

static TTY_F26Dot6 tty_round_according_to_round_state(TTY_Program_Context* ctx, TTY_F26Dot6 val) {
    // TODO: No idea how to apply "engine compensation" described in the spec

    switch (ctx->font->hint.gs.roundState) {
        case TTY_ROUND_TO_HALF_GRID:
            return tty_f26dot6_round_to_half_grid(val);
        case TTY_ROUND_TO_GRID:
            return tty_f26dot6_round_to_grid(val);
        case TTY_ROUND_TO_DOUBLE_GRID:
            return tty_f26dot6_round_to_double_grid(val);
        case TTY_ROUND_DOWN_TO_GRID:
            return tty_f26dot6_round_down_to_grid(val);
        case TTY_ROUND_UP_TO_GRID:
            return tty_f26dot6_round_up_to_grid(val);
        case TTY_ROUND_OFF:
            return val;
    }

    TTY_ASSERT(0);
    return 0;
}

static TTY_F26Dot6 tty_apply_single_width_cut_in(TTY_Program_Context* ctx, TTY_F26Dot6 value) {
    TTY_F26Dot6 absDiff = labs(value - ctx->font->hint.gs.singleWidthValue);
    if (absDiff < ctx->font->hint.gs.singleWidthCutIn) {
        if (value < 0) {
            return -ctx->font->hint.gs.singleWidthValue;
        }
        return ctx->font->hint.gs.singleWidthValue;
    }
    return value;
}

static TTY_F26Dot6 tty_apply_min_dist(TTY_Program_Context* ctx, TTY_F26Dot6 value) {
    if (labs(value) < ctx->font->hint.gs.minDist) {
        if (value < 0) {
            return -ctx->font->hint.gs.minDist;
        }
        return ctx->font->hint.gs.minDist;
    }
    return value;
}

static TTY_Zone* tty_get_zone_pointer(TTY_Program_Context* ctx, TTY_U8 zone) {
    switch (zone) {
        case 0:
            return &ctx->instance->hint.zone0;
        case 1:
            return &ctx->font->hint.zone1;
    }
    TTY_ASSERT(0);
    return NULL;
}

static void tty_ABS(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 val = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, labs(val));
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_ADD(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 n1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_F26Dot6 n2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, n1 + n2);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_ALIGNRP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_ASSERT(ctx->font->hint.gs.rp0 < ctx->font->hint.gs.zp0->numPoints);
    TTY_F26Dot6_V2* rp0Cur = ctx->font->hint.gs.zp0->cur + ctx->font->hint.gs.rp0;

    for (TTY_U32 i = 0; i < ctx->font->hint.gs.loop; i++) {
        TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp1->numPoints);

        TTY_F26Dot6 dist = tty_sub_proj(ctx, rp0Cur, ctx->font->hint.gs.zp1->cur + pointIdx);
        ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp1, pointIdx, dist);

        TTY_LOG_POINT(ctx->font->hint.gs.zp1->cur[pointIdx]);
    }

    ctx->font->hint.gs.loop = 1;
}

static void tty_AND(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 e2  = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 e1  = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 != 0 && e2 != 0 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_CALL(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_call_func(ctx, tty_interp_stack_pop(&ctx->font->hint.stack), 1);
}

static void tty_CINDEX(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 pos = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 val = ctx->font->hint.stack.buff[ctx->font->hint.stack.count - pos];
    tty_interp_stack_push(&ctx->font->hint.stack, val);
    TTY_LOG_VALUE(val);
}

static TTY_Bool tty_get_delta_value(TTY_Program_Context* ctx, TTY_U32 exc, TTY_U8 range, TTY_F26Dot6* deltaVal) {
    TTY_U32 ppem = ((exc & 0xF0) >> 4) + ctx->font->hint.gs.deltaBase + range;

    if (ctx->instance->ppem != ppem) {
        return TTY_FALSE;
    }

    TTY_S8 numSteps = (exc & 0xF) - 8;
    if (numSteps > 0) {
        numSteps++;
    }

    *deltaVal = numSteps * (1l << (6 - ctx->font->hint.gs.deltaShift));
    return TTY_TRUE;
}

static void tty_deltac_impl(TTY_Program_Context* ctx, TTY_U8 range) {
    TTY_U32 count = tty_interp_stack_pop(&ctx->font->hint.stack);

    while (count > 0) {
        TTY_U32 cvtIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_ASSERT(cvtIdx < ctx->instance->hint.cvt.cap);

        TTY_U32 exc = tty_interp_stack_pop(&ctx->font->hint.stack);

        TTY_F26Dot6 deltaVal;
        if (tty_get_delta_value(ctx, exc, range, &deltaVal)) {
            ctx->instance->hint.cvt.buff[cvtIdx] += deltaVal;
            TTY_LOG_VALUE(deltaVal);
        }

        count--;
    }
}

static void tty_deltap_impl(TTY_Program_Context* ctx, TTY_U8 range) {
    TTY_U32 count = tty_interp_stack_pop(&ctx->font->hint.stack);

    while (count > 0) {
        TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp0->numPoints);

        TTY_U32 exc = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_F26Dot6 deltaVal;

        if (tty_get_delta_value(ctx, exc, range, &deltaVal)) {
            // In accordance with the FreeType's v40 interpreter (with backward
            // compatability enabled), DELTAP instructions can only move a point if 
            // one of the following is true:
            //     - The glyph is composite and being moved along the y-axis
            //     - The point was previously touched on the y-axis

            if (ctx->iupState != TTY_IUP_STATE_XY                                      &&
                ((ctx->glyph->numContours < 0 && ctx->font->hint.gs.freedomVec.y != 0) ||
                (ctx->font->hint.gs.zp0->touchFlags[pointIdx] & TTY_TOUCH_Y))) 
            {
                ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp0, pointIdx, deltaVal);
                TTY_LOG_VALUE(deltaVal);
            }
        }

        count--;
    }
}

static void tty_DELTAC1(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_deltac_impl(ctx, 0);
}

static void tty_DELTAC2(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_deltac_impl(ctx, 16);
}

static void tty_DELTAC3(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_deltac_impl(ctx, 32);
}

static void tty_DELTAP1(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_deltap_impl(ctx, 0);
}

static void tty_DELTAP2(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_deltap_impl(ctx, 16);
}

static void tty_DELTAP3(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_deltap_impl(ctx, 32);
}

static void tty_DEPTH(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_interp_stack_push(&ctx->font->hint.stack, ctx->font->hint.stack.count);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_DIV(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 n1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_F26Dot6 n2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(n1 != 0);

    TTY_Bool isNeg = TTY_FALSE;
    
    if (n2 < 0) {
        n2    = -n2;
        isNeg = TTY_TRUE;
    }
    if (n1 < 0) {
        n1    = -n1;
        isNeg = !isNeg;
    }

    TTY_F26Dot6 result = ((TTY_S64)n2 << 6) / n1;
    if (isNeg) {
        result = -result;
    }

    tty_interp_stack_push(&ctx->font->hint.stack, result);
    TTY_LOG_VALUE(result);
}

static void tty_DUP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 e = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e);
    tty_interp_stack_push(&ctx->font->hint.stack, e);
    TTY_LOG_VALUE(e);
}

static void tty_EQ(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 == e2 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_FDEF(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_U32 funcId = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(funcId < ctx->font->hint.funcs.cap);

    ctx->font->hint.funcs.insPtrs[funcId] = tty_ins_stream_next_ptr(&ctx->stream);
    ctx->font->hint.funcs.sizes  [funcId] = 1;

    while (tty_ins_stream_next(&ctx->stream) != TTY_ENDF) {
        ctx->font->hint.funcs.sizes[funcId]++;
    }

    TTY_LOG_VALUE(funcId);
}

static void tty_FLOOR(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 val = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, tty_f26dot6_floor(val));
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_GC(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp2->numPoints);

    TTY_F26Dot6 val =
        ins & 0x1 ?
        tty_dual_proj(ctx, ctx->font->hint.gs.zp2->orgScaled + pointIdx) :
        tty_proj(ctx, ctx->font->hint.gs.zp2->cur + pointIdx);

    tty_interp_stack_push(&ctx->font->hint.stack, val);
    TTY_LOG_VALUE(val);
}

static void tty_GETINFO(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_U32 result   = 0;
    TTY_U32 selector = tty_interp_stack_pop(&ctx->font->hint.stack);

    if (selector & 0x00000001) {
        result = TTY_SCALAR_VERSION;
    }

    // Is the glyph rotated?
    if ((selector & 0x00000002) && ctx->instance->isRotated) {
        result |= (1 << 8);
    }

    // Is the glyph stretched?
    if ((selector & 0x00000004) && ctx->instance->isStretched) {
        result |= (1 << 9);
    }

    // Using Windows font smoothing grayscale?
    // Note: FreeType enables this when using grayscale rendering
    if ((selector & 0x00000020) && !ctx->instance->useSubpixelRendering) {
        result |= (1 << 12);
    }

    // Using subpixel hinting? 
    // -- Always true in accordance with FreeType's V40 interpreter
    if ((selector & 0x00000040)) {
        result |= (1 << 13);
    }

    tty_interp_stack_push(&ctx->font->hint.stack, result);
    TTY_LOG_VALUE(result);
}

static void tty_GPV(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_interp_stack_push(&ctx->font->hint.stack, ctx->font->hint.gs.projVec.x);
    tty_interp_stack_push(&ctx->font->hint.stack, ctx->font->hint.gs.projVec.y);
}

static void tty_GT(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 > e2 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_GTEQ(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 >= e2 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_IF(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    if (tty_interp_stack_pop(&ctx->font->hint.stack) == 0) {
        TTY_LOG_VALUE(0);
        if (tty_ins_stream_jump_to_else_or_eif(&ctx->stream) == TTY_EIF) {
            // Condition is false and there is no else instruction
            return;
        }
    }
    else {
        TTY_LOG_VALUE(1);
    }

    while (TTY_TRUE) {
        TTY_U8 ins = tty_ins_stream_peek(&ctx->stream);

        if (ins == TTY_ELSE) {
            tty_ins_stream_consume(&ctx->stream);
            tty_ins_stream_jump_to_else_or_eif(&ctx->stream);
            return;
        }

        if (ins == TTY_EIF) {
            tty_ins_stream_consume(&ctx->stream);
            return;
        }

        ctx->stream.execute_next_ins(ctx);
        if (ctx->foundUnknownIns) {
            return;
        }
    }
}

static void tty_IP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_ASSERT(ctx->font->hint.gs.rp1 < ctx->font->hint.gs.zp0->numPoints);
    TTY_ASSERT(ctx->font->hint.gs.rp2 < ctx->font->hint.gs.zp1->numPoints);

    TTY_F26Dot6_V2* rp1Cur = ctx->font->hint.gs.zp0->cur + ctx->font->hint.gs.rp1;
    TTY_F26Dot6_V2* rp2Cur = ctx->font->hint.gs.zp1->cur + ctx->font->hint.gs.rp2;

    TTY_Bool isTwilightZone = 
        ctx->font->hint.gs.gep0 == 0 || 
        ctx->font->hint.gs.gep1 == 0 || 
        ctx->font->hint.gs.gep2 == 0;

    TTY_F26Dot6_V2* rp1Org, *rp2Org;

    if (isTwilightZone) {
        // Twilight zone doesn't have unscaled coordinates
        rp1Org = ctx->font->hint.gs.zp0->orgScaled + ctx->font->hint.gs.rp1;
        rp2Org = ctx->font->hint.gs.zp1->orgScaled + ctx->font->hint.gs.rp2;
    }
    else {
        // Use unscaled coordinates for more precision
        rp1Org = ctx->font->hint.gs.zp0->org + ctx->font->hint.gs.rp1;
        rp2Org = ctx->font->hint.gs.zp1->org + ctx->font->hint.gs.rp2;
    }

    TTY_F26Dot6 totalDistCur = tty_sub_proj(ctx, rp2Cur, rp1Cur);
    TTY_F26Dot6 totalDistOrg = tty_sub_dual_proj(ctx, rp2Org, rp1Org);

    for (TTY_U32 i = 0; i < ctx->font->hint.gs.loop; i++) {
        TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp2->numPoints);

        TTY_V2* pointCur = ctx->font->hint.gs.zp2->cur + pointIdx;
        TTY_V2* pointOrg = (isTwilightZone ? ctx->font->hint.gs.zp2->orgScaled : ctx->font->hint.gs.zp2->org) + pointIdx;

        TTY_F26Dot6 distCur = tty_sub_proj(ctx, pointCur, rp1Cur);
        TTY_F26Dot6 distOrg = tty_sub_dual_proj(ctx, pointOrg, rp1Org);
        TTY_F26Dot6 distNew = TTY_F26DOT6_DIV(TTY_F26DOT6_MUL(distOrg, totalDistCur), totalDistOrg);

        ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp2, pointIdx, distNew - distCur);

        TTY_LOG_POINT(*pointCur);
    }

    ctx->font->hint.gs.loop = 1;
}

static void tty_ISECT(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_F26Dot6_V2* point;
    TTY_F26Dot6 x1, y1;
    TTY_F26Dot6 x2, y2;
    TTY_F26Dot6 x3, y3;
    TTY_F26Dot6 x4, y4;

    {
        TTY_U32 a2Idx    = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_U32 a1Idx    = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_U32 b2Idx    = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_U32 b1Idx    = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);

        TTY_ASSERT(a2Idx    < ctx->font->hint.gs.zp1->numPoints);
        TTY_ASSERT(a1Idx    < ctx->font->hint.gs.zp1->numPoints);
        TTY_ASSERT(b2Idx    < ctx->font->hint.gs.zp0->numPoints);
        TTY_ASSERT(b1Idx    < ctx->font->hint.gs.zp0->numPoints);
        TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp2->numPoints);

        x1 = ctx->font->hint.gs.zp1->cur[a1Idx].x;
        y1 = ctx->font->hint.gs.zp1->cur[a1Idx].y;

        x2 = ctx->font->hint.gs.zp1->cur[a2Idx].x;
        y2 = ctx->font->hint.gs.zp1->cur[a2Idx].y;

        x3 = ctx->font->hint.gs.zp0->cur[b1Idx].x;
        y3 = ctx->font->hint.gs.zp0->cur[b1Idx].y;

        x4 = ctx->font->hint.gs.zp0->cur[b2Idx].x;
        y4 = ctx->font->hint.gs.zp0->cur[b2Idx].y;

        point = ctx->font->hint.gs.zp2->cur + pointIdx;
        ctx->font->hint.gs.zp2->touchFlags[pointIdx] |= TTY_TOUCH_XY;
    }

    TTY_F26Dot6 denom  = TTY_F26DOT6_MUL(x1 - x2, y3 - y4) - TTY_F26DOT6_MUL(y1 - y2, x3 - x4);
    TTY_F26Dot6 lShare = TTY_F26DOT6_MUL(x1, y2) - TTY_F26DOT6_MUL(y1, x2);
    TTY_F26Dot6 rShare = TTY_F26DOT6_MUL(x3, y4) - TTY_F26DOT6_MUL(y3, x4);
    TTY_F26Dot6 l, r;

    TTY_ASSERT(denom != 0);

    l        = TTY_F26DOT6_MUL(lShare, x3 - x4);
    r        = TTY_F26DOT6_MUL(rShare, x1 - x2);
    point->x = TTY_F26DOT6_DIV(l - r, denom);

    l        = TTY_F26DOT6_MUL(lShare, y3 - y4);
    r        = TTY_F26DOT6_MUL(rShare, y1 - y2);
    point->y = TTY_F26DOT6_DIV(l - r, denom);

    TTY_LOG_POINT(*point);
}

static void tty_iup_interpolate_or_shift(TTY_Zone* zone1, TTY_U8 touchFlag, TTY_U16 startPointIdx, TTY_U16 endPointIdx, TTY_U16 touch0, TTY_U16 touch1) {
    #define TTY_IUP_INTERPOLATE(coord)\
        TTY_F26Dot6 totalDistCur = zone1->cur[touch1].coord - zone1->cur[touch0].coord;\
        TTY_S32     totalDistOrg = zone1->org[touch1].coord - zone1->org[touch0].coord;\
        TTY_S32     orgDist      = zone1->org[i].coord      - zone1->org[touch0].coord;\
        \
        TTY_F10Dot22 scale   = tty_rounded_div((TTY_S64)totalDistCur << 16, totalDistOrg);\
        TTY_F26Dot6  newDist = TTY_F10DOT22_MUL((TTY_S64)orgDist << 6, scale);\
        zone1->cur[i].coord = zone1->cur[touch0].coord + newDist;\
        \
        TTY_LOGF("Interp %3d: %5d", i, zone1->cur[i].coord)

    #define TTY_IUP_SHIFT(coord)\
        TTY_S32 diff0 = labs(zone1->org[touch0].coord - zone1->org[i].coord);\
        TTY_S32 diff1 = labs(zone1->org[touch1].coord - zone1->org[i].coord);\
        \
        if (diff0 < diff1) {\
            TTY_S32 diff = zone1->cur[touch0].coord - zone1->orgScaled[touch0].coord;\
            zone1->cur[i].coord += diff;\
        }\
        else {\
            TTY_S32 diff = zone1->cur[touch1].coord - zone1->orgScaled[touch1].coord;\
            zone1->cur[i].coord += diff;\
        }\
        TTY_LOGF("Shift %3d: %5d", i, zone1->cur[i].coord)

    #define TTY_IUP_INTERPOLATE_OR_SHIFT()\
        if (touchFlag == TTY_TOUCH_X) {\
            if (coord0 <= zone1->org[i].x && zone1->org[i].x <= coord1) {\
                TTY_IUP_INTERPOLATE(x);\
            }\
            else {\
                TTY_IUP_SHIFT(x);\
            }\
        }\
        else {\
            if (coord0 <= zone1->org[i].y && zone1->org[i].y <= coord1) {\
                TTY_IUP_INTERPOLATE(y);\
            }\
            else {\
                TTY_IUP_SHIFT(y);\
            }\
        }

    TTY_S32 coord0, coord1;

    if (touchFlag == TTY_TOUCH_X) {
        tty_max_min(zone1->org[touch0].x, zone1->org[touch1].x, &coord1, &coord0);
    }
    else {
        tty_max_min(zone1->org[touch0].y, zone1->org[touch1].y, &coord1, &coord0);
    }

    if (touch0 >= touch1) {
        for (TTY_U32 i = touch0 + 1; i <= endPointIdx; i++) {
            TTY_IUP_INTERPOLATE_OR_SHIFT();
        }

        for (TTY_U32 i = startPointIdx; i < touch1; i++) {
            TTY_IUP_INTERPOLATE_OR_SHIFT();
        } 
    }
    else {
        for (TTY_U32 i = touch0 + 1; i < touch1; i++) {
            TTY_IUP_INTERPOLATE_OR_SHIFT();
        }
    }

    #undef TTY_IUP_INTERPOLATE
    #undef TTY_IUP_SHIFT
    #undef TTY_IUP_INTERPOLATE_OR_SHIFT
}

static void tty_IUP(TTY_Program_Context* ctx, TTY_S8 ins) {
    TTY_LOG_INS();

    // Applying IUP to zone0 is an error
    TTY_ASSERT(ctx->font->hint.gs.gep2 == 1);

    // In accordance with the FreeType's v40 interpreter (with backward 
    // compatability enabled), points cannot be moved on either axis post-IUP.
    // Post-IUP occurs after IUP has been executed using both the x and y axes.
    if (ctx->iupState == TTY_IUP_STATE_XY) {
        return;
    }

    TTY_U8  touchFlag = ins & 0x1 ? TTY_TOUCH_X : TTY_TOUCH_Y;
    TTY_U32 pointIdx  = 0;

    ctx->iupState |= touchFlag;

    for (TTY_U32 i = 0; i < ctx->font->hint.zone1.numEndPoints; i++) {
        TTY_U16  startPointIdx = pointIdx;
        TTY_U16  endPointIdx   = ctx->font->hint.zone1.endPointIndices[i];
        TTY_U16  touch0        = 0;
        TTY_Bool findingTouch1 = TTY_FALSE;

        while (pointIdx <= endPointIdx) {
            if (ctx->font->hint.zone1.touchFlags[pointIdx] & touchFlag) {
                if (findingTouch1) {
                    tty_iup_interpolate_or_shift(&ctx->font->hint.zone1, touchFlag, startPointIdx, endPointIdx, touch0, pointIdx);

                    findingTouch1 = 
                        pointIdx != endPointIdx || 
                        (ctx->font->hint.zone1.touchFlags[startPointIdx] & touchFlag) == 0;

                    if (findingTouch1) {
                        touch0 = pointIdx;
                    }
                }
                else {
                    touch0        = pointIdx;
                    findingTouch1 = TTY_TRUE;
                }
            }

            pointIdx++;
        }

        if (findingTouch1) {
            // The index of the second touched point wraps back to the 
            // beginning.
            for (TTY_U32 i = startPointIdx; i <= touch0; i++) {
                if (ctx->font->hint.zone1.touchFlags[i] & touchFlag) {
                    tty_iup_interpolate_or_shift(&ctx->font->hint.zone1, touchFlag, startPointIdx, endPointIdx, touch0, i);

                    break;
                }
            }
        }
    }
}

static void tty_JROT(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    
    TTY_U32 val = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 off = tty_interp_stack_pop(&ctx->font->hint.stack); 

    if (val != 0) {
        tty_ins_stream_jump(&ctx->stream, off - 1);
        TTY_LOG_VALUE(off - 1);
    }
    else {
        TTY_LOG_VALUE(0);
    }
}

static void tty_JMPR(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 off = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_ins_stream_jump(&ctx->stream, off - 1);
    TTY_LOG_VALUE(off - 1);
}

static void tty_LOOPCALL(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 funcId = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 times  = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_call_func(ctx, funcId, times);
}

static void tty_LT(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 < e2 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_LTEQ(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 <= e2 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_MAX(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 > e2 ? e1 : e2);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_MD(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();
    
    TTY_U32     pointIdx0 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32     pointIdx1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_F26Dot6 dist;

    TTY_ASSERT(pointIdx0 < ctx->font->hint.gs.zp1->numPoints);
    TTY_ASSERT(pointIdx1 < ctx->font->hint.gs.zp0->numPoints);

    // TODO: Spec says if ins & 0x1 = 1 then use original outline, but FreeType
    //       uses current outline.

    if (ins & 0x1) {
        dist = tty_sub_proj(ctx, ctx->font->hint.gs.zp0->cur + pointIdx1, ctx->font->hint.gs.zp1->cur + pointIdx0);
    }
    else {
        TTY_Bool isTwilightZone = ctx->font->hint.gs.gep0 == 0 || ctx->font->hint.gs.gep1 == 0;

        if (isTwilightZone) {
            dist = tty_sub_dual_proj(ctx, ctx->font->hint.gs.zp0->orgScaled + pointIdx1, ctx->font->hint.gs.zp1->orgScaled + pointIdx0);
        }
        else {
            dist = tty_sub_dual_proj(ctx, ctx->font->hint.gs.zp0->org + pointIdx1, ctx->font->hint.gs.zp1->org + pointIdx0);
            dist = TTY_F10DOT22_MUL(dist << 6, ctx->instance->scale);
        }
    }

    tty_interp_stack_push(&ctx->font->hint.stack, dist);
    TTY_LOG_VALUE(dist);
}

static void tty_MDAP(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp0->numPoints);

    TTY_F26Dot6_V2* point = ctx->font->hint.gs.zp0->cur + pointIdx;

    if (ins & 0x1) {
        TTY_F26Dot6 curDist     = tty_proj(ctx, point);
        TTY_F26Dot6 roundedDist = tty_round_according_to_round_state(ctx, curDist);
        ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp0, pointIdx, roundedDist - curDist);
    }
    else {
        // Don't move the point, just mark it as touched

        if (ctx->font->hint.gs.freedomVec.x != 0) {
            if (ctx->font->hint.gs.freedomVec.y != 0) {
                ctx->font->hint.gs.zp0->touchFlags[pointIdx] = TTY_TOUCH_XY;
            }
            else {
                ctx->font->hint.gs.zp0->touchFlags[pointIdx] |= TTY_TOUCH_X;
            }
        }
        else {
            ctx->font->hint.gs.zp0->touchFlags[pointIdx] |= TTY_TOUCH_Y;
        }
    }

    ctx->font->hint.gs.rp0 = pointIdx;
    ctx->font->hint.gs.rp1 = pointIdx;

    TTY_LOG_POINT(*point);
}

static void tty_MDRP(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_ASSERT(ctx->font->hint.gs.rp0 < ctx->font->hint.gs.zp0->numPoints);

    TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp1->numPoints);

    TTY_F26Dot6_V2* rp0Cur         = ctx->font->hint.gs.zp0->cur + ctx->font->hint.gs.rp0;
    TTY_F26Dot6_V2* pointCur       = ctx->font->hint.gs.zp1->cur + pointIdx;
    TTY_Bool        isTwilightZone = ctx->font->hint.gs.gep0 == 0 || ctx->font->hint.gs.gep1 == 0;

    TTY_F26Dot6_V2* rp0Org, *pointOrg;

    if (isTwilightZone) {
        // Twilight zone doesn't have unscaled coordinates
        rp0Org   = ctx->font->hint.gs.zp0->orgScaled + ctx->font->hint.gs.rp0;
        pointOrg = ctx->font->hint.gs.zp1->orgScaled + pointIdx;
    }
    else {
        // Use unscaled coordinates for more precision
        rp0Org   = ctx->font->hint.gs.zp0->org + ctx->font->hint.gs.rp0;
        pointOrg = ctx->font->hint.gs.zp1->org + pointIdx;
    }

    TTY_F26Dot6 distCur = tty_sub_proj(ctx, pointCur, rp0Cur);
    TTY_F26Dot6 distOrg = tty_sub_dual_proj(ctx, pointOrg, rp0Org);

    if (!isTwilightZone) {
        distOrg = TTY_F10DOT22_MUL(distOrg << 6, ctx->instance->scale);
    }

    distOrg = tty_apply_single_width_cut_in(ctx, distOrg);

    if (ins & 0x04) {
        distOrg = tty_round_according_to_round_state(ctx, distOrg);
    }

    if (ins & 0x08) {
        distOrg = tty_apply_min_dist(ctx, distOrg);
    }

    if (ins & 0x10) {
        ctx->font->hint.gs.rp0 = pointIdx;
    }

    ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp1, pointIdx, distOrg - distCur);
    ctx->font->hint.gs.rp1 = ctx->font->hint.gs.rp0;
    ctx->font->hint.gs.rp2 = pointIdx;

    TTY_LOG_POINT(*pointCur);
}

static void tty_MIAP(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_U32 cvtIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(cvtIdx < ctx->instance->hint.cvt.cap);

    TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp0->numPoints);

    TTY_F26Dot6 newDist = ctx->instance->hint.cvt.buff[cvtIdx];

    if (ctx->font->hint.gs.gep0 == 0) {
        TTY_F26Dot6_V2* org = ctx->font->hint.gs.zp0->orgScaled + pointIdx;

        org->x = TTY_F2DOT14_MUL(newDist, ctx->font->hint.gs.freedomVec.x);
        org->y = TTY_F2DOT14_MUL(newDist, ctx->font->hint.gs.freedomVec.y);

        ctx->font->hint.gs.zp0->cur[pointIdx] = *org;
    }

    TTY_F26Dot6 curDist = tty_proj(ctx, ctx->font->hint.gs.zp0->cur + pointIdx);
    
    if (ins & 0x1) {
        if (labs(newDist - curDist) > ctx->font->hint.gs.controlValueCutIn) {
            newDist = curDist;
        }
        newDist = tty_round_according_to_round_state(ctx, newDist);
    }

    ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp0, pointIdx, newDist - curDist);
    
    ctx->font->hint.gs.rp0 = pointIdx;
    ctx->font->hint.gs.rp1 = pointIdx;

    TTY_LOG_POINT(ctx->font->hint.gs.zp0->cur[pointIdx]);
}

static void tty_MIN(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 < e2 ? e1 : e2);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_MINDEX(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_U32 idx  = ctx->font->hint.stack.count - ctx->font->hint.stack.buff[ctx->font->hint.stack.count - 1] - 1;
    size_t  size = sizeof(TTY_S32) * (ctx->font->hint.stack.count - idx - 1);

    ctx->font->hint.stack.count--;
    ctx->font->hint.stack.buff[ctx->font->hint.stack.count] = ctx->font->hint.stack.buff[idx];
    memcpy(ctx->font->hint.stack.buff + idx, ctx->font->hint.stack.buff + idx + 1, size);
}

static void tty_MIRP(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_U32 cvtIdx   = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);

    TTY_ASSERT(cvtIdx   < ctx->instance->hint.cvt.cap);
    TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp1->numPoints);

    TTY_F26Dot6 cvtVal = tty_apply_single_width_cut_in(ctx, ctx->instance->hint.cvt.buff[cvtIdx]);

    TTY_F26Dot6_V2* rp0Org = ctx->font->hint.gs.zp0->orgScaled + ctx->font->hint.gs.rp0;
    TTY_F26Dot6_V2* rp0Cur = ctx->font->hint.gs.zp0->cur       + ctx->font->hint.gs.rp0;

    TTY_F26Dot6_V2* pointOrg = ctx->font->hint.gs.zp1->orgScaled + pointIdx;
    TTY_F26Dot6_V2* pointCur = ctx->font->hint.gs.zp1->cur       + pointIdx;

    if (ctx->font->hint.gs.gep1 == 0) {
        pointOrg->x = rp0Org->x + TTY_F2DOT14_MUL(cvtVal, ctx->font->hint.gs.freedomVec.x);
        pointOrg->y = rp0Org->y + TTY_F2DOT14_MUL(cvtVal, ctx->font->hint.gs.freedomVec.y);
        *pointCur   = *pointOrg;
    }

    TTY_S32 distCur = tty_sub_proj(ctx, pointCur, rp0Cur);
    TTY_S32 distOrg = tty_sub_dual_proj(ctx, pointOrg, rp0Org);

    if (ctx->font->hint.gs.autoFlip) {
        if ((distOrg ^ cvtVal) < 0) {
            // Match the sign of distOrg
            cvtVal = -cvtVal;
        }
    }

    TTY_S32 distNew;
    
    if (ins & 0x4) {
        if (ctx->font->hint.gs.gep0 == ctx->font->hint.gs.gep1) {
            if (labs(cvtVal - distOrg) > ctx->font->hint.gs.controlValueCutIn) {
                cvtVal = distOrg;
            }
        }
        distNew = tty_round_according_to_round_state(ctx, cvtVal);
    }
    else {
        distNew = cvtVal;
    }

    if (ins & 0x08) {
        distNew = tty_apply_min_dist(ctx, distNew);
    }

    ctx->font->hint.gs.move_point(ctx, ctx->font->hint.gs.zp1, pointIdx, distNew - distCur);
    ctx->font->hint.gs.rp1 = ctx->font->hint.gs.rp0;
    ctx->font->hint.gs.rp2 = pointIdx;

    if (ins & 0x10) {
        ctx->font->hint.gs.rp0 = pointIdx;
    }

    TTY_LOG_POINT(*pointCur);
}

static void tty_MPPEM(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_interp_stack_push(&ctx->font->hint.stack, ctx->instance->ppem);
    TTY_LOG_VALUE(ctx->instance->ppem);
}

static void tty_MUL(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 n1     = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_F26Dot6 n2     = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_F26Dot6 result = TTY_F26DOT6_MUL(n1, n2);
    tty_interp_stack_push(&ctx->font->hint.stack, result);
    TTY_LOG_VALUE(result);
}

static void tty_NEG(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 val = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, -val);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_NEQ(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e1 != e2 ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_NOT(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 val = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, !val);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_NPUSHB(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U8 ins = tty_ins_stream_next(&ctx->stream);
    tty_interp_stack_push_bytes_from_stream(&ctx->font->hint.stack, &ctx->stream, ins);
}

static void tty_NPUSHW(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U8 ins = tty_ins_stream_next(&ctx->stream);
    tty_interp_stack_push_words_from_stream(&ctx->font->hint.stack, &ctx->stream, ins);
}

static void tty_OR(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_S32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_S32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, (e1 != 0 || e2 != 0) ? 1 : 0);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_POP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    tty_interp_stack_pop(&ctx->font->hint.stack);
}

static void tty_PUSHB(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();
    tty_interp_stack_push_bytes_from_stream(&ctx->font->hint.stack, &ctx->stream, 1 + (ins & 0x7));
}

static void tty_PUSHW(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();
    tty_interp_stack_push_words_from_stream(&ctx->font->hint.stack, &ctx->stream, 1 + (ins & 0x7));
}

static void tty_RCVT(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    
    TTY_U32 cvtIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(cvtIdx < ctx->instance->hint.cvt.cap);

    tty_interp_stack_push(&ctx->font->hint.stack, ctx->instance->hint.cvt.buff[cvtIdx]);
    TTY_LOG_VALUE(ctx->instance->hint.cvt.buff[cvtIdx]);
}

static void tty_RDTG(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.roundState = TTY_ROUND_DOWN_TO_GRID;
}

static void tty_ROFF(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.roundState = TTY_ROUND_OFF;
}

static void tty_ROLL(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 a = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 b = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 c = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, b);
    tty_interp_stack_push(&ctx->font->hint.stack, a);
    tty_interp_stack_push(&ctx->font->hint.stack, c);
}

static void tty_ROUND(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();
    TTY_F26Dot6 dist = tty_interp_stack_pop(&ctx->font->hint.stack);
    dist = tty_round_according_to_round_state(ctx, dist);
    tty_interp_stack_push(&ctx->font->hint.stack, dist);
    TTY_LOG_VALUE(dist);
}

static void tty_RS(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 idx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(idx < ctx->instance->hint.storage.cap);
    tty_interp_stack_push(&ctx->font->hint.stack, ctx->instance->hint.storage.buff[idx]);
    TTY_LOG_VALUE(ctx->instance->hint.storage.buff[idx]);
}

static void tty_RTDG(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.roundState = TTY_ROUND_TO_DOUBLE_GRID;
}

static void tty_RTG(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.roundState = TTY_ROUND_TO_GRID;
}

static void tty_RTHG(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.roundState = TTY_ROUND_TO_HALF_GRID;
}

static void tty_RUTG(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.roundState = TTY_ROUND_UP_TO_GRID;
}

static void tty_SCANCTRL(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_U16 flags  = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U8  thresh = flags & 0xFF;
    
    if (thresh == 0xFF) {
        ctx->font->hint.gs.scanControl = TTY_TRUE;
    }
    else if (thresh == 0x0) {
        ctx->font->hint.gs.scanControl = TTY_FALSE;
    }
    else {
        if ((flags & 0x100) && ctx->instance->ppem <= thresh) {
            ctx->font->hint.gs.scanControl = TTY_TRUE;
        }

        if ((flags & 0x200) && ctx->instance->isRotated) {
            ctx->font->hint.gs.scanControl = TTY_TRUE;
        }

        if ((flags & 0x400) && ctx->instance->isStretched) {
            ctx->font->hint.gs.scanControl = TTY_TRUE;
        }

        if ((flags & 0x800) && thresh > ctx->instance->ppem) {
            ctx->font->hint.gs.scanControl = TTY_FALSE;
        }

        if ((flags & 0x1000) && !ctx->instance->isRotated) {
            ctx->font->hint.gs.scanControl = TTY_FALSE;
        }

        if ((flags & 0x2000) && !ctx->instance->isStretched) {
            ctx->font->hint.gs.scanControl = TTY_FALSE;
        }
    }

    TTY_LOG_VALUE(ctx->font->hint.gs.scanControl);
}

static void tty_SCANTYPE(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.scanType = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.scanType);
}

static void tty_SCVTCI(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.controlValueCutIn = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.controlValueCutIn);
}

static void tty_SDB(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.deltaBase = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.deltaBase);
}

static void tty_SDPVTL(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_U32 p1Idx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 p2Idx = tty_interp_stack_pop(&ctx->font->hint.stack);

    TTY_ASSERT(p1Idx < ctx->font->hint.gs.zp2->numPoints);
    TTY_ASSERT(p2Idx < ctx->font->hint.gs.zp1->numPoints);

    TTY_F26Dot6_V2* p1;
    TTY_F26Dot6_V2* p2;


    p1 = ctx->font->hint.gs.zp2->orgScaled + p1Idx;
    p2 = ctx->font->hint.gs.zp1->orgScaled + p2Idx;

    ctx->font->hint.gs.dualProjVec.x = p2->x - p1->x;
    ctx->font->hint.gs.dualProjVec.y = p2->y - p1->y;

    if (ctx->font->hint.gs.dualProjVec.x == 0) {
        if (ctx->font->hint.gs.dualProjVec.y == 0) {
            ctx->font->hint.gs.dualProjVec.x = 0x4000;
            ins = 0;
        }
    }


    if (ins & 0x1) {
        // Perpendicular (counter clockwise rotation)
        TTY_F26Dot6 temp = ctx->font->hint.gs.dualProjVec.y;
        ctx->font->hint.gs.dualProjVec.y = ctx->font->hint.gs.dualProjVec.x;
        ctx->font->hint.gs.dualProjVec.x = -temp;
    }

    tty_normalize_f26dot6_to_f2dot14(&ctx->font->hint.gs.dualProjVec);


    p1 = ctx->font->hint.gs.zp2->cur + p1Idx;
    p2 = ctx->font->hint.gs.zp1->cur + p2Idx;

    ctx->font->hint.gs.projVec.x = p2->x - p1->x;
    ctx->font->hint.gs.projVec.y = p2->y - p1->y;

    if (ins & 0x1) {
        // Perpendicular (counter clockwise rotation)
        TTY_F26Dot6 temp = ctx->font->hint.gs.projVec.y;
        ctx->font->hint.gs.projVec.y = ctx->font->hint.gs.projVec.x;
        ctx->font->hint.gs.projVec.x = -temp;
    }

    tty_normalize_f26dot6_to_f2dot14(&ctx->font->hint.gs.projVec);
    tty_update_proj_dot_free(ctx);
    tty_update_move_point_func(ctx);

    TTY_LOG_POINT(ctx->font->hint.gs.dualProjVec);
    TTY_LOG_POINT(ctx->font->hint.gs.projVec);
    TTY_LOG_VALUE(ctx->font->hint.gs.projDotFree);
}

static void tty_SFVTL(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_U32 p1Idx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 p2Idx = tty_interp_stack_pop(&ctx->font->hint.stack);

    TTY_ASSERT(p1Idx < ctx->font->hint.gs.zp2->numPoints);
    TTY_ASSERT(p2Idx < ctx->font->hint.gs.zp1->numPoints);

    TTY_F26Dot6_V2* p1 = ctx->font->hint.gs.zp2->cur + p1Idx;
    TTY_F26Dot6_V2* p2 = ctx->font->hint.gs.zp1->cur + p2Idx;

    TTY_F26Dot6_V2 diff;
    TTY_FIX_V2_SUB(p2, p1, &diff);
    tty_normalize_f26dot6_to_f2dot14(&diff);

    if (ins & 0x1) {
        ctx->font->hint.gs.freedomVec.x = -diff.y;
        ctx->font->hint.gs.freedomVec.y = diff.x;
    }
    else {
        ctx->font->hint.gs.freedomVec.x = diff.x;
        ctx->font->hint.gs.freedomVec.y = diff.y;
    }

    tty_update_proj_dot_free(ctx);
    tty_update_move_point_func(ctx);

    TTY_LOG_POINT(ctx->font->hint.gs.freedomVec);
    TTY_LOG_VALUE(ctx->font->hint.gs.projDotFree);
}

static void tty_SDS(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.deltaShift = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.deltaShift);
}

static void tty_SFVTCA(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    if (ins & 0x1) {
        ctx->font->hint.gs.freedomVec.x = 0x4000;
        ctx->font->hint.gs.freedomVec.y = 0;
    }
    else {
        ctx->font->hint.gs.freedomVec.x = 0;
        ctx->font->hint.gs.freedomVec.y = 0x4000;
    }

    tty_update_proj_dot_free(ctx);
    tty_update_move_point_func(ctx);

    TTY_LOG_POINT(ctx->font->hint.gs.freedomVec);
    TTY_LOG_VALUE(ctx->font->hint.gs.projDotFree);
}

static void tty_SFVTPV(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.freedomVec = ctx->font->hint.gs.projVec;
    tty_update_proj_dot_free(ctx);
    tty_update_move_point_func(ctx);
    TTY_LOG_POINT(ctx->font->hint.gs.freedomVec);
    TTY_LOG_VALUE(ctx->font->hint.gs.projDotFree);
}

static void tty_SHP(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    TTY_F26Dot6_V2 dist;
    {
        TTY_F26Dot6_V2* refPointCur, *refPointOrg;

        if (ins & 0x1) {
            TTY_ASSERT(ctx->font->hint.gs.rp1 < ctx->font->hint.gs.zp0->numPoints);
            refPointCur = ctx->font->hint.gs.zp0->cur       + ctx->font->hint.gs.rp1;
            refPointOrg = ctx->font->hint.gs.zp0->orgScaled + ctx->font->hint.gs.rp1;
        }
        else {
            TTY_ASSERT(ctx->font->hint.gs.rp2 < ctx->font->hint.gs.zp1->numPoints);
            refPointCur = ctx->font->hint.gs.zp1->cur       + ctx->font->hint.gs.rp2;
            refPointOrg = ctx->font->hint.gs.zp1->orgScaled + ctx->font->hint.gs.rp2;
        }

        TTY_F26Dot6 d = tty_sub_proj(ctx, refPointCur, refPointOrg);

        dist.x = tty_mul_x_free_div_proj_dot_free(ctx, d);
        dist.y = tty_mul_y_free_div_proj_dot_free(ctx, d);
    }

    for (TTY_U32 i = 0; i < ctx->font->hint.gs.loop; i++) {
        TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp2->numPoints);

        tty_move_point_zp2(ctx, pointIdx, &dist, TTY_TRUE);
        TTY_LOG_POINT(ctx->font->hint.gs.zp2->cur[pointIdx]);
    }

    ctx->font->hint.gs.loop = 1;
}

static void tty_SHPIX(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    
    TTY_F26Dot6_V2 dist;
    {
        TTY_F26Dot6 amt = tty_interp_stack_pop(&ctx->font->hint.stack);
        dist.x = TTY_F2DOT14_MUL(amt, ctx->font->hint.gs.freedomVec.x);
        dist.y = TTY_F2DOT14_MUL(amt, ctx->font->hint.gs.freedomVec.y);
    }

    TTY_Bool isTwilightZone =
        ctx->font->hint.gs.gep0 == 0 && ctx->font->hint.gs.gep1 == 0 && ctx->font->hint.gs.gep2 == 0;

    for (TTY_U32 i = 0; i < ctx->font->hint.gs.loop; i++) {
        TTY_U32 pointIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
        TTY_ASSERT(pointIdx < ctx->font->hint.gs.zp2->numPoints);

        // In accordance with the FreeType's v40 interpreter (with backward 
        // compatability enabled), SHPIX can only move a point if one of the 
        // following is true:
        //     - The point is in the twilight zone
        //     - The glyph is composite and being moved along the y-axis
        //     - The point was previously touched on the y-axis

        TTY_Bool shouldMove = isTwilightZone;

        if (!shouldMove && ctx->iupState != TTY_IUP_STATE_XY) {
            shouldMove = 
                (ctx->glyph->numContours < 0 && ctx->font->hint.gs.freedomVec.y != 0) ||
                (ctx->font->hint.gs.zp2->touchFlags[pointIdx] & TTY_TOUCH_Y);
        }

        if (shouldMove) {
            tty_move_point_zp2(ctx, pointIdx, &dist, TTY_TRUE);
            TTY_LOG_POINT(ctx->font->hint.gs.zp2->cur[pointIdx]);
        }
    }

    ctx->font->hint.gs.loop = 1;
}

static void tty_SLOOP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.loop = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.loop);
}

static void tty_SMD(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.minDist = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.minDist);
}

static void tty_SPVTCA(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    if (ins & 0x1) {
        ctx->font->hint.gs.projVec.x = 0x4000;
        ctx->font->hint.gs.projVec.y = 0;
    }
    else {
        ctx->font->hint.gs.projVec.x = 0;
        ctx->font->hint.gs.projVec.y = 0x4000;
    }

    ctx->font->hint.gs.dualProjVec = ctx->font->hint.gs.projVec;
    tty_update_proj_dot_free(ctx);
    tty_update_move_point_func(ctx);

    TTY_LOG_POINT(ctx->font->hint.gs.projVec);
    TTY_LOG_POINT(ctx->font->hint.gs.dualProjVec);
    TTY_LOG_VALUE(ctx->font->hint.gs.projDotFree);
}

static void tty_SRP0(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.rp0 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.rp0);
}

static void tty_SRP1(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.rp1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.rp1);
}

static void tty_SRP2(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    ctx->font->hint.gs.rp2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_LOG_VALUE(ctx->font->hint.gs.rp2);
}

static void tty_SUB(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_F26Dot6 n1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_F26Dot6 n2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, n2 - n1);
    TTY_LOG_INTERP_STACK_TOP(ctx->font->hint.stack);
}

static void tty_SVTCA(TTY_Program_Context* ctx, TTY_U8 ins) {
    TTY_LOG_INS();

    if (ins & 0x1) {
        ctx->font->hint.gs.freedomVec.x = 0x4000;
        ctx->font->hint.gs.freedomVec.y = 0;
        ctx->font->hint.gs.move_point   = tty_move_point_x;
    }
    else {
        ctx->font->hint.gs.freedomVec.x = 0;
        ctx->font->hint.gs.freedomVec.y = 0x4000;
        ctx->font->hint.gs.move_point   = tty_move_point_y;
    }

    ctx->font->hint.gs.projVec     = ctx->font->hint.gs.freedomVec;
    ctx->font->hint.gs.dualProjVec = ctx->font->hint.gs.freedomVec;
    ctx->font->hint.gs.projDotFree = 0x40000000;

    TTY_LOG_POINT(ctx->font->hint.gs.projVec);
    TTY_LOG_POINT(ctx->font->hint.gs.dualProjVec);
    TTY_LOG_POINT(ctx->font->hint.gs.freedomVec);
    TTY_LOG_VALUE(ctx->font->hint.gs.projDotFree);
}

static void tty_SWAP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 e2 = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 e1 = tty_interp_stack_pop(&ctx->font->hint.stack);
    tty_interp_stack_push(&ctx->font->hint.stack, e2);
    tty_interp_stack_push(&ctx->font->hint.stack, e1);
}

static void tty_SZPS(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 zone = tty_interp_stack_pop(&ctx->font->hint.stack);
    ctx->font->hint.gs.zp0  = tty_get_zone_pointer(ctx, zone);
    ctx->font->hint.gs.zp1  = ctx->font->hint.gs.zp0;
    ctx->font->hint.gs.zp2  = ctx->font->hint.gs.zp0;
    ctx->font->hint.gs.gep0 = zone;
    ctx->font->hint.gs.gep1 = zone;
    ctx->font->hint.gs.gep2 = zone;
    TTY_LOG_VALUE(zone);
}

static void tty_SZP0(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 zone = tty_interp_stack_pop(&ctx->font->hint.stack);
    ctx->font->hint.gs.zp0  = tty_get_zone_pointer(ctx, zone);
    ctx->font->hint.gs.gep0 = zone;
    TTY_LOG_VALUE(zone);
}

static void tty_SZP1(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 zone = tty_interp_stack_pop(&ctx->font->hint.stack);
    ctx->font->hint.gs.zp1  = tty_get_zone_pointer(ctx, zone);
    ctx->font->hint.gs.gep1 = zone;
    TTY_LOG_VALUE(zone);
}

static void tty_SZP2(TTY_Program_Context* ctx) {
    TTY_LOG_INS();
    TTY_U32 zone = tty_interp_stack_pop(&ctx->font->hint.stack);
    ctx->font->hint.gs.zp2  = tty_get_zone_pointer(ctx, zone);
    ctx->font->hint.gs.gep2 = zone;
    TTY_LOG_VALUE(zone);
}

static void tty_WCVTF(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_U32 funits = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 cvtIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(cvtIdx < ctx->instance->hint.cvt.cap);

    ctx->instance->hint.cvt.buff[cvtIdx] = TTY_F10DOT22_MUL(funits << 6, ctx->instance->scale);

    TTY_LOG_VALUE(ctx->instance->hint.cvt.buff[cvtIdx]);
}

static void tty_WCVTP(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_U32 pixels = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 cvtIdx = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(cvtIdx < ctx->instance->hint.cvt.cap);

    ctx->instance->hint.cvt.buff[cvtIdx] = pixels;
    TTY_LOG_VALUE(ctx->instance->hint.cvt.buff[cvtIdx]);
}

static void tty_WS(TTY_Program_Context* ctx) {
    TTY_LOG_INS();

    TTY_S32 value = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_U32 idx   = tty_interp_stack_pop(&ctx->font->hint.stack);
    TTY_ASSERT(idx < ctx->instance->hint.storage.cap);

    ctx->instance->hint.storage.buff[idx] = value;
    TTY_LOG_VALUE(ctx->instance->hint.storage.buff[idx]);
}

static TTY_Error tty_execute_program(TTY_Program_Context* ctx) {
    while (tty_ins_stream_has_next(&ctx->stream)) {
        ctx->stream.execute_next_ins(ctx);
        if (ctx->foundUnknownIns) {
            return TTY_ERROR_UNKNOWN_INSTRUCTION;
        }
    }

    return TTY_ERROR_NONE;
}

/* A shared instruction is one that can appear in both CV and glyph programs */
static TTY_Bool tty_try_execute_shared_ins(TTY_Program_Context* ctx, TTY_U8 ins) {
    switch (ins) {
        case TTY_ABS:
            tty_ABS(ctx);
            return TTY_TRUE;
        case TTY_ADD:
            tty_ADD(ctx);
            return TTY_TRUE;
        case TTY_AND:
            tty_AND(ctx);
            return TTY_TRUE;
        case TTY_CALL:
            tty_CALL(ctx);
            return TTY_TRUE;
        case TTY_CINDEX:
            tty_CINDEX(ctx);
            return TTY_TRUE;
        case TTY_DELTAC1:
            tty_DELTAC1(ctx);
            return TTY_TRUE;
        case TTY_DELTAC2:
            tty_DELTAC2(ctx);
            return TTY_TRUE;
        case TTY_DELTAC3:
            tty_DELTAC3(ctx);
            return TTY_TRUE;
        case TTY_DEPTH:
            tty_DEPTH(ctx);
            return TTY_TRUE;
        case TTY_DIV:
            tty_DIV(ctx);
            return TTY_TRUE;
        case TTY_DUP:
            tty_DUP(ctx);
            return TTY_TRUE;
        case TTY_EQ:
            tty_EQ(ctx);
            return TTY_TRUE;
        case TTY_FLOOR:
            tty_FLOOR(ctx);
            return TTY_TRUE;
        case TTY_GETINFO:
            tty_GETINFO(ctx);
            return TTY_TRUE;
        case TTY_GPV:
            tty_GPV(ctx);
            return TTY_TRUE;
        case TTY_GT:
            tty_GT(ctx);
            return TTY_TRUE;
        case TTY_GTEQ:
            tty_GTEQ(ctx);
            return TTY_TRUE;
        case TTY_IF:
            tty_IF(ctx);
            return TTY_TRUE;
        case TTY_JROT:
            tty_JROT(ctx);
            return TTY_TRUE;
        case TTY_JMPR:
            tty_JMPR(ctx);
            return TTY_TRUE;
        case TTY_LOOPCALL:
            tty_LOOPCALL(ctx);
            return TTY_TRUE;
        case TTY_LT:
            tty_LT(ctx);
            return TTY_TRUE;
        case TTY_LTEQ:
            tty_LTEQ(ctx);
            return TTY_TRUE;
        case TTY_MAX:
            tty_MAX(ctx);
            return TTY_TRUE;
        case TTY_MIN:
            tty_MIN(ctx);
            return TTY_TRUE;
        case TTY_MINDEX:
            tty_MINDEX(ctx);
            return TTY_TRUE;
        case TTY_MPPEM:
            tty_MPPEM(ctx);
            return TTY_TRUE;
        case TTY_MUL:
            tty_MUL(ctx);
            return TTY_TRUE;
        case TTY_NEG:
            tty_NEG(ctx);
            return TTY_TRUE;
        case TTY_NEQ:
            tty_NEQ(ctx);
            return TTY_TRUE;
        case TTY_NOT:
            tty_NOT(ctx);
            return TTY_TRUE;
        case TTY_NPUSHB:
            tty_NPUSHB(ctx);
            return TTY_TRUE;
        case TTY_NPUSHW:
            tty_NPUSHW(ctx);
            return TTY_TRUE;
        case TTY_OR:
            tty_OR(ctx);
            return TTY_TRUE;
        case TTY_POP:
            tty_POP(ctx);
            return TTY_TRUE;
        case TTY_RCVT:
            tty_RCVT(ctx);
            return TTY_TRUE;
        case TTY_RDTG:
            tty_RDTG(ctx);
            return TTY_TRUE;
        case TTY_ROFF:
            tty_ROFF(ctx);
            return TTY_TRUE;
        case TTY_ROLL:
            tty_ROLL(ctx);
            return TTY_TRUE;
        case TTY_RS:
            tty_RS(ctx);
            return TTY_TRUE;
        case TTY_RTDG:
            tty_RTDG(ctx);
            return TTY_TRUE;
        case TTY_RTG:
            tty_RTG(ctx);
            return TTY_TRUE;
        case TTY_RTHG:
            tty_RTHG(ctx);
            return TTY_TRUE;
        case TTY_RUTG:
            tty_RUTG(ctx);
            return TTY_TRUE;
        case TTY_SCANCTRL:
            tty_SCANCTRL(ctx);
            return TTY_TRUE;
        case TTY_SCANTYPE:
            tty_SCANTYPE(ctx);
            return TTY_TRUE;
        case TTY_SCVTCI:
            tty_SCVTCI(ctx);
            return TTY_TRUE;
        case TTY_SDB:
            tty_SDB(ctx);
            return TTY_TRUE;
        case TTY_SDS:
            tty_SDS(ctx);
            return TTY_TRUE;
        case TTY_SFVTPV:
            tty_SFVTPV(ctx);
            return TTY_TRUE;
        case TTY_SLOOP:
            tty_SLOOP(ctx);
            return TTY_TRUE;
        case TTY_SUB:
            tty_SUB(ctx);
            return TTY_TRUE;
        case TTY_SWAP:
            tty_SWAP(ctx);
            return TTY_TRUE;
        case TTY_WCVTF:
            tty_WCVTF(ctx);
            return TTY_TRUE;
        case TTY_WCVTP:
            tty_WCVTP(ctx);
            return TTY_TRUE;
        case TTY_WS:
            tty_WS(ctx);
            return TTY_TRUE;
    }

    if (ins >= TTY_PUSHB && ins <= TTY_PUSHB_MAX) {
        tty_PUSHB(ctx, ins);
    }
    else if (ins >= TTY_PUSHW && ins <= TTY_PUSHW_MAX) {
        tty_PUSHW(ctx, ins);
    }
    else if (ins >= TTY_ROUND && ins <= TTY_ROUND_MAX) {
        tty_ROUND(ctx, ins);
    }
    else if (ins >= TTY_SFVTCA && ins <= TTY_SFVTCA_MAX) {
        tty_SFVTCA(ctx, ins);
    }
    else if (ins >= TTY_SPVTCA && ins <= TTY_SPVTCA_MAX) {
        tty_SPVTCA(ctx, ins);
    }
    else if (ins >= TTY_SVTCA && ins <= TTY_SVTCA_MAX) {
        tty_SVTCA(ctx, ins);
    }
    else {
        return TTY_FALSE;
    }

    return TTY_TRUE;
}


/* ------------ */
/* Font Loading */
/* ------------ */
static char f_buf[1024*8];
static TTY_Error tty_read_file_into_buff(TTY_Font* font, const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return TTY_ERROR_FAILED_TO_READ_FILE;
    }

    if (fseek(f, 0, SEEK_END)   != 0  ||
        (font->size = ftell(f)) <  0  ||
        fseek(f, 0, SEEK_SET)   != 0)
    {
        fclose(f);
        return TTY_ERROR_FAILED_TO_READ_FILE;
    }
    
    font->data = (TTY_U8*)malloc(font->size);
    if (font->data == NULL) {
        fclose(f);
        return TTY_ERROR_OUT_OF_MEMORY;
    }
    
    setbuffer(f, f_buf, 1024*8);
    if ((TTY_S32)fread(font->data, 1, font->size, f) != font->size) {
        fclose(f);
        free(font->data);
        return TTY_ERROR_FAILED_TO_READ_FILE;
    }
    
    if (fclose(f) != 0) {
        free(font->data);
        return TTY_ERROR_FAILED_TO_READ_FILE;
    }
    
    return TTY_ERROR_NONE;
}

static TTY_Error tty_verify_file_is_ttf(TTY_Font* font) {
    TTY_U32 sfntVersion = tty_get_u32(font->data);
    
    if (sfntVersion == 0x00010000            ||
        TTY_TAG_EQUALS(&sfntVersion, "true") ||
        TTY_TAG_EQUALS(&sfntVersion, "typ1"))
    {
        return TTY_ERROR_NONE;
    }
    
    return TTY_ERROR_FILE_IS_NOT_TTF;
}

static TTY_Error tty_extract_table_directory(TTY_Font* font) {
    TTY_U16 numTables = tty_get_u16(font->data + 4);

    for (TTY_U32 i = 0; i < numTables; i++) {
        TTY_U32    off = 12 + 16 * i;
        TTY_U8*    tag = font->data + off;
        TTY_Table* table;
        
        if (!font->cmap.exists && TTY_TAG_EQUALS(tag, "cmap")) {
            table = &font->cmap;
        }
        else if (!font->cvt.exists && TTY_TAG_EQUALS(tag, "cvt ")) {
            table = &font->cvt;
        }
        else if (!font->fpgm.exists && TTY_TAG_EQUALS(tag, "fpgm")) {
            table = &font->fpgm;
        }
        else if (!font->glyf.exists && TTY_TAG_EQUALS(tag, "glyf")) {
            table = &font->glyf;
        }
        else if (!font->head.exists && TTY_TAG_EQUALS(tag, "head")) {
            table = &font->head;
        }
        else if (!font->hhea.exists && TTY_TAG_EQUALS(tag, "hhea")) {
            table = &font->hhea;
        }
        else if (!font->hmtx.exists && TTY_TAG_EQUALS(tag, "hmtx")) {
            table = &font->hmtx;
        }
        else if (!font->loca.exists && TTY_TAG_EQUALS(tag, "loca")) {
            table = &font->loca;
        }
        else if (!font->maxp.exists && TTY_TAG_EQUALS(tag, "maxp")) {
            table = &font->maxp;
        }
        else if (!font->OS2.exists && TTY_TAG_EQUALS(tag, "OS/2")) {
            table = &font->OS2;
        }
        else if (!font->prep.exists && TTY_TAG_EQUALS(tag, "prep")) {
            table = &font->prep;
        }
        else if (!font->vmtx.exists && TTY_TAG_EQUALS(tag, "vmtx")) {
            table = &font->vmtx;
        }
        else {
            table = NULL;
        }

        if (table != NULL) {
            table->off    = tty_get_u32(font->data + off + 8);
            table->size   = tty_get_u32(font->data + off + 12);
            table->exists = TTY_TRUE;
        }
    }
    
    if (font->cmap.exists     && 
        font->glyf.exists     && 
        font->head.exists     && 
        font->hhea.exists     &&
        font->hmtx.exists     && 
        font->loca.exists     &&
        font->maxp.exists)
    {
        return TTY_ERROR_NONE;
    }
    
    return TTY_ERROR_FILE_IS_CORRUPTED;
}

static TTY_Bool tty_format_is_supported(TTY_U16 format) {
    switch (format) {
        case 4:
        // case 6:
        // case 8:
        // case 10:
        // case 12:
        // case 13:
        // case 14:
            return TTY_TRUE;
    }
    return TTY_FALSE;
}

static TTY_Error tty_extract_char_encoding(TTY_Font* font) {
    TTY_U16 numTables = tty_get_u16(font->data + font->cmap.off + 2);
    
    for (TTY_U16 i = 0; i < numTables; i++) {
        TTY_U8*  data       = font->data + font->cmap.off + 4 + i * 8;
        TTY_U16  platformId = tty_get_u16(data);
        TTY_U16  encodingId = tty_get_u16(data + 2);
        TTY_Bool foundValid = TTY_FALSE;

        switch (platformId) {
            case 0:
                foundValid = encodingId >= 3 && encodingId <= 6;
                break;
            case 3:
                foundValid = encodingId == 1 || encodingId == 10;
                break;
        }

        if (foundValid) {
            font->encoding.platformId = platformId;
            font->encoding.encodingId = encodingId;
            font->encoding.off        = tty_get_u32(data + 4);

            TTY_U8* subtable = font->data + font->cmap.off + font->encoding.off;
            TTY_U16 format   = tty_get_u16(subtable);

            if (tty_format_is_supported(format)) {
                return TTY_ERROR_NONE;
            }
        }
    }
    
    return TTY_ERROR_UNSUPPORTED_FEATURE;
}

static TTY_Error tty_alloc_font_hinting_data(TTY_Font* font) {
    font->hint.stack.cap = tty_get_u16(font->data + font->maxp.off + 24);
    font->hint.funcs.cap = tty_get_u16(font->data + font->maxp.off + 20);
    
    {
        TTY_U16 maxContours          = tty_get_u16(font->data + font->maxp.off + 8);
        TTY_U16 maxCompositeContours = tty_get_u16(font->data + font->maxp.off + 12);
        font->hint.zone1.maxEndPoints = TTY_MAX(maxContours, maxCompositeContours);
    }
    
    {
        // Not sure if maxPoints or maxCompositePoints includes phantom points,
        // so will add them just to be safe
        TTY_U16 maxPoints            = tty_get_u16(font->data + font->maxp.off + 6);
        TTY_U16 maxCompositePoints   = tty_get_u16(font->data + font->maxp.off + 10);
        font->hint.zone1.maxPoints = TTY_MAX(maxPoints, maxCompositePoints) + TTY_NUM_PHANTOM_POINTS;
    }

    #define ALIGN(x) (x - (x % 4) + 4)
    size_t stackSize             = ALIGN(tty_add_padding_to_align_u8_ptr(font->hint.stack.cap * sizeof(TTY_U32)));
    size_t funcInsPtrsSize       = ALIGN(tty_add_padding_to_align_u32   (font->hint.funcs.cap * sizeof(TTY_U8*)));
    size_t funcSizesSize         = ALIGN(tty_add_padding_to_align_v2    (font->hint.funcs.cap * sizeof(TTY_U32)));
    size_t z1OrgSize             = ALIGN(font->hint.zone1.maxPoints * sizeof(TTY_V2));
    size_t z1OrgScaledSize       = ALIGN(z1OrgSize);
    size_t z1CurSize             = ALIGN(tty_add_padding_to_align_u8 (z1OrgSize));
    size_t z1TouchTypesSize      = ALIGN(tty_add_padding_to_align_u8 (font->hint.zone1.maxPoints * sizeof(TTY_U8)));
    size_t z1PointTypesSize      = ALIGN(tty_add_padding_to_align_u16(font->hint.zone1.maxPoints * sizeof(TTY_U8)));
    size_t z1EndPointIndicesSize = ALIGN(font->hint.zone1.maxEndPoints * sizeof(TTY_U16));

    font->hint.mem = (TTY_U8*)malloc(stackSize + funcInsPtrsSize + funcSizesSize + z1OrgSize + z1OrgScaledSize + z1CurSize + z1TouchTypesSize + z1PointTypesSize + z1EndPointIndicesSize + 9*4);
    if (font->hint.mem == NULL) {
        return TTY_ERROR_OUT_OF_MEMORY;
    }
    size_t off = 0;
    font->hint.stack.buff            = (TTY_U32*)(font->hint.mem);
    font->hint.funcs.insPtrs         = (TTY_U8**)(font->hint.mem + (off += stackSize));
    font->hint.funcs.sizes           = (TTY_U32*)(font->hint.mem + (off += funcInsPtrsSize));
    font->hint.zone1.org             = (TTY_V2*) (font->hint.mem + (off += funcSizesSize));
    font->hint.zone1.orgScaled       = (TTY_V2*) (font->hint.mem + (off += z1OrgSize));
    font->hint.zone1.cur             = (TTY_V2*) (font->hint.mem + (off += z1OrgScaledSize));
    font->hint.zone1.touchFlags      = (TTY_U8*) (font->hint.mem + (off += z1CurSize));
    font->hint.zone1.pointTypes      = (TTY_U8*) (font->hint.mem + (off += z1TouchTypesSize));
    font->hint.zone1.endPointIndices = (TTY_U16*)(font->hint.mem + (off += z1PointTypesSize));
    
    return TTY_ERROR_NONE;
}

static void tty_execute_next_font_program_ins(TTY_Program_Context* ctx) {
    TTY_U8 ins = tty_ins_stream_next(&ctx->stream);

    switch (ins) {
        case TTY_FDEF:
            tty_FDEF(ctx);
            return;
        // case 0x89:
        //     tty_IDEF(ctx);
        //     return;
        case TTY_NPUSHB:
            tty_NPUSHB(ctx);
            return;
        case TTY_NPUSHW:
            tty_NPUSHW(ctx);
            return;
    }

    if (ins >= TTY_PUSHB && ins <= TTY_PUSHB_MAX) {
        tty_PUSHB(ctx, ins);
    }
    else if (ins >= TTY_PUSHW && ins <= TTY_PUSHW_MAX) {
        tty_PUSHW(ctx, ins);
    }
    else {
        TTY_LOG_UNKNOWN_INS(ins);
        ctx->foundUnknownIns = TTY_TRUE;
    }
}

static TTY_Error tty_execute_font_program(TTY_Font* font) {
    TTY_LOG_PROGRAM("Font Program");
    
    TTY_Program_Context ctx = {
        .font            = font,
        .instance        = NULL,
        .glyph           = NULL,
        .iupState        = TTY_IUP_STATE_DEFAULT,
        .foundUnknownIns = TTY_FALSE,
        .stream   = {
            .execute_next_ins = tty_execute_next_font_program_ins,
            .buff             = font->data + font->fpgm.off,
            .cap              = font->fpgm.size,
            .off              = 0,
        }
    };

    return tty_execute_program(&ctx);
}

TTY_Error tty_font_init(TTY_Font* font, const char* path) {
    memset(font, 0, sizeof(TTY_Font));

    TTY_Error error;
    
    // Hinting data is allocated even if the font doesn't have hinting because
    // zone1 will still be used to store glyph points
    if ((error = tty_read_file_into_buff    (font, path)) ||
        (error = tty_verify_file_is_ttf     (font))       ||
        (error = tty_extract_table_directory(font))       ||
        (error = tty_extract_char_encoding  (font))       ||
        (error = tty_alloc_font_hinting_data(font)))
    {
        goto failure;
    }
    
    font->startingEdgeCap = 100;
    font->upem            = tty_get_u16(font->data + font->head.off + 18);
    font->numGlyphs       = tty_get_u16(font->data + font->maxp.off + 4);
    font->ascender        = tty_get_s16(font->data + font->hhea.off + 4);
    font->descender       = tty_get_s16(font->data + font->hhea.off + 6);
    font->lineGap         = tty_get_s16(font->data + font->hhea.off + 8);
    font->maxHoriExtent   = tty_get_s16(font->data + font->hhea.off + 16);
    font->hasHinting      = font->cvt.exists && font->fpgm.exists && font->prep.exists;
    
    if (font->hasHinting && (error = tty_execute_font_program(font))) {
        goto failure;
    }

    return TTY_ERROR_NONE;
    
failure:
    tty_font_free(font);
    return error;
}

void tty_font_free(TTY_Font* font) {
    if (font != NULL) {
        free(font->data);
        free(font->hint.mem);
    }
}


/* ---------------- */
/* Instance Loading */
/* ---------------- */
static TTY_Error tty_alloc_instance_hinting_data(TTY_Font* font, TTY_Instance* instance) {
    instance->hint.cvt.cap         = font->cvt.size / sizeof(TTY_S16);
    instance->hint.storage.cap     = tty_get_u16(font->data + font->maxp.off + 18);
    instance->hint.zone0.maxPoints = tty_get_u16(font->data + font->maxp.off + 16);
    
    size_t cvtSize         = tty_add_padding_to_align_s32(instance->hint.cvt.cap     * sizeof(TTY_F26Dot6));
    size_t storeSize       = tty_add_padding_to_align_v2 (instance->hint.storage.cap * sizeof(TTY_S32));
    size_t z0OrgScaledSize = instance->hint.zone0.maxPoints * sizeof(TTY_V2);
    size_t z0CurSize       = tty_add_padding_to_align_u8(z0OrgScaledSize);
    size_t z0TouchSize     = instance->hint.zone0.maxPoints * sizeof(TTY_U8);
    
    instance->hint.mem = (TTY_U8*)malloc(cvtSize + storeSize + z0OrgScaledSize + z0CurSize + z0TouchSize);
    if (instance->hint.mem == NULL) {
        return TTY_ERROR_OUT_OF_MEMORY;
    }
    
    size_t off = 0;
    instance->hint.cvt.buff         = (TTY_F26Dot6*)(instance->hint.mem);
    instance->hint.storage.buff     = (TTY_S32*)    (instance->hint.mem + (off += cvtSize));
    instance->hint.zone0.orgScaled  = (TTY_V2*)     (instance->hint.mem + (off += storeSize));
    instance->hint.zone0.cur        = (TTY_V2*)     (instance->hint.mem + (off += z0OrgScaledSize));
    instance->hint.zone0.touchFlags = (TTY_U8*)     (instance->hint.mem + (off += z0CurSize));

    return TTY_ERROR_NONE;
}

static void tty_fill_control_value_table(TTY_Font* font, TTY_Instance* instance) {
    // Convert default CVT values from font units to 26.6 pixel units
    TTY_U32 idx = 0;

    for (TTY_U32 off = 0; off < font->cvt.size; off += 2) {
        TTY_S32 funits = tty_get_s16(font->data + font->cvt.off + off);
        instance->hint.cvt.buff[idx++] = TTY_F10DOT22_MUL(funits << 6, instance->scale);
    }
}

static void tty_reset_graphics_state(TTY_Graphics_State* gs, TTY_Zone* zone1) {
    gs->move_point        = tty_move_point_x;
    gs->zp0               = zone1;
    gs->zp1               = zone1;
    gs->zp2               = zone1;
    gs->projDotFree       = 0x40000000;
    gs->minDist           = 0x40;
    gs->controlValueCutIn = 68;
    gs->singleWidthCutIn  = 0;
    gs->singleWidthValue  = 0;
    gs->dualProjVec.x     = 0x4000;
    gs->dualProjVec.y     = 0;
    gs->freedomVec.x      = 0x4000;
    gs->freedomVec.y      = 0;
    gs->projVec.x         = 0x4000;
    gs->projVec.y         = 0;
    gs->deltaBase         = 9;
    gs->deltaShift        = 3;
    gs->loop              = 1;
    gs->rp0               = 0;
    gs->rp1               = 0;
    gs->rp2               = 0;
    gs->gep0              = 1;
    gs->gep1              = 1;
    gs->gep2              = 1;
    gs->roundState        = TTY_ROUND_TO_GRID;
    gs->scanType          = 0;
    gs->scanControl       = TTY_FALSE;
    gs->autoFlip          = TTY_TRUE;
}

static void tty_execute_next_cv_program_ins(TTY_Program_Context* ctx) {
    TTY_U8 ins = tty_ins_stream_next(&ctx->stream);

    if (tty_try_execute_shared_ins(ctx, ins)) {
        return;
    }

    switch (ins) {
        case TTY_FDEF:
            tty_FDEF(ctx);
            return;
        // case TTY_IDEF:
        //     tty_IDEF(ctx);
        //     return TTY_TRUE;
    }

    TTY_LOG_UNKNOWN_INS(ins);
    ctx->foundUnknownIns = TTY_TRUE;
}

static TTY_Error tty_execute_cv_program(TTY_Font* font, TTY_Instance* instance) {
    TTY_LOG_PROGRAM("CV Program");

    // "Every time the control value program is run, the zone 0 contour data is
    //  initialized to 0s."
    memset(instance->hint.zone0.orgScaled,  0, instance->hint.zone0.maxPoints * sizeof(TTY_V2));
    memset(instance->hint.zone0.cur,        0, instance->hint.zone0.maxPoints * sizeof(TTY_V2));
    memset(instance->hint.zone0.touchFlags, 0, instance->hint.zone0.maxPoints * sizeof(TTY_U8));

    tty_reset_graphics_state(&font->hint.gs, &font->hint.zone1);
    tty_interp_stack_clear(&font->hint.stack);

    TTY_Program_Context ctx = {
        .font            = font,
        .instance        = instance,
        .glyph           = NULL,
        .iupState        = TTY_IUP_STATE_DEFAULT,
        .foundUnknownIns = TTY_FALSE,
        .stream   = {
            .execute_next_ins = tty_execute_next_cv_program_ins,
            .buff             = font->data + font->prep.off,
            .cap              = font->prep.size,
            .off              = 0,
        }
    };
    
    return tty_execute_program(&ctx);
}

TTY_Error tty_instance_init(TTY_Font* font, TTY_Instance* instance, TTY_U32 ppem, TTY_U32 flags) {
    memset(instance, 0, sizeof(TTY_Instance));
    
    instance->useHinting           = font->hasHinting && (flags ^ TTY_INSTANCE_NO_HINTING);
    instance->useSubpixelRendering = flags & TTY_INSTANCE_SUBPIXEL_RENDERING_RGB;
    instance->isRotated            = TTY_FALSE;
    instance->isStretched          = TTY_FALSE;

    if (instance->useHinting) {
        TTY_Error error;
        if ((error = tty_alloc_instance_hinting_data(font, instance))) {
            return error;
        }
    }

    return tty_instance_resize(font, instance, ppem);
}

TTY_Error tty_instance_resize(TTY_Font* font, TTY_Instance* instance, TTY_U32 ppem) {
    instance->scale          = tty_rounded_div((TTY_S64)ppem << 22, font->upem);
    instance->ppem           = ppem;
    instance->ascender       = tty_f26dot6_ceil(TTY_F10DOT22_MUL(font->ascender      << 6, instance->scale)) >> 6;
    instance->descender      = tty_f26dot6_ceil(TTY_F10DOT22_MUL(font->descender     << 6, instance->scale)) >> 6;
    instance->lineGap        = tty_f26dot6_ceil(TTY_F10DOT22_MUL(font->lineGap       << 6, instance->scale)) >> 6;
    instance->maxGlyphSize.x = tty_f26dot6_ceil(TTY_F10DOT22_MUL(font->maxHoriExtent << 6, instance->scale)) >> 6;
    instance->maxGlyphSize.y = instance->ascender - instance->descender;

    if (!instance->useHinting) {
        return TTY_ERROR_NONE;
    }

    tty_fill_control_value_table(font, instance);
    return tty_execute_cv_program(font, instance);
}

void tty_instance_free(TTY_Instance* instance) {
    if (instance != NULL) {
        free(instance->hint.mem);
        instance->hint.mem = NULL;
    }
}


/* ------------- */
/* Glyph Loading */
/* ------------- */
static TTY_U16 tty_get_glyph_index_format_4(TTY_U8* subtable, TTY_U32 cp) {
    #define TTY_GET_END_CODE(index) tty_get_u16(subtable + 14 + 2 * (index))
    
    TTY_U16 segCount = tty_get_u16(subtable + 6) >> 1;
    TTY_U16 left     = 0;
    TTY_U16 right    = segCount - 1;

    while (left <= right) {
        TTY_U16 mid     = (left + right) / 2;
        TTY_U16 endCode = TTY_GET_END_CODE(mid);

        if (endCode >= cp) {
            if (mid == 0 || TTY_GET_END_CODE(mid - 1) < cp) {
                TTY_U32 off            = 16 + 2 * mid;
                TTY_U8* idRangeOffsets = subtable + 6 * segCount + off;
                TTY_U16 idRangeOffset  = tty_get_u16(idRangeOffsets);
                TTY_U16 startCode      = tty_get_u16(subtable + 2 * segCount + off);

                if (startCode > cp) {
                    return 0;
                }
                
                if (idRangeOffset == 0) {
                    TTY_U16 idDelta = tty_get_s16(subtable + 4 * segCount + off);
                    return cp + idDelta;
                }

                return tty_get_u16(idRangeOffset + 2 * (cp - startCode) + idRangeOffsets);
            }
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    return 0;

    #undef TTY_GET_END_CODE
}

static TTY_U8* tty_get_glyf_data_block(TTY_Font* font, TTY_U32 glyphIdx) {
    #define TTY_GET_OFF_16(idx)\
        (tty_get_u16(font->data + font->loca.off + (2 * (idx))) * 2)

    #define TTY_GET_OFF_32(idx)\
        tty_get_u32(font->data + font->loca.off + (4 * (idx)))

    TTY_S16 version = tty_get_s16(font->data + font->head.off + 50);
    TTY_U32 blockOff;
    TTY_U32 nextBlockOff;

    if (glyphIdx == font->numGlyphs - 1u) {
        blockOff = version == 0 ? TTY_GET_OFF_16(glyphIdx) : TTY_GET_OFF_32(glyphIdx);
        return font->data + font->glyf.off + blockOff;
    }

    if (version == 0) {
        blockOff     = TTY_GET_OFF_16(glyphIdx);
        nextBlockOff = TTY_GET_OFF_16(glyphIdx + 1);
    }
    else {
        blockOff     = TTY_GET_OFF_32(glyphIdx);
        nextBlockOff = TTY_GET_OFF_32(glyphIdx + 1);
    }
    
    if (blockOff == nextBlockOff) {
        // "If a glyph has no outline, then loca[n] = loca [n+1]"
        return NULL;
    }

    return font->data + font->glyf.off + blockOff;
    
    #undef TTY_GET_OFF_16
    #undef TTY_GET_OFF_32
}

TTY_Error tty_get_glyph_index(TTY_Font* font, TTY_U32 codePoint, TTY_U32* idx) {
    TTY_U8* subtable = font->data + font->cmap.off + font->encoding.off;

    switch (tty_get_u16(subtable)) {
        case 4:
            *idx = tty_get_glyph_index_format_4(subtable, codePoint);
            return TTY_ERROR_NONE;
        case 6:
            // TODO
            return TTY_ERROR_UNSUPPORTED_FEATURE;
        case 8:
            // TODO
            return TTY_ERROR_UNSUPPORTED_FEATURE;
        case 10:
            // TODO
            return TTY_ERROR_UNSUPPORTED_FEATURE;
        case 12:
            // TODO
            return TTY_ERROR_UNSUPPORTED_FEATURE;
        case 13:
            // TODO
            return TTY_ERROR_UNSUPPORTED_FEATURE;
        case 14:
            // TODO
            return TTY_ERROR_UNSUPPORTED_FEATURE;
    }
    
    // This shouldn't happen because 'tty_font_init' fails if a supported 
    // encoding is not found
    TTY_ASSERT(TTY_FALSE);
    return TTY_ERROR_UNSUPPORTED_FEATURE;
}

TTY_Error tty_glyph_init(TTY_Font* font, TTY_Glyph* glyph, TTY_U32 idx) {
    glyph->idx       = idx;
    glyph->glyfBlock = tty_get_glyf_data_block(font, idx);
    
    if (glyph->glyfBlock == NULL) {
        // The glyph is an empty glyph (i.e. space)
        glyph->numContours = 0;
    }
    else {
        glyph->numContours = tty_get_s16(glyph->glyfBlock);
    }
    
    // These are not calculated until the glyph is rendered
    glyph->advance.x = 0;
    glyph->advance.y = 0;
    glyph->offset.x  = 0;
    glyph->offset.y  = 0;
    glyph->size.x    = 0;
    glyph->size.y    = 0;
    glyph->cache     = NULL;
    
    return TTY_ERROR_NONE;
}


/* --------- */
/* Rendering */
/* --------- */
enum {
    TTY_ON_CURVE_POINT ,
    TTY_OFF_CURVE_POINT,
};

enum {
    TTY_GLYF_ON_CURVE_POINT = 0x01,
    TTY_GLYF_X_SHORT_VECTOR = 0x02,
    TTY_GLYF_Y_SHORT_VECTOR = 0x04,
    TTY_GLYF_REPEAT_FLAG    = 0x08,
    TTY_GLYF_X_DUAL         = 0x10, /* X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR */
    TTY_GLYF_Y_DUAL         = 0x20, /* Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR */
    TTY_GLYF_OVERLAP_SIMPLE = 0x40,
};

enum {
    TTY_GLYF_ARG_1_AND_2_ARE_WORDS     = 0x01  ,
    TTY_GLYF_ARGS_ARE_XY_VALUES        = 0x02  ,
    TTY_GLYF_ROUND_XY_TO_GRID          = 0x04  ,
    TTY_GLYF_WE_HAVE_A_SCALE           = 0x08  ,
    TTY_GLYF_MORE_COMPONENTS           = 0x20  ,
    TTY_GLYF_WE_HAVE_AN_X_AND_Y_SCALE  = 0x40  ,
    TTY_GLYF_WE_HAVE_A_TWO_BY_TWO      = 0x80  ,
    TTY_GLYF_WE_HAVE_INSTRUCTIONS      = 0x100 ,
    TTY_GLYF_USE_MY_METRICS            = 0x200 ,
    TTY_GLYF_OVERLAP_COMPOUND          = 0x400 ,
    TTY_GLYF_SCALED_COMPONENT_OFFSET   = 0x800 ,
    TTY_GLYF_UNSCALED_COMPONENT_OFFSET = 0x1000,
};

typedef struct {
    TTY_F26Dot6_V2  p0;
    TTY_F26Dot6_V2  p1; /* Control point */
    TTY_F26Dot6_V2  p2;
} TTY_Curve;

typedef struct {
    TTY_Curve*  buff;
    TTY_U32     cap;
    TTY_U32     count;
} TTY_Curves;


typedef struct {
    TTY_F26Dot6_V2  p0;
    TTY_F26Dot6_V2  p1;
    TTY_F26Dot6     yMin;
    TTY_F26Dot6     yMax;
    TTY_F26Dot6     xMin;
    TTY_F16Dot16    invSlope; /* TODO: Should this be 26.6? */
    TTY_S8          direction;
} TTY_Edge;

typedef struct {
    TTY_Edge*  buff;
    TTY_U32    cap;
    TTY_U32    count;
    TTY_U32    off;
} TTY_Edges;


typedef struct TTY_Active_Edge {
    TTY_Edge*                edge;
    TTY_F26Dot6              xIntersection;
    struct TTY_Active_Edge*  next;
} TTY_Active_Edge;

typedef struct TTY_Active_Chunk {
    TTY_Active_Edge           edges[TTY_ACTIVE_EDGES_PER_CHUNK];
    TTY_U32                   numEdges;
    struct TTY_Active_Chunk*  next;
} TTY_Active_Chunk;

typedef struct {
    TTY_Active_Chunk*  headChunk;
    TTY_Active_Edge*   headEdge;
    TTY_Active_Edge*   reusableEdges;
} TTY_Active_Edge_List;


static TTY_Error tty_add_glyph_points_to_zone_1(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph);


static TTY_U16 tty_get_glyph_advance_width(TTY_Font* font, TTY_U32 glyphIdx) {
    TTY_U8* hmtxData    = font->data + font->hmtx.off;
    TTY_U16 numHMetrics = tty_get_u16(font->data + font->hhea.off + 34);
    if (glyphIdx < numHMetrics) {
        return tty_get_u16(hmtxData + 4 * glyphIdx);
    }
    return 0;
}

static TTY_S32 tty_get_glyph_advance_height(TTY_Font* font) {
    if (font->vmtx.exists) {
        // TODO: Get from vmtx
        TTY_ASSERT(TTY_FALSE);
    }
    
    if (font->OS2.exists) {
        TTY_U8* os2Data   = font->data + font->OS2.off;
        TTY_S16 ascender  = tty_get_s16(os2Data + 68);
        TTY_S16 descender = tty_get_s16(os2Data + 70);
        return ascender - descender;
    }
    
    // Use hhea
    return font->ascender - font->descender;
}

static TTY_S16 tty_get_glyph_left_side_bearing(TTY_Font* font, TTY_U32 glyphIdx) {
    TTY_U8* hmtxData    = font->data + font->hmtx.off;
    TTY_U16 numHMetrics = tty_get_u16(font->data + font->hhea.off + 34);
    if (glyphIdx < numHMetrics) {
        return tty_get_s16(hmtxData + 4 * glyphIdx + 2);
    }
    return tty_get_s16(hmtxData + 4 * numHMetrics + 2 * (numHMetrics - glyphIdx));
}

static TTY_S32 tty_get_glyph_top_side_bearing(TTY_Font* font, TTY_S16 yMax) {
    if (font->vmtx.exists) {
        // TODO: Get from vmtx
        TTY_ASSERT(TTY_FALSE);
    }

    if (font->OS2.exists) {
        TTY_S16 ascender = tty_get_s16(font->data + font->OS2.off + 68);
        return ascender - yMax;
    }

    // Use hhea
    return font->ascender - yMax;
}

static void tty_get_phantom_points_and_types(TTY_Font* font, TTY_Glyph* glyph, TTY_V2* phantomPoints, TTY_U8* phantomTypes) {
    TTY_S16 xMin = tty_get_s16(glyph->glyfBlock + 2);
    TTY_S16 yMax = tty_get_s16(glyph->glyfBlock + 8);
    TTY_U16 xAdv = tty_get_glyph_advance_width(font, glyph->idx);
    TTY_S32 yAdv = tty_get_glyph_advance_height(font);
    TTY_S16 lsb  = tty_get_glyph_left_side_bearing(font, glyph->idx);
    TTY_S32 tsb  = tty_get_glyph_top_side_bearing(font, yMax);

    phantomPoints[0].x = xMin - lsb;
    phantomPoints[0].y = 0;

    phantomPoints[1].x = phantomPoints[0].x + xAdv;
    phantomPoints[1].y = 0;

    phantomPoints[2].y = yMax + tsb;
    phantomPoints[2].x = 0;

    phantomPoints[3].y = phantomPoints[2].y - yAdv;
    phantomPoints[3].x = 0;
    
    for (TTY_U32 i = 0; i < TTY_NUM_PHANTOM_POINTS; i++) {
        phantomTypes[i] = TTY_OFF_CURVE_POINT;
    }
}

static void tty_scale_points(TTY_V2* points, TTY_U32 numPoints, TTY_F10Dot22 scale, TTY_F26Dot6_V2* scaledPoints) {
    for (TTY_U32 i = 0; i < numPoints; i++) {
        scaledPoints[i].x = TTY_F10DOT22_MUL(points[i].x << 6, scale);
        scaledPoints[i].y = TTY_F10DOT22_MUL(points[i].y << 6, scale);
    }
}

static void tty_round_phantom_points(TTY_V2* phantomPoints) {
    phantomPoints[0].x = tty_f26dot6_round(phantomPoints[0].x);
    phantomPoints[1].x = tty_f26dot6_round(phantomPoints[1].x);
    phantomPoints[2].y = tty_f26dot6_round(phantomPoints[2].y);
    phantomPoints[3].y = tty_f26dot6_round(phantomPoints[3].y);
}

static void tty_execute_next_glyph_program_ins(TTY_Program_Context* ctx) {
    TTY_U8 ins = tty_ins_stream_next(&ctx->stream);

    if (tty_try_execute_shared_ins(ctx, ins)) {
        return;
    }

    switch (ins) {
        case TTY_ALIGNRP:
            tty_ALIGNRP(ctx);
            return;
        case TTY_DELTAP1:
            tty_DELTAP1(ctx);
            return;
        case TTY_DELTAP2:
            tty_DELTAP2(ctx);
            return;
        case TTY_DELTAP3:
            tty_DELTAP3(ctx);
            return;
        case TTY_IP:
            tty_IP(ctx);
            return;
        case TTY_ISECT:
            tty_ISECT(ctx);
            return;
        case TTY_SHPIX:
            tty_SHPIX(ctx);
            return;
        case TTY_SMD:
            tty_SMD(ctx);
            return;
        case TTY_SRP0:
            tty_SRP0(ctx);
            return;
        case TTY_SRP1:
            tty_SRP1(ctx);
            return;
        case TTY_SRP2:
            tty_SRP2(ctx);
            return;
        case TTY_SZPS:
            tty_SZPS(ctx);
            return;
        case TTY_SZP0:
            tty_SZP0(ctx);
            return;
        case TTY_SZP1:
            tty_SZP1(ctx);
            return;
        case TTY_SZP2:
            tty_SZP2(ctx);
            return;
    }

    if (ins >= TTY_GC && ins <= TTY_GC_MAX) {
        tty_GC(ctx, ins);
    }
    else if (ins >= TTY_IUP && ins <= TTY_IUP_MAX) {
        tty_IUP(ctx, ins);
    }
    else if (ins >= TTY_MD && ins <= TTY_MD_MAX) {
        tty_MD(ctx, ins);
    }
    else if (ins >= TTY_MDAP && ins <= TTY_MDAP_MAX) {
        tty_MDAP(ctx, ins);
    }
    else if (ins >= TTY_MDRP && ins <= TTY_MDRP_MAX) {
        tty_MDRP(ctx, ins);
    }
    else if (ins >= TTY_MIAP && ins <= TTY_MIAP_MAX) {
        tty_MIAP(ctx, ins);
    }
    else if (ins >= TTY_MIRP && ins <= TTY_MIRP_MAX) {
        tty_MIRP(ctx, ins);
    }
    else if (ins >= TTY_SDPVTL && ins <= TTY_SDPVTL_MAX) {
        tty_SDPVTL(ctx, ins);
    }
    else if (ins >= TTY_SFVTL && ins <= TTY_SFVTL_MAX) {
        tty_SFVTL(ctx, ins);
    }
    else if (ins >= TTY_SHP && ins <= TTY_SHP_MAX) {
        tty_SHP(ctx, ins);
    }
    else {
        TTY_LOG_UNKNOWN_INS(ins);
        ctx->foundUnknownIns = TTY_TRUE;
    }
}

static TTY_Error tty_execute_glyph_program(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph, TTY_U8* insBuff, TTY_U32 insCount) {
    TTY_LOG_PROGRAM("Glyph Program");

    tty_reset_graphics_state(&font->hint.gs, &font->hint.zone1);
    tty_interp_stack_clear(&font->hint.stack);

    TTY_Program_Context ctx = {
        .font            = font,
        .instance        = instance,
        .glyph           = glyph,
        .iupState        = TTY_IUP_STATE_DEFAULT,
        .foundUnknownIns = TTY_FALSE,
        .stream   = {
            .execute_next_ins = tty_execute_next_glyph_program_ins,
            .buff             = insBuff,
            .cap              = insCount,
            .off              = 0,
        }
    };

    return tty_execute_program(&ctx);
}

static TTY_S16 tty_get_next_simple_glyph_coord_off(TTY_U8** data, TTY_U8 dualFlag, TTY_U8 shortFlag, TTY_U8 flags) {
    TTY_S16 coord;

    if (flags & shortFlag) {
        coord = !(flags & dualFlag) ? -(**data) : **data;
        (*data)++;
    }
    else if (flags & dualFlag) {
        coord = 0;
    }
    else {
        coord = tty_get_s16(*data);
        (*data) += 2;
    }

    return coord;
}

static TTY_Error tty_add_simple_glyph_points_to_zone1(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph) {
    font->hint.zone1.numOutlinePoints = tty_get_u16(glyph->glyfBlock + 8 + 2 * glyph->numContours) + 1;
    font->hint.zone1.numPoints        = font->hint.zone1.numOutlinePoints + TTY_NUM_PHANTOM_POINTS;
    font->hint.zone1.numEndPoints     = glyph->numContours;

    TTY_U8* flagData, *xData, *yData;
    
    {
        // Calculate pointers to the glyph's flag data, x-coordinate data, and
        // y-coordinate data
        
        TTY_U32 flagsSize = 0;
        TTY_U32 xDataSize = 0;
        
        flagData  = glyph->glyfBlock + (10 + 2 * font->hint.zone1.numEndPoints);
        flagData += 2 + tty_get_u16(flagData);
        
        for (TTY_U32 i = 0; i < font->hint.zone1.numOutlinePoints;) {
            TTY_U8 flags = flagData[flagsSize];
            TTY_U8 xSize = flags & TTY_GLYF_X_SHORT_VECTOR ? 1 : flags & TTY_GLYF_X_DUAL ? 0 : 2;
            TTY_U8 flagsReps;

            if (flags & TTY_GLYF_REPEAT_FLAG) {
                flagsReps = 1 + flagData[flagsSize + 1];
                flagsSize += 2;
            }
            else {
                flagsReps = 1;
                flagsSize++;
            }

            i += flagsReps;

            while (flagsReps > 0) {
                xDataSize += xSize;
                flagsReps--;
            }
        }
        
        xData = flagData + flagsSize;
        yData = xData    + xDataSize;
    }
    
    {
        // Add the points that make up the glyph's contours (i.e. outline points)
        
        TTY_V2 absPos = { 0 };
        
        for (TTY_U32 i = 0; i < font->hint.zone1.numOutlinePoints;) {
            TTY_U8 flags = *flagData;
            TTY_U8 flagsReps;
            
            if (flags & TTY_GLYF_REPEAT_FLAG) {
                flagsReps = 1 + flagData[1];
                flagData += 2;
            }
            else {
                flagsReps = 1;
                flagData++;
            }
            
            for (; flagsReps > 0; ++i, --flagsReps) {
                TTY_S16 xOff = tty_get_next_simple_glyph_coord_off(&xData, TTY_GLYF_X_DUAL, TTY_GLYF_X_SHORT_VECTOR, flags);
                TTY_S16 yOff = tty_get_next_simple_glyph_coord_off(&yData, TTY_GLYF_Y_DUAL, TTY_GLYF_Y_SHORT_VECTOR, flags);

                font->hint.zone1.pointTypes[i] = flags & TTY_GLYF_ON_CURVE_POINT ? TTY_ON_CURVE_POINT : TTY_OFF_CURVE_POINT;
                font->hint.zone1.org[i].x      = absPos.x + xOff;
                font->hint.zone1.org[i].y      = absPos.y + yOff;
                absPos                         = font->hint.zone1.org[i];
            }
        }
    }

    tty_get_phantom_points_and_types(font, glyph, font->hint.zone1.org + font->hint.zone1.numOutlinePoints, font->hint.zone1.pointTypes + font->hint.zone1.numOutlinePoints);
    tty_scale_points(font->hint.zone1.org, font->hint.zone1.numPoints, instance->scale, font->hint.zone1.orgScaled);
    memcpy(font->hint.zone1.cur, font->hint.zone1.orgScaled, font->hint.zone1.numPoints * sizeof(TTY_V2));
    tty_round_phantom_points(font->hint.zone1.cur + font->hint.zone1.numOutlinePoints);
    
    for (TTY_U32 i = 0; i < font->hint.zone1.numEndPoints; i++) {
        font->hint.zone1.endPointIndices[i] = tty_get_u16(glyph->glyfBlock + 10 + 2 * i);
    }

    if (instance->useHinting) {
        TTY_U32 off      = 10 + glyph->numContours * 2;
        TTY_U16 insCount = tty_get_u16(glyph->glyfBlock + off);
        TTY_U8* insBuff  = glyph->glyfBlock + off + 2;
        return tty_execute_glyph_program(font, instance, glyph, insBuff, insCount);
    }

    return TTY_ERROR_NONE;
}

static void tty_offset_zone1_buffs(TTY_Zone* zone1, TTY_S32 pointOff, TTY_S32 endPointOff) {
    zone1->org             += pointOff;
    zone1->orgScaled       += pointOff;
    zone1->cur             += pointOff;
    zone1->pointTypes      += pointOff;
    zone1->touchFlags      += pointOff;
    zone1->endPointIndices += endPointOff;
}

static TTY_Error tty_add_composite_glyph_points_to_zone1(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph) {
    TTY_U32  off             = 10;
    TTY_U32  totalPoints     = 0;
    TTY_U32  totalEndPoints  = 0;
    TTY_U32  endPointOff     = 0;
    TTY_Bool hasInstructions = TTY_FALSE;
    
    while (TTY_TRUE) {
        TTY_U16 flags         = tty_get_u16(glyph->glyfBlock + off);
        TTY_U16 childGlyphIdx = tty_get_u16(glyph->glyfBlock + off + 2);
        off += 4;
        
        TTY_Glyph childGlyph;
        tty_glyph_init(font, &childGlyph, childGlyphIdx);
        
        if (childGlyph.glyfBlock == NULL) {
            // The glyph is an empty glyph (I don't see why this would ever occur)
            continue;
        }
        
        tty_add_glyph_points_to_zone_1(font, instance, &childGlyph);
        
        // Make the end point indices of the current child glyph a continuation
        // of the end point indices of the prev child glyph
        for (TTY_U32 i = 0; i < font->hint.zone1.numEndPoints; i++) {
            font->hint.zone1.endPointIndices[i] += endPointOff;
        }

        totalPoints    += font->hint.zone1.numOutlinePoints;
        totalEndPoints += font->hint.zone1.numEndPoints;
        endPointOff     = font->hint.zone1.endPointIndices[totalEndPoints - 1];
        
        {
            TTY_S32 arg1, arg2;
            
            if (flags & TTY_GLYF_ARGS_ARE_XY_VALUES) {
                if (flags & TTY_GLYF_ARG_1_AND_2_ARE_WORDS) {
                    arg1 = tty_get_s16(glyph->glyfBlock + off);
                    arg2 = tty_get_s16(glyph->glyfBlock + off + 2);
                    off += 4;
                }
                else {
                    arg1 = (TTY_S8)glyph->glyfBlock[off];
                    arg2 = (TTY_S8)glyph->glyfBlock[off + 1];
                    off += 2;
                }
                
                if ((flags & TTY_GLYF_UNSCALED_COMPONENT_OFFSET) == 0 && 
                    (flags & TTY_GLYF_SCALED_COMPONENT_OFFSET)) 
                {
                    // TODO: This needs testing
                    TTY_ASSERT(TTY_FALSE);
                    
                    // TODO: Spec says to apply the transform to the glyph's points,
                    //       however, FreeType only applies it to the offset (when the
                    //       flags say to).
                    TTY_F2Dot14 transform[4] = {0};

                    if (flags & TTY_GLYF_WE_HAVE_A_SCALE) {
                        transform[0] = tty_get_s16(glyph->glyfBlock + off);
                        transform[3] = 0x4000;
                        off += 2;
                    }
                    else if (flags & TTY_GLYF_WE_HAVE_AN_X_AND_Y_SCALE) {
                        transform[0] = tty_get_s16(glyph->glyfBlock + off    );
                        transform[3] = tty_get_s16(glyph->glyfBlock + off + 2);
                        off += 4;
                    }
                    else if (flags & TTY_GLYF_WE_HAVE_A_TWO_BY_TWO) {
                        transform[0] = tty_get_s16(glyph->glyfBlock + off    );
                        transform[1] = tty_get_s16(glyph->glyfBlock + off + 2);
                        transform[2] = tty_get_s16(glyph->glyfBlock + off + 4);
                        transform[3] = tty_get_s16(glyph->glyfBlock + off + 6);
                        off += 8;
                    }
                    else {
                        transform[0] = 0x4000;
                        transform[3] = 0x4000;
                    }

                    arg1 = transform[0] * arg1 + transform[1] * arg2;
                    arg2 = transform[2] * arg1 + transform[3] * arg2;
                    arg1 = TTY_F10DOT22_MUL(arg1, instance->scale);
                    arg2 = TTY_F10DOT22_MUL(arg2, instance->scale);
                }
                else {
                    if (flags & TTY_GLYF_WE_HAVE_A_SCALE) {
                        off += 2;
                    }
                    else if (flags & TTY_GLYF_WE_HAVE_AN_X_AND_Y_SCALE) {
                        off += 4;
                    }
                    else if (flags & TTY_GLYF_WE_HAVE_A_TWO_BY_TWO) {
                        off += 8;
                    }
                    arg1 = TTY_F10DOT22_MUL(arg1 << 14, instance->scale);
                    arg2 = TTY_F10DOT22_MUL(arg2 << 14, instance->scale);
                }
                
                if (flags & TTY_GLYF_ROUND_XY_TO_GRID) {
                    arg1 = tty_f2dot14_round_to_grid(arg1) >> 8;
                    arg2 = tty_f2dot14_round_to_grid(arg2) >> 8;
                }
                else {
                    arg1 = TTY_ROUNDED_DIV_POW2(arg1, 0x80, 8);
                    arg2 = TTY_ROUNDED_DIV_POW2(arg2, 0x80, 8);
                }
                
                for (TTY_U32 i = 0; i < font->hint.zone1.numPoints; i++) {
                    font->hint.zone1.cur[i].x += arg1;
                    font->hint.zone1.cur[i].y += arg2;
                }
            }
            else {
                // TODO: Handle point matching (stb_truetype doesn't even
                //       bother implementing this)
                return TTY_ERROR_UNSUPPORTED_FEATURE;
            }
        }

        // Temporarily offset the zone1 buffers so the data of the next
        // child glyph can be added successively
        tty_offset_zone1_buffs(&font->hint.zone1, font->hint.zone1.numOutlinePoints, font->hint.zone1.numEndPoints);
        
        if (!(flags & TTY_GLYF_MORE_COMPONENTS)) {
            hasInstructions = (flags & TTY_GLYF_WE_HAVE_INSTRUCTIONS) != 0;
            break;
        }
    }
    
    // Note: The zone1 buffers still have the temporary offsets applied to them
    //       so they point to the phantom points
    tty_get_phantom_points_and_types(font, glyph, font->hint.zone1.org, font->hint.zone1.pointTypes);
    tty_scale_points(font->hint.zone1.org, TTY_NUM_PHANTOM_POINTS, instance->scale, font->hint.zone1.orgScaled);
    memcpy(font->hint.zone1.cur, font->hint.zone1.orgScaled, TTY_NUM_PHANTOM_POINTS * sizeof(TTY_F26Dot6_V2));
    tty_round_phantom_points(font->hint.zone1.cur);
    
    tty_offset_zone1_buffs(&font->hint.zone1, -(TTY_S32)totalPoints, -(TTY_S32)totalEndPoints);

    // Undo the temporary offset applied to the zone1 buffers
    font->hint.zone1.numPoints        = totalPoints + TTY_NUM_PHANTOM_POINTS;
    font->hint.zone1.numOutlinePoints = totalPoints;
    font->hint.zone1.numEndPoints     = totalEndPoints;

    if (instance->useHinting && hasInstructions) {
        TTY_U16 insCount = tty_get_u16(glyph->glyfBlock + off);
        TTY_U8* insBuff  = glyph->glyfBlock + off + 2;
        return tty_execute_glyph_program(font, instance, glyph, insBuff, insCount);
    }

    return TTY_ERROR_NONE;
}

static TTY_Error tty_add_glyph_points_to_zone_1(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph) {
    if (glyph->glyfBlock == NULL) {
        return TTY_ERROR_NONE;
    }
    if (glyph->numContours < 0) {
        return tty_add_composite_glyph_points_to_zone1(font, instance, glyph);
    }
    return tty_add_simple_glyph_points_to_zone1(font, instance, glyph);
}

static TTY_Error tty_convert_zone1_points_into_curves(TTY_Font* font, TTY_Curves* curves) {
    TTY_U32 startPointIdx = 0;
    
    curves->cap   = font->hint.zone1.numOutlinePoints; // The number of curves needed is <= the number of points
    curves->count = 0;
    curves->buff  = (TTY_Curve*)malloc(curves->cap * sizeof(TTY_Curve));
    if (curves->buff == NULL) {
        return TTY_ERROR_OUT_OF_MEMORY;
    }

    for (TTY_U32 i = 0; i < font->hint.zone1.numEndPoints; i++) {
        TTY_U32  endPointIdx   = font->hint.zone1.endPointIndices[i];
        TTY_Bool addFinalCurve = TTY_TRUE;

        TTY_F26Dot6_V2* startPoint = font->hint.zone1.cur + startPointIdx;
        TTY_F26Dot6_V2* nextP0     = startPoint;
        
        for (TTY_U32 j = startPointIdx + 1; j <= endPointIdx; j++) {
            TTY_Curve* curve = curves->buff + curves->count;
            curve->p0 = *nextP0;
            curve->p1 = font->hint.zone1.cur[j];

            if (font->hint.zone1.pointTypes[j] == TTY_ON_CURVE_POINT) {
                curve->p2 = curve->p1;
            }
            else if (j == endPointIdx) {
                curve->p2     = *startPoint;
                addFinalCurve = TTY_FALSE;
            }
            else if (font->hint.zone1.pointTypes[j + 1] == TTY_ON_CURVE_POINT) {
                curve->p2 = font->hint.zone1.cur[++j];
                if(j >= endPointIdx)
                    break;
            }
            else { // Implied on-curve point
                TTY_F26Dot6_V2* nextPoint = font->hint.zone1.cur + j + 1;
                TTY_FIX_V2_SUB(&curve->p1, nextPoint, &curve->p2);
                curve->p2.x = TTY_F26DOT6_MUL(curve->p2.x, 0x20);
                curve->p2.y = TTY_F26DOT6_MUL(curve->p2.y, 0x20);
                TTY_FIX_V2_ADD(nextPoint, &curve->p2, &curve->p2);
            }

            nextP0 = &curve->p2;
            curves->count++;
            if(curves->count >= curves->cap)
                return TTY_ERROR_NONE;
        }

        if (addFinalCurve) {
            TTY_Curve* finalCurve = curves->buff + curves->count;
            finalCurve->p0 = *nextP0;
            finalCurve->p1 = *startPoint;
            finalCurve->p2 = *startPoint;
            curves->count++;
            if(curves->count >= curves->cap)
                return TTY_ERROR_NONE;
        }

        startPointIdx = endPointIdx + 1;
    }

    return TTY_ERROR_NONE;
}

static TTY_F16Dot16 tty_get_inv_slope(TTY_F26Dot6_V2 p0, TTY_F26Dot6_V2 p1) {
    if (p0.x == p1.x) {
        return 0;
    }
    TTY_F16Dot16 slope = TTY_F16DOT16_DIV(p1.y - p0.y, p1.x - p0.x);
    return TTY_F16DOT16_DIV(0x10000, slope);
}

static void tty_edge_init(TTY_Edge* edge, TTY_F26Dot6_V2 p0, TTY_F26Dot6_V2 p1) {
    edge->p0        = p0;
    edge->p1        = p1;
    edge->invSlope  = tty_get_inv_slope(p0, p1);
    edge->direction = p1.y > p0.y ? 1 : -1;
    edge->xMin      = TTY_MIN(p0.x, p1.x);
    tty_max_min(p0.y, p1.y, &edge->yMax, &edge->yMin);
}

static TTY_Error tty_subdivide_curve_into_edges(TTY_Edges* edges, TTY_F26Dot6_V2 p0, TTY_F26Dot6_V2 p1, TTY_F26Dot6_V2 p2) {
    #define TTY_SUBDIVIDE(a, b)\
        { TTY_F26DOT6_MUL((a.x + b.x), 0x20),\
          TTY_F26DOT6_MUL((a.y + b.y), 0x20) }

    TTY_F26Dot6_V2 mid0 = TTY_SUBDIVIDE(p0, p1);
    TTY_F26Dot6_V2 mid1 = TTY_SUBDIVIDE(p1, p2);
    TTY_F26Dot6_V2 mid2 = TTY_SUBDIVIDE(mid0, mid1);

    {
        TTY_F26Dot6_V2 d = TTY_SUBDIVIDE(p0, p2);
        d.x -= mid2.x;
        d.y -= mid2.y;

        TTY_F26Dot6 sqrdError = TTY_F26DOT6_MUL(d.x, d.x) + TTY_F26DOT6_MUL(d.y, d.y);

        if (sqrdError <= TTY_SUBDIVIDE_SQRD_ERROR) {
            if (edges->count == edges->cap) {
                TTY_U32   newCap  = edges->cap * 2;
                TTY_Edge* newBuff = (TTY_Edge*)realloc(edges->buff, newCap * sizeof(TTY_Edge));
                if (newBuff == NULL) {
                    return TTY_ERROR_OUT_OF_MEMORY;
                }

                edges->cap  = newCap;
                edges->buff = newBuff;
            }

            tty_edge_init(edges->buff + edges->count, p0, p2);
            edges->count++;
            return TTY_ERROR_NONE;
        }
    }

    {
        TTY_Error error;

        if ((error = tty_subdivide_curve_into_edges(edges, p0, mid0, mid2)) ||
            (error = tty_subdivide_curve_into_edges(edges, mid2, mid1, p2)))
        {
            return error;
        }
    }

    return TTY_ERROR_NONE;
    #undef TTY_SUBDIVIDE
}

static TTY_Error tty_subdivide_curves_into_edges(TTY_Curves* curves, TTY_Edges* edges, TTY_U32 startingEdgeCap) {
    edges->cap   = startingEdgeCap;
    edges->count = 0;
    edges->buff  = (TTY_Edge*)malloc(edges->cap * sizeof(TTY_Edge));
    if (edges->buff == NULL) {
        return TTY_ERROR_OUT_OF_MEMORY;
    }

    for (TTY_U32 i = 0; i < curves->count; i++) {
        TTY_Curve* curve = curves->buff + i;

        if (curve->p1.x == curve->p2.x && curve->p1.y == curve->p2.y) {
            // The curve is a already straight line, no need to flatten it

            if (curve->p0.y != curve->p2.y) { // Horizontal lines can be ignored 
                tty_edge_init(edges->buff + edges->count, curve->p0, curve->p2);
                edges->count++;
                if(edges->count >= edges->cap)
                    return TTY_ERROR_NONE;
            }
        }
        else {
            TTY_Error error;
            if ((error = tty_subdivide_curve_into_edges(edges, curve->p0, curve->p1, curve->p2))) {
                return error;
            }
        }
    }

    return TTY_ERROR_NONE;
}

static int tty_compare_edges(const void* edge0, const void* edge1) {
    if (((TTY_Edge*)edge0)->yMax > ((TTY_Edge*)edge1)->yMax) {
        return -1;
    }
    return 1;
}

static TTY_S32 tty_get_unhinted_glyph_x_advance(TTY_Font* font, TTY_U32 glyphIdx, TTY_F10Dot22 scale) {
    TTY_S32 adv;
    adv = tty_get_glyph_advance_width(font, glyphIdx);
    adv = TTY_F10DOT22_MUL(adv << 6, scale);
    adv = tty_f26dot6_round(adv) >> 6;
    return adv;
}

static TTY_S32 tty_get_unhinted_glyph_y_advance(TTY_Font* font, TTY_F10Dot22 scale) {
    TTY_S32 adv;
    adv = tty_get_glyph_advance_height(font);
    adv = TTY_F10DOT22_MUL(adv << 6, scale);
    adv = tty_f26dot6_round(adv) >> 6;
    return adv;
}

static void tty_get_min_and_max_zone1_points(TTY_Zone* zone1, TTY_F26Dot6_V2* min, TTY_F26Dot6_V2* max) {
    *min = *zone1->cur;
    *max = *zone1->cur;

    for (TTY_U32 i = 1; i < zone1->numOutlinePoints; i++) {
        if (zone1->cur[i].x < min->x) {
            min->x = zone1->cur[i].x;
        }
        else if (zone1->cur[i].x > max->x) {
            max->x = zone1->cur[i].x;
        }

        if (zone1->cur[i].y < min->y) {
            min->y = zone1->cur[i].y;
        }
        else if (zone1->cur[i].y > max->y) {
            max->y = zone1->cur[i].y;
        }
    }
}

static void tty_set_hinted_glyph_metrics(TTY_Font* font, TTY_Glyph* glyph, TTY_F26Dot6_V2 min, TTY_F26Dot6_V2 max, TTY_F10Dot22 scale) {
    TTY_F26Dot6_V2* phantomPoints = font->hint.zone1.cur + font->hint.zone1.numOutlinePoints;
    
    if (font->vmtx.exists) {
        glyph->advance.y = 
            phantomPoints[2].y > phantomPoints[3].y ? 
            phantomPoints[2].y - phantomPoints[3].y : 
            0;
    }
    else {
        TTY_S64 advHeight = tty_get_glyph_advance_height(font) << 6;
        glyph->advance.y = TTY_F10DOT22_MUL(advHeight, scale);
    }

    glyph->advance.x = tty_f26dot6_round(phantomPoints[1].x - phantomPoints[0].x) >> 6;
    glyph->advance.y = tty_f26dot6_round(glyph->advance.y) >> 6;

    glyph->size.x = (tty_f26dot6_ceil(max.x) - tty_f26dot6_floor(min.x)) >> 6;
    glyph->size.y = (tty_f26dot6_ceil(max.y) - tty_f26dot6_floor(min.y)) >> 6;

    glyph->offset.x = tty_f26dot6_floor(min.x) >> 6;
    glyph->offset.y = tty_f26dot6_ceil(max.y)  >> 6;
}

static void tty_set_unhinted_glyph_metrics(TTY_Font* font, TTY_Glyph* glyph, TTY_F26Dot6_V2 min, TTY_F26Dot6_V2 max, TTY_F10Dot22 scale) {
    glyph->size.x = (tty_f26dot6_ceil(max.x) - tty_f26dot6_floor(min.x)) >> 6;
    glyph->size.y = (tty_f26dot6_ceil(max.y) - tty_f26dot6_floor(min.y)) >> 6;

    glyph->advance.x = tty_get_unhinted_glyph_x_advance(font, glyph->idx, scale);
    glyph->advance.y = tty_get_unhinted_glyph_y_advance(font, scale);
    
    glyph->offset.x = tty_f26dot6_floor(min.x) >> 6;
    glyph->offset.y = tty_f26dot6_ceil(max.y)  >> 6;
}

static void tty_set_glyph_metrics(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph, TTY_V2 min, TTY_V2 max) {
    if (instance->useHinting) {
        tty_set_hinted_glyph_metrics(font, glyph, min, max, instance->scale);
    }
    else {
        tty_set_unhinted_glyph_metrics(font, glyph, min, max, instance->scale);
    }
}

static TTY_Error tty_active_edge_list_init(TTY_Active_Edge_List* list) {
    list->headChunk = (TTY_Active_Chunk*)calloc(1, sizeof(TTY_Active_Chunk));
    if (list->headChunk != NULL) {
        list->headEdge      = NULL;
        list->reusableEdges = NULL;
        return TTY_ERROR_NONE;
    }
    return TTY_ERROR_OUT_OF_MEMORY;
}

static void tty_active_edge_list_free(TTY_Active_Edge_List* list) {
    while (list->headChunk != NULL) {
        TTY_Active_Chunk* next = list->headChunk->next;
        free(list->headChunk);
        list->headChunk = next;
    }
}

static TTY_Error tty_get_available_active_edge(TTY_Active_Edge_List* list, TTY_Active_Edge** edge) {
    if (list->reusableEdges != NULL) {
        // Reuse the memory from a previously removed edge
        *edge               = list->reusableEdges;
        list->reusableEdges = (*edge)->next;
        (*edge)->next          = NULL;
        return TTY_ERROR_NONE;
    }
    
    if (list->headChunk->numEdges == TTY_ACTIVE_EDGES_PER_CHUNK) {
        // The current chunk is full, so allocate a new one
        TTY_Active_Chunk* chunk = (TTY_Active_Chunk*)calloc(1, sizeof(TTY_Active_Chunk));
        if (chunk == NULL) {
            return TTY_ERROR_OUT_OF_MEMORY;
        }
        chunk->next     = list->headChunk;
        list->headChunk = chunk;
    }

    *edge = list->headChunk->edges + list->headChunk->numEdges;
    list->headChunk->numEdges++;
    return TTY_ERROR_NONE;
}

static TTY_Error tty_insert_active_edge_first(TTY_Active_Edge_List* list, TTY_Active_Edge** edge) {
    {
        TTY_Error error;
        if ((error = tty_get_available_active_edge(list, edge))) {
            return error;
        }
    }

    (*edge)->next  = list->headEdge;
    list->headEdge = *edge;
    return TTY_ERROR_NONE;
}

static TTY_Error tty_insert_active_edge_after(TTY_Active_Edge_List* list, TTY_Active_Edge* after, TTY_Active_Edge** edge) {
    {
        TTY_Error error;
        if ((error = tty_get_available_active_edge(list, edge))) {
            return error;
        }
    }

    (*edge)->next = after->next;
    after->next   = *edge;
    return TTY_ERROR_NONE;
}

static void tty_remove_active_edge(TTY_Active_Edge_List* list, TTY_Active_Edge* prev, TTY_Active_Edge* remove) {
    if (prev == NULL) {
        list->headEdge = list->headEdge->next;
    }
    else {
        prev->next = remove->next;
    }
    
    // Add the edge to the list of reusable edges so its memory can be reused
    remove->edge          = NULL;
    remove->xIntersection = 0;
    remove->next          = list->reusableEdges;
    list->reusableEdges   = remove;
}

static void tty_swap_active_edge_with_next(TTY_Active_Edge_List* list, TTY_Active_Edge* prev, TTY_Active_Edge* edge) {
    TTY_ASSERT(edge->next != NULL);
    
    if (prev != NULL) {
        prev->next = edge->next;
    }
    else {
        list->headEdge = edge->next;
    }

    TTY_Active_Edge* temp = edge->next->next;
    edge->next->next = edge;
    edge->next       = temp;
}

static TTY_F26Dot6 tty_get_edge_scanline_x_intersection(TTY_Edge* edge, TTY_F26Dot6 scanline) {
    return TTY_F16DOT16_MUL(scanline - edge->p0.y, edge->invSlope) + edge->p0.x;
}

static void tty_update_or_remove_active_edges(TTY_Active_Edge_List* list, TTY_F26Dot6 scanline, TTY_F26Dot6 xIntersectionOff) {
    // If an edge is no longer active, remove it from the list, else update its
    // x-intersection with the current scanline

    TTY_Active_Edge* activeEdge     = list->headEdge;
    TTY_Active_Edge* prevActiveEdge = NULL;

    while (activeEdge != NULL) {
        TTY_Active_Edge* next = activeEdge->next;
        
        if (activeEdge->edge->yMin >= scanline) {
            tty_remove_active_edge(list, prevActiveEdge, activeEdge);
        }
        else {
            activeEdge->xIntersection = xIntersectionOff + tty_get_edge_scanline_x_intersection(activeEdge->edge, scanline);
            prevActiveEdge            = activeEdge;
        }
        
        activeEdge = next;
    }
}

static void tty_sort_active_edges(TTY_Active_Edge_List* list) {
    // Active edges are sorted from smallest to largest x-intersection. If
    // x-intersections are equal, sort by smallest x-minimum.

    TTY_Active_Edge* activeEdge     = list->headEdge;
    TTY_Active_Edge* prevActiveEdge = NULL; // prevActiveEdge->next also needs to be updated during the swapping process
    
    while (activeEdge != NULL) {
        while (activeEdge->next != NULL                                       &&
               (activeEdge->xIntersection > activeEdge->next->xIntersection   ||
                (activeEdge->xIntersection == activeEdge->next->xIntersection &&
                 activeEdge->edge->xMin > activeEdge->next->edge->xMin)))
        {
            tty_swap_active_edge_with_next(list, prevActiveEdge, activeEdge);
        }

        prevActiveEdge = activeEdge;
        activeEdge     = activeEdge->next;
    }
}

static TTY_Error tty_insert_new_active_edges(TTY_Active_Edge_List* list, TTY_Edges* edges, TTY_F26Dot6 scanline, TTY_F26Dot6 xIntersectionOff) {
    // Find any edges that intersect the current scanline and insert them into
    // the active edge list

    while (edges->off < edges->count && edges->buff[edges->off].yMax >= scanline) {
        TTY_Edge*        edge           = edges->buff + edges->off;
        TTY_F26Dot6      xIntersection  = xIntersectionOff + tty_get_edge_scanline_x_intersection(edge, scanline);
        TTY_Active_Edge* activeEdge     = list->headEdge;
        TTY_Active_Edge* prevActiveEdge = NULL;
        TTY_Active_Edge* newActiveEdge  = NULL;

        if (edge->yMin >= scanline) {
            edges->off++;
            continue;
        }
        
        while (activeEdge != NULL) {
            if (xIntersection < activeEdge->xIntersection) {
                break;
            }
            else if (xIntersection == activeEdge->xIntersection) {
                if (edge->xMin < activeEdge->edge->xMin) {
                    break;
                }
            }
            prevActiveEdge = activeEdge;
            activeEdge     = activeEdge->next;
        }

        TTY_Error error =
            prevActiveEdge == NULL ?
            tty_insert_active_edge_first(list, &newActiveEdge) :
            tty_insert_active_edge_after(list, prevActiveEdge, &newActiveEdge);

        if (error) {
            return error;
        }

        newActiveEdge->edge          = edge;
        newActiveEdge->xIntersection = xIntersection;
        edges->off++;
    }

    return TTY_ERROR_NONE;
}

static void tty_rasterize_using_active_edges(TTY_Active_Edge_List* activeEdges, TTY_F26Dot6* pixelBuff, TTY_U32 pixelBuffLen) {
    #define TTY_INCREMENT_PIXEL_VALUE(idx, increment)\
        pixelBuff[idx]


    if (activeEdges->headEdge == NULL || activeEdges->headEdge->next == NULL) {
        // There should always be at least two edges (probably)
        return;
    }

    TTY_Active_Edge* activeEdge    = activeEdges->headEdge;
    TTY_F26Dot6      weightedAlpha = TTY_F26DOT6_MUL(0x3FC0, TTY_PIXELS_PER_SCANLINE);
    TTY_S32          windingNumber = 0;
    
    while (TTY_TRUE) {
        windingNumber += activeEdge->edge->direction;

        if (windingNumber == 0) {
            goto set_next_active_edge;
        }
        else if (activeEdge->xIntersection == activeEdge->next->xIntersection) {
            goto set_next_active_edge;
        }

        {
            TTY_F26Dot6 x = 0x40 + tty_f26dot6_floor(activeEdge->xIntersection);
            TTY_F26Dot6 coverage;
            TTY_U32     idx;

            if (x >= activeEdge->next->xIntersection) {
                // The next x-intersection is in the same pixel as the current
                // x-intersection
                coverage = activeEdge->next->xIntersection - activeEdge->xIntersection;
            }
            else {
                // Calculate the coverage of the pixel containing the current
                // x-intersection
                idx      = (x >> 6) - 1;
                coverage = x - activeEdge->xIntersection;
                
                TTY_ASSERT(idx < pixelBuffLen);
                pixelBuff[idx] += TTY_F26DOT6_MUL(weightedAlpha, coverage);

                x   += 0x40;
                idx += 1;

                // All pixels after the current x-intersection and before the
                // next x-intersection are fully covered
                while (x < activeEdge->next->xIntersection) {
                    TTY_ASSERT(idx < pixelBuffLen);
                    pixelBuff[idx] += weightedAlpha;
                    x              += 0x40;
                    idx            += 1;
                }

                // Calculate the coverage of the pixel containing the next
                // x-intersection
                coverage = activeEdge->next->xIntersection - (x - 0x40);
            }

            idx = (tty_f26dot6_ceil(activeEdge->next->xIntersection) >> 6) - 1;
            
            TTY_ASSERT(idx < pixelBuffLen);
            pixelBuff[idx] += TTY_F26DOT6_MUL(weightedAlpha, coverage);
        }

    set_next_active_edge:
        activeEdge = activeEdge->next;
        if (activeEdge->next == NULL) {
            break;
        }
    }
}

TTY_Error tty_render_glyph_cache(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph) {
    if (glyph->glyfBlock == NULL) {
        // The glyph is an empty glyph (i.e. space)
        glyph->advance.x = tty_get_unhinted_glyph_x_advance(font, glyph->idx, instance->scale);
        glyph->advance.y = tty_get_unhinted_glyph_y_advance(font, instance->scale);
        return TTY_ERROR_NONE;
    }

    // The glyph's points are converted into curves and the curves are 
    // approximated by edges.
    TTY_Edges edges = {0};

    // An active edge is an edge that is intersected by the current scanline.
    TTY_Active_Edge_List activeEdges = {0};

    // The minimum and maximum points of the glyph.
    TTY_F26Dot6_V2 min = {0};
    TTY_F26Dot6_V2 max = {0};

    // The y-coordinates corresponding to the scanline.
    TTY_F26Dot6 scanlineStart = 0;
    TTY_F26Dot6 scanlineEnd   = 0;
    TTY_F26Dot6 scanline      = 0;

    // This will be applied to x-intersections to prevent negative values which
    // will allow for easier calculations during rasterization.
    TTY_F26Dot6 xIntersectionOff = 0;

    // An intermediate buffer is rendered to before the image. This is because
    // using the image's pixels directly would result in a loss of precision
    // since each pixel is only one byte.
    TTY_F26Dot6* pixelBuff    = NULL;
    TTY_U32      pixelBuffLen = 0;

    // If the image's pixels were allocated by this function, this function
    // needs to free them if an error occurs.
    TTY_Bool imagePixelsWereAllocated = TTY_FALSE;


    {
        // Get the glyph's points, convert them into curves, and then
        // approximate the curves using edges

        TTY_Curves curves = {0};
        memset(&curves, 0, sizeof(TTY_Curves));

        TTY_Error error = tty_add_glyph_points_to_zone_1(font, instance, glyph);
        if (error) {
            return error;
        }

        error = tty_convert_zone1_points_into_curves(font, &curves);
        if (error) {
            if(curves.buff != NULL)
                free(curves.buff);
            return error;
        }

        error = tty_subdivide_curves_into_edges(&curves, &edges, font->startingEdgeCap);
        if(curves.buff != NULL)
            free(curves.buff);

        if (error) {
            if(edges.buff != NULL)
                free(edges.buff);
            return error;
        }

        if (instance->useHinting) {
            // Touch flags need to be cleared here (Everything else in zone1 is 
            // cleared elsewhere)
            memset(font->hint.zone1.touchFlags, TTY_UNTOUCHED, sizeof(TTY_U8) * font->hint.zone1.numOutlinePoints);
        }
    }

    // Edges are sorted from largest to smallest y-coordinate
    qsort(edges.buff, edges.count, sizeof(TTY_Edge), tty_compare_edges);

    if (edges.count > font->startingEdgeCap) {
        // Increase the starting edge capacity to potentially prevent a realloc
        // of the edge buffer when rasterizing future glyphs
        font->startingEdgeCap = edges.count;
    }


    tty_get_min_and_max_zone1_points(&font->hint.zone1, &min, &max);
    /*if(max.x < 0 || max.y < 0) { // TODO: Are negative maximum coordinates allowed?
        free(edges.buff);
        return TTY_ERROR_GLYPH_DOES_NOT_FIT_IN_IMAGE;
    }
    */
    
    tty_set_glyph_metrics(font, instance, glyph, min, max);
    if(glyph->size.x <= 0 || glyph->size.y <= 0) {
        free(edges.buff);
        return TTY_ERROR_GLYPH_DOES_NOT_FIT_IN_IMAGE;
    }

    // The length of the pixel buffer needs to be equivalent to ceil(max.x).
    // Note: When min.x is < 0, all x-intersections are offset by ceil(-min.x).
    //       This means max.x needs to also be offset by this much.
    xIntersectionOff = min.x < 0 ? tty_f26dot6_ceil(-min.x) : 0;
    pixelBuffLen     = (tty_f26dot6_ceil(max.x) >> 6) + (xIntersectionOff >> 6);
    pixelBuff        = (TTY_F26Dot6*)calloc(pixelBuffLen, sizeof(TTY_F26Dot6));
    if (pixelBuff == NULL) {
        free(edges.buff);
        return TTY_ERROR_OUT_OF_MEMORY;
    }


    {
        TTY_Error error;
        if ((error = tty_active_edge_list_init(&activeEdges))) {
            free(edges.buff);
            free(pixelBuff);
            return error;
        }
    }


    scanlineStart = tty_f26dot6_ceil(max.y);
    scanlineEnd   = tty_f26dot6_floor(min.y);
    scanline      = scanlineStart;

    TTY_S32 y = instance->maxGlyphSize.y - glyph->offset.y - instance->maxGlyphSize.y /4;
    TTY_S32 x = 0;
    TTY_U32 cache_size = instance->maxGlyphSize.x*instance->maxGlyphSize.y;
    glyph->cache = (TTY_U8*)calloc(1, cache_size);
    while (scanline >= scanlineEnd) {
        tty_update_or_remove_active_edges(&activeEdges, scanline, xIntersectionOff);
        tty_sort_active_edges(&activeEdges);

        {
            TTY_Error error;
            if ((error = tty_insert_new_active_edges(&activeEdges, &edges, scanline, xIntersectionOff))) {
                free(edges.buff);
                free(pixelBuff);
                free(glyph->cache);
                glyph->cache = NULL;
                tty_active_edge_list_free(&activeEdges);
                return error;
            }
        }

        tty_rasterize_using_active_edges(&activeEdges, pixelBuff, pixelBuffLen);
        scanline -= TTY_PIXELS_PER_SCANLINE;

        if ((scanline & 0x3F) == 0) {
            // A new row of pixels has been reached, transfer the values
            // accumulated in the pixel buffer to the image

            TTY_U32 pixelBuffOff = glyph->offset.x <= 0 ? 0 : glyph->offset.x;
            
            for (TTY_S32 i = 0; i < glyph->size.x; i++) {
                TTY_U32 pixelBuffIdx = i + pixelBuffOff;
                TTY_ASSERT(pixelBuffIdx < pixelBuffLen);
                TTY_F26Dot6 pixelValue = pixelBuff[pixelBuffIdx] >> 6;
                if(pixelValue != 0) {
                    int at = y*instance->maxGlyphSize.x+x+pixelBuffIdx;
                    if(at >=0 && at < cache_size)
                        glyph->cache[at] = pixelValue;
                }
            }
            
            y++;
            memset(pixelBuff, 0, pixelBuffLen * sizeof(TTY_F26Dot6));
        }
    }

    free(edges.buff);
    free(pixelBuff);
    tty_active_edge_list_free(&activeEdges);
    return TTY_ERROR_NONE;
}

#ifdef __cplusplus
}
#endif