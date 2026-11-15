#ifndef PIX86_ANIM_H
#define PIX86_ANIM_H
#include <stdlib.h>
#include <string.h>
#ifdef __WATCOMC__
#include <malloc.h>
/* Strict ANSI mode hides CRT far-memory declarations in string.h.
   Declare only these documented runtime interfaces; ordinary modules stay -za. */
extern void __far *_fmemcpy(void __far *,const void __far *,size_t);
extern void __far *_fmemmove(void __far *,const void __far *,size_t);
extern void __far *_fmemset(void __far *,int,size_t);
extern int _fmemcmp(const void __far *,const void __far *,size_t);
#define ANIM_FAR __far
#define anim_alloc _fmalloc
#define anim_free _ffree
#define anim_copy _fmemcpy
#define anim_move _fmemmove
#define anim_set _fmemset
#define anim_compare _fmemcmp
#else
#define ANIM_FAR
#define anim_alloc malloc
#define anim_free free
#define anim_copy memcpy
#define anim_move memmove
#define anim_set memset
#define anim_compare memcmp
#endif
#define ANIM_FRAMES 16
#define ANIM_PIXELS 16384
#define ANIM_MASK_BYTES (ANIM_PIXELS / 8)
#define ANIM_DEFAULT_MS 100
typedef struct Animation {
    unsigned width,height;
    unsigned char foreground,background;
    unsigned char pixels[ANIM_PIXELS];
    unsigned char mask[ANIM_MASK_BYTES];
    unsigned char transparent; /* explicit transparent blank-frame policy */
    unsigned frames;
    unsigned duration[ANIM_FRAMES];
} Animation;
/* Opaque indexed images. Zero is an ordinary colour. */
int anim_dimensions(unsigned width,unsigned height);
#endif
