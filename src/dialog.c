#include <string.h>
#include <dos.h>
#include "dialog.h"
#include "render.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "cursor.h"
#include "font.h"
#include "theme.h"
#include "uipaint.h"
#include "workpal.h"
#include "layout.h"
#include <stdio.h>
static int has_mouse;
static unsigned previous_buttons;
static const Editor *underneath;
void dialog_mouse(int available) { has_mouse=available; }
void dialog_editor(const Editor *editor) { underneath=editor; }
static int hit(const Rect *r,const MouseState *m)
{ return m->x>=r->x && m->x<r->x+r->w && m->y>=r->y && m->y<r->y+r->h; }
static void protect(const Rect *r) { cursor_protect(r->x,r->y,r->w,r->h); }
static int finish(const DialogLayout *l,int value)
{
    if(underneath) render_restore(underneath,&l->box);
    return value;
}
static void begin(const DialogLayout *l,const char *title,const char *body,const char *hint,MouseState *m)
{
    const char *p;
    char part[DIALOG_COLUMNS+1];
    int y;
    protect(&l->box);
    /* Paint only border strips black. No black-then-grey interior sweep. */
    ui_border(&l->box,UI_INK);
    video_rect(l->box.x+1,l->box.y+1,l->box.w-2,l->box.h-2,UI_PAPER);
    video_text_box(l->body.x,l->box.y+3,l->body.w,FONT_HEIGHT,title,UI_INK,UI_PAPER);
    p=body; y=l->body.y;
    while(y<l->body.y+l->body.h && dialog_line(&p,part,(unsigned)l->columns)) {
        video_text_bg(l->body.x,y,part,UI_INK,UI_PAPER); y+=FONT_HEIGHT;
    }
    video_text_bg(l->body.x,l->hint_y,hint,UI_INK,UI_PAPER);
    mouse_poll(m); previous_buttons=m->buttons;
}
/* Idle and pointer motion never return to a painter. */
static int event(MouseState *m)
{
    int key; unsigned pressed;
    for(;;) {
        if(has_mouse) cursor_show(m->x,m->y);
        delay(10); key=keyboard_poll(); mouse_poll(m);
        pressed=m->buttons & ~previous_buttons; previous_buttons=m->buttons;
        if(key>=0) return key;
        if(pressed&MOUSE_LEFT) return -2;
        if(pressed&MOUSE_RIGHT) return -3;
    }
}
static void field(const DialogLayout *l,const DialogInput *input,unsigned *offset,int focused)
{
    char text[DIALOG_COLUMNS+1];
    unsigned i;
    unsigned char paper,ink;
    if(input->cursor<*offset) *offset=input->cursor;
    if(input->cursor>*offset+(unsigned)l->columns-1) *offset=input->cursor-(unsigned)l->columns+1;
    memset(text,' ',(unsigned)l->columns); text[l->columns]='\0';
    for(i=0;i<(unsigned)l->columns && input->text[*offset+i];++i) text[i]=input->text[*offset+i];
    paper=focused?(input->replace?UI_FOCUS:UI_FIELD):UI_PAPER;
    ink=focused && input->replace?UI_WHITE:UI_INK;
    protect(&l->field);
    video_rect(l->field.x,l->field.y,l->field.w,1,paper);
    video_text_box(l->field.x,l->field.y+1,l->field.w,FONT_HEIGHT,text,ink,paper);
    video_rect(l->field.x,l->field.y+l->field.h-1,l->field.w,1,paper);
    if(focused) video_rect(l->field.x+(int)(input->cursor-*offset)*FONT_WIDTH,
                          l->field.y+l->field.h-1,5,1,UI_INK);
}
static int focus_key(int key)
{ return key==9 || key==KEY_SHIFT_TAB || key==KEY_LEFT || key==KEY_RIGHT; }
int dialog_filename(const char *title,const char *initial,char *output)
{
    DialogLayout l; DialogInput input,before; MouseState m;
    unsigned offset,position; int key,focus,oldfocus,result;
    dialog_layout(&l,DIALOG_FILE,title,DIALOG_FILE_HINT); dialog_input(&input,initial);
    focus=0; offset=0;
    begin(&l,title,DIALOG_FILE_HINT,"Tab focus  Enter OK  Esc cancel",&m);
    field(&l,&input,&offset,1); ui_button(&l.accept,"OK",0); ui_button(&l.cancel,"Cancel",0);
    for(;;) {
        key=event(&m); oldfocus=focus; before=input; result=0;
        if(key==-2) {
            if(hit(&l.cancel,&m)) return finish(&l,0);
            if(hit(&l.accept,&m)) { key=13; focus=1; }
            else if(hit(&l.field,&m)) {
                position=offset+(unsigned)(m.x-l.field.x)/FONT_WIDTH;
                input.cursor=position<input.length?position:input.length;
                input.replace=0; focus=0;
            }
        }
        if(key==27 || (key==13 && focus==2)) return finish(&l,0);
        if(key==9) focus=(focus+1)%3;
        else if(key==KEY_SHIFT_TAB) focus=(focus+2)%3;
        else if(focus==0 || key==13) result=dialog_edit(&input,key);
        if(result>0) { strcpy(output,input.text); return finish(&l,1); }
        if(oldfocus!=focus || before.cursor!=input.cursor || before.replace!=input.replace || strcmp(before.text,input.text)) field(&l,&input,&offset,focus==0);
        if(oldfocus!=focus) { ui_button(&l.accept,"OK",focus==1); ui_button(&l.cancel,"Cancel",focus==2); }
    }
}
int dialog_confirm(const char *title,const char *question)
{
    DialogLayout l; MouseState m; int key,focus,oldfocus;
    dialog_layout(&l,DIALOG_CONFIRM,title,question); focus=1;
    begin(&l,title,question,"Y yes  N/Esc cancel",&m);
    ui_button(&l.accept,"Yes",0); ui_button(&l.cancel,"Cancel",1);
    for(;;) {
        key=event(&m); oldfocus=focus;
        if(key==-2) {
            if(hit(&l.accept,&m)) return finish(&l,1);
            if(hit(&l.cancel,&m)) return finish(&l,0);
        }
        if(key==27 || key=='n' || key=='N') return finish(&l,0);
        if(key=='y' || key=='Y') return finish(&l,1);
        if(key==13) return finish(&l,!focus);
        if(focus_key(key)) focus=!focus;
        if(focus!=oldfocus) { ui_button(&l.accept,"Yes",!focus); ui_button(&l.cancel,"Cancel",focus); }
    }
}
static void choices(const DialogLayout *l,int choice)
{
    int i,x;
    char text[9];
    for(i=0;i<3;++i) {
        x=l->body.x+i*8*FONT_WIDTH;
        protect(&l->body); memcpy(text,DIALOG_NEW_CHOICES+i*8,7); text[7]='\0';
        video_text_bg(x,l->body.y,text,i==choice?UI_WHITE:UI_INK,i==choice?UI_SELECT:UI_PAPER);
    }
}
int dialog_new_size(unsigned current)
{
    DialogLayout l; MouseState m; int key,choice,oldchoice,focus,oldfocus;
    dialog_layout(&l,DIALOG_NEW,"New canvas",DIALOG_NEW_CHOICES);
    choice=current==16?0:current==32?1:2; focus=0;
    begin(&l,"New canvas",DIALOG_NEW_CHOICES,"Arrows size  Tab focus",&m);
    choices(&l,choice); ui_button(&l.accept,"Create",1); ui_button(&l.cancel,"Cancel",0);
    for(;;) {
        key=event(&m); oldchoice=choice; oldfocus=focus;
        if(key==-2) {
            if(hit(&l.cancel,&m)) return finish(&l,0);
            if(hit(&l.accept,&m)) return finish(&l,16<<choice);
            if(hit(&l.body,&m) && m.x<l.body.x+24*FONT_WIDTH) { choice=(m.x-l.body.x)/(8*FONT_WIDTH); focus=0; }
        }
        if(key==27) return finish(&l,0);
        if(key==13) return finish(&l,focus?0:16<<choice);
        if(key>='1' && key<='3') return finish(&l,16<<(key-'1'));
        if(key==9 || key==KEY_SHIFT_TAB) focus=!focus;
        if(key==KEY_LEFT || key==KEY_UP) choice=(choice+2)%3;
        if(key==KEY_RIGHT || key==KEY_DOWN) choice=(choice+1)%3;
        if(choice!=oldchoice) choices(&l,choice);
        if(focus!=oldfocus) { ui_button(&l.accept,"Create",!focus); ui_button(&l.cancel,"Cancel",focus); }
    }
}
static void append(char *body,const char *text)
{
    unsigned n;
    n=(unsigned)strlen(body);
    if(n<318 && text && *text) {
        body[n++]='\n'; body[n]='\0'; strncat(body,text,319-n);
    }
}
void dialog_message(const char *title,const char *message,const FileReport *report)
{
    DialogLayout l; MouseState m; int key; char body[320];
    strncpy(body,message,319); body[319]='\0';
    if(report) { append(body,report->backup); append(body,report->temporary); }
    dialog_layout(&l,DIALOG_MESSAGE,title,body);
    begin(&l,title,body,"Enter/Esc OK",&m); ui_button(&l.accept,"OK",1);
    for(;;) { key=event(&m); if(key>=0 || (key==-2 && hit(&l.accept,&m))) { (void)finish(&l,0); return; } }
}

static void full_cell(int index,int selected)
{
    Rect r;
    int inset;
    r.x=FULLPAL_X+index%16*FULLPAL_CELL_W;
    r.y=FULLPAL_Y+index/16*FULLPAL_CELL_H;
    r.w=FULLPAL_CELL_W; r.h=FULLPAL_CELL_H;
    protect(&r); inset=selected?2:1;
    ui_border(&r,selected?UI_WHITE:UI_INK);
    if(selected) { ++r.x; ++r.y; r.w-=2; r.h-=2; ui_border(&r,UI_INK); }
    video_rect(FULLPAL_X+index%16*FULLPAL_CELL_W+inset,
               FULLPAL_Y+index/16*FULLPAL_CELL_H+inset,
               FULLPAL_CELL_W-2*inset,FULLPAL_CELL_H-2*inset,(unsigned char)index);
}
static void full_value(int index,int background)
{
    char text[26];
    sprintf(text,"%s %03d  Tab changes FG/BG",background?"BG":"FG",index);
    cursor_protect(74,158,172,FONT_HEIGHT);
    video_text_bg(74,158,text,UI_INK,UI_PAPER);
}
int dialog_palette(unsigned char *colour,int *background)
{
    DialogLayout l;
    MouseState m;
    int i,key,selected,previous,target,oldtarget;
    unsigned char index;
    l.box.x=66; l.box.y=18; l.box.w=188; l.box.h=164;
    protect(&l.box); ui_border(&l.box,UI_INK);
    video_rect(67,19,186,162,UI_PAPER);
    video_text_bg(74,22,"All 256 colours",UI_INK,UI_PAPER);
    video_text_bg(74,32,"Left:FG  Right:BG",UI_INK,UI_PAPER);
    selected=*colour; target=*background;
    for(i=0;i<256;++i) full_cell(i,i==selected);
    full_value(selected,target);
    video_text_bg(74,170,"Arrows Enter:OK Esc:cancel",UI_INK,UI_PAPER);
    mouse_poll(&m); previous_buttons=m.buttons;
    for(;;) {
        key=event(&m); previous=selected; oldtarget=target;
        if(key==27) return finish(&l,0);
        if(key==-2 || key==-3) {
            if(workpal_full_hit(m.x,m.y,&index)) {
                *colour=index; *background=key==-3;
                return finish(&l,1);
            }
            if(!hit(&l.box,&m)) return finish(&l,0);
        }
        if(key==13) { *colour=(unsigned char)selected; *background=target; return finish(&l,1); }
        if(key==9 || key==KEY_SHIFT_TAB) target=!target;
        if(key==KEY_LEFT) selected=(selected+255)&255;
        if(key==KEY_RIGHT) selected=(selected+1)&255;
        if(key==KEY_UP) selected=(selected+240)&255;
        if(key==KEY_DOWN) selected=(selected+16)&255;
        if(key==KEY_HOME) selected=0;
        if(key==KEY_END) selected=255;
        if(selected!=previous) { full_cell(previous,0); full_cell(selected,1); }
        if(selected!=previous || target!=oldtarget) full_value(selected,target);
    }
}
void dialog_font(void)
{
    char body[240];
    int code,n;
    n=0;
    for(code=32;code<=126;++code) {
        body[n++]=(char)code;
        if((code-31)%32==0) body[n++]='\n';
    }
    strcpy(body+n,"\nABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\na e o p m rn  0O 1Il\n! : ; . , ' ` \\ /\nPLAYER01.P86  mixed_Case-09\nTools File Edit View Options");
    dialog_message("Original 6x8 font",body,NULL);
}
