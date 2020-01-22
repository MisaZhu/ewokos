GTEST_OBJS = $(ROOT_DIR)/bin/gtest/gtest.o

GTEST = $(TARGET_DIR)/$(ROOT_DIR)/bin/gtest

PROGS += $(GTEST)
CLEAN += $(GTEST_OBJS)

$(GTEST): $(GTEST_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(GTEST_OBJS) $(LIB_OBJS) -o $(GTEST) $(LDFLAGS)
