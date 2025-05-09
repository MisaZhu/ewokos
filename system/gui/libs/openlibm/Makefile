SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -Isrc -I $(TARGET_DIR)/include

LIB_LIBOPENLIBM_OBJS = arm/fenv.o \
	src/common.o \
	src/e_acos.o \
	src/e_acosf.o \
	src/e_acosh.o \
	src/e_acoshf.o \
	src/e_asin.o \
	src/e_asinf.o \
	src/e_atan2.o \
	src/e_atan2f.o \
	src/e_atanh.o \
	src/e_atanhf.o \
	src/e_cosh.o \
	src/e_coshf.o \
	src/e_exp.o \
	src/e_expf.o \
	src/e_fmod.o \
	src/e_fmodf.o \
	src/e_hypot.o \
	src/e_hypotf.o \
	src/e_j0.o \
	src/e_j0f.o \
	src/e_j1.o \
	src/e_j1f.o \
	src/e_jn.o \
	src/e_jnf.o \
	src/e_lgamma.o \
	src/e_lgamma_r.o \
	src/e_lgammaf.o \
	src/e_lgammaf_r.o \
	src/e_log.o \
	src/e_log10.o \
	src/e_log10f.o \
	src/e_log2.o \
	src/e_log2f.o \
	src/e_logf.o \
	src/e_pow.o \
	src/e_powf.o \
	src/e_rem_pio2.o \
	src/e_rem_pio2f.o \
	src/e_remainder.o \
	src/e_remainderf.o \
	src/e_sinh.o \
	src/e_sinhf.o \
	src/e_sqrt.o \
	src/e_sqrtf.o \
	src/k_cos.o \
	src/k_cosf.o \
	src/k_exp.o \
	src/k_expf.o \
	src/k_rem_pio2.o \
	src/k_sin.o \
	src/k_sinf.o \
	src/k_tan.o \
	src/k_tanf.o \
	src/polevll.o \
	src/s_asinh.o \
	src/s_asinhf.o \
	src/s_atan.o \
	src/s_atanf.o \
	src/s_cabs.o \
	src/s_cabsf.o \
	src/s_cacos.o \
	src/s_cacosf.o \
	src/s_cacosh.o \
	src/s_cacoshf.o \
	src/s_carg.o \
	src/s_cargf.o \
	src/s_casin.o \
	src/s_casinf.o \
	src/s_casinh.o \
	src/s_casinhf.o \
	src/s_catan.o \
	src/s_catanf.o \
	src/s_catanh.o \
	src/s_catanhf.o \
	src/s_cbrt.o \
	src/s_cbrtf.o \
	src/s_ccos.o \
	src/s_ccosf.o \
	src/s_ccosh.o \
	src/s_ccoshf.o \
	src/s_ceilf.o \
	src/s_cexp.o \
	src/s_cexpf.o \
	src/s_cimag.o \
	src/s_cimagf.o \
	src/s_clog.o \
	src/s_clogf.o \
	src/s_conj.o \
	src/s_conjf.o \
	src/s_copysign.o \
	src/s_copysignf.o \
	src/s_cos.o \
	src/s_cosf.o \
	src/s_cpow.o \
	src/s_cpowf.o \
	src/s_cproj.o \
	src/s_cprojf.o \
	src/s_crealf.o \
	src/s_csin.o \
	src/s_csinf.o \
	src/s_csinh.o \
	src/s_csinhf.o \
	src/s_csqrt.o \
	src/s_csqrtf.o \
	src/s_ctan.o \
	src/s_ctanf.o \
	src/s_ctanh.o \
	src/s_ctanhf.o \
	src/s_erf.o \
	src/s_erff.o \
	src/s_exp2.o \
	src/s_exp2f.o \
	src/s_expm1.o \
	src/s_expm1f.o \
	src/s_fabs.o \
	src/s_fabsf.o \
	src/s_fdim.o \
	src/s_floor.o \
	src/s_floorf.o \
	src/s_fma.o \
	src/s_fmaf.o \
	src/s_fmax.o \
	src/s_fmaxf.o \
	src/s_fmin.o \
	src/s_fminf.o \
	src/s_fpclassify.o \
	src/s_frexp.o \
	src/s_frexpf.o \
	src/s_ilogb.o \
	src/s_ilogbf.o \
	src/s_isfinite.o \
	src/s_isinf.o \
	src/s_isnan.o \
	src/s_llrint.o \
	src/s_llrintf.o \
	src/s_llround.o \
	src/s_llroundf.o \
	src/s_log1p.o \
	src/s_log1pf.o \
	src/s_logb.o \
	src/s_logbf.o \
	src/s_lrint.o \
	src/s_lrintf.o \
	src/s_lround.o \
	src/s_lroundf.o \
	src/s_modf.o \
	src/s_modff.o \
	src/s_nan.o \
	src/s_nearbyint.o \
	src/s_nextafter.o \
	src/s_nextafterf.o \
	src/s_nexttowardf.o \
	src/s_remquo.o \
	src/s_remquof.o \
	src/s_rint.o \
	src/s_rintf.o \
	src/s_round.o \
	src/s_roundf.o \
	src/s_scalbln.o \
	src/s_scalbn.o \
	src/s_scalbnf.o \
	src/s_signbit.o \
	src/s_signgam.o \
	src/s_sin.o \
	src/s_sincos.o \
	src/s_sincosf.o \
	src/s_sinf.o \
	src/s_tan.o \
	src/s_tanf.o \
	src/s_tanh.o \
	src/s_tanhf.o \
	src/s_tgammaf.o \
	src/s_trunc.o \
	src/s_truncf.o \
	src/w_cabs.o \
	src/w_cabsf.o \
	src/w_cabsl.o

LIBOPENLIBM = $(TARGET_DIR)/lib/libopenlibm.a

$(LIBOPENLIBM): $(LIB_LIBOPENLIBM_OBJS)
	$(AR) rcs $(LIBOPENLIBM) $(LIB_LIBOPENLIBM_OBJS)
	cp -r include/* $(TARGET_DIR)/include
	cp -r src/*.h $(TARGET_DIR)/include


clean:
	rm -f $(LIB_LIBOPENLIBM_OBJS)
	rm -f $(LIBOPENLIBM)

