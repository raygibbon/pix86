#include "profile.h"

#ifdef PIX86_PROFILE
#include <stdio.h>
#include "timer.h"

ProfileStats pix86_profile;
unsigned pix86_render_strategy;

unsigned long profile_now(void)
{
    timer_precise();
    return timer_now();
}

void profile_record(unsigned long *total,unsigned long *samples,
                    unsigned long *maximum,unsigned long start)
{
    unsigned long elapsed;
    elapsed=profile_now()-start;
    *total+=elapsed;
    ++*samples;
    if(elapsed>*maximum) *maximum=elapsed;
}

void profile_report(void)
{
    printf("PROFILE writes=%lu rects=%lu scanlines=%lu\n",
           pix86_profile.framebuffer_pixel_writes,
           pix86_profile.rectangles,pix86_profile.scanlines);
    printf("PROFILE canvas full=%lu partial=%lu cells=%lu compares=%lu timeline full=%lu partial=%lu\n",
           pix86_profile.canvas_full,pix86_profile.canvas_partial,
           pix86_profile.canvas_cells,pix86_profile.canvas_compares,
           pix86_profile.timeline_full,
           pix86_profile.timeline_partial);
    printf("PROFILE playback cells=%lu spans=%lu full-art=%lu\n",
           pix86_profile.playback_cell_draws,
           pix86_profile.playback_span_draws,
           pix86_profile.playback_full_draws);
    printf("PROFILE UI full=%lu controls=%lu groups=%lu flyouts=%lu menus=%lu overlays=%lu\n",
           pix86_profile.ui_full,pix86_profile.ui_controls,
           pix86_profile.ui_groups,pix86_profile.ui_flyouts,
           pix86_profile.ui_menus,pix86_profile.overlay_restores);
    printf("PROFILE VBE banks=%lu mouse polls=%lu status calls=%lu chars=%lu\n",
           pix86_profile.vbe_bank_switches,pix86_profile.mouse_polls,
           pix86_profile.status_calls,pix86_profile.status_chars);
    printf("PROFILE screen ticks=%lu samples=%lu max=%lu input ticks=%lu samples=%lu max=%lu\n",
           pix86_profile.screen_ticks,pix86_profile.screen_samples,
           pix86_profile.screen_max_ticks,pix86_profile.input_ticks,
           pix86_profile.input_samples,pix86_profile.input_max_ticks);
    printf("PROFILE timer=%lu units/sec precise=%d (read-only PIT on AT+)\n",
           TIMER_HZ,timer_precise());
}
#endif
