SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include \
	-I $(SYS_DIR)/platform/$(ARCH)/include/$(ARCH)

# A2D alignment applies to surface pitch/base, not rectangle width. Route
# the complete Q8B crop to the BSP so the daemon never CPU-rasterizes tails.
ifeq ($(HW),q8b)
CFLAGS += -DG2D_PITCH_ALIGN=4
# With no CPU tail the geometry helper first pulls g2d_arch.o from libbsp,
# after EWOK_LIB_GRAPH's libopenlibm was scanned. Resolve its math imports.
G2DD_LFLAGS = -lopenlibm
endif

G2DD_OBJS = $(ARCH)/g2dd.o

G2DD = $(TARGET_DIR)/drivers/$(HW)/g2dd

$(G2DD): $(G2DD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libdisplay.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libdisplayman.a
	$(LD) -Ttext=100 $(G2DD_OBJS) -o $(G2DD) $(LDFLAGS) $(EWOK_LIB_GRAPH) $(G2DD_LFLAGS) $(EWOK_LIBC)

ifeq ($(HW),q8b)
$(G2DD): $(BUILD_DIR)/lib/libadreno.a
endif

clean:
	rm -f $(G2DD_OBJS)
