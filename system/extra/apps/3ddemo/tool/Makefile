
all: png2c obj2c

obj2c: obj2c.c  mesh.c array.c
	gcc -I../src -o obj2c obj2c.c mesh.c array.c


png2c: png2c.c  texture.c upng.c
	gcc -I../src -o png2c png2c.c texture.c upng.c

clean:
	rm *.o obj2c png2c


