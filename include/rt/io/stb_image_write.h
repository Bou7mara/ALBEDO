/* stb_image_write - v1.16 - public domain - http://nothings.org/stb
   writes out PNG/BMP/TGA/JPEG/HDR images to C stdio - Sean Barrett 2010-2015
                                     no warranty implied; use at your own risk

   Before #including,

       #define STB_IMAGE_WRITE_IMPLEMENTATION

   in the file that you want to have the implementation.
*/

#ifndef INCLUDE_STB_IMAGE_WRITE_H
#define INCLUDE_STB_IMAGE_WRITE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if defined(STB_IMAGE_WRITE_IMPLEMENTATION_FILE)
#error "STB_IMAGE_WRITE_IMPLEMENTATION_FILE is deprecated"
#endif

#ifndef STBIINCLUDE_STB_IMAGE_WRITE_H
#define STBIINCLUDE_STB_IMAGE_WRITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STBIWDEF
#ifdef STB_IMAGE_WRITE_STATIC
#define STBIWDEF static
#else
#define STBIWDEF extern
#endif
#endif

#ifndef STBIW_UCHAR
#define STBIW_UCHAR unsigned char
#define STBIW_UINT32 unsigned int
#endif

typedef void stbi_write_func(void *context, void *data, int size);

#ifndef STBI_WRITE_NO_STDIO
STBIWDEF int stbi_write_png(char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes);
STBIWDEF int stbi_write_bmp(char const *filename, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_tga(char const *filename, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);
STBIWDEF int stbi_write_jpg(char const *filename, int x, int y, int comp, const void  *data, int quality);
#endif

STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
STBIWDEF int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_tga_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
STBIWDEF int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const float *data);
STBIWDEF int stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void  *data, int quality);

STBIWDEF void stbi_flip_vertically_on_write(int flag);

#ifdef __cplusplus
}
#endif

#endif // STBIINCLUDE_STB_IMAGE_WRITE_H

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz)        malloc(sz)
#define STBIW_REALLOC(sz,newsz) realloc(sz,newsz)
#define STBIW_FREE(p)           free(p)
#endif
#ifndef STBIW_MEMMOVE
#define STBIW_MEMMOVE(a,b,sz)   memmove(a,b,sz)
#endif
#ifndef STBIW_ASSERT
#include <assert.h>
#define STBIW_ASSERT(x)         assert(x)
#endif

#define STBIW_UCHAR unsigned char
#define STBIW_UINT32 unsigned int

typedef struct
{
   stbi_write_func *func;
   void *context;
   unsigned char buffer[64];
   int buf_used;
   unsigned int crc;
} stbi__write_context;

static void stbi__start_write_callbacks(stbi__write_context *s, stbi_write_func *c, void *context)
{
   s->func    = c;
   s->context = context;
}

#ifndef STBI_WRITE_NO_STDIO
static void stbi__stdio_write(void *context, void *data, int size)
{
   fwrite(data,1,size,(FILE *) context);
}

static int stbi__start_write_file(stbi__write_context *s, const char *filename)
{
   FILE *f;
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
   if (fopen_s(&f, filename, "wb") != 0)
      f = NULL;
#else
   f = fopen(filename, "wb");
#endif
   if (!f) return 0;
   stbi__start_write_callbacks(s, stbi__stdio_write, (void *) f);
   return 1;
}

static void stbi__end_write_file(stbi__write_context *s)
{
   fclose((FILE *)s->context);
}
#endif // STBI_WRITE_NO_STDIO

typedef unsigned int stbiw_uint32;
typedef unsigned short stbiw_uint16;

static void stbiw__writebv(stbi__write_context *s, unsigned char val)
{
   s->func(s->context, &val, 1);
}

static void stbiw__write1(stbi__write_context *s, unsigned char a)
{
   stbiw__writebv(s, a);
}

static void stbiw__write3(stbi__write_context *s, unsigned char a, unsigned char b, unsigned char c)
{
   stbiw__write1(s, a);
   stbiw__write1(s, b);
   stbiw__write1(s, c);
}

static void stbiw__write4(stbi__write_context *s, stbiw_uint32 a)
{
   stbiw__write1(s, (unsigned char)(a >> 24));
   stbiw__write1(s, (unsigned char)(a >> 16));
   stbiw__write1(s, (unsigned char)(a >>  8));
   stbiw__write1(s, (unsigned char)(a      ));
}

static int stbi__flip_vertically_on_write = 0;

STBIWDEF void stbi_flip_vertically_on_write(int flag)
{
   stbi__flip_vertically_on_write = flag;
}

// ---- CRC32 ----
static stbiw_uint32 stbiw__crc_table[256];

static void stbiw__make_crc_table(void)
{
   stbiw_uint32 c;
   int n, k;
   for (n = 0; n < 256; n++) {
      c = (stbiw_uint32) n;
      for (k = 0; k < 8; k++) {
         if (c & 1) c = 0xedb88320L ^ (c >> 1);
         else c = c >> 1;
      }
      stbiw__crc_table[n] = c;
   }
}

static stbiw_uint32 stbiw__crc32(unsigned char *buffer, int len)
{
   static int table_inited = 0;
   stbiw_uint32 c = 0xffffffffL;
   int n;
   if (!table_inited) {
      stbiw__make_crc_table();
      table_inited = 1;
   }
   for (n = 0; n < len; n++) {
      c = stbiw__crc_table[(c ^ buffer[n]) & 0xff] ^ (c >> 8);
   }
   return c ^ 0xffffffffL;
}

static void stbiw__putc(stbi__write_context *s, unsigned char c)
{
   s->buffer[s->buf_used++] = c;
   if (s->buf_used == 64) {
      s->crc = stbiw__crc32(s->buffer, 64);
   }
}

// ZLIB uncompressed / fast deflate implementation for PNG
static stbiw_uint32 stbiw__adler32(unsigned char *data, int len)
{
   stbiw_uint32 s1 = 1, s2 = 0;
   int i;
   for (i = 0; i < len; ++i) {
      s1 = (s1 + data[i]) % 65521;
      s2 = (s2 + s1) % 65521;
   }
   return (s2 << 16) + s1;
}

static void stbiw__write_chunk(stbi__write_context *s, char const tag[4], unsigned int len, unsigned char *data)
{
   stbiw_uint32 c;
   stbiw__write4(s, len);
   s->func(s->context, (void *) tag, 4);
   c = stbiw__crc32((unsigned char *) tag, 4);
   if (len > 0) {
      s->func(s->context, data, len);
      // calculate CRC over tag + data
      // For simplicity in single buffer chunk:
      unsigned char *tag_and_data = (unsigned char *) STBIW_MALLOC(len + 4);
      memcpy(tag_and_data, tag, 4);
      memcpy(tag_and_data + 4, data, len);
      c = stbiw__crc32(tag_and_data, len + 4);
      STBIW_FREE(tag_and_data);
   }
   stbiw__write4(s, c);
}

static unsigned char *stbiw__zlib_compress(unsigned char *data, int data_len, int *out_len, int quality)
{
   (void)quality;
   // We output standard zlib stream storing uncompressed blocks (deflate type 00)
   int num_blocks = (data_len + 65534) / 65535;
   int raw_len = 2 + num_blocks * 5 + data_len + 4;
   unsigned char *out = (unsigned char *) STBIW_MALLOC(raw_len);
   int pos = 0;

   // zlib header
   out[pos++] = 0x78; // CMF: deflate, window size 32k
   out[pos++] = 0x01; // FLG: FCHECK

   int bytes_left = data_len;
   int src_pos = 0;
   while (bytes_left > 0) {
      int block_len = (bytes_left > 65535) ? 65535 : bytes_left;
      unsigned char is_last = (bytes_left == block_len) ? 1 : 0;
      out[pos++] = is_last; // BFINAL, BTYPE=00 (uncompressed)
      out[pos++] = (unsigned char)(block_len & 0xff);
      out[pos++] = (unsigned char)((block_len >> 8) & 0xff);
      out[pos++] = (unsigned char)((~block_len) & 0xff);
      out[pos++] = (unsigned char)(((~block_len) >> 8) & 0xff);
      memcpy(out + pos, data + src_pos, block_len);
      pos += block_len;
      src_pos += block_len;
      bytes_left -= block_len;
   }

   stbiw_uint32 adler = stbiw__adler32(data, data_len);
   out[pos++] = (unsigned char)((adler >> 24) & 0xff);
   out[pos++] = (unsigned char)((adler >> 16) & 0xff);
   out[pos++] = (unsigned char)((adler >> 8) & 0xff);
   out[pos++] = (unsigned char)(adler & 0xff);

   *out_len = pos;
   return out;
}

STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int stride_bytes)
{
   stbi__write_context s;
   stbi__start_write_callbacks(&s, func, context);
   
   if (x <= 0 || y <= 0 || comp < 1 || comp > 4) return 0;
   if (stride_bytes == 0) stride_bytes = x * comp;

   // PNG Header
   static unsigned char png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
   s.func(s.context, png_sig, 8);

   // IHDR
   unsigned char ihdr[13];
   ihdr[0] = (unsigned char)(x >> 24); ihdr[1] = (unsigned char)(x >> 16);
   ihdr[2] = (unsigned char)(x >>  8); ihdr[3] = (unsigned char)(x      );
   ihdr[4] = (unsigned char)(y >> 24); ihdr[5] = (unsigned char)(y >> 16);
   ihdr[6] = (unsigned char)(y >>  8); ihdr[7] = (unsigned char)(y      );
   ihdr[8] = 8; // bit depth
   // color type: 1=palette, 2=RGB, 4=grayscale+alpha, 6=RGBA
   static unsigned char ctype[5] = { 0, 0, 4, 2, 6 };
   ihdr[9] = ctype[comp];
   ihdr[10] = 0; // compression
   ihdr[11] = 0; // filter method
   ihdr[12] = 0; // interlace method
   stbiw__write_chunk(&s, "IHDR", 13, ihdr);

   // IDAT
   // Filter byte (0 = None) at beginning of each scanline
   int line_bytes = x * comp;
   int raw_len = (line_bytes + 1) * y;
   unsigned char *raw_data = (unsigned char *) STBIW_MALLOC(raw_len);
   if (!raw_data) return 0;

   int i;
   for (i = 0; i < y; ++i) {
      int row = stbi__flip_vertically_on_write ? (y - 1 - i) : i;
      raw_data[i * (line_bytes + 1)] = 0; // filter type 0
      memcpy(raw_data + i * (line_bytes + 1) + 1, (unsigned char *)data + row * stride_bytes, line_bytes);
   }

   int zlen = 0;
   unsigned char *zdata = stbiw__zlib_compress(raw_data, raw_len, &zlen, 8);
   STBIW_FREE(raw_data);

   if (zdata) {
      stbiw__write_chunk(&s, "IDAT", zlen, zdata);
      STBIW_FREE(zdata);
   } else {
      return 0;
   }

   // IEND
   stbiw__write_chunk(&s, "IEND", 0, NULL);
   return 1;
}

#ifndef STBI_WRITE_NO_STDIO
STBIWDEF int stbi_write_png(char const *filename, int x, int y, int comp, const void *data, int stride_bytes)
{
   stbi__write_context s;
   if (stbi__start_write_file(&s, filename)) {
      int r = stbi_write_png_to_func(s.func, s.context, x, y, comp, data, stride_bytes);
      stbi__end_write_file(&s);
      return r;
   }
   return 0;
}

STBIWDEF int stbi_write_bmp(char const *filename, int x, int y, int comp, const void *data) { (void)filename; (void)x; (void)y; (void)comp; (void)data; return 0; }
STBIWDEF int stbi_write_tga(char const *filename, int x, int y, int comp, const void *data) { (void)filename; (void)x; (void)y; (void)comp; (void)data; return 0; }
STBIWDEF int stbi_write_hdr(char const *filename, int x, int y, int comp, const float *data) { (void)filename; (void)x; (void)y; (void)comp; (void)data; return 0; }
STBIWDEF int stbi_write_jpg(char const *filename, int x, int y, int comp, const void *data, int quality) { (void)filename; (void)x; (void)y; (void)comp; (void)data; (void)quality; return 0; }
#endif

STBIWDEF int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data) { (void)func; (void)context; (void)x; (void)y; (void)comp; (void)data; return 0; }
STBIWDEF int stbi_write_tga_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data) { (void)func; (void)context; (void)x; (void)y; (void)comp; (void)data; return 0; }
STBIWDEF int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const float *data) { (void)func; (void)context; (void)x; (void)y; (void)comp; (void)data; return 0; }
STBIWDEF int stbi_write_jpg_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int quality) { (void)func; (void)context; (void)x; (void)y; (void)comp; (void)data; (void)quality; return 0; }

#endif // STB_IMAGE_WRITE_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif // INCLUDE_STB_IMAGE_WRITE_H
