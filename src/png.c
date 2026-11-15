#include <string.h>
#include "png.h"
#include "fileio.h"
#define PNG_SPAN 32
static void be32(unsigned char *p,unsigned long v)
{ p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16); p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v; }
static unsigned long crc_byte(unsigned long crc,unsigned char b)
{
    unsigned bit;
    crc^=b;
    for(bit=0;bit<8;++bit) crc=(crc>>1)^((crc&1UL)?0xedb88320UL:0UL);
    return crc;
}
static int part(FILE *f,const unsigned char *data,unsigned n,unsigned long *crc)
{
    unsigned i;
    for(i=0;i<n;++i) *crc=crc_byte(*crc,data[i]);
    return fwrite(data,1,n,f)==n;
}
static int start(FILE *f,const char *type,unsigned long length,unsigned long *crc)
{
    unsigned char p[4];
    be32(p,length); *crc=0xffffffffUL;
    return fwrite(p,1,4,f)==4 && part(f,(const unsigned char *)type,4,crc);
}
static int end(FILE *f,unsigned long crc)
{
    unsigned char p[4]; be32(p,crc^0xffffffffUL); return fwrite(p,1,4,f)==4;
}
int png_size(unsigned w,unsigned h,unsigned long *bytes)
{
    unsigned long row,blocks,total;
    if(!bytes || !w || !h || w>32767U || h>32767U) return FILE_DIMENSIONS;
    /* One filter block and ceil(w/32) pixel blocks per row, each in an IDAT. */
    row=1UL+4UL*w; blocks=1UL+((unsigned long)w+31UL)/32UL;
    total=row+17UL*blocks;
    if((unsigned long)h>(0xffffffffUL-51UL)/total) return FILE_LENGTH;
    *bytes=51UL+total*h; return FILE_OK;
}
static int block(FILE *f,const unsigned char *data,unsigned n,int first,int last,
                 unsigned long *s1,unsigned long *s2)
{
    unsigned char header[7],tail[4];
    unsigned i,p;
    unsigned long crc;
    p=0;
    if(first) { header[p++]=0x78; header[p++]=0x01; }
    header[p++]=(unsigned char)(last?1:0); /* stored block, byte aligned */
    header[p++]=(unsigned char)n; header[p++]=(unsigned char)(n>>8);
    header[p++]=(unsigned char)(~n); header[p++]=(unsigned char)((~n)>>8);
    for(i=0;i<n;++i) { *s1+=data[i]; if(*s1>=65521UL) *s1-=65521UL;
        *s2+=*s1; if(*s2>=65521UL) *s2-=65521UL; }
    if(!start(f,"IDAT",(unsigned long)p+n+(last?4:0),&crc) ||
       !part(f,header,p,&crc) || !part(f,data,n,&crc)) return 0;
    if(last) { be32(tail,(*s2<<16)|*s1); if(!part(f,tail,4,&crc)) return 0; }
    return end(f,crc);
}
int png_write(FILE *f,unsigned w,unsigned h,PngRead read,const void *ctx)
{
    unsigned char ihdr[13],data[128];
    unsigned x,y,n;
    unsigned long crc,bytes,s1,s2;
    int status;
    if(!f || !read) return FILE_WRITE_ERROR;
    status=png_size(w,h,&bytes); if(status!=FILE_OK) return status;
    if(fwrite("\211PNG\r\n\032\n",1,8,f)!=8) return FILE_WRITE_ERROR;
    memset(ihdr,0,sizeof(ihdr)); be32(ihdr,w); be32(ihdr+4,h); ihdr[8]=8; ihdr[9]=6;
    if(!start(f,"IHDR",13UL,&crc) || !part(f,ihdr,13,&crc) || !end(f,crc)) return FILE_WRITE_ERROR;
    s1=1; s2=0;
    for(y=0;y<h;++y) {
        data[0]=0; /* filter None */
        if(!block(f,data,1,y==0,0,&s1,&s2)) return FILE_WRITE_ERROR;
        for(x=0;x<w;x+=n) {
            n=w-x; if(n>PNG_SPAN) n=PNG_SPAN;
            if(!read(ctx,x,y,data,n)) return FILE_READ_ERROR;
            if(!block(f,data,n*4,0,y+1==h && x+n==w,&s1,&s2)) return FILE_WRITE_ERROR;
        }
    }
    if(!start(f,"IEND",0UL,&crc) || !end(f,crc) || ferror(f)) return FILE_WRITE_ERROR;
    return FILE_OK;
}
