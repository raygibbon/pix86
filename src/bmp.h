#ifndef PIX86_BMP_H
#define PIX86_BMP_H
#include <stdio.h>
/* Fill a bounded span of palette indices at top-down logical (x,y).
   Returns nonzero on success. No hardware or animation dependency. */
typedef int (*BmpRead)(const void *context,unsigned x,unsigned y,
                       unsigned char *pixels,unsigned count);
int bmp_size(unsigned width,unsigned height,unsigned long *bytes);
int bmp_write(FILE *stream,unsigned width,unsigned height,BmpRead read,
              const void *context);
#endif
