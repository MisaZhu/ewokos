SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -Isrc -I $(TARGET_DIR)/include

OPENLIBM_ARCH_DIR := $(ARCH)
ifeq ($(ARCH),x86)
OPENLIBM_ARCH_DIR := amd64
endif
ifeq ($(ARCH),riscv)
OPENLIBM_ARCH_DIR := riscv64
endif

LIB_LIBOPENLIBM_OBJS = $(ARCH)/$(OPENLIBM_ARCH_DIR)/fenv.o \
	$(ARCH)/src/common.o \
	$(ARCH)/src/e_acos.o \
	$(ARCH)/src/e_acosf.o \
	$(ARCH)/src/e_acosh.o \
	$(ARCH)/src/e_acoshf.o \
	$(ARCH)/src/e_asin.o \
	$(ARCH)/src/e_asinf.o \
	$(ARCH)/src/e_atan2.o \
	$(ARCH)/src/e_atan2f.o \
	$(ARCH)/src/e_atanh.o \
	$(ARCH)/src/e_atanhf.o \
	$(ARCH)/src/e_cosh.o \
	$(ARCH)/src/e_coshf.o \
	$(ARCH)/src/e_exp.o \
	$(ARCH)/src/e_expf.o \
	$(ARCH)/src/e_fmod.o \
	$(ARCH)/src/e_fmodf.o \
	$(ARCH)/src/e_hypot.o \
	$(ARCH)/src/e_hypotf.o \
	$(ARCH)/src/e_j0.o \
	$(ARCH)/src/e_j0f.o \
	$(ARCH)/src/e_j1.o \
	$(ARCH)/src/e_j1f.o \
	$(ARCH)/src/e_jn.o \
	$(ARCH)/src/e_jnf.o \
	$(ARCH)/src/e_lgamma.o \
	$(ARCH)/src/e_lgamma_r.o \
	$(ARCH)/src/e_lgammaf.o \
	$(ARCH)/src/e_lgammaf_r.o \
	$(ARCH)/src/e_log.o \
	$(ARCH)/src/e_log10.o \
	$(ARCH)/src/e_log10f.o \
	$(ARCH)/src/e_log2.o \
	$(ARCH)/src/e_log2f.o \
	$(ARCH)/src/e_logf.o \
	$(ARCH)/src/e_pow.o \
	$(ARCH)/src/e_powf.o \
	$(ARCH)/src/e_rem_pio2.o \
	$(ARCH)/src/e_rem_pio2f.o \
	$(ARCH)/src/e_remainder.o \
	$(ARCH)/src/e_remainderf.o \
	$(ARCH)/src/e_sinh.o \
	$(ARCH)/src/e_sinhf.o \
	$(ARCH)/src/e_sqrt.o \
	$(ARCH)/src/e_sqrtf.o \
	$(ARCH)/src/k_cos.o \
	$(ARCH)/src/k_cosf.o \
	$(ARCH)/src/k_exp.o \
	$(ARCH)/src/k_expf.o \
	$(ARCH)/src/k_rem_pio2.o \
	$(ARCH)/src/k_sin.o \
	$(ARCH)/src/k_sinf.o \
	$(ARCH)/src/k_tan.o \
	$(ARCH)/src/k_tanf.o \
	$(ARCH)/src/polevll.o \
	$(ARCH)/src/s_asinh.o \
	$(ARCH)/src/s_asinhf.o \
	$(ARCH)/src/s_atan.o \
	$(ARCH)/src/s_atanf.o \
	$(ARCH)/src/s_cabs.o \
	$(ARCH)/src/s_cabsf.o \
	$(ARCH)/src/s_cacos.o \
	$(ARCH)/src/s_cacosf.o \
	$(ARCH)/src/s_cacosh.o \
	$(ARCH)/src/s_cacoshf.o \
	$(ARCH)/src/s_carg.o \
	$(ARCH)/src/s_cargf.o \
	$(ARCH)/src/s_casin.o \
	$(ARCH)/src/s_casinf.o \
	$(ARCH)/src/s_casinh.o \
	$(ARCH)/src/s_casinhf.o \
	$(ARCH)/src/s_catan.o \
	$(ARCH)/src/s_catanf.o \
	$(ARCH)/src/s_catanh.o \
	$(ARCH)/src/s_catanhf.o \
	$(ARCH)/src/s_cbrt.o \
	$(ARCH)/src/s_cbrtf.o \
	$(ARCH)/src/s_ccos.o \
	$(ARCH)/src/s_ccosf.o \
	$(ARCH)/src/s_ccosh.o \
	$(ARCH)/src/s_ccoshf.o \
	$(ARCH)/src/s_ceilf.o \
	$(ARCH)/ldouble/s_ceil.o \
	$(ARCH)/src/s_cexp.o \
	$(ARCH)/src/s_cexpf.o \
	$(ARCH)/src/s_cimag.o \
	$(ARCH)/src/s_cimagf.o \
	$(ARCH)/src/s_clog.o \
	$(ARCH)/src/s_clogf.o \
	$(ARCH)/src/s_conj.o \
	$(ARCH)/src/s_conjf.o \
	$(ARCH)/src/s_copysign.o \
	$(ARCH)/src/s_copysignf.o \
	$(ARCH)/src/s_cos.o \
	$(ARCH)/src/s_cosf.o \
	$(ARCH)/src/s_cpow.o \
	$(ARCH)/src/s_cpowf.o \
	$(ARCH)/src/s_cproj.o \
	$(ARCH)/src/s_cprojf.o \
	$(ARCH)/ldouble/s_creal.o \
	$(ARCH)/src/s_crealf.o \
	$(ARCH)/src/s_csin.o \
	$(ARCH)/src/s_csinf.o \
	$(ARCH)/src/s_csinh.o \
	$(ARCH)/src/s_csinhf.o \
	$(ARCH)/src/s_csqrt.o \
	$(ARCH)/src/s_csqrtf.o \
	$(ARCH)/src/s_ctan.o \
	$(ARCH)/src/s_ctanf.o \
	$(ARCH)/src/s_ctanh.o \
	$(ARCH)/src/s_ctanhf.o \
	$(ARCH)/src/s_erf.o \
	$(ARCH)/src/s_erff.o \
	$(ARCH)/src/s_exp2.o \
	$(ARCH)/src/s_exp2f.o \
	$(ARCH)/src/s_expm1.o \
	$(ARCH)/src/s_expm1f.o \
	$(ARCH)/src/s_fabs.o \
	$(ARCH)/src/s_fabsf.o \
	$(ARCH)/src/s_fdim.o \
	$(ARCH)/src/s_floor.o \
	$(ARCH)/src/s_floorf.o \
	$(ARCH)/src/s_fma.o \
	$(ARCH)/src/s_fmaf.o \
	$(ARCH)/src/s_fmax.o \
	$(ARCH)/src/s_fmaxf.o \
	$(ARCH)/src/s_fmin.o \
	$(ARCH)/src/s_fminf.o \
	$(ARCH)/src/s_fpclassify.o \
	$(ARCH)/src/s_frexp.o \
	$(ARCH)/src/s_frexpf.o \
	$(ARCH)/src/s_ilogb.o \
	$(ARCH)/src/s_ilogbf.o \
	$(ARCH)/src/s_isfinite.o \
	$(ARCH)/src/s_isinf.o \
	$(ARCH)/src/s_isnan.o \
	$(ARCH)/src/s_llrint.o \
	$(ARCH)/src/s_llrintf.o \
	$(ARCH)/src/s_llround.o \
	$(ARCH)/src/s_llroundf.o \
	$(ARCH)/src/s_log1p.o \
	$(ARCH)/src/s_log1pf.o \
	$(ARCH)/src/s_logb.o \
	$(ARCH)/src/s_logbf.o \
	$(ARCH)/src/s_lrint.o \
	$(ARCH)/src/s_lrintf.o \
	$(ARCH)/src/s_lround.o \
	$(ARCH)/src/s_lroundf.o \
	$(ARCH)/src/s_modf.o \
	$(ARCH)/src/s_modff.o \
	$(ARCH)/src/s_nan.o \
	$(ARCH)/src/s_nearbyint.o \
	$(ARCH)/src/s_nextafter.o \
	$(ARCH)/src/s_nextafterf.o \
	$(ARCH)/src/s_nexttowardf.o \
	$(ARCH)/src/s_remquo.o \
	$(ARCH)/src/s_remquof.o \
	$(ARCH)/src/s_rint.o \
	$(ARCH)/src/s_rintf.o \
	$(ARCH)/src/s_round.o \
	$(ARCH)/src/s_roundf.o \
	$(ARCH)/src/s_scalbln.o \
	$(ARCH)/src/s_scalbn.o \
	$(ARCH)/src/s_scalbnf.o \
	$(ARCH)/src/s_signbit.o \
	$(ARCH)/src/s_signgam.o \
	$(ARCH)/src/s_sin.o \
	$(ARCH)/src/s_sincos.o \
	$(ARCH)/src/s_sincosf.o \
	$(ARCH)/src/s_sinf.o \
	$(ARCH)/src/s_tan.o \
	$(ARCH)/src/s_tanf.o \
	$(ARCH)/src/s_tanh.o \
	$(ARCH)/src/s_tanhf.o \
	$(ARCH)/src/s_tgammaf.o \
	$(ARCH)/src/s_trunc.o \
	$(ARCH)/src/s_truncf.o \
	$(ARCH)/src/w_cabs.o \
	$(ARCH)/src/w_cabsf.o \
	$(ARCH)/src/w_cabsl.o

LIBOPENLIBM = $(TARGET_DIR)/lib/libopenlibm.a
LIBM = $(TARGET_DIR)/lib/libm.a

$(LIBOPENLIBM): $(LIB_LIBOPENLIBM_OBJS)
	$(AR) rcs $(LIBOPENLIBM) $(LIB_LIBOPENLIBM_OBJS)
	$(AR) rcs $(LIBM) $(LIB_LIBOPENLIBM_OBJS)
	cp -r include/* $(TARGET_DIR)/include
	cp -r src/*.h $(TARGET_DIR)/include


clean:
	rm -f $(LIB_LIBOPENLIBM_OBJS)
	rm -f $(LIBOPENLIBM) $(LIBM)
