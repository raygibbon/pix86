#include <string.h>
#include "export.h"
#include "bmp.h"
#include "png.h"
#include "colour.h"

typedef struct ExportSource {
    const Animation ANIM_FAR *doc;
    unsigned frame,columns;
    int sheet,flatten;
    unsigned background;
} ExportSource;
int export_name(const char *input,char *output)
{
    char name[FILE_PATH_MAX];
    unsigned n,start,i;
    int dot;
    if(!input || !output) return 0;
    for(n=0;n<FILE_PATH_MAX-1 && input[n];++n) name[n]=input[n];
    if(!n || input[n]) return 0;
    name[n]='\0'; start=0; dot=0;
    for(i=0;i<n;++i) if(name[i]=='/' || name[i]=='\\' || name[i]==':') start=i+1;
    for(i=start;i<n;++i) if(name[i]=='.') dot=1;
    if(!dot) { if(n+4>=FILE_PATH_MAX) return 0; strcpy(name+n,".BMP"); }
    if(!file_name(name,output)) return 0;
    n=(unsigned)strlen(output);
    return n>=4 && (!strcmp(output+n-4,".BMP") || !strcmp(output+n-4,".PNG"));
}
int export_target(const char *base,int mode,unsigned frame,char *output)
{
    char name[FILE_PATH_MAX];
    unsigned start,i,n;
    if(!export_name(base,name)) return 0;
    if(mode!=EXPORT_ALL) { strcpy(output,name); return 1; }
    if(frame>=ANIM_FRAMES) return 0;
    start=0;
    for(i=0;name[i];++i) if(name[i]=='/' || name[i]=='\\' || name[i]==':') start=i+1;
    for(n=0;name[start+n]!='.' && n<4;++n) ;
    if(start+n+8>=FILE_PATH_MAX) return 0;
    memcpy(output,name,start+n); sprintf(output+start+n,"%04u%s",frame+1,name+strlen(name)-4);
    return 1;
}
static int valid(const Animation ANIM_FAR *d)
{
    return d && anim_dimensions(d->width,d->height) && d->frames &&
        d->frames<=ANIM_FRAMES && d->frames<=ANIM_PIXELS/(d->width*d->height);
}
int export_size(const Animation ANIM_FAR *d,const ExportOptions *o,unsigned *w,unsigned *h)
{
    unsigned columns,rows;
    unsigned long width,height,bytes;
    if(!valid(d) || !o || !w || !h || o->mode<EXPORT_CURRENT || o->mode>EXPORT_SHEET || o->format<0 || o->format>1 || o->flatten<0 || o->flatten>1 || o->background>255)
        return FILE_DIMENSIONS;
    columns=rows=1;
    if(o->mode==EXPORT_SHEET) {
        if(o->layout==SHEET_HORIZONTAL) columns=d->frames;
        else if(o->layout==SHEET_VERTICAL) rows=d->frames;
        else if(o->layout==SHEET_GRID && o->columns>=1 && o->columns<=ANIM_FRAMES) {
            columns=o->columns; rows=(d->frames+columns-1)/columns;
        } else return FILE_DIMENSIONS;
    }
    width=(unsigned long)d->width*columns; height=(unsigned long)d->height*rows;
    if(width>32767UL || height>32767UL) return FILE_DIMENSIONS;
    *w=(unsigned)width; *h=(unsigned)height;
    return o->format==EXPORT_PNG?png_size(*w,*h,&bytes):bmp_size(*w,*h,&bytes);
}
static void sample(const ExportSource *s,unsigned x,unsigned y,unsigned char *index,unsigned char *alpha)
{
    const Animation ANIM_FAR *d;
    unsigned f,offset,global;
    d=s->doc; f=s->sheet?(y/d->height)*s->columns+x/d->width:s->frame;
    offset=(y%d->height)*d->width+x%d->width; global=f*d->width*d->height+offset;
    *index=f<d->frames?d->pixels[global]:d->background;
    *alpha=f<d->frames?((d->mask[global/8]>>(global%8))&1?0:255):0;
    if(s->flatten && !*alpha) { *index=(unsigned char)s->background; *alpha=255; }
}
static int read_span(const void *ctx,unsigned x,unsigned y,unsigned char *p,unsigned n)
{
    const ExportSource *s;
    unsigned i;
    unsigned char alpha,index;
    s=(const ExportSource *)ctx;
    for(i=0;i<n;++i) { sample(s,x+i,y,&index,&alpha); p[i]=index; }
    return 1;
}
static int read_rgba(const void *ctx,unsigned x,unsigned y,unsigned char *p,unsigned n)
{
    const ExportSource *s;
    unsigned i;
    unsigned char alpha,index;
    s=(const ExportSource *)ctx;
    for(i=0;i<n;++i) { sample(s,x+i,y,&index,&alpha); colour_rgb(index,p+4*i); p[4*i+3]=alpha; }
    return 1;
}
typedef struct BmpJob { ExportSource source; unsigned width,height; int format; } BmpJob;
static int write_bmp(FILE *f,const void *ctx)
{
    const BmpJob *job;
    job=(const BmpJob *)ctx;
    if(job->format==EXPORT_PNG) return png_write(f,job->width,job->height,read_rgba,&job->source);
    return bmp_write(f,job->width,job->height,read_span,&job->source);
}
int export_run(const Animation ANIM_FAR *d,unsigned frame,const char *path,
               const ExportOptions *o,int overwrite,const char *protected_path,ExportReport *r)
{
    BmpJob job;
    unsigned i,n,start,length,j;
    int status,present,extension;
    char protect[FILE_PATH_MAX],base[FILE_PATH_MAX],raw[FILE_PATH_MAX];
    if(!r) return FILE_WRITE_ERROR;
    memset(r,0,sizeof(*r));
    status=export_size(d,o,&job.width,&job.height);
    if(status!=FILE_OK || frame>=d->frames) return status==FILE_OK?FILE_DIMENSIONS:status;
    if(o->format==EXPORT_BMP && !o->flatten) {
        start=o->mode==EXPORT_CURRENT?frame:0;
        n=o->mode==EXPORT_CURRENT?1:d->frames;
        for(i=start;i<start+n;++i) for(j=0;j<d->width*d->height/8;++j)
            if(d->mask[i*d->width*d->height/8+j]) return FILE_ALPHA_POLICY;
    }
    if(!path) return FILE_BAD_NAME;
    for(length=0;length<FILE_PATH_MAX-1 && path[length];++length) raw[length]=path[length];
    if(!length || path[length]) return FILE_BAD_NAME;
    raw[length]='\0'; extension=0;
    for(i=0;i<length;++i) {
        if(raw[i]=='/' || raw[i]=='\\' || raw[i]==':') extension=0;
        else if(raw[i]=='.') extension=1;
    }
    if(!extension) { if(length+4>=FILE_PATH_MAX) return FILE_BAD_NAME; strcpy(raw+length,o->format==EXPORT_PNG?".PNG":".BMP"); }
    if(!export_name(raw,base) || strcmp(base+strlen(base)-4,o->format==EXPORT_PNG?".PNG":".BMP")) return FILE_BAD_NAME;
    r->total=o->mode==EXPORT_ALL?d->frames:1;
    protect[0]='\0';
    if(protected_path && *protected_path && !file_name(protected_path,protect)) return FILE_BAD_NAME;
    /* Preflight the complete batch. No output exists yet when overwrite is declined. */
    for(i=0;i<r->total;++i) {
        if(!export_target(base,o->mode,i,r->path)) return FILE_BAD_NAME;
        if(protect[0] && !strcmp(protect,r->path)) return FILE_BAD_NAME;
        present=file_exists(r->path);
        if(present<0) return FILE_OPEN_ERROR;
        if(present && !overwrite) return FILE_EXISTS;
    }
    job.format=o->format; job.source.flatten=o->flatten; job.source.background=o->background;
    job.source.doc=d; job.source.sheet=o->mode==EXPORT_SHEET;
    job.source.columns=o->layout==SHEET_HORIZONTAL?d->frames:o->layout==SHEET_VERTICAL?1:o->columns;
    for(i=0;i<r->total;++i) {
        export_target(base,o->mode,i,r->path);
        job.source.frame=o->mode==EXPORT_ALL?i:frame;
        status=file_write(r->path,write_bmp,&job,overwrite,&r->recovery);
        if(status==FILE_OK || status==FILE_BACKUP_LEFT) ++r->completed;
        /* Stop on retained backup too, so its recovery path cannot be lost. */
        if(status!=FILE_OK) return status;
    }
    return FILE_OK;
}
