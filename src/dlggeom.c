#include <string.h>
#include "dialog.h"
#include "layout.h"
int dialog_line(const char **text,char *line,unsigned columns)
{
    unsigned i;
    if(!*text || !**text || !columns) return 0;
    if(columns>DIALOG_COLUMNS) columns=DIALOG_COLUMNS;
    i=0;
    while(**text && **text!='\n' && i<columns) line[i++]=*(*text)++;
    if(**text=='\n') ++*text;
    line[i]='\0'; return 1;
}
void dialog_layout(DialogLayout *l,int kind,const char *title,const char *body)
{
    const char *p;
    char part[DIALOG_COLUMNS+1];
    int cols,n,lines,width,height,y;
    cols=kind==DIALOG_FILE?32:22;
    n=font_width(title)/FONT_WIDTH; if(n>cols) cols=n;
    p=body;
    while(dialog_line(&p,part,DIALOG_COLUMNS)) {
        n=(int)strlen(part); if(n>cols) cols=n;
    }
    if(cols>DIALOG_COLUMNS) cols=DIALOG_COLUMNS;
    lines=0; p=body;
    while(lines<16 && dialog_line(&p,part,(unsigned)cols)) ++lines;
    if(!lines) lines=1;
    width=cols*FONT_WIDTH+8;
    height=44+lines*FONT_HEIGHT+(kind==DIALOG_FILE?FONT_HEIGHT+6:0);
    l->columns=cols;
    l->box.x=(UI_WIDTH-width)/2; l->box.y=(UI_HEIGHT-height)/2; l->box.w=width; l->box.h=height;
    l->body.x=l->box.x+4; l->body.y=l->box.y+14;
    l->body.w=width-8; l->body.h=lines*FONT_HEIGHT;
    y=l->body.y+l->body.h+4;
    l->field.x=l->body.x; l->field.y=y; l->field.w=l->body.w;
    l->field.h=kind==DIALOG_FILE?FONT_HEIGHT+2:0;
    if(kind==DIALOG_FILE) y+=l->field.h+4;
    l->hint_y=y;
    l->accept.w=l->cancel.w=font_width("Cancel")+8;
    l->accept.h=l->cancel.h=FONT_HEIGHT+2;
    l->accept.y=l->cancel.y=y+FONT_HEIGHT+4;
    l->accept.x=(UI_WIDTH-2*l->accept.w-2*FONT_WIDTH)/2;
    l->cancel.x=l->accept.x+l->accept.w+2*FONT_WIDTH;
}
