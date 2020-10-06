STDERRD_OBJS = $(ROOT_DIR)/drivers/stderrd/stderrd.o

STDERRD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/stderrd

PROGS += $(STDERRD)
CLEAN += $(STDERRD_OBJS)

$(STDERRD): $(STDERRD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(STDERRD_OBJS) -o $(STDERRD) $(LDFLAGS) -lewokc -lc
