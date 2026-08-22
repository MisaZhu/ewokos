SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include


LIB_GRAPH_OBJS = \
	$(SYS_DIR)/platform/$(ARCH)/src/$(ARCH)/graph_arch.o \
	$(ARCH)/src/graph.o \
	$(ARCH)/src/graph_ex.o \
	$(ARCH)/src/graph_png.o \
	$(ARCH)/src/graph_jpeg.o \
	$(ARCH)/src/graph_gif.o \
	$(ARCH)/src/graph_tga.o \
	$(ARCH)/src/graph_svg.o \
	$(ARCH)/src/graph_image.o \
	$(ARCH)/src/blt.o \
	$(ARCH)/src/uv12.o \
	$(ARCH)/src/rgb15.o \
	$(ARCH)/src/rgb24.o \
	$(ARCH)/src/pixel.o \
	$(ARCH)/src/line.o \
	$(ARCH)/src/box.o \
	$(ARCH)/src/circle.o \
	$(ARCH)/src/arc.o \
	$(ARCH)/src/round.o \
	$(ARCH)/src/scale.o \
	$(ARCH)/src/rotate.o \
	$(ARCH)/src/bitmap.o

GRAPH = $(TARGET_DIR)/lib/libgraph.a

$(GRAPH): $(LIB_GRAPH_OBJS)
	$(AR) rcs $(GRAPH) $(LIB_GRAPH_OBJS)
	cp -r include/* $(TARGET_DIR)/include

$(SYS_DIR)/platform/$(ARCH)/src/$(ARCH)/%.o: $(SYS_DIR)/platform/$(ARCH)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(LIB_GRAPH_OBJS)
	rm -f $(GRAPH)
