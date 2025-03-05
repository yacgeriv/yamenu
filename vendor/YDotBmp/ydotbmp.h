#ifndef Y_DOT_BMP_FILE_H
#define Y_DOT_BMP_FILE_H

#pragma pack(push)
#pragma pack(2)
typedef struct {
    unsigned short int identifier;
    unsigned int size;
    unsigned short int reserved1;
    unsigned short int reserved2;
    unsigned int image_offset;
}YDB_BitMap_Header;
#pragma pack(pop)
typedef struct {
    unsigned int header_size;
    int width_in_pixels;
    int height_in_pixels;
    unsigned short int color_planes;
    unsigned short int bits_per_pixel;
    unsigned int compression;
    unsigned int image_size;
}YDB_BitMap;

typedef struct {
    unsigned char r,g,b;
}YDB_RGB_Channels;

typedef struct {
    YDB_RGB_Channels rgb_channels;
    int width, height;
    FILE *current_file;
}YDB_Image;


inline YDB_Image *ydb_load_from_file(const char *file_path, YDB_BitMap *bit_map, YDB_BitMap_Header *bit_map_header);
inline int ydb_read_to_mem(unsigned char **image_buffer, YDB_BitMap_Header *bit_map_header, YDB_Image *image);
inline void ydb_free_mem(unsigned char *image_buffer);
inline void ydb_free_image(YDB_Image *image);

#ifdef Y_DOT_BMP_IMPLEMENTATION_H

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

YDB_Image *ydb_load_from_file(const char *file_path, YDB_BitMap *bit_map, YDB_BitMap_Header *bit_map_header) {
    if (strlen(file_path) <= 0) {
        perror("invalid file\n");
    }

    FILE *file;
    YDB_Image* image = (YDB_Image*) malloc(sizeof(YDB_Image));
    
    file = fopen(file_path, "rb");
    
    fread(bit_map_header, sizeof(YDB_BitMap_Header), 1, file);

    fread(bit_map, sizeof(YDB_BitMap), 1, file);
    
    if (bit_map->compression != 0) {
        fclose(file);
        perror("unsupported bmp file\n");
    }
    
    image->width = bit_map->width_in_pixels;
    image->height = bit_map->height_in_pixels;
    image->current_file = file;
    printf("idk %d\n", bit_map_header->image_offset);
    return image;
}

int ydb_read_to_mem(unsigned char **image_buffer, YDB_BitMap_Header *bit_map_header, YDB_Image *image) {
    fseek(image->current_file, bit_map_header->image_offset, SEEK_SET);
    uint32_t nbytes = image->width * image->height * 3;
    *image_buffer = (unsigned char*) malloc(sizeof(unsigned char) * nbytes); 
    fread(image_buffer, sizeof(unsigned char), nbytes, image->current_file);
    printf("idk %d\n", bit_map_header->image_offset);
    for (uint32_t i = 0; i < nbytes; i += 3) {
        unsigned char tmp = *image_buffer[i];
        *image_buffer[i] = *image_buffer[i + 2];
        *image_buffer[i + 2] = tmp;
    }
    
    return 1;
}

void ydb_free_mem(unsigned char *image_buffer) {
    free(image_buffer);
}

void ydb_free_image(YDB_Image *image) {
    fclose(image->current_file);
    free(image);
}

#endif

#endif
