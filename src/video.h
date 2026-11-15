#ifndef PIX86_VIDEO_H
#define PIX86_VIDEO_H
#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 200
#define VIDEO_MAX_WIDTH 640
#define VIDEO_MAX_HEIGHT 480
void video_request(int hires);
int video_width(void);
int video_height(void);
int video_hires(void);
int video_failed(void);

void video_clip(int x, int y, int width, int height);
void video_unclip(void);
int video_init(void);
void video_shutdown(void);
void video_clear(unsigned char colour);
void video_pixel(int x, int y, unsigned char colour);
unsigned char video_read(int x, int y);
void video_rect(int x, int y, int width, int height, unsigned char colour);
/* Original 6x8 font in font.c; transparent ink, clipped on all edges. */
void video_text(int x, int y, const char *text, unsigned char colour);
void video_text_bg(int x, int y, const char *text, unsigned char ink, unsigned char paper);
void video_text_box(int x, int y, int width, int height, const char *text,
                    unsigned char ink, unsigned char paper);
#endif
