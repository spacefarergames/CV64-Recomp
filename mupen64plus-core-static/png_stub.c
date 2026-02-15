/* png_stub.c - Stub implementations for libpng functions */
#include <stddef.h>

typedef void* png_structp;
typedef void* png_infop;
typedef void** png_bytepp;

void png_write_png(png_structp png_ptr, png_infop info_ptr, int transforms, void* params) { }
void png_set_rows(png_structp png_ptr, png_infop info_ptr, png_bytepp row_pointers) { }
void* png_jmpbuf(png_structp png_ptr) { return NULL; }
