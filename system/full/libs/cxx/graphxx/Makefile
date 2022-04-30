include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_GRAPHXX_OBJS = src/graphxx.o

LIB_GRAPHXX = $(TARGET_DIR)/lib/libgraphxx.a

$(LIB_GRAPHXX): $(LIB_GRAPHXX_OBJS) $(TARGET_DIR)/lib/libgraph.a $(TARGET_DIR)/lib/libfonts.a
	$(AR) rT $(LIB_GRAPHXX) $(LIB_GRAPHXX_OBJS) $(TARGET_DIR)/lib/libfonts.a $(TARGET_DIR)/lib/libgraph.a
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_GRAPHXX_OBJS)
	rm -f $(LIB_GRAPHXX)

