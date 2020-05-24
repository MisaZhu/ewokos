RM_OBJS = $(ROOT_DIR)/bin/rm/rm.o

RM = $(TARGET_DIR)/$(ROOT_DIR)/bin/rm

PROGS += $(RM)
CLEAN += $(RM_OBJS)

$(RM): $(RM_OBJS)
	$(LD) -Ttext=100 $(RM_OBJS) -o $(RM) $(LDFLAGS) -lewokc -lc
