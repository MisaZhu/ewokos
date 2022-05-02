SYSINFOD_OBJS = $(ROOT_DIR)/drivers/proc/sysinfo/sysinfod.o

SYSINFOD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/proc/sysinfod

PROGS += $(SYSINFOD)
CLEAN += $(SYSINFOD_OBJS) $(SYSINFOD)

$(SYSINFOD): $(SYSINFOD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(SYSINFOD_OBJS) -o $(SYSINFOD) $(LDFLAGS) -lewokc -lc
