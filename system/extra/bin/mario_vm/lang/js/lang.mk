MARIO_LANG = js
lang_OBJS = $(MARIO_VM)/lang/$(MARIO_LANG)/compiler.o 
NATIVE_PATH = $(MARIO_VM)/lang/$(MARIO_LANG)/native/builtin
NATIVE_OBJS= $(NATIVE_PATH)/native_builtin.o  \
	$(NATIVE_PATH)/object/native_object.o  \
	$(NATIVE_PATH)/number/native_number.o  \
	$(NATIVE_PATH)/bytes/native_bytes.o  \
	$(NATIVE_PATH)/console/native_console.o \
	$(NATIVE_PATH)/string/native_string.o \
	$(NATIVE_PATH)/array/native_array.o \
	$(NATIVE_PATH)/json/native_json.o \
#	$(NATIVE_PATH)/math/native_math.o

