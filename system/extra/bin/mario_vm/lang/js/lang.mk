MARIO_LANG = js
lang_OBJS = $(MARIO_VM)/lang/$(MARIO_LANG)/compiler.o 

NATIVE_PATH_BUILTIN = $(MARIO_VM)/lang/$(MARIO_LANG)/native/builtin
NATIVE_PATH_X = $(MARIO_VM)/lang/$(MARIO_LANG)/native/x
NATIVE_PATH_GRAPH = $(MARIO_VM)/lang/$(MARIO_LANG)/native/graph

NATIVE_OBJS= $(NATIVE_PATH_BUILTIN)/native_builtin.o  \
	$(NATIVE_PATH_BUILTIN)/object/native_object.o  \
	$(NATIVE_PATH_BUILTIN)/number/native_number.o  \
	$(NATIVE_PATH_BUILTIN)/bytes/native_bytes.o  \
	$(NATIVE_PATH_BUILTIN)/console/native_console.o \
	$(NATIVE_PATH_BUILTIN)/string/native_string.o \
	$(NATIVE_PATH_BUILTIN)/array/native_array.o \
	$(NATIVE_PATH_BUILTIN)/json/native_json.o \
	\
	$(NATIVE_PATH_GRAPH)/native_graph.o  \
	\
	$(NATIVE_PATH_X)/native_x.o  \

#	$(NATIVE_PATH_BUILTIN)/math/native_math.o



