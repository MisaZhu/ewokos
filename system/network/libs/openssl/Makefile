SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I. -Iinclude -Iproviders/common/include -Iproviders/implementations/include -I$(TARGET_DIR)/include
CFLAGS += -DOPENSSLDIR=\"/etc/ssl\"
CFLAGS += -DENGINESDIR=\"/etc/ssl/engines\"
CFLAGS += -DMODULESDIR=\"/etc/ssl/modules\"
CFLAGS += -DOPENSSL_NO_DEPRECATED_3_0
CFLAGS += -DOPENSSL_NO_ENGINE
CFLAGS += -DOPENSSL_NO_ASM
CFLAGS += -DOPENSSL_NO_HW
CFLAGS += -DOPENSSL_NO_DEPRECATED
CFLAGS += -DOPENSSL_NO_KTLS
CFLAGS += -DOPENSSL_NO_MDC2
CFLAGS += -DOPENSSL_NO_OCB
CFLAGS += -DOPENSSL_NO_RC5
CFLAGS += -DOPENSSL_NO_SCTP
CFLAGS += -DOPENSSL_NO_SSL3
CFLAGS += -DOPENSSL_NO_TLS1_3
CFLAGS += -DOPENSSL_NO_DTLS1_2
CFLAGS += -DOPENSSL_NO_DTLS1
CFLAGS += -DOPENSSL_NO_CMS
CFLAGS += -DOPENSSL_NO_SOCK
CFLAGS += -DOPENSSL_NO_DES
CFLAGS += -DOPENSSL_NO_DSO

# Find all .c files in crypto and ssl directories
CRYPTO_SRCS := $(shell find crypto -name "*.c" -not -path "crypto/des/*" -not -path "crypto/dso/*" -not -path "crypto/engine/*" -not -path "crypto/cms/*" -not -path "crypto/mdc2/*" 2>/dev/null)

# Filter out problematic files
CRYPTO_SRCS := $(filter-out crypto/bio/bss_acpt.c crypto/bio/bss_conn.c crypto/bio/bss_dgram.c crypto/bio/bss_sock.c crypto/bio/bss_log.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/ec/ecp_nistp224.c crypto/ec/ecp_nistp256.c crypto/ec/ecp_nistp521.c crypto/ec/ecp_nistputil.c crypto/ec/ecp_nistz256.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/evp/e_des.c crypto/evp/e_des3.c crypto/evp/e_xcbc_d.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/evp/legacy_mdc2.c crypto/evp/legacy_meth.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/mdc2/mdc2_one.c crypto/mdc2/mdc2dgst.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/ui/ui_openssl.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/LPdir_wince.c crypto/LPdir_win.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rc5/%,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/md2/%,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/LPdir_%.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/ppccap.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/s390xcap.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/armcap.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/o_dir.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rsa/rsa_acvp_test_params.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/ec/ecx_s390x.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/ec/ecp_nistz256_table.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/ec/ecp_s390x_nistp.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/x509/x509type.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/poly1305/poly1305_ieee754.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/poly1305/poly1305_base2_44.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/evp/legacy_md2.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/sparcv9cap.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/bn/asm/x86_64-gcc.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rand/rand_meth.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rand/rand_egd.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rand/randfile.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/conf/conf_def.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/conf/conf_mod.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/provider%.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rand/rand_lib.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/rand/prov_seed.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/srp/%,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/aes/aes_x86core.c,$(CRYPTO_SRCS))
CRYPTO_SRCS := $(filter-out crypto/store/%,$(CRYPTO_SRCS))

SSL_SRCS := $(shell find ssl -name "*.c" 2>/dev/null)
SSL_SRCS := $(filter-out ssl/ssl_rsa_legacy.c,$(SSL_SRCS))
SSL_SRCS := $(filter-out ssl/tls_srp.c,$(SSL_SRCS))

CRYPTO_OBJS = $(CRYPTO_SRCS:.c=.o)
SSL_OBJS = $(SSL_SRCS:.c=.o)

LIBCRYPTO = $(TARGET_DIR)/lib/libcrypto.a
LIBSSL = $(TARGET_DIR)/lib/libssl.a

all: $(LIBCRYPTO) $(LIBSSL)

$(LIBCRYPTO): $(CRYPTO_OBJS)
	$(AR) rcs $(LIBCRYPTO) $(CRYPTO_OBJS)
	cp -r include/* $(TARGET_DIR)/include/

$(LIBSSL): $(SSL_OBJS) $(LIBCRYPTO)
	$(AR) rcs $(LIBSSL) $(SSL_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CRYPTO_OBJS) $(SSL_OBJS)
	rm -f $(LIBCRYPTO) $(LIBSSL)
