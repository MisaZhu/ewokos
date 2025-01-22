#include <stdio.h>
#include "mesh.h"
#include "array.h"

int main(int argc, char* argv[]){

	load_obj_file_data(argv[1]);
	printf("#include \"mesh.h\"\n");
	printf("\n");

	int num_vertices = array_length(mesh.vertices);
	printf("static vec3_t vertices[%d] = {\n", num_vertices);
	for (int i = 0; i < num_vertices; i++) {
		printf("\t{%f, %f, %f},\n", mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
	}
	printf("};\n\n");
	
	int num_faces = array_length(mesh.faces);
	printf("static face_t faces[%d] = {\n", num_faces);
	for (int i = 0; i < num_faces; i++) {
		printf("\t{%d, %d, %d, {%f, %f}, {%f, %f}, {%f, %f}, 0x%08x},\n", 
					mesh.faces[i].a, mesh.faces[i].b, mesh.faces[i].c, 
					mesh.faces[i].a_uv.u, mesh.faces[i].a_uv.v,
					mesh.faces[i].b_uv.u, mesh.faces[i].b_uv.v,
					mesh.faces[i].c_uv.u, mesh.faces[i].c_uv.v,
					mesh.faces[i].color
				);
	}
	printf("};\n\n");

	printf("int num_faces = %d;\n", num_faces);
	printf("mesh_t mesh = {\n");
	printf("\t.vertices = vertices,\n");
	printf("\t.faces = faces,\n");
	printf("\t.rotation = {.x = 0, .y = 0, .z = 0},\n");
	printf("\t.scale = {.x = 1.0, .y = 1.0, .z = 1.0},\n");
	printf("\t.translation = {.x = 0, .y = 0, .z = 0}\n};\n");
}


