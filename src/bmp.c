#include <string.h>
#include "bmp.h"
#include "fileio.h"
#include "colour.h"
#define BMP_OFFSET 1078UL
#define BMP_CHUNK 128
static void put16(unsigned char *p,unsigned n)
{ p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8); }
static void put32(unsigned char *p,unsigned long n)
{ put16(p,(unsigned)(n&65535UL)); put16(p+2,(unsigned)(n>>16)); }
int bmp_size(unsigned w,unsigned h,unsigned long *bytes)
{
    unsigned long stride;
    if(!bytes || !w || !h || w>32767U || h>32767U) return FILE_DIMENSIONS;
    stride=((unsigned long)w+3UL)&~3UL;
    if((unsigned long)h>(0xffffffffUL-BMP_OFFSET)/stride) return FILE_LENGTH;
    *bytes=BMP_OFFSET+stride*h; return FILE_OK;
}
int bmp_write(FILE *f,unsigned w,unsigned h,BmpRead read,const void *ctx)
{
    unsigned char header[54],buf[BMP_CHUNK],colour[4];
    unsigned i,x,y,n,pad;
    unsigned char rgb[3];
    unsigned long bytes;
    int status;
    if(!f || !read) return FILE_WRITE_ERROR;
    status=bmp_size(w,h,&bytes); if(status!=FILE_OK) return status;
    memset(header,0,sizeof(header)); header[0]='B'; header[1]='M';
    put32(header+2,bytes); put32(header+10,BMP_OFFSET); put32(header+14,40UL);
    put32(header+18,(unsigned long)w); put32(header+22,(unsigned long)h);
    put16(header+26,1); put16(header+28,8); put32(header+34,bytes-BMP_OFFSET);
    put32(header+46,256UL);
    if(fwrite(header,1,sizeof(header),f)!=sizeof(header)) return FILE_WRITE_ERROR;
    for(i=0;i<256;++i) {
        colour_rgb((unsigned char)i,rgb);
        colour[0]=rgb[2]; colour[1]=rgb[1]; colour[2]=rgb[0]; colour[3]=0;
        if(fwrite(colour,1,4,f)!=4) return FILE_WRITE_ERROR;
    }
    pad=(4-w%4)%4;
    for(y=h;y>0;--y) {
        for(x=0;x<w;x+=n) {
            n=w-x; if(n>BMP_CHUNK) n=BMP_CHUNK;
            if(!read(ctx,x,y-1,buf,n)) return FILE_READ_ERROR;
            if(fwrite(buf,1,n,f)!=n) return FILE_WRITE_ERROR;
        }
        memset(buf,0,4);
        if(pad && fwrite(buf,1,pad,f)!=pad) return FILE_WRITE_ERROR;
    }
    return ferror(f)?FILE_WRITE_ERROR:FILE_OK;
}
