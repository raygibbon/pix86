#ifndef PIX86_VIDEO_H
#define PIX86_VIDEO_H
#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 200

int video_init(void);
void video_shutdown(void);
void video_clear(unsigned char colour);
void video_pixel(int x, int y, unsigned char colour);
unsigned char video_read(int x, int y);
void video_rect(int x, int y, int width, int height, unsigned char colour);
/* Uses the machine's BIOS 8x8 font; no bundled font assets. */
void video_text(int x, int y, const char *text, unsigned char colour);
#endif
