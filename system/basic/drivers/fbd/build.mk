FBD_OBJS = $(ROOT_DIR)/drivers/fbd/fbd.o

FBD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/fbd

PROGS += $(FBD)
CLEAN += $(FBD_OBJS)

$(FBD): $(FBD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(FBD_OBJS) -o $(FBD) $(LDFLAGS) -lgraph -lewokc -lc
