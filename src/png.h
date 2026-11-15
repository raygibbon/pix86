#ifndef PIX86_PNG_H
#define PIX86_PNG_H
#include <stdio.h>
/* Fill count RGBA pixels (count <=32), top-down, 8-bit straight alpha. */
typedef int (*PngRead)(const void *context,unsigned x,unsigned y,unsigned char *rgba,unsigned count);
int png_size(unsigned width,unsigned height,unsigned long *bytes);
int png_write(FILE *stream,unsigned width,unsigned height,PngRead read,const void *context);
#endif
