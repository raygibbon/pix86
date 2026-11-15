#ifndef PIX86_PROFILE_H
#define PIX86_PROFILE_H

#ifdef PIX86_PROFILE
typedef struct ProfileStats {
    unsigned long framebuffer_pixel_writes;
    unsigned long rectangles,scanlines;
    unsigned long canvas_full,canvas_partial,canvas_cells,canvas_compares;
    unsigned long playback_cell_draws,playback_span_draws,playback_full_draws;
    unsigned long ui_full,ui_controls,ui_groups,ui_flyouts,ui_menus;
    unsigned long timeline_full,timeline_partial,overlay_restores;
    unsigned long vbe_bank_switches,mouse_polls,status_calls,status_chars;
    unsigned long screen_ticks,screen_samples,screen_max_ticks;
    unsigned long input_ticks,input_samples,input_max_ticks;
} ProfileStats;

extern ProfileStats pix86_profile;
/* Harness only: 0 adaptive, 1 cells, 2 spans, 3 complete artwork. */
extern unsigned pix86_render_strategy;
unsigned long profile_now(void);
void profile_record(unsigned long *total,unsigned long *samples,
                    unsigned long *maximum,unsigned long start);
void profile_report(void);

#define PROFILE_COUNT(field) (++pix86_profile.field)
#define PROFILE_ADD(field,value) (pix86_profile.field+=(unsigned long)(value))
#define PROFILE_START(name) unsigned long name=profile_now()
#define PROFILE_STOP(field,name) \
    profile_record(&pix86_profile.field##_ticks, \
                   &pix86_profile.field##_samples, \
                   &pix86_profile.field##_max_ticks,name)
#else
#define PROFILE_COUNT(field) ((void)0)
#define PROFILE_ADD(field,value) ((void)0)
#define PROFILE_START(name) ((void)0)
#define PROFILE_STOP(field,name) ((void)0)
#define profile_report() ((void)0)
#endif

#endif
