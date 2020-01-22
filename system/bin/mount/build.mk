MOUNT_OBJS = $(ROOT_DIR)/bin/mount/mount.o

MOUNT = $(TARGET_DIR)/$(ROOT_DIR)/bin/mount

PROGS += $(MOUNT)
CLEAN += $(MOUNT_OBJS)

$(MOUNT): $(MOUNT_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(MOUNT_OBJS) $(LIB_OBJS) -o $(MOUNT) $(LDFLAGS)
