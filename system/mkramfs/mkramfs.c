#include "base16.h" 
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

static uint32_t _totalSize = 0;
const uint32_t BLOCK_SIZE = 64;

void dump(FILE* out, const char*dname, const char* fname) {
	char full[256];
	snprintf(full, 256, "%s/%s", dname, fname);

	FILE* f = fopen(full, "r");
	if(f == NULL)
		return;
	
	//write name info
	int32_t nameLen = strlen(fname);
	//write content info
	fseek(f, 0, SEEK_END); // seek to end of file
	int32_t size = ftell(f); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file
	
	uint32_t fsize = 4 + nameLen + 4 + size; //namelen, name, size, content.
	char* p = malloc(fsize); 
	char* start = p;
	memcpy(p, &nameLen, 4);
	p += 4;
	memcpy(p, fname, nameLen);
	p += nameLen;
	memcpy(p, &size, 4);
	p += 4;
	fread(p, 1, size, f);
	fclose(f);

	_totalSize += fsize;
	
	char dst[BLOCK_SIZE*2];
	int32_t dstLen;
	p = start;
	while(fsize > 0) {
		int32_t sz = fsize >= BLOCK_SIZE ? BLOCK_SIZE : fsize;
		fprintf(out, "\"");
		base16Encode(p, sz, dst, &dstLen);
		fwrite(dst, 1, dstLen, out);
		fprintf(out, "\"");
		fsize -= sz;
		p += sz;
		fprintf(out, ",\n");
	}

	free(start);
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

	fprintf(fp, "#ifndef INITFS_H\n#define INITFS_H\nconst char* _initfs[] = {");
	//fwrite(s, 1, strlen(s), fp);

	while(1) {
		struct dirent* r = readdir(dir);
		if(r == NULL)
			break;

		if(r->d_name[0] == '.')
			continue;
		dump(fp, argv[2], r->d_name);
	}

	
	fprintf(fp, "\"\"};\nconst uint32_t _initfsSize = %d;\n#endif", _totalSize);	
	fclose(fp);
	
	closedir(dir);
	return 0;
}
