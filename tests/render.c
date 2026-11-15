#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"
#include "command.h"
#include "render.h"
#include "video.h"
#include "theme.h"
#include "workpal.h"

static Editor editor;
static unsigned short queue[4096];
static unsigned char screen[64000];
static int clip_x,clip_y,clip_r=320,clip_b=200;
void video_clip(int x,int y,int w,int h) { clip_x=x;clip_y=y;clip_r=x+w;clip_b=y+h; }
void video_unclip(void) { clip_x=clip_y=0;clip_r=320;clip_b=200; }
void cursor_protect(int x,int y,int w,int h) { (void)x;(void)y;(void)w;(void)h; }

void video_rect(int x, int y, int w, int h, unsigned char colour)
{
    int xx, yy;
    for (yy = y; yy < y + h; ++yy)
        for (xx = x; xx < x + w; ++xx)
            if (xx >= clip_x && yy >= clip_y && xx < clip_r && yy < clip_b)
                screen[yy * 320 + xx] = colour;
}

void video_pixel(int x,int y,unsigned char colour)
{
    if(x>=clip_x && y>=clip_y && x<clip_r && y<clip_b) screen[y*320+x]=colour;
}

void video_text(int x, int y, const char *text, unsigned char colour)
{
    (void)x; (void)y; (void)colour;
    assert(strlen(text) <= 53);
}

void video_text_bg(int x,int y,const char *text,unsigned char ink,unsigned char paper)
{ (void)paper; video_text(x,y,text,ink); }
static void test_render(unsigned side)
{
    unsigned x, y, index;
    int zoom, sx, sy;
    editor_dispose(&editor);
    assert(editor_init(&editor, side, queue));
    for (y = 0; y < side; ++y)
        for (x = 0; x < side; ++x)
            canvas_set(&editor.canvas, x, y, (unsigned char)(y * side + x));
    for (zoom = view_max_zoom(&editor.canvas); zoom >= 1; --zoom) {
        view_set(&editor.view, &editor.canvas, zoom);
        render_canvas(&editor);
        for (y = 0; y < side; ++y)
            for (x = 0; x < side; ++x) {
                sx = editor.view.x + (int)x * zoom;
                sy = editor.view.y + (int)y * zoom;
                assert(screen[sy * 320 + sx] == (unsigned char)(y * side + x));
                if (zoom > 1)
                    assert(screen[(sy + zoom - 1) * 320 + sx + zoom - 1] == (unsigned char)(y * side + x));
            }
        assert(screen[DRAW_Y * 320 + DRAW_X] == UI_DESK); /* old zoom cleared */
    }
    editor.grid = 1;
    for(zoom=1;zoom<=view_max_zoom(&editor.canvas);++zoom) {
        view_set(&editor.view,&editor.canvas,zoom); render_canvas(&editor);
        sx=editor.view.x; sy=editor.view.y;
        assert(screen[sy*320+sx]==0);
        if(zoom>=3) {
            assert(screen[(sy+1)*320+sx+zoom-1]==3); /* subdued grey on black */
            assert(screen[(sy+zoom)*320+sx+zoom-1]>=3 && screen[(sy+zoom)*320+sx+zoom-1]<=12);
        }
        for(y=0;y<side;++y) for(x=0;x<side;++x)
            assert(editor.canvas.pixels[y*side+x]==(unsigned char)(y*side+x));
    }
    editor.grid = 0;
    view_set(&editor.view, &editor.canvas, view_max_zoom(&editor.canvas));
    render_canvas(&editor);
    sx = editor.view.x; sy = editor.view.y;
    zoom = editor.view.zoom;
    assert(screen[(sy + zoom - 1) * 320 + sx + zoom - 1] == 0);
    render_ui(&editor);
    for (index = 0; index < 16; ++index) {
        sx = PALETTE_X + (int)(index % PALETTE_COLS) * PALETTE_CELL_W + 1;
        sy = PALETTE_Y + (int)(index / PALETTE_COLS) * PALETTE_CELL_H + 1;
        assert(screen[sy * 320 + sx] == workpal_colour((int)index));
    }
    render_status(&editor, 319, 199, 1);
    render_status(&editor, editor.view.x, editor.view.y, 1);
    render_status(&editor, 0, 0, 0);
    sx = editor.view.x + zoom;
    sy = editor.view.y + zoom;
    assert(editor_mouse(&editor, sx, sy, EDIT_LEFT) & EDIT_STROKE);
    assert(editor.dirty_x0 == 1 && editor.dirty_x1 == 1);
    assert(editor.dirty_y0 == 1 && editor.dirty_y1 == 1);
    render_stroke(&editor);
    assert(screen[sy * 320 + sx] == editor.foreground);
    assert(screen[editor.view.y * 320 + editor.view.x] == 0);
    editor.grid = 1;
    editor.canvas.pixels[0]=15;
    render_canvas(&editor);
    if(zoom>=3) assert(screen[(editor.view.y+1)*320+editor.view.x+zoom-1]==12);
    assert(editor.canvas.pixels[0]==15);
    render_canvas(&editor);
    editor.foreground = 255;
    editor_mouse(&editor, sx, sy, 0);
    editor_mouse(&editor, sx, sy, EDIT_LEFT);
    render_stroke(&editor);
    assert(screen[sy * 320 + sx] == 255);
    if(zoom>=3) assert(screen[(sy+1)*320+sx+zoom-1] != 255);
}

static void guides(void)
{
    unsigned char before[4096];
    int sx,sy,z;
    unsigned i;
    editor_dispose(&editor); assert(editor_init(&editor,32,queue));
    for(i=0;i<1024;++i) editor.pixels[i]=(unsigned char)i;
    memcpy(before,editor.pixels,1024); z=editor.view.zoom;
    editor_key(&editor,CMD_GUIDE8); render_canvas(&editor);
    sx=editor.view.x+8*z-1; sy=editor.view.y;
    assert(screen[sy*320+sx]!=editor.pixels[7]);
    assert(screen[(sy+1)*320+sx]!=editor.pixels[7]);
    assert(!memcmp(before,editor.pixels,1024));
    editor_key(&editor,CMD_GUIDE16); render_canvas(&editor);
    sx=editor.view.x+16*z-1;
    assert(screen[sy*320+sx]!=editor.pixels[15]);
    assert(!memcmp(before,editor.pixels,1024));
}

int main(void)
{
    test_render(16);
    test_render(32);
    test_render(64); guides();
    puts("PASS: canvas rendering at every zoom, grids and all palette indices.");
    editor_dispose(&editor);
    return 0;
}
