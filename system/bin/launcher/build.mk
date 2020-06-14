LAUNCHER_OBJS = $(ROOT_DIR)/bin/launcher/launcher.o

LAUNCHER = $(TARGET_DIR)/$(ROOT_DIR)/bin/launcher

PROGS += $(LAUNCHER)
CLEAN += $(LAUNCHER_OBJS)

$(LAUNCHER): $(LAUNCHER_OBJS)
	$(LD) -Ttext=100 $(LAUNCHER_OBJS) -o $(LAUNCHER) $(LDFLAGS) -lx -lsconf  -lgraph -lupng -lewokc -lc
