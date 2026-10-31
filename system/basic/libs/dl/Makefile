SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

# every platform make.rule spells the archiver <triple>-ar, so the matching nm
# is derivable rather than a fourth per-arch variable to keep in sync
NM = $(patsubst %-ar,%-nm,$(AR))

#
# Archives the host ABI export table is generated from.
#
# This is deliberately exactly the set EWOK_LIBC names, and that is what makes
# the strong references gen_export.sh emits safe: an app linking libdl also
# links these three archives, so every symbol in the table resolves. Verified
# by link - a 731-symbol table produces zero undefined references and a 207K
# binary, because the heavy subsystems (xwin, graph, socket) live in their own
# archives and are not pulled in.
#
# Adding an archive here means every plugin host must also link it. Anything
# outside this set belongs in dl_host_publish() instead - see dl/dl.h.
#
DL_EXPORT_LIBS = libewoksys.a libc.a libgloss.a
DL_EXPORT_DEPS = $(addprefix $(TARGET_DIR)/lib/,$(DL_EXPORT_LIBS))

# generated into the arch build dir, not the source tree: it is derived from
# whatever the libraries happen to contain, so committing it would guarantee a
# stale copy, and `clean` removing $(ARCH) takes it with it
DL_EXPORT_SRC = $(ARCH)/gen/dl_export.c
DL_EXPORT_OBJ = $(ARCH)/gen/dl_export.o

LIB_DL_OBJS = $(ARCH)/src/dl.o \
              $(ARCH)/src/dl_elf.o \
              $(ARCH)/src/dl_abi.o \
              $(DL_EXPORT_OBJ)

DL = $(TARGET_DIR)/lib/libdl.a

# Same reason as libs/elf: dlfcn.h and dl/dl.h must be reinstalled on a
# header-only edit, which a recipe line on $(DL) would skip because the archive
# is already up to date.
all: $(DL) headers

headers:
	@mkdir -p $(TARGET_DIR)/include
	cp -r include/* $(TARGET_DIR)/include

$(DL): $(LIB_DL_OBJS)
	$(AR) rcs $(DL) $(LIB_DL_OBJS)

#
# Regenerate whenever any source archive changes.
#
# A stale table fails in both directions: it can name a symbol the library no
# longer defines (link error in every plugin host), or miss one that was added
# (a plugin fails at load with "undefined symbol" for something the host does
# have). Both are confusing far away from their cause, so the dependency is
# explicit rather than leaving it to a manual rebuild.
#
$(DL_EXPORT_SRC): gen_export.sh $(DL_EXPORT_DEPS)
	@mkdir -p $(dir $@)
	sh gen_export.sh $(NM) $@ $(DL_EXPORT_DEPS)

# explicit rule: the generated source lives under $(ARCH)/, which the generic
# $(ARCH)/%.o: %.c pattern in make.rule cannot reach
$(DL_EXPORT_OBJ): $(DL_EXPORT_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_DL_OBJS)
	rm -fr $(ARCH)/gen
	rm -f $(DL)

.PHONY: all headers clean
