#include <stddef.h>
#include "history.h"

int history_create(History *h,Animation ANIM_FAR *doc)
{
    unsigned i;
    memset(h,0,sizeof(*h)); h->document=doc;
    for(i=0;i<HISTORY_STATES;++i) {
        h->states[i]=anim_alloc(sizeof(Animation));
        if(!h->states[i]) { history_destroy(h); return 0; }
    }
    return 1;
}
void history_destroy(History *h)
{
    unsigned i;
    for(i=0;i<HISTORY_STATES;++i) {
        if(h->states[i]) anim_free(h->states[i]);
        h->states[i]=NULL;
    }
}
static int matches(const History *h,const Canvas *c)
{ return h && c && c->pixels && h->count && h->width==c->width && h->height==c->height; }
static void capture(History *h,const Canvas *c)
{
    anim_copy(h->document->pixels+h->active*h->size,c->pixels,h->size);
}
static int equal(const Animation ANIM_FAR *a,const Animation ANIM_FAR *b,unsigned size)
{
    return a->frames==b->frames &&
        !anim_compare(a->duration,b->duration,a->frames*sizeof(unsigned)) &&
        !anim_compare(a->pixels,b->pixels,a->frames*size);
}
int history_init(History *h,const Canvas *c)
{
    unsigned long size;
    if(!h || !c || !c->pixels || !h->document || !h->states[0]) return 0;
    size=(unsigned long)c->width*c->height;
    if(!size || size>HISTORY_PIXELS) return 0;
    h->width=c->width; h->height=c->height; h->size=(unsigned)size;
    h->count=1; h->position=0;
    capture(h,c);
    anim_copy(h->states[0],h->document,sizeof(Animation));
    h->selected[0]=h->active;
    return 1;
}
int history_commit(History *h,const Canvas *c)
{
    unsigned i;
    Animation ANIM_FAR *old;
    if(!matches(h,c)) return 0;
    capture(h,c);
    if(equal(h->states[h->position],h->document,h->size)) return 0;
    h->selected[h->position]=h->active<h->states[h->position]->frames?
        h->active:h->states[h->position]->frames-1;
    h->count=h->position+1;
    if(h->count==HISTORY_STATES) {
        old=h->states[0];
        for(i=1;i<HISTORY_STATES;++i) {
            h->states[i-1]=h->states[i]; h->selected[i-1]=h->selected[i];
        }
        h->states[HISTORY_STATES-1]=old; --h->count;
    }
    h->position=h->count++;
    anim_copy(h->states[h->position],h->document,sizeof(Animation));
    h->selected[h->position]=h->active;
    return 1;
}
static void restore(History *h,Canvas *c)
{
    anim_copy(h->document,h->states[h->position],sizeof(Animation));
    h->active=h->selected[h->position];
    anim_copy(c->pixels,h->document->pixels+h->active*h->size,h->size);
}
int history_undo(History *h,Canvas *c)
{
    if(!matches(h,c) || !h->position) return 0;
    /* Navigation is not an action, but the next undo must know which frame
       owned the previous edit. Snapshots retain that edit's selection. */
    --h->position; restore(h,c); return 1;
}
int history_redo(History *h,Canvas *c)
{
    if(!matches(h,c) || h->position+1>=h->count) return 0;
    ++h->position; restore(h,c); return 1;
}
