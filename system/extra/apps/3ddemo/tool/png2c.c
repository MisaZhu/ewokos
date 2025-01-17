#include <stdio.h>
#include "texture.h"

int main(int argc, char* argv[]){

	load_png_texture_data(argv[1]);

	int size = texture_width * texture_width;
	printf("#include \"texture.h\"\n");
	printf("int texture_width = %d;\n", texture_width);
	printf("int texture_height = %d;\n", texture_height);
	printf("\n");
	printf("static uint32_t _texture[] = {");
	for(int i = 0; i < size; i++){
		if(i%4 == 0)
			printf("\n\t");
		printf("0x%08x,", mesh_texture[i]);
	}
	printf("\n};\n");

	printf("uint32_t *mesh_texture = _texture;\n");
}


