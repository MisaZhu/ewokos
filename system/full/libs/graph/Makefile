include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_GRAPH_OBJS = src/font.o \
	src/graph.o

GRAPH = $(TARGET_DIR)/lib/libgraph.a

$(GRAPH): $(LIB_GRAPH_OBJS)
	$(AR) rT $(GRAPH) $(LIB_GRAPH_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_GRAPH_OBJS)
	rm -f $(GRAPH)

