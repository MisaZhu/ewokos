#include <base16.h>

void b16_encode(const char *input, uint32_t input_len, char *output, uint32_t *output_len) {
	uint32_t i;
	for (i = 0; i < input_len; i++) {
		output[2 * i] = (input[i] & 0xF) + 'A';
		output[2 * i + 1] = ((input[i] >> 4) & 0xF) + 'A';
	}
	*output_len = 2 * input_len;
}

void b16_decode(const char *input, uint32_t input_len, char *output, uint32_t *output_len) {
	uint32_t i;
	for (i = 0; i < input_len / 2; i++) {
		uint8_t low = input[2 * i] - 'A';
		uint8_t high = input[2 * i + 1] - 'A';
		output[i] = low | (high << 4);
	}
	*output_len = input_len / 2;
}
