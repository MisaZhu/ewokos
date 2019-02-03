#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

void dump(FILE* out, const char*dname, const char* fname) {
	char full[256];
	snprintf(full, 256, "%s/%s", dname, fname);
	FILE* f = fopen(full, "r");
	if(f == NULL)
		return;
	
	//write name info
	int32_t nameLen = strlen(fname);
	fwrite(&nameLen, 1, 4, out);
	fwrite(fname, 1, nameLen, out);

	//write content info
	fseek(f, 0, SEEK_END); // seek to end of file
	int32_t size = ftell(f); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file
	fwrite(&size, 1, 4, out);

	char* p = malloc(size);
	fread(p, 1, size, f);
	fwrite(p, 1, size, out);

	free(p);
	fclose(f);
}


/*
mkramfs [output_file] [source_dir]
*/
int main(int argc, char** argv) {
	if(argc < 3) {
		return -1;
	}

	DIR* dir = opendir(argv[2]);
	if(dir == NULL)
		return -2;

	FILE* fp = fopen(argv[1], "w");
	if(fp == NULL) {
		closedir(dir);
		return -3;
	}

	while(1) {
		struct dirent* r = readdir(dir);
		if(r == NULL)
			break;

		if(r->d_name[0] == '.')
			continue;
		dump(fp, argv[2], r->d_name);
	}

	int32_t end = 0;
	fwrite(&end, 1, 4, fp);
	
	closedir(dir);
	fclose(fp);

	return 0;
}
