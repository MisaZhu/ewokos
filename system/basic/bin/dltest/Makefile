SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

DLTEST_OBJS = $(ARCH)/dltest.o
DLTEST = $(TARGET_DIR)/bin/dltest

#
# The plugins install into the rootfs /lib, which is one of the directories
# dl_resolve_path() searches by default. There is no rpath mechanism in the
# EwokOS rootfs, so /lib is where a bare soname has to be findable - including
# the DT_NEEDED "libdlplug_a.so" that plugin B carries.
#
# Nothing else in the tree puts anything in /lib, so this directory is created
# here. `make final` copies rootfs/* into the machine's build dir and `make sd`
# packs it with e2cp -d, which creates the directory in the image.
#
LIBDIR = $(TARGET_DIR)/lib
PLUG_A   = $(LIBDIR)/libdlplug_a.so
PLUG_B   = $(LIBDIR)/libdlplug_b.so
PLUG_BAD = $(LIBDIR)/libdlplug_bad.so

#
# Eight independent objects built from ONE source (plugin_many.c) with a
# different -DPLUG_ID each, so dltest can hold many distinct shared objects
# resident at once. That is the case the two-object A/B chain cannot cover:
# concurrent window-slot allocation, per-object relocation, and - after closing
# and reopening the set - proof that dlclose really released the slots and pages.
#
MANY_IDS  := 1 2 3 4 5 6 7 8
MANY_LIBS := $(foreach i,$(MANY_IDS),$(LIBDIR)/libdlplug_m$(i).so)

all: $(PLUG_A) $(PLUG_B) $(PLUG_BAD) $(MANY_LIBS) $(DLTEST)

#
# The host links $(EWOK_LIB_DL) = -ldl $(EWOK_LIBC). -ldl must come first: its
# generated export table holds strong references to every global symbol of
# libewoksys/libc/libgloss, and it is those references that make ld extract the
# archive members a plugin might call. See libs/dl/gen_export.sh.
#
$(DLTEST): $(DLTEST_OBJS) $(BUILD_DIR)/lib/libdl.a $(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(DLTEST_OBJS) -o $(DLTEST) $(LDFLAGS) $(EWOK_LIB_DL)

#
# -Wl,-soname is what lets one object be found under two names: dltest opens A
# by absolute path and B references it as the bare "libdlplug_a.so", and both
# must land on the SAME mapping or the shared-instance checks in dltest fail.
#
$(PLUG_A): $(ARCH)/plugin_a.o
	@mkdir -p $(LIBDIR)
	$(CC) $(EWOK_SO_FLAGS) -Wl,-soname,libdlplug_a.so \
		$(ARCH)/plugin_a.o -o $(PLUG_A)

# Linked against A so the linker records DT_NEEDED "libdlplug_a.so".
$(PLUG_B): $(ARCH)/plugin_b.o $(PLUG_A)
	@mkdir -p $(LIBDIR)
	$(CC) $(EWOK_SO_FLAGS) -Wl,-soname,libdlplug_b.so \
		$(ARCH)/plugin_b.o -o $(PLUG_B) -L$(LIBDIR) -ldlplug_a

# Must fail to load: it references a symbol that exists nowhere.
$(PLUG_BAD): $(ARCH)/plugin_bad.o
	@mkdir -p $(LIBDIR)
	$(CC) $(EWOK_SO_FLAGS) -Wl,-soname,libdlplug_bad.so \
		$(ARCH)/plugin_bad.o -o $(PLUG_BAD)

#
# The many-object set. Each .so comes from plugin_many.c compiled with its own
# PLUG_ID/PLUG_TAG. The object rule is spelled plugin_many<id>.o so the generic
# $(ARCH)/%.o: %.c pattern (which would look for a nonexistent plugin_many1.c)
# does not match; make picks this rule because plugin_many.c is the prerequisite
# that actually exists. Only printf is referenced, so no host_callback publish is
# needed and every object loads standalone.
#
$(ARCH)/plugin_many%.o: plugin_many.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DPLUG_ID=$* -DPLUG_TAG='"M$*"' -c $< -o $@

$(LIBDIR)/libdlplug_m%.so: $(ARCH)/plugin_many%.o
	@mkdir -p $(LIBDIR)
	$(CC) $(EWOK_SO_FLAGS) -Wl,-soname,libdlplug_m$*.so $< -o $@

clean:
	rm -f $(DLTEST_OBJS) $(DLTEST)
	rm -f $(ARCH)/plugin_a.o $(ARCH)/plugin_b.o $(ARCH)/plugin_bad.o
	rm -f $(foreach i,$(MANY_IDS),$(ARCH)/plugin_many$(i).o)
	rm -f $(PLUG_A) $(PLUG_B) $(PLUG_BAD) $(MANY_LIBS)

.PHONY: all clean
