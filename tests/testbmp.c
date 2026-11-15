#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bmp.h"
#include "export.h"
#include "editor.h"
#include "command.h"
static Animation image;
static Editor editor;
static unsigned short queue[4096];
static ExportOptions options;
static ExportReport report;
static long budget=-1;
static int flush_fail,close_fail;
#ifdef BMP_FAULTS
size_t bmp_fault_write(const void *ptr,size_t size,size_t n,FILE *f)
{
    size_t actual;
    if(budget<0) return fwrite(ptr,size,n,f);
    actual=n;
    if(size && actual>(unsigned long)budget/size) actual=(size_t)((unsigned long)budget/size);
    actual=fwrite(ptr,size,actual,f); budget-=(long)(actual*size); return actual;
}
int bmp_fault_flush(FILE *f)
{ int result; result=fflush(f); return flush_fail?EOF:result; }
int bmp_fault_close(FILE *f)
{ int result; result=fclose(f); return close_fail?EOF:result; }
#endif
static unsigned u16(const unsigned char *p)
{ return (unsigned)p[0]+256U*p[1]; }
static unsigned long u32(const unsigned char *p)
{ return (unsigned long)u16(p)+65536UL*u16(p+2); }
static void pattern(unsigned w,unsigned h,unsigned frames)
{
    unsigned i;
    memset(&image,0,sizeof(image)); image.width=w; image.height=h; image.frames=frames;
    image.background=141;
    for(i=0;i<w*h*frames;++i) image.pixels[i]=(unsigned char)(i*13U+i/(w*h)*71U);
}
/* Independent decoder: parse headers, palette and physical bottom-up bytes.
   No writer geometry or palette helpers are used for the expected image. */
static void check(const char *path,unsigned w,unsigned h,int mode,unsigned columns,unsigned frame)
{
    FILE *f;
    unsigned char head[54],quad[4];
    unsigned x,y,index,tile,offset,pad,i,dac[3],q;
    unsigned long stride;
    int byte;
    f=fopen(path,"rb"); assert(f!=NULL); assert(fread(head,1,54,f)==54);
    stride=((unsigned long)w+3UL)/4UL*4UL;
    assert(head[0]=='B' && head[1]=='M' && !u32(head+6));
    assert(u32(head+2)==1078UL+stride*h && u32(head+10)==1078UL);
    assert(u32(head+14)==40UL && u32(head+18)==w && u32(head+22)==h);
    assert(u16(head+26)==1 && u16(head+28)==8 && !u32(head+30));
    assert(u32(head+34)==stride*h && u32(head+46)==256UL);
    for(i=0;i<256;++i) {
        assert(fread(quad,1,4,f)==4 && !quad[3]);
        if(i<16) dac[0]=dac[1]=dac[2]=i*4;
        else { q=i-16; dac[0]=(q/36%6)*12; dac[1]=(q/6%6)*12; dac[2]=(q%6)*12; }
        for(x=0;x<3;++x) assert(quad[x]==(dac[x]*255U+31U)/63U);
    }
    pad=(unsigned)(stride-w);
    for(y=h;y>0;--y) {
        for(x=0;x<w;++x) {
            if(mode==99) index=(x+3U*(y-1))%256U;
            else {
                tile=mode==EXPORT_SHEET?((y-1)/image.height)*columns+x/image.width:frame;
                offset=((y-1)%image.height)*image.width+x%image.width;
                index=tile<image.frames?image.pixels[tile*image.width*image.height+offset]:image.background;
            }
            byte=fgetc(f); assert(byte==(int)index);
        }
        for(x=0;x<pad;++x) assert(fgetc(f)==0);
    }
    assert(fgetc(f)==EOF && !ferror(f)); assert(!fclose(f));
}
static int span(const void *ctx,unsigned x,unsigned y,unsigned char *p,unsigned count)
{
    unsigned i;
    (void)ctx;
    for(i=0;i<count;++i) p[i]=(unsigned char)((x+i+3U*y)%256U);
    return 1;
}
static void writer(void)
{
    FILE *f;
    unsigned long bytes;
    assert(bmp_size(0,1,&bytes)==FILE_DIMENSIONS);
    assert(bmp_size(65535U,1,&bytes)==FILE_DIMENSIONS);
    assert(bmp_size(32767U,32767U,&bytes)==FILE_OK && bytes==1078UL+32768UL*32767UL);
    f=fopen("ODD.BMP","wb"); assert(f!=NULL);
    assert(bmp_write(f,3,5,span,NULL)==FILE_OK); assert(!fclose(f)); check("ODD.BMP",3,5,99,0,0);
    f=fopen("LARGE.BMP","wb"); assert(f!=NULL);
    assert(bmp_write(f,257,257,span,NULL)==FILE_OK); assert(!fclose(f)); check("LARGE.BMP",257,257,99,0,0);
}
static void names(void)
{
    char path[FILE_PATH_MAX];
    assert(export_name("player",path) && !strcmp(path,"PLAYER.BMP"));
    assert(export_name("C:\\ART\\PLY.bmp",path));
    assert(export_target("PLY",EXPORT_ALL,0,path) && !strcmp(path,"PLY0001.BMP"));
    assert(export_target("PLAYER.BMP",EXPORT_ALL,15,path) && !strcmp(path,"PLAY0016.BMP"));
    assert(!export_name("PLAYER.P86",path)); assert(!export_name("NUL.BMP",path));
    assert(!export_name("LONGFNAME.BMP",path)); assert(!export_name("A B.BMP",path));
    assert(!export_target("PLAYER",EXPORT_ALL,16,path));
}
static void exports(void)
{
    unsigned i,w,h;
    char path[FILE_PATH_MAX];
    pattern(16,24,5); options.mode=EXPORT_CURRENT; options.layout=SHEET_HORIZONTAL; options.columns=3;
    assert(export_run(&image,3,"FRAME",&options,1,NULL,&report)==FILE_OK);
    check("FRAME.BMP",16,24,EXPORT_CURRENT,1,3);
    options.mode=EXPORT_ALL;
    assert(export_run(&image,4,"PLY",&options,1,NULL,&report)==FILE_OK && report.completed==5);
    for(i=0;i<5;++i) { assert(export_target("PLY",EXPORT_ALL,i,path)); check(path,16,24,EXPORT_ALL,1,i); }
    /* A later collision prevents earlier missing files being created. */
    remove("PLY0001.BMP");
    assert(export_run(&image,0,"PLY",&options,0,NULL,&report)==FILE_EXISTS && !report.completed);
    assert(file_exists("PLY0001.BMP")==0);
    options.mode=EXPORT_SHEET;
    for(i=0;i<3;++i) {
        options.layout=(int)i;
        assert(export_size(&image,&options,&w,&h)==FILE_OK);
        assert(w==(i==0?80:i==1?16:48) && h==(i==0?24:i==1?120:48));
        assert(export_run(&image,0,"SHEET",&options,1,NULL,&report)==FILE_OK);
        check("SHEET.BMP",w,h,EXPORT_SHEET,i==0?5:i==1?1:3,0);
    }
    options.columns=0; assert(export_run(&image,0,"SHEET",&options,1,NULL,&report)==FILE_DIMENSIONS);
    options.columns=17; assert(export_size(&image,&options,&w,&h)==FILE_DIMENSIONS);
    pattern(64,64,4); options.columns=16;
    assert(export_run(&image,0,"WIDE",&options,1,NULL,&report)==FILE_OK);
    check("WIDE.BMP",1024,64,EXPORT_SHEET,16,0);
    assert(export_run(&image,0,"WIDE",&options,1,"WIDE.BMP",&report)==FILE_BAD_NAME);
    assert(export_run(&image,0,"ART.P86",&options,1,NULL,&report)==FILE_BAD_NAME);
}
static void failures(void)
{
#ifdef BMP_FAULTS
    long size;
    pattern(16,24,3); options.mode=EXPORT_ALL; options.layout=SHEET_HORIZONTAL;
    size=1078L+384L;
    remove("FAIL0001.BMP"); remove("FAIL0002.BMP"); remove("FAIL0003.BMP");
    budget=size+100;
    assert(export_run(&image,0,"FAIL",&options,0,NULL,&report)==FILE_WRITE_ERROR);
    assert(report.completed==1 && report.total==3 && !strcmp(report.path,"FAIL0002.BMP"));
    assert(file_exists("FAIL0002.BMP")==0 && file_exists("FAIL0003.BMP")==0);
    budget=-1; check("FAIL0001.BMP",16,24,EXPORT_ALL,1,0);
    options.mode=EXPORT_CURRENT;
    assert(export_run(&image,1,"OLD",&options,1,NULL,&report)==FILE_OK);
    budget=100;
    assert(export_run(&image,2,"OLD",&options,1,NULL,&report)==FILE_WRITE_ERROR);
    budget=-1; check("OLD.BMP",16,24,EXPORT_CURRENT,1,1);
    flush_fail=1; assert(export_run(&image,2,"OLD",&options,1,NULL,&report)==FILE_WRITE_ERROR);
    flush_fail=0; close_fail=1; assert(export_run(&image,2,"OLD",&options,1,NULL,&report)==FILE_WRITE_ERROR);
    close_fail=0; check("OLD.BMP",16,24,EXPORT_CURRENT,1,1);
#endif
}
static void unchanged(void)
{
    unsigned pos;
    assert(editor_init(&editor,32,queue));
    editor.pixels[0]=255; history_commit(&editor.history,&editor.canvas);
    editor_key(&editor,CMD_FRAME_DUP); editor_select(&editor,0);
    pos=editor.history.position; editor_mark_saved(&editor);
    editor_image(&editor,&image); options.mode=EXPORT_SHEET; options.layout=SHEET_HORIZONTAL;
    assert(export_run(&image,0,"CLEAN",&options,1,NULL,&report)==FILE_OK);
    assert(!editor_is_dirty(&editor) && editor.history.position==pos && editor.history.active==0);
    editor.pixels[7]=123; assert(editor_is_dirty(&editor)); editor_image(&editor,&image);
    assert(export_run(&image,0,"DIRTY",&options,1,NULL,&report)==FILE_OK);
    assert(editor_is_dirty(&editor) && editor.pixels[7]==123 && editor.history.position==pos);
    editor_dispose(&editor);
}
int main(void)
{
    names(); writer(); exports(); failures(); unchanged();
    puts("PASS: indexed BMP, palette, padding, sheets, batches, failures and unchanged document.");
    return 0;
}
