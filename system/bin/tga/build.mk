TGATEST_OBJS = $(ROOT_DIR)/bin/tga/tga.o

TGATEST = $(TARGET_DIR)/$(ROOT_DIR)/bin/tga

PROGS += $(TGATEST)
CLEAN += $(TGATEST_OBJS)

$(TGATEST): $(TGATEST_OBJS)
	$(LD) -Ttext=100 $(TGATEST_OBJS) -o $(TGATEST) $(LDFLAGS) -lgraph -ltga -lx -lewokc  -lc
