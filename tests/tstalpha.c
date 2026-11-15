#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"
#include "command.h"
#include "export.h"
#include "png.h"
static Editor e;
static unsigned short queue[4096];
static Animation image;
static ExportOptions options;
static ExportReport report;
static unsigned char raw[1024];
static long budget=-1;
static unsigned check_frame;
#ifdef ALPHA_FAULTS
size_t alpha_write(const void *p,size_t size,size_t n,FILE *f)
{
    size_t actual;
    if(budget<0) return fwrite(p,size,n,f);
    actual=n;
    if(size && actual>(unsigned long)budget/size) actual=(size_t)((unsigned long)budget/size);
    actual=fwrite(p,size,actual,f); budget-=(long)(actual*size); return actual;
}
#endif
static void click(unsigned x,unsigned y)
{
    int sx,sy;
    sx=e.view.x+(int)x*e.view.zoom; sy=e.view.y+(int)y*e.view.zoom;
    editor_mouse(&e,sx,sy,EDIT_LEFT); editor_mouse(&e,sx,sy,0);
}
static void model(void)
{
    unsigned pos;
    assert(editor_init(&e,32,queue)); assert(editor_new_shape(&e,16,24));
    e.foreground=0; editor_key(&e,CMD_TRANS_ERASE); e.tool=TOOL_ERASER;
    click(0,0); assert(e.pixels[0]==0 && canvas_transparent(&e.canvas,0,0) && editor_is_dirty(&e));
    e.tool=TOOL_PENCIL; click(0,0); assert(e.pixels[0]==0 && !canvas_transparent(&e.canvas,0,0));
    editor_key(&e,CMD_UNDO); assert(canvas_transparent(&e.canvas,0,0));
    editor_key(&e,CMD_REDO); assert(!canvas_transparent(&e.canvas,0,0));
    editor_background(&e,141); e.tool=TOOL_ERASER; editor_key(&e,CMD_TRANS_ERASE);
    click(1,0); assert(e.pixels[1]==141 && !canvas_transparent(&e.canvas,1,0));
    editor_key(&e,CMD_TRANS_BLANK); assert(e.document->transparent && e.transparent_eraser);
    editor_key(&e,CMD_FRAME_ADD); assert(canvas_transparent(&e.canvas,0,0));
    e.foreground=0; e.tool=TOOL_PENCIL; click(0,0);
    editor_key(&e,CMD_FRAME_DUP); assert(!canvas_transparent(&e.canvas,0,0) && canvas_transparent(&e.canvas,1,0));
    editor_key(&e,CMD_FRAME_DELETE); editor_key(&e,CMD_UNDO);
    assert(e.document->frames==3 && canvas_transparent(&e.canvas,1,0));
    editor_mark_saved(&e); pos=e.history.position;
    e.preview_link=1; editor_background(&e,12); assert(e.pixels[0]==0);
    editor_key(&e,CMD_PREVIEW_LINK); editor_key(&e,CMD_CHECKER);
    assert(e.history.position==pos); /* view/BG selection does not enter artwork history */
    editor_key(&e,CMD_FILL_BG); assert(e.pixels[0]==12 && !canvas_transparent(&e.canvas,1,0));
    editor_key(&e,CMD_UNDO); assert(e.pixels[0]==0 && canvas_transparent(&e.canvas,1,0));
    editor_key(&e,CMD_CLEAR); assert(canvas_transparent(&e.canvas,0,0) && e.pixels[0]==0);
    /* Flooding transparency makes it opaque even when the hidden index matches. */
    e.tool=TOOL_FILL; e.foreground=0; click(0,0);
    assert(!canvas_transparent(&e.canvas,15,23) && e.pixels[383]==0);
    editor_dispose(&e);
}
static void persistence(void)
{
    FILE *f;
    unsigned i,length;
    assert(editor_init(&e,32,queue)); assert(editor_new_shape(&e,16,24));
    e.foreground=0; e.tool=TOOL_PENCIL; click(0,0);
    editor_key(&e,CMD_TRANS_BLANK); editor_key(&e,CMD_TRANS_ERASE); /* select opaque eraser */
    editor_key(&e,CMD_TRANS_ERASE); e.tool=TOOL_ERASER; click(1,0);
    editor_key(&e,CMD_FRAME_DUP); editor_image(&e,&image);
    f=fopen("MASK.P86","wb"); assert(f!=NULL); assert(file_encode(f,&image)==FILE_OK); assert(!fclose(f));
    f=fopen("MASK.P86","rb"); assert(f!=NULL); length=(unsigned)fread(raw,1,sizeof(raw),f); fclose(f);
    assert(length==892 && raw[4]==3 && raw[22]==1 && raw[410]==2);
    /* Every truncated prefix must be rejected, without applying scratch. */
    for(i=0;i<length;++i) {
        f=tmpfile(); assert(f!=NULL); assert(fwrite(raw,1,i,f)==i); rewind(f);
        assert(file_decode(f,&image)==FILE_LENGTH); fclose(f);
    }
    f=fopen("MASK.P86","rb"); assert(f!=NULL); assert(file_decode(f,&image)==FILE_OK); fclose(f);
    assert(image.mask[0]==2 && image.mask[48]==2 && image.pixels[0]==0);
    assert(editor_load(&e,&image)); assert(canvas_transparent(&e.canvas,1,0) && !editor_is_dirty(&e));
    image.transparent=2; assert(!editor_load(&e,&image)); assert(!editor_is_dirty(&e)); image.transparent=1;
    raw[22]=2; f=tmpfile(); assert(f!=NULL); fwrite(raw,1,length,f); rewind(f);
    assert(file_decode(f,&image)==FILE_HEADER); fclose(f); raw[22]=1;
    /* Explicit old-format fixtures always decode as fully opaque. */
    memset(&image,0,sizeof(image)); image.width=image.height=16; image.frames=2;
    image.version=2; image.duration[0]=image.duration[1]=100; image.pixels[0]=0;
    f=tmpfile(); assert(f!=NULL); assert(file_encode(f,&image)==FILE_OK); rewind(f);
    memset(image.mask,255,sizeof(image.mask)); assert(file_decode(f,&image)==FILE_OK); fclose(f);
    assert(!image.transparent && image.mask[0]==0 && image.mask[32]==0 && image.pixels[0]==0);
    image.frames=0; image.version=1;
    f=tmpfile(); assert(f!=NULL); assert(file_encode(f,&image)==FILE_OK); rewind(f);
    memset(image.mask,255,sizeof(image.mask)); assert(file_decode(f,&image)==FILE_OK); fclose(f);
    assert(!image.mask[0] && !image.transparent); editor_dispose(&e);
}
static unsigned long big32(const unsigned char *p)
{ return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3]; }
static unsigned long read32(FILE *f)
{ unsigned char p[4]; assert(fread(p,1,4,f)==4); return big32(p); }
static unsigned long crc(unsigned long value,unsigned char byte)
{
    unsigned i; value^=byte;
    for(i=0;i<8;++i) { if(value&1) value=(value>>1)^0xedb88320UL; else value>>=1; }
    return value;
}
static unsigned char expected(unsigned long pos,unsigned w,int flatten)
{
    unsigned x,y,f,offset,index,dac,channel;
    unsigned long row;
    int alpha;
    row=4UL*w+1; y=(unsigned)(pos/row); pos%=row;
    if(!pos) return 0;
    --pos; x=(unsigned)(pos/4); channel=(unsigned)(pos%4);
    f=options.mode==EXPORT_SHEET?(y/image.height)*(w/image.width)+x/image.width:check_frame;
    offset=(y%image.height)*image.width+x%image.width;
    alpha=f>=image.frames || (image.mask[(f*image.width*image.height+offset)/8]>>(offset%8)&1);
    index=f<image.frames?image.pixels[f*image.width*image.height+offset]:image.background;
    if(flatten && alpha) index=options.background;
    if(channel==3) return (unsigned char)(alpha && !flatten?0:255);
    if(index<16) dac=index*4;
    else { index-=16; dac=(channel==0?index%6:channel==1?index/6%6:index/36%6)*12; }
    return (unsigned char)((dac*255U+31U)/63U);
}
/* Independent stored-DEFLATE reader: concatenate IDAT on disk and parse its
   complete zlib stream separately from chunk boundaries. */
static void check_png(const char *path,unsigned w,unsigned h,int flatten)
{
    FILE *f,*z;
    unsigned char data[256],type[5],signature[8];
    unsigned long length,c,value,pos,s1,s2,total;
    unsigned i,len,inverse;
    int final,byte;
    f=fopen(path,"rb"); z=tmpfile(); assert(f!=NULL && z!=NULL);
    assert(fread(signature,1,8,f)==8 && !memcmp(signature,"\211PNG\r\n\032\n",8));
    do {
        length=read32(f); assert(length<=sizeof(data));
        assert(fread(type,1,4,f)==4); type[4]=0; c=0xffffffffUL;
        for(i=0;i<4;++i) c=crc(c,type[i]);
        assert(fread(data,1,(unsigned)length,f)==length);
        for(i=0;i<(unsigned)length;++i) c=crc(c,data[i]);
        assert(read32(f)==(c^0xffffffffUL));
        if(!strcmp((char *)type,"IHDR")) assert(length==13 && big32(data)==w && big32(data+4)==h && data[8]==8 && data[9]==6);
        if(!strcmp((char *)type,"IDAT")) assert(fwrite(data,1,(unsigned)length,z)==length);
    } while(strcmp((char *)type,"IEND"));
    assert(fgetc(f)==EOF); fclose(f); rewind(z);
    assert(fgetc(z)==0x78 && fgetc(z)==1); pos=0; s1=1; s2=0;
    do {
        final=fgetc(z); assert(final==0 || final==1);
        len=(unsigned)fgetc(z); len|=(unsigned)fgetc(z)<<8;
        inverse=(unsigned)fgetc(z); inverse|=(unsigned)fgetc(z)<<8;
        assert((len^inverse)==65535U);
        for(i=0;i<len;++i) {
            byte=fgetc(z); assert(byte>=0 && byte==(int)expected(pos,w,flatten)); ++pos;
            s1=(s1+(unsigned)byte)%65521UL; s2=(s2+s1)%65521UL;
        }
    } while(!final);
    value=read32(z); total=((unsigned long)w*4+1)*h;
    assert(value==((s2<<16)|s1) && pos==total && fgetc(z)==EOF); fclose(z);
}
static void exports(void)
{
    unsigned i;
    FILE *f;
    unsigned long bytes;
    memset(&image,0,sizeof(image)); image.width=16; image.height=24; image.frames=3;
    for(i=0;i<1152;++i) image.pixels[i]=(unsigned char)i;
    image.mask[0]=2; image.mask[48]=1; image.mask[96]=255;
    memset(&options,0,sizeof(options)); options.format=EXPORT_PNG; options.columns=2;
    assert(export_run(&image,0,"ALPHA",&options,1,NULL,&report)==FILE_OK);
    check_png("ALPHA.PNG",16,24,0);
    options.mode=EXPORT_ALL;
    assert(export_run(&image,0,"RGBA",&options,1,NULL,&report)==FILE_OK && report.completed==3);
    check_frame=1; check_png("RGBA0002.PNG",16,24,0); check_frame=2; check_png("RGBA0003.PNG",16,24,0); check_frame=0;
    options.mode=EXPORT_SHEET; options.layout=SHEET_HORIZONTAL;
    assert(export_run(&image,0,"HORIZ",&options,1,NULL,&report)==FILE_OK); check_png("HORIZ.PNG",48,24,0);
    options.layout=SHEET_VERTICAL;
    assert(export_run(&image,0,"VERT",&options,1,NULL,&report)==FILE_OK); check_png("VERT.PNG",16,72,0);
    options.mode=EXPORT_SHEET; options.layout=SHEET_GRID;
    assert(export_run(&image,0,"SHEET",&options,1,NULL,&report)==FILE_OK);
    check_png("SHEET.PNG",32,48,0);
    options.flatten=1; options.background=141;
    assert(export_run(&image,0,"FLAT",&options,1,NULL,&report)==FILE_OK); check_png("FLAT.PNG",32,48,1);
    options.format=EXPORT_BMP; options.flatten=0;
    assert(export_run(&image,0,"NOALPHA",&options,1,NULL,&report)==FILE_ALPHA_POLICY);
    options.flatten=1;
    assert(export_run(&image,0,"FLAT",&options,1,NULL,&report)==FILE_OK);
    f=fopen("FLAT.BMP","rb"); assert(f!=NULL); assert(!fseek(f,1078L+47L*32L,SEEK_SET));
    assert(fgetc(f)==0 && fgetc(f)==141); fclose(f); /* opaque black and flattened pixel */
    assert(png_size(32767U,32767U,&bytes)==FILE_LENGTH);
#ifdef ALPHA_FAULTS
    options.mode=EXPORT_CURRENT; options.format=EXPORT_PNG; budget=100;
    remove("BROKEN.PNG");
    assert(export_run(&image,0,"BROKEN",&options,0,NULL,&report)==FILE_WRITE_ERROR && !report.completed);
    assert(!file_exists("BROKEN.PNG")); budget=-1;
#endif
    memset(&image,0,sizeof(image)); image.width=image.height=64; image.frames=4; image.mask[0]=1;
    options.format=EXPORT_PNG; options.mode=EXPORT_SHEET; options.layout=SHEET_GRID; options.columns=4; options.flatten=0;
    assert(export_run(&image,0,"BIGRGBA",&options,1,NULL,&report)==FILE_OK); check_png("BIGRGBA.PNG",256,64,0);
}
int main(void)
{
    model(); persistence(); exports();
    puts("PASS: masks, opaque black, BG/erasers, frame undo, v1/v2/v3 and PNG/BMP alpha.");
    return 0;
}
