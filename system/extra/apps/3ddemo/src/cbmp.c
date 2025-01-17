#include <stdlib.h>
#include <stdio.h>
#include "cbmp.h"

// Constants

#define BITS_PER_BYTE 8

#define BLUE 0
#define GREEN 1
#define RED 2
#define ALPHA 3

#define PIXEL_ARRAY_START_BYTES 4
#define PIXEL_ARRAY_START_OFFSET 10

#define WIDTH_BYTES 4
#define WIDTH_OFFSET 18

#define HEIGHT_BYTES 4
#define HEIGHT_OFFSET 22

#define DEPTH_BYTES 2
#define DEPTH_OFFSET 28

// Private function declarations

void _throw_error(char* message);
unsigned int _get_int_from_buffer(unsigned int bytes, 
                                  unsigned int offset, 
                                  unsigned char* buffer);
unsigned int _get_file_byte_number(FILE* fp);
unsigned char* _get_file_byte_contents(FILE* fp, unsigned int file_byte_number);
int _validate_file_type(unsigned char* file_byte_contents);
int _validate_depth(unsigned int depth);
unsigned int _get_pixel_array_start(unsigned char* file_byte_contents);
int _get_width(unsigned char* file_byte_contents);
int _get_height(unsigned char* file_byte_contents);
unsigned int _get_depth(unsigned char* file_byte_contents);
void _update_file_byte_contents(BMP* bmp, int index, int offset, int channel);
void _populate_pixel_array(BMP* bmp);
void _map(BMP* bmp, void (*f)(BMP* bmp, int, int, int));
void _get_pixel(BMP* bmp, int index, int offset, int channel);

// Public function implementations

BMP* bopen(char* file_path)
{
    FILE* fp = fopen(file_path, "rb");
  
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    BMP* bmp = (BMP*) malloc(sizeof(BMP));
    bmp->file_byte_number = _get_file_byte_number(fp);
    bmp->file_byte_contents = _get_file_byte_contents(fp, bmp->file_byte_number);
    fclose(fp);

    if(!_validate_file_type(bmp->file_byte_contents))
    {
        _throw_error("Invalid file type");
    }

    bmp->pixel_array_start = _get_pixel_array_start(bmp->file_byte_contents);

    bmp->width = _get_width(bmp->file_byte_contents);
    bmp->height = _get_height(bmp->file_byte_contents);
    bmp->depth = _get_depth(bmp->file_byte_contents);

    if(!_validate_depth(bmp->depth))
    {
        _throw_error("Invalid file depth");
    }

    _populate_pixel_array(bmp);

    return bmp;
}

BMP* b_deep_copy(BMP* to_copy)
{
    BMP* copy = (BMP*) malloc(sizeof(BMP));
    copy->file_byte_number = to_copy->file_byte_number;
    copy->pixel_array_start = to_copy->pixel_array_start;
    copy->width = to_copy->width;
    copy->height = to_copy->height;
    copy->depth = to_copy->depth;

    copy->file_byte_contents = (unsigned char*) malloc(copy->file_byte_number * sizeof(unsigned char));

    unsigned int i;
    for (i = 0; i < copy->file_byte_number; i++)
    {
        copy->file_byte_contents[i] = to_copy->file_byte_contents[i];
    }

    copy->pixels = (pixel*) malloc(copy->width * copy->height * sizeof(pixel));

    unsigned int x, y;
    int index;
    for (y = 0; y < copy->height; y++)
    {
        for (x = 0; x < copy->width; x++)
        {
            index = y * copy->width + x;
            copy->pixels[index].red = to_copy->pixels[index].red;
            copy->pixels[index].green = to_copy->pixels[index].green;
            copy->pixels[index].blue = to_copy->pixels[index].blue;
            copy->pixels[index].alpha = to_copy->pixels[index].alpha;
        }
    }

    return copy;
}

int get_width(BMP* bmp)
{
    return bmp->width;
}

int get_height(BMP* bmp)
{
    return bmp->height;
}

unsigned int get_depth(BMP* bmp)
{
    return bmp->depth;
}

void get_pixel_rgb(BMP* bmp, int x, int y, unsigned char* r, unsigned char* g, unsigned char* b)
{
    int index = y * bmp->width + x;
    *r = bmp->pixels[index].red;
    *g = bmp->pixels[index].green;
    *b = bmp->pixels[index].blue;
}

void set_pixel_rgb(BMP* bmp, int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    int index = y * bmp->width + x;
    bmp->pixels[index].red = r;
    bmp->pixels[index].green = g;
    bmp->pixels[index].blue = b;
}

void bwrite(BMP* bmp, char* file_name)
{
    _map(bmp, _update_file_byte_contents);

    FILE* fp = fopen(file_name, "wb");
    fwrite(bmp->file_byte_contents, sizeof(char), bmp->file_byte_number, fp);
    fclose(fp);
}

void bclose(BMP* bmp)
{
    free(bmp->pixels);
    bmp->pixels = NULL;
    free(bmp->file_byte_contents);
    bmp->file_byte_contents = NULL;
    free(bmp);
    bmp = NULL;
}


// Private function implementations

void _throw_error(char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

unsigned int _get_int_from_buffer(unsigned int bytes, 
                                  unsigned int offset, 
                                  unsigned char* buffer)
{
    unsigned int value = 0;
    int i;

    for (i = bytes - 1; i >= 0; --i)
    {
        value <<= 8;
        value += buffer[i + offset];
    }

    return value;
}

unsigned int _get_file_byte_number(FILE* fp)
{
    unsigned int byte_number;
    fseek(fp, 0, SEEK_END);
    byte_number = ftell(fp);
    rewind(fp);
    return byte_number;
}

unsigned char* _get_file_byte_contents(FILE* fp, unsigned int file_byte_number)
{
    unsigned char* buffer = (unsigned char*) malloc(file_byte_number * sizeof(char));
    unsigned int result = fread(buffer, 1, file_byte_number, fp);

    if (result != file_byte_number)
    {
        _throw_error("There was a problem reading the file");
    }


    return buffer;
}

int _validate_file_type(unsigned char* file_byte_contents)
{
    return file_byte_contents[0] == 'B' && file_byte_contents[1] == 'M';
}

int _validate_depth(unsigned int depth)
{
    return depth == 24 || depth == 32;
}

unsigned int _get_pixel_array_start(unsigned char* file_byte_contents)
{
    return _get_int_from_buffer(PIXEL_ARRAY_START_BYTES, PIXEL_ARRAY_START_OFFSET, file_byte_contents);
}

int _get_width(unsigned char* file_byte_contents)
{
    return (int) _get_int_from_buffer(WIDTH_BYTES, WIDTH_OFFSET, file_byte_contents);
}

int _get_height(unsigned char* file_byte_contents)
{
    return (int) _get_int_from_buffer(HEIGHT_BYTES, HEIGHT_OFFSET, file_byte_contents);
}

unsigned int _get_depth(unsigned char* file_byte_contents)
{
    return _get_int_from_buffer(DEPTH_BYTES, DEPTH_OFFSET, file_byte_contents);
}

void _update_file_byte_contents(BMP* bmp, int index, int offset, int channel)
{
    char value;
    switch(channel)
    {
        case BLUE:
            value = bmp->pixels[index].blue;
            break;
        case GREEN:
            value = bmp->pixels[index].green;
            break;
        case RED:
            value = bmp->pixels[index].red;
            break;
        case ALPHA:
            value = bmp->pixels[index].alpha;
            break;
    }
    bmp->file_byte_contents[offset + channel] = value;
}

void _populate_pixel_array(BMP* bmp)
{
    bmp->pixels = (pixel*) malloc(bmp->width * bmp->height * sizeof(pixel));
    _map(bmp, _get_pixel);
}

void _map(BMP* bmp, void (*f)(BMP*, int, int, int))
{
    int channels = bmp->depth / (sizeof(unsigned char) * BITS_PER_BYTE);
    int row_size = ((int) (bmp->depth * bmp->width + 31) / 32) * 4;
    int padding = row_size - bmp->width * channels;

    int c;
    unsigned int x, y, index, offset;
    for (y = 0; y < bmp->height; y++)
    {
        for (x = 0; x < bmp->width; x++)
        {
            index = y * bmp->width + x;
            offset = bmp->pixel_array_start + index * channels + y * padding;
            for (c = 0; c < channels; c++)
            {
                (*f)(bmp, index, offset, c);
            }
        }
    }
}

void _get_pixel(BMP* bmp, int index, int offset, int channel)
{
    unsigned char value = _get_int_from_buffer(sizeof(unsigned char), offset + channel, bmp->file_byte_contents);
    switch(channel)
    {
        case BLUE:
            bmp->pixels[index].blue = value;
            break;
        case GREEN:
            bmp->pixels[index].green = value;
            break;
        case RED:
            bmp->pixels[index].red = value;
            break;
        case ALPHA:
            bmp->pixels[index].alpha = value;
            break;
    }
}
