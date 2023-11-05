include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_GRAPH_OBJS = src/graph.o \
	src/graph_ex.o \
	src/blt.o \
	src/pixel.o \
	src/line.o \
	src/box.o \
	src/circle.o \
	src/round.o \
	src/bitmap.o

GRAPH = $(TARGET_DIR)/lib/libgraph.a

$(GRAPH): $(LIB_GRAPH_OBJS)
	$(AR) rT $(GRAPH) $(LIB_GRAPH_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_GRAPH_OBJS)
	rm -f $(GRAPH)

