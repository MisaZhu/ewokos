CC = gcc

# InfoNES
.CFILES = K6502.cpp \
		InfoNES.cpp \
		InfoNES_Mapper.cpp \
		InfoNES_pAPU.cpp \
		test.cpp

.OFILES	=	$(.CFILES:.cpp=.o)

CCFLAGS = `sdl-config --cflags` -O3 -pipe
CCFLAGS_NOOPT = -O0 -pipe
LDFLAGS = `sdl-config --libs` -lstdc++			# gcc3.x.x

all: InfoNES

InfoNES: $(.OFILES)
	$(CC) $(INCLUDES) -o $@ $(.OFILES) $(LDFLAGS) -lm -lz

# To avoid the gcc optimization bug
./../InfoNES_pAPU.o: ./../InfoNES_pAPU.cpp
	$(CC) $(INCLUDES) -c $(CCFLAGS_NOOPT) InfoNES_pAPU.cpp -o InfoNES_pAPU.o 

.cpp.o:
	$(CC) $(INCLUDES) -c $(CCFLAGS) $*.cpp  -o $@

clean:
	rm -f $(.OFILES)  InfoNES 

# End of Makefile
