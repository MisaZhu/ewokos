ifneq ($(CROSS_COMPILE),)
$(info CROSS_COMPILE=$(CROSS_COMPILE))
endif

CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)gcc

MARIO_VM = ../../../../mario
mario_OBJS= $(MARIO_VM)/mario.o

lang_OBJS = ../../compiler.o

OBJS = demo.o $(mario_OBJS) $(lang_OBJS)

#CFLAGS =  -g -I$(MARIO_VM) -Wall -fPIC -DMARIO_CACHE -DMARIO_DEBUG
CFLAGS =  -g -I$(MARIO_VM) -Wall -fPIC -DMARIO_DEBUG -DMARIO_CACHE -DMARIO_THREAD
LDFLAGS =  -lm -ldl -lpthread

TARGET=demo
INST_DST=/usr/local/mario

all: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -fr $(TARGET) $(OBJS) *.dSYM

install:
	mkdir -p $(INST_DST)/bin
	cp $(TARGET) $(INST_DST)/bin
