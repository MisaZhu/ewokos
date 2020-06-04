GLOBAL_OBJS = $(ROOT_DIR)/bin/global/global.o

GLOBAL = $(TARGET_DIR)/$(ROOT_DIR)/bin/global

PROGS += $(GLOBAL)
CLEAN += $(GLOBAL_OBJS)

$(GLOBAL): $(GLOBAL_OBJS)
	$(LD) -Ttext=100 $(GLOBAL_OBJS) -o $(GLOBAL) $(LDFLAGS) -lewokc -lc
