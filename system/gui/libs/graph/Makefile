SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include


LIB_GRAPH_OBJS = $(SYS_DIR)/platform/$(ARCH)/bsp/src/bsp_graph.o \
	src/graph.o \
	src/graph_ex.o \
	src/graph_png.o \
	src/blt.o \
	src/pixel.o \
	src/line.o \
	src/box.o \
	src/circle.o \
	src/arc.o \
	src/round.o \
	src/bitmap.o

GRAPH = $(TARGET_DIR)/lib/libgraph.a

$(GRAPH): $(LIB_GRAPH_OBJS)
	$(AR) rcs $(GRAPH) $(LIB_GRAPH_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_GRAPH_OBJS)
	rm -f $(GRAPH)

