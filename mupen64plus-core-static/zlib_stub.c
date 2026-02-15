/* zlib_stub.c - Stub implementations for zlib functions */
/* These are minimal implementations to satisfy linker requirements */
/* Full zlib functionality requires proper zlib library */

#include <stdlib.h>

typedef void* gzFile;
typedef unsigned long uLong;
typedef unsigned int uInt;

/* Compression stubs */
int deflateInit2_(void* strm, int level, int method, int windowBits, int memLevel, int strategy, const char* version, int stream_size) { return -1; }
int deflate(void* strm, int flush) { return -1; }
int deflateEnd(void* strm) { return 0; }

/* Decompression stubs */
int inflateInit2_(void* strm, int windowBits, const char* version, int stream_size) { return -1; }
int inflate(void* strm, int flush) { return -1; }
int inflateEnd(void* strm) { return 0; }

/* CRC stubs */
uLong crc32(uLong crc, const unsigned char* buf, uInt len) { return 0; }
const uLong* get_crc_table(void) { static uLong table[256] = {0}; return table; }

/* gzip file stubs */
gzFile gzopen_w(const wchar_t* path, const char* mode) { return NULL; }
int gzclose(gzFile file) { return 0; }
int gzread(gzFile file, void* buf, unsigned len) { return -1; }
int gzwrite(gzFile file, const void* buf, unsigned len) { return 0; }
