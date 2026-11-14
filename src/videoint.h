#ifndef PIX86_VIDEOINT_H
#define PIX86_VIDEOINT_H

int videovga_start(void);
void videovga_pixel(int x, int y, unsigned char colour);
unsigned char videovga_read(int x, int y);
void videovga_rect(int x, int y, int width, int height, unsigned char colour);

int videovbe_start(unsigned char restore_mode);
void videovbe_stop(void);
int videovbe_failed(void);
void videovbe_pixel(int x, int y, unsigned char colour);
unsigned char videovbe_read(int x, int y);
void videovbe_rect(int x, int y, int width, int height, unsigned char colour);

#endif
