#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_LEN 4096
char buff[BUF_LEN];
char b16[BUF_LEN * 2 + 1];

void b16encode(char *input, int input_len, char *output, int *output_len)
{
	int i;
	for (i = 0; i < input_len; i++) {
		output[2 * i] = (input[i] & 0xF) + 'A';
		output[2 * i + 1] = ((input[i] >> 4) & 0xF) + 'A';
	}

	*output_len = 2 * input_len;
}

void do_file(const char* fname, const char* dname) {
	int buff_len;
	int b16_len;
	int total_buff_len = 0;

	struct stat st;
	stat(fname, &st);

	printf("\"%s\", \"%d\",\n", dname, (int)st.st_size);

	FILE* fp  = fopen(fname, "r");
	while (1) {
		buff_len = fread(buff, 1, BUF_LEN, fp);
		if(buff_len <= 0)
			break;
		b16encode(buff, buff_len, b16, &b16_len);
		b16[b16_len] = 0;
		printf("\"%s\",\n", b16);
		total_buff_len += buff_len;
	}
	printf("0,\n");
	fclose(fp);
}

void do_dir(const char* name, const char* dname) {
	DIR *dir = opendir(name);
	if(dir == NULL)
		return;
	if(dname[0] != 0)
		printf("\"%s\", \"r\",\n", dname);
	
	while(1) {
		struct dirent* r = readdir(dir);
		if(r == NULL)
			break;
		if(r->d_name[0] == '.')
			continue;
		
		char full[512];
		snprintf(full, 511, "%s/%s", name, r->d_name);
		if(r->d_type == DT_DIR)
			do_dir(full, r->d_name);
		else
			do_file(full, r->d_name);
	}
	closedir(dir);
	printf("0,\n");
}

int main(int argc, char** argv) {
	if(argc < 2) {
		return 1;
	}

	printf("const char* fs_data[] = {");
	do_dir(argv[1], "");
	printf("0};\n");
	return 0;
}
