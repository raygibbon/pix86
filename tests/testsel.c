#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"
#include "command.h"
static Editor e;
static unsigned short queue[4096];
static unsigned char original[4096],masks[512],expected[4096],expected_mask[512];
static P86Image image;
static void fresh(unsigned width,unsigned height,int transparent)
{
    unsigned i;
    assert(editor_new_document(&e,width,height,transparent,141));
    for(i=0;i<width*height;++i) e.pixels[i]=(unsigned char)i;
    memset(e.mask,0,width*height/8);
    if(transparent) { e.mask[0]=2; e.mask[1]=128; }
    history_init(&e.history,&e.canvas); editor_mark_saved(&e);
    memcpy(original,e.pixels,width*height); memcpy(masks,e.mask,width*height/8);
}
static void pointer(int x,int y,unsigned buttons)
{
    editor_mouse(&e,e.view.x+x*e.view.zoom+e.view.zoom/2,
                 e.view.y+y*e.view.zoom+e.view.zoom/2,buttons);
}
static void rectangle(int x0,int y0,int x1,int y1)
{
    editor_key(&e,CMD_DESELECT); editor_key(&e,CMD_SELECT);
    pointer(x0,y0,1); pointer(x1,y1,1); pointer(x1,y1,0);
}
static void unchanged(void)
{
    assert(!memcmp(e.pixels,original,e.history.size));
    assert(!memcmp(e.mask,masks,e.history.size/8));
    assert(!editor_is_dirty(&e) && e.history.position==0);
}
static void geometry(void)
{
    int dir,tool,zoom;
    unsigned x,y,source;
    unsigned sides[6][2]={{16,16},{32,32},{64,64},{16,24},{16,32},{32,24}},i;
    for(i=0;i<6;++i) {
        fresh(sides[i][0],sides[i][1],0);
        for(tool=0;tool<TOOL_COUNT;++tool) {
            e.tool=tool; editor_key(&e,1);
            assert(e.selection.state==SEL_ACTIVE && e.selection.w==(int)e.canvas.width && e.selection.h==(int)e.canvas.height);
            unchanged(); editor_key(&e,4); assert(!e.selection.state); unchanged();
        }
        editor_key(&e,CMD_SELECT_ALL); editor_key(&e,CMD_MOVE);
        editor_key(&e,KEY_RIGHT); editor_key(&e,KEY_DOWN); editor_key(&e,CMD_APPLY);
        for(y=0;y<e.canvas.height;++y) for(x=0;x<e.canvas.width;++x) {
            source=(y?y-1:0)*e.canvas.width+(x?x-1:0);
            assert(e.pixels[y*e.canvas.width+x]==(x && y?original[source]:141));
        }
        assert(e.history.position==1); editor_key(&e,CMD_UNDO); unchanged();
        for(dir=0;dir<4;++dir) {
            rectangle(dir&1?9:2,dir&2?11:3,dir&1?2:9,dir&2?3:11);
            assert(e.selection.x==2 && e.selection.y==3 && e.selection.w==8 && e.selection.h==9);
            unchanged();
        }
        for(zoom=1;view_fit(&e.view,&e.canvas,0,zoom);++zoom) {
            rectangle(2,3,4,6); pointer(3,4,1); pointer(5,3,1); pointer(5,3,0);
            assert(selection_floating(&e) && e.selection.dx==2 && e.selection.dy==-1);
            unchanged();
            pointer(4,3,1); pointer(3,5,1); pointer(3,5,0);
            assert(e.selection.dx==1 && e.selection.dy==1); unchanged();
            editor_key(&e,CMD_EXIT); assert(!e.selection.state); unchanged();
        }
        view_fit(&e.view,&e.canvas,0,0);
        rectangle(2,3,-20,100);
        assert(e.selection.x==0 && e.selection.y==3 && e.selection.w==3 && e.selection.h==(int)e.canvas.height-3);
        unchanged();
        editor_key(&e,CMD_DESELECT); editor_key(&e,CMD_MOVE);
        assert(!selection_floating(&e)); pointer(2,2,1); pointer(3,3,0);
        assert(e.selection.state==SEL_ACTIVE && e.selection.w==2);
    }
    fresh(32,32,0); editor_key(&e,CMD_SELECT);
    pointer(2,3,2); pointer(2,3,0);
    assert(e.foreground==98 && !memcmp(e.pixels,original,1024) && e.history.position==0);
    for(tool=0;tool<TOOL_COUNT;++tool) {
        e.tool=0; pointer(-1,-1,0);
        editor_mouse(&e,TOOL_X+2,TOOL_Y+tool*TOOL_STEP+2,1);
        editor_mouse(&e,TOOL_X+2,TOOL_Y+tool*TOOL_STEP+2,0);
        assert(e.tool==tool);
        assert(toolbar_hit(TOOL_X+2,TOOL_Y+tool*TOOL_STEP+2)==tool);
    }
}
static void movement(int transparent)
{
    int x,y,alpha;
    unsigned char colour;
    unsigned pos;
    FileReport report;
    fresh(32,32,transparent); rectangle(0,0,7,7);
    editor_key(&e,CMD_MOVE); assert(selection_floating(&e));
    editor_key(&e,KEY_RIGHT); editor_key(&e,KEY_DOWN); editor_key(&e,KEY_RIGHT);
    assert(e.selection.dx==2 && e.selection.dy==1); unchanged();
    /* Preview values are identical to the eventual committed canvas. */
    for(y=0;y<32;++y) for(x=0;x<32;++x) {
        selection_sample(&e,x,y,&colour,&alpha); pos=(unsigned)y*32+x;
        expected[pos]=colour;
        if(alpha) expected_mask[pos/8]|=(unsigned char)(1U<<(pos%8));
        else expected_mask[pos/8]&=(unsigned char)~(1U<<(pos%8));
    }
    assert(expected[34]==0); /* opaque black, no colour-key assumption */
    if(transparent) assert(expected[35]==35); /* captured mask hole composites */
    editor_key(&e,CMD_PENCIL); assert(selection_floating(&e) && e.tool==TOOL_MOVE);
    editor_key(&e,CMD_FRAME_DUP); assert(e.document->frames==1 && selection_floating(&e));
    editor_key(&e,CMD_APPLY);
    assert(!selection_floating(&e) && e.history.position==1 && editor_is_dirty(&e));
    assert(!memcmp(expected,e.pixels,1024) && !memcmp(expected_mask,e.mask,128));
    editor_key(&e,CMD_UNDO); unchanged(); editor_key(&e,CMD_REDO);
    assert(!memcmp(expected,e.pixels,1024) && !memcmp(expected_mask,e.mask,128));
    editor_image(&e,&image); assert(file_save("SELECT.P86",&image,1,&report)==FILE_OK);
    assert(file_load("SELECT.P86",&image)==FILE_OK); assert(editor_load(&e,&image));
    assert(!memcmp(expected,e.pixels,1024) && !memcmp(expected_mask,e.mask,128));
    remove("SELECT.P86");
    fresh(32,32,transparent); editor_key(&e,CMD_SELECT_ALL); editor_key(&e,CMD_MOVE);
    editor_key(&e,CMD_APPLY); unchanged(); /* no-op */
    editor_key(&e,CMD_DESELECT); rectangle(0,0,3,3); editor_key(&e,CMD_MOVE);
    for(x=0;x<6;++x) editor_key(&e,KEY_LEFT);
    editor_key(&e,KEY_UP); unchanged(); editor_key(&e,CMD_APPLY);
    assert(e.history.position==1 && !e.selection.state);
    editor_key(&e,CMD_UNDO); unchanged();
    rectangle(2,3,4,6); editor_key(&e,CMD_FRAME_DELETE);
    assert(e.document->frames==1 && e.history.position==1);
    assert(transparent?canvas_transparent(&e.canvas,2,3):e.pixels[3*32+2]==141);
    editor_key(&e,CMD_UNDO); unchanged();
    for(x=0;x<12;++x) {
        rectangle(2,3,4,6); editor_key(&e,KEY_RIGHT); editor_key(&e,KEY_LEFT);
        editor_key(&e,CMD_APPLY); unchanged();
    }
}
int main(void)
{
    int mode;
    assert(editor_init(&e,32,queue));
    for(mode=0;mode<2;++mode) {
        layout_set(mode?640:320,mode?480:200);
        geometry(); movement(0); movement(1);
        assert(DRAW_X+DRAW_WIDTH<UI_RIGHT_X && TOOL_X>=UI_RIGHT_X);
        assert(TOOL_Y+5*TOOL_STEP+TOOL_HEIGHT<UI_STATUS_Y);
    }
    editor_dispose(&e);
    puts("PASS: selection rectangles, both layouts, floating preview, masks, clipping, cancel, atomic undo/redo and save/load.");
    return 0;
}
