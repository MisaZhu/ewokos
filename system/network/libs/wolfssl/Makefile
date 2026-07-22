SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule


BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

PREFIX := /opt/ewokos

ASFLAGS = --warn
CFLAGS += -O2 -ffreestanding -DWOLFSSL_USER_SETTINGS \
	-I. \
	-I./include \
	-I$(BUILD_DIR)/include \
	-I$(SYS_DIR)/basic/libs/include \
	-I$(SYS_DIR)/network/libs/socket/include
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBWOLFSSL = $(TARGET_DIR)/lib/libwolfssl.a

all: $(LIBWOLFSSL)

# WolfCrypt source files (without sp_int.c and related SP math files)
WOLFCRYPT_SRCS = \
	wolfcrypt/src/aes.c \
	wolfcrypt/src/arc4.c \
	wolfcrypt/src/asn.c \
	wolfcrypt/src/blake2b.c \
	wolfcrypt/src/blake2s.c \
	wolfcrypt/src/camellia.c \
	wolfcrypt/src/chacha.c \
	wolfcrypt/src/chacha20_poly1305.c \
	wolfcrypt/src/cmac.c \
	wolfcrypt/src/coding.c \
	wolfcrypt/src/compress.c \
	wolfcrypt/src/cpuid.c \
	wolfcrypt/src/cryptocb.c \
	wolfcrypt/src/curve25519.c \
	wolfcrypt/src/curve448.c \
	wolfcrypt/src/des3.c \
	wolfcrypt/src/dh.c \
	wolfcrypt/src/dsa.c \
	wolfcrypt/src/ecc.c \
	wolfcrypt/src/ed25519.c \
	wolfcrypt/src/ed448.c \
	wolfcrypt/src/error.c \
	wolfcrypt/src/fe_448.c \
	wolfcrypt/src/fe_low_mem.c \
	wolfcrypt/src/fe_operations.c \
	wolfcrypt/src/ge_448.c \
	wolfcrypt/src/ge_low_mem.c \
	wolfcrypt/src/ge_operations.c \
	wolfcrypt/src/hash.c \
	wolfcrypt/src/hmac.c \
	wolfcrypt/src/hpke.c \
	wolfcrypt/src/integer.c \
	wolfcrypt/src/kdf.c \
	wolfcrypt/src/logging.c \
	wolfcrypt/src/md2.c \
	wolfcrypt/src/md4.c \
	wolfcrypt/src/md5.c \
	wolfcrypt/src/memory.c \
	wolfcrypt/src/misc.c \
	wolfcrypt/src/pkcs12.c \
	wolfcrypt/src/pkcs7.c \
	wolfcrypt/src/poly1305.c \
	wolfcrypt/src/pwdbased.c \
	wolfcrypt/src/random.c \
	wolfcrypt/src/rc2.c \
	wolfcrypt/src/ripemd.c \
	wolfcrypt/src/rsa.c \
	wolfcrypt/src/sha.c \
	wolfcrypt/src/sha256.c \
	wolfcrypt/src/sha3.c \
	wolfcrypt/src/sha512.c \
	wolfcrypt/src/signature.c \
	wolfcrypt/src/siphash.c \
	wolfcrypt/src/sm2.c \
	wolfcrypt/src/sm3.c \
	wolfcrypt/src/sm4.c \
	wolfcrypt/src/srp.c \
	wolfcrypt/src/tfm.c \
	wolfcrypt/src/wc_encrypt.c \
	wolfcrypt/src/wc_lms.c \
	wolfcrypt/src/wc_lms_impl.c \
	wolfcrypt/src/wc_mlkem.c \
	wolfcrypt/src/wc_mlkem_poly.c \
	wolfcrypt/src/wc_port.c \
	wolfcrypt/src/wc_slhdsa.c \
	wolfcrypt/src/wc_xmss.c \
	wolfcrypt/src/wc_xmss_impl.c \
	wolfcrypt/src/wolfevent.c \
	wolfcrypt/src/wolfmath.c \
	wolfcrypt/src/evp.c \
	wolfcrypt/src/evp_pk.c \
	wolfcrypt/src/port/ewokos/ewokos_port.c

# SSL/TLS source files
SSL_SRCS = \
	src/bio.c \
	src/conf.c \
	src/crl.c \
	src/dtls.c \
	src/dtls13.c \
	src/internal.c \
	src/keys.c \
	src/ocsp.c \
	src/pk.c \
	src/pk_ec.c \
	src/pk_rsa.c \
	src/quic.c \
	src/sniffer.c \
	src/ssl.c \
	src/ssl_api_cert.c \
	src/ssl_api_crl_ocsp.c \
	src/ssl_api_pk.c \
	src/ssl_asn1.c \
	src/ssl_bn.c \
	src/ssl_certman.c \
	src/ssl_crypto.c \
	src/ssl_ech.c \
	src/ssl_load.c \
	src/ssl_misc.c \
	src/ssl_p7p12.c \
	src/ssl_sess.c \
	src/ssl_sk.c \
	src/tls.c \
	src/tls13.c \
	src/wolfio.c \
	src/x509.c \
	src/x509_str.c

SRCS = $(WOLFCRYPT_SRCS) $(SSL_SRCS)

OBJS = $(SRCS:.c=.o)

$(LIBWOLFSSL): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	@mkdir -p $(TARGET_DIR)/include/wolfssl
	@mkdir -p $(TARGET_DIR)/include/wolfssl/wolfcrypt
	@mkdir -p $(TARGET_DIR)/include/wolfssl/openssl
	cp -r wolfssl/*.h $(TARGET_DIR)/include/wolfssl/
	cp -r wolfssl/wolfcrypt/*.h $(TARGET_DIR)/include/wolfssl/wolfcrypt/
	cp -r wolfssl/openssl/*.h $(TARGET_DIR)/include/wolfssl/openssl/
	cp include/user_settings.h $(TARGET_DIR)/include/

%.o: %.c
	$(CC) $(CFLAGS) -std=c99 -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(LIBWOLFSSL)
	find . -name "*.o" -delete
