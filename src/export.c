#include <string.h>
#include "export.h"
#include "bmp.h"

typedef struct ExportSource {
    const Animation ANIM_FAR *doc;
    unsigned frame,columns;
    int sheet;
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
    return n>=4 && !strcmp(output+n-4,".BMP");
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
    memcpy(output,name,start+n); sprintf(output+start+n,"%04u.BMP",frame+1);
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
    if(!valid(d) || !o || !w || !h || o->mode<EXPORT_CURRENT || o->mode>EXPORT_SHEET)
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
    return bmp_size(*w,*h,&bytes);
}
static int read_span(const void *ctx,unsigned x,unsigned y,unsigned char *p,unsigned n)
{
    const ExportSource *s;
    const Animation ANIM_FAR *d;
    unsigned i,f,offset;
    s=(const ExportSource *)ctx; d=s->doc;
    for(i=0;i<n;++i,++x) {
        f=s->sheet?(y/d->height)*s->columns+x/d->width:s->frame;
        offset=(y%d->height)*d->width+x%d->width;
        p[i]=f<d->frames?d->pixels[f*d->width*d->height+offset]:d->background;
    }
    return 1;
}
typedef struct BmpJob { ExportSource source; unsigned width,height; } BmpJob;
static int write_bmp(FILE *f,const void *ctx)
{
    const BmpJob *job;
    job=(const BmpJob *)ctx;
    return bmp_write(f,job->width,job->height,read_span,&job->source);
}
int export_run(const Animation ANIM_FAR *d,unsigned frame,const char *path,
               const ExportOptions *o,int overwrite,const char *protected_path,ExportReport *r)
{
    BmpJob job;
    unsigned i;
    int status,present;
    char protect[FILE_PATH_MAX];
    if(!r) return FILE_WRITE_ERROR;
    memset(r,0,sizeof(*r));
    status=export_size(d,o,&job.width,&job.height);
    if(status!=FILE_OK || frame>=d->frames) return status==FILE_OK?FILE_DIMENSIONS:status;
    r->total=o->mode==EXPORT_ALL?d->frames:1;
    protect[0]='\0';
    if(protected_path && *protected_path && !file_name(protected_path,protect)) return FILE_BAD_NAME;
    /* Preflight the complete batch. No output exists yet when overwrite is declined. */
    for(i=0;i<r->total;++i) {
        if(!export_target(path,o->mode,i,r->path)) return FILE_BAD_NAME;
        if(protect[0] && !strcmp(protect,r->path)) return FILE_BAD_NAME;
        present=file_exists(r->path);
        if(present<0) return FILE_OPEN_ERROR;
        if(present && !overwrite) return FILE_EXISTS;
    }
    job.source.doc=d; job.source.sheet=o->mode==EXPORT_SHEET;
    job.source.columns=o->layout==SHEET_HORIZONTAL?d->frames:o->layout==SHEET_VERTICAL?1:o->columns;
    for(i=0;i<r->total;++i) {
        export_target(path,o->mode,i,r->path);
        job.source.frame=o->mode==EXPORT_ALL?i:frame;
        status=file_write(r->path,write_bmp,&job,overwrite,&r->recovery);
        if(status==FILE_OK || status==FILE_BACKUP_LEFT) ++r->completed;
        /* Stop on retained backup too, so its recovery path cannot be lost. */
        if(status!=FILE_OK) return status;
    }
    return FILE_OK;
}
