#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"
#include "command.h"
#include "project.h"
static Editor e;
static unsigned short queue[4096];
static P86Image im;
static Project project;
static unsigned char raw[16450];
#ifdef ALLOC_FAULTS
static int fail_at,allocation,live;
void ANIM_FAR *fault_alloc(size_t size)
{
    void ANIM_FAR *p;
    if(++allocation==fail_at) return NULL;
    p=anim_alloc(size); if(p) ++live; return p;
}
void fault_free(void ANIM_FAR *p) { if(p) --live; anim_free(p); }
static void failures(void)
{
    int i;
    for(i=1;i<=7;++i) {
        fail_at=i; allocation=0;
        assert(!editor_init(&e,32,queue)); assert(!live);
    }
    fail_at=0; allocation=0;
    assert(editor_init(&e,32,queue)); assert(live==7);
    editor_dispose(&e); assert(!live);
}
#endif
static void edit(unsigned char colour)
{
    e.canvas.pixels[0]=colour;
    assert(history_commit(&e.history,&e.canvas));
}
static void model(void)
{
    unsigned i,pos;
    assert(editor_init(&e,32,queue));
    assert(!editor_is_dirty(&e));
    edit(201);
    editor_key(&e,CMD_FRAME_DUP);
    assert(e.document->frames==2 && e.history.active==1 && e.pixels[0]==201);
    edit(255);
    editor_select(&e,0); pos=e.history.position;
    editor_select(&e,1); assert(e.history.position==pos);
    editor_key(&e,CMD_UNDO);
    assert(e.history.active==1 && e.pixels[0]==201);
    editor_key(&e,CMD_REDO); assert(e.pixels[0]==255);
    editor_mark_saved(&e); assert(!editor_is_dirty(&e));
    editor_select(&e,0); assert(!editor_is_dirty(&e));
    editor_key(&e,CMD_SLOWER); assert(editor_is_dirty(&e));
    editor_key(&e,CMD_UNDO); assert(!editor_is_dirty(&e));
    editor_key(&e,CMD_FRAME_ADD); assert(e.pixels[0]==0 && e.document->frames==3);
    editor_key(&e,CMD_FRAME_DELETE); assert(e.document->frames==2);
    editor_key(&e,CMD_UNDO); assert(e.document->frames==3 && e.pixels[0]==0);
    editor_key(&e,CMD_REDO); assert(e.document->frames==2);
    editor_select(&e,0); editor_key(&e,CMD_PLAY);
    assert(!editor_tick(&e,99));
    assert(editor_tick(&e,1)&EDIT_CANVAS); assert(e.history.active==1 && e.pixels[0]==255);
    assert(editor_tick(&e,100)&EDIT_CANVAS); assert(e.history.active==0 && e.pixels[0]==201);
    assert(!editor_tick(&e,60000)); assert(e.history.active==0);
    editor_key(&e,CMD_PLAY); assert(!e.playing);
    editor_mark_saved(&e); assert(!editor_is_dirty(&e));
    editor_key(&e,CMD_TIMELINE); assert(e.view.zoom==4);
    assert(e.view.y+(int)e.view.height*e.view.zoom<TIMELINE_Y);
    editor_key(&e,CMD_TIMELINE); assert(e.view.zoom==5 && !editor_is_dirty(&e));
    assert(editor_new_shape(&e,64,64));
    for(i=1;i<4;++i) editor_key(&e,CMD_FRAME_ADD);
    assert(e.document->frames==4);
    pos=e.history.position; editor_key(&e,CMD_FRAME_ADD);
    assert(e.document->frames==4 && e.history.position==pos);
    assert(editor_new_shape(&e,16,24)); assert(e.canvas.height==24 && e.view.zoom==7);
    editor_key(&e,CMD_TIMELINE); assert(e.view.zoom==5);
    for(i=1;i<16;++i) editor_key(&e,CMD_FRAME_DUP);
    assert(e.document->frames==16 && e.first_frame==7);
    editor_select(&e,0); assert(!e.first_frame);
    for(i=1;i<16;++i) editor_key(&e,CMD_FRAME_DELETE);
    editor_key(&e,CMD_FRAME_DELETE); assert(e.document->frames==1);
    editor_dispose(&e);
}
static int decode(unsigned length)
{
    FILE *f;
    int result;
    f=tmpfile(); assert(f!=NULL);
    assert(fwrite(raw,1,length,f)==length); rewind(f);
    result=file_decode(f,&im); assert(!fclose(f)); return result;
}
static void files(void)
{
    FILE *f;
    unsigned i,length;
    assert(editor_init(&e,32,queue));
    assert(editor_new_shape(&e,16,24));
    edit(252); editor_key(&e,CMD_FRAME_DUP); edit(1); editor_key(&e,CMD_SLOWER);
    editor_image(&e,&im);
    f=tmpfile(); assert(f!=NULL); assert(file_encode(f,&im)==FILE_OK);
    rewind(f); length=(unsigned)fread(raw,1,sizeof(raw),f); assert(!fclose(f));
    assert(length==24+2*(384+2)); assert(raw[4]==2 && raw[20]==2);
    for(i=0;i<length;++i) assert(decode(i)==FILE_LENGTH);
    assert(decode(length)==FILE_OK); assert(im.frames==2 && im.height==24);
    assert(im.duration[1]==110 && im.pixels[0]==252 && im.pixels[384]==1);
    raw[length]=0; assert(decode(length+1)==FILE_LENGTH);
    raw[20]=0; assert(decode(length)==FILE_LENGTH); raw[20]=2;
    raw[24]=0; assert(decode(length)==FILE_HEADER); raw[24]=100;
    raw[22]=1; assert(decode(length)==FILE_HEADER); raw[22]=0;
    raw[18]=1; assert(decode(length)==FILE_LENGTH); raw[18]=0;
    raw[4]=3; assert(decode(length)==FILE_VERSION); raw[4]=2;
    project_init(&project,&e,&im);
    remove("ANIM.P86"); assert(project_save(&project,"ANIM",0)==FILE_OK);
    assert(!editor_is_dirty(&e)); edit(55);
    f=fopen("BADANIM.P86","wb"); assert(f!=NULL); assert(fwrite(raw,1,length-1,f)==length-1); assert(!fclose(f));
    i=e.history.position;
    assert(project_open(&project,"BADANIM")==FILE_LENGTH);
    assert(e.pixels[0]==55 && editor_is_dirty(&e) && e.history.position==i && !strcmp(project.filename,"ANIM.P86"));
    assert(project_open(&project,"ANIM")==FILE_OK);
    assert(!editor_is_dirty(&e) && e.document->frames==2 && e.pixels[0]==252);
    assert(e.document->duration[1]==110);
    remove("ANIM.P86"); remove("BADANIM.P86"); editor_dispose(&e);
}
int main(void)
{
#ifdef ALLOC_FAULTS
    failures();
#endif
    model(); files();
    puts("PASS: animation structure, frame history, timing, geometry, v2 validation and preservation.");
    return 0;
}
