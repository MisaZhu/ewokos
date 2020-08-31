ACTLED_OBJS = $(ROOT_DIR)/bin/actled/actled.o

ACTLED = $(TARGET_DIR)/$(ROOT_DIR)/bin/actled

PROGS += $(ACTLED)
CLEAN += $(ACTLED_OBJS)

$(ACTLED): $(ACTLED_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(ACTLED_OBJS) -o $(ACTLED) $(LDFLAGS) -lewokc -lc
