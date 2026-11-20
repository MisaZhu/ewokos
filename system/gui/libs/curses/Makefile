# curses builds against the EwokOS SDK installed under
# system/build_$(ARCH)/$(HW). Default ARCH/HW so this Makefile also works
# when invoked standalone (make -C system/gui/libs/curses).
ifeq ($(ARCH),)
export ARCH=aarch64
endif
ifeq ($(HW),)
export HW=virt
endif

SYS_ROOT_DIR = ../../..
include $(SYS_ROOT_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

# This is a trimmed port of netbsd-curses for EwokOS:
#   - no wide-character support (DISABLE_WCHAR); EwokOS libc has no wide runtime
#   - terminfo uses only the embedded compiled_terms[] (no TERMINFO_DB on-disk
#     database, no runtime tic compiler), so cdbr/compile are not built
#   - termcap.c provides the termcap compatibility layer (tgetent/tgetstr/
#     tgetflag/tgetnum/tgoto, UP/BC) on top of terminfo; it uses termcap_map.c
#     with a linear scan instead of the generated termcap_hash.c perfect hash
# netbsd_sys/ provides the small cdefs/endian compatibility layer.
CFLAGS += -DDISABLE_WCHAR \
	-I. -Ilibcurses -Ilibterminfo \
	-I$(TARGET_DIR)/include

# --- libterminfo -----------------------------------------------------------
TI_SRCS = libterminfo/term.c libterminfo/ti.c libterminfo/setupterm.c \
	libterminfo/curterm.c libterminfo/tparm.c libterminfo/tputs.c \
	libterminfo/mi_vector_hash.c libterminfo/hash.c libterminfo/termcap.c
TI_OBJS = $(patsubst %.c,$(ARCH)/%.o,$(TI_SRCS))
LIBTERMINFO = $(TARGET_DIR)/lib/libterminfo.a

# --- libcurses -------------------------------------------------------------
CU_SRCS = $(wildcard libcurses/*.c)
CU_OBJS = $(patsubst %.c,$(ARCH)/%.o,$(CU_SRCS))
LIBCURSES = $(TARGET_DIR)/lib/libcurses.a

all: $(LIBTERMINFO) $(LIBCURSES) install_headers
	@echo "curses done."

$(LIBTERMINFO): $(TI_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(TI_OBJS)

$(LIBCURSES): $(CU_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(CU_OBJS)

install_headers:
	@mkdir -p $(TARGET_DIR)/include
	cp libcurses/curses.h libcurses/unctrl.h $(TARGET_DIR)/include/
	cp libterminfo/term.h libterminfo/termcap.h $(TARGET_DIR)/include/

clean:
	rm -rf $(ARCH)
	rm -f $(LIBTERMINFO) $(LIBCURSES)

.PHONY: all install_headers clean
