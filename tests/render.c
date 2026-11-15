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
static unsigned char screen[640*480];
static int clip_x,clip_y,clip_r=320,clip_b=200;
void video_clip(int x,int y,int w,int h) { clip_x=x;clip_y=y;clip_r=x+w;clip_b=y+h; }
void video_unclip(void) { clip_x=clip_y=0;clip_r=UI_WIDTH;clip_b=UI_HEIGHT; }
void cursor_protect(int x,int y,int w,int h) { (void)x;(void)y;(void)w;(void)h; }

void video_rect(int x, int y, int w, int h, unsigned char colour)
{
    int xx, yy;
    for (yy = y; yy < y + h; ++yy)
        for (xx = x; xx < x + w; ++xx)
            if (xx >= clip_x && yy >= clip_y && xx < clip_r && yy < clip_b)
                screen[yy * UI_WIDTH + xx] = colour;
}

void video_pixel(int x,int y,unsigned char colour)
{
    if(x>=clip_x && y>=clip_y && x<clip_r && y<clip_b) screen[y*UI_WIDTH+x]=colour;
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

static void transparency(void)
{
    int x,y,z,sx,sy;
    unsigned char before[384];
    editor_dispose(&editor); assert(editor_init(&editor,16,queue));
    assert(editor_new_shape(&editor,16,24));
    memcpy(before,editor.pixels,384); editor.mask[0]=1;
    editor_mark_saved(&editor); sx=editor.view.x; sy=editor.view.y; z=editor.view.zoom;
    render_canvas(&editor);
    for(y=sy;y<sy+z;++y) for(x=sx;x<sx+z;++x) assert(screen[y*320+x]==7 || screen[y*320+x]==8);
    editor.checker=0; editor.preview_colour=11; render_canvas(&editor);
    for(y=sy;y<sy+z;++y) for(x=sx;x<sx+z;++x) assert(screen[y*320+x]==11);
    editor.grid=1; render_canvas(&editor);
    assert(screen[(sy+1)*320+sx+1]==11 && screen[(sy+1)*320+sx+z-1]!=11);
    editor.grid=0; editor.preview_link=1; assert(editor_background(&editor,5)&EDIT_CANVAS); render_canvas(&editor);
    assert(screen[(sy+1)*320+sx+1]==5);
    assert(screen[(sy+1)*320+sx+z+1]==0); /* opaque black remains black */
    assert(!memcmp(before,editor.pixels,384) && editor.mask[0]==1);
    assert(editor_is_dirty(&editor)); editor_mark_saved(&editor);
    editor_key(&editor,CMD_CHECKER); editor_key(&editor,CMD_GUIDE8); editor_key(&editor,CMD_GUIDE16);
    assert(editor.guide==3 && !editor_is_dirty(&editor));
    editor_key(&editor,CMD_GUIDE8); assert(editor.guide==2);
}
static void selections(void)
{
    int mode,z,sx,sy,x,y,seen_black,seen_white;
    for(mode=0;mode<2;++mode) {
        layout_set(mode?640:320,mode?480:200); video_unclip();
        assert(editor_new_document(&editor,32,32,0,141));
        memset(editor.pixels,8,1024); editor.pixels[33]=0; editor.tool=TOOL_PENCIL;
        render_ui(&editor);
        assert(screen[TOOL_Y*UI_WIDTH+TOOL_X]==UI_WHITE);
        editor_key(&editor,CMD_SELECT_ALL);
        for(z=1;view_fit(&editor.view,&editor.canvas,0,z);++z) {
            render_canvas(&editor); sx=editor.view.x; sy=editor.view.y;
            seen_black=seen_white=0;
            for(x=sx;x<sx+32*z;++x) {
                if(screen[sy*UI_WIDTH+x]==0) seen_black=1;
                if(screen[sy*UI_WIDTH+x]==15) seen_white=1;
            }
            assert(seen_black && seen_white);
        }
        view_fit(&editor.view,&editor.canvas,0,0); z=editor.view.zoom;
        editor_key(&editor,CMD_MOVE); editor_key(&editor,KEY_RIGHT);
        render_canvas(&editor); sx=editor.view.x; sy=editor.view.y;
        assert(screen[(sy+z+1)*UI_WIDTH+sx+2*z+1]==0);
        /* Pure selection redraw touches only the dirty canvas footprint. */
        for(y=0;y<UI_HEIGHT;++y) for(x=UI_RIGHT_X;x<UI_WIDTH;++x)
            screen[y*UI_WIDTH+x]=231;
        editor_key(&editor,KEY_DOWN); render_stroke(&editor);
        for(y=0;y<UI_HEIGHT;++y) for(x=UI_RIGHT_X;x<UI_WIDTH;++x)
            assert(screen[y*UI_WIDTH+x]==231);
        editor_key(&editor,CMD_EXIT);
    }
    layout_set(320,200); video_unclip();
}
int main(void)
{
    test_render(16);
    test_render(32);
    test_render(64); guides(); transparency(); selections();
    puts("PASS: canvas rendering at every zoom, grids and all palette indices.");
    editor_dispose(&editor);
    return 0;
}
