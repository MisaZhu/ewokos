#include <stdio.h>

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

int main() {
	int buff_len;
	int b16_len;

	printf("{");
	while (buff_len = fread(buff, 1, BUF_LEN, stdin)) {
		b16encode(buff, buff_len, b16, &b16_len);
		b16[b16_len] = 0;
		printf("\"%s\",\n", b16);
	}
	printf("0}");

	return 0;
}
