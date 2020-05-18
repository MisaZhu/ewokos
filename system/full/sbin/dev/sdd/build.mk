SDD_OBJS = $(ROOT_DIR)/sbin/dev/sdd/sdd.o
SDD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/sdd

PROGS += $(SDD)
CLEAN += $(SDD_OBJS)

$(SDD): $(SDD_OBJS) 
	$(LD) -Ttext=100 $(SDD_OBJS) -o $(SDD) $(LDFLAGS) -lewokc -lc
