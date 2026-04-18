SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -Iinclude/litehtml -Iinclude/gumbo -I $(TARGET_DIR)/include -fno-rtti -nostdinc++
CXXFLAGS += -Iinclude -Iinclude/litehtml -Iinclude/gumbo -I $(TARGET_DIR)/include -fno-rtti -nostdinc++

LITEHTML_OBJS = \
	src/el_break.o \
	src/el_para.o \
	src/el_text.o \
	src/el_script.o \
	src/el_style.o \
	src/el_table.o \
	src/media_query.o \
	src/document.o \
	src/element.o \
	src/el_image.o \
	src/el_base.o \
	src/el_anchor.o \
	src/html_tag.o \
	src/el_comment.o \
	src/iterators.o \
	src/background.o \
	src/el_font.o \
	src/html.o \
	src/css_selector.o \
	src/el_td.o \
	src/el_tr.o \
	src/context.o \
	src/el_body.o \
	src/table.o \
	src/style.o \
	src/box.o \
	src/el_div.o \
	src/el_before_after.o \
	src/el_link.o \
	src/css_length.o \
	src/el_space.o \
	src/el_cdata.o \
	src/el_title.o \
	src/web_color.o \
	src/stylesheet.o

# Gumbo parser source files
GUMBO_OBJS = \
	src/gumbo/vector.o \
	src/gumbo/util.o \
	src/gumbo/utf8.o \
	src/gumbo/tokenizer.o \
	src/gumbo/string_piece.o \
	src/gumbo/tag.o \
	src/gumbo/string_buffer.o \
	src/gumbo/parser.o \
	src/gumbo/error.o \
	src/gumbo/char_ref.o \
	src/gumbo/attribute.o

TASK_OBJS = $(LITEHTML_OBJS) $(GUMBO_OBJS)

TASK = $(TARGET_DIR)/lib/liblitehtml.a

$(TASK): $(TASK_OBJS)
	$(AR) crs $(TASK) $(TASK_OBJS) 
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(TASK_OBJS)
	rm -f $(TASK)

