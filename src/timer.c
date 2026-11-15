#include "timer.h"
#ifdef __WATCOMC__
#include <dos.h>
#include <conio.h>
/* Preserve the caller's interrupt flag, including callers already in CLI. */
static unsigned lock_timer(void);
#pragma aux lock_timer = "pushf" "pop ax" "cli" __value [__ax] __modify __exact [__ax]
static void unlock_timer(unsigned flags);
#pragma aux unlock_timer = "push ax" "popf" __parm [__ax] __modify __exact []
static int precise=-1;
static unsigned long last_ticks,day_bias;
int timer_precise(void)
{
    unsigned char model;
    if(precise<0) {
        model=*(unsigned char __far *)MK_FP(0xf000,0xfffe);
        /* Original PC/XT/PCjr use 8253, without status read-back. */
        precise=model!=0xff && model!=0xfe && model!=0xfd;
    }
    return precise;
}
unsigned long timer_now(void)
{
    unsigned flags,count,phase,status,pending,retry;
    unsigned long ticks;
    flags=lock_timer();
    ticks=*(volatile unsigned long __far *)MK_FP(0x40,0x6c);
    phase=0;
    if(precise>0) {
        /* 8254 latch count AND status of channel 0, leave divisor untouched. */
        retry=3;
        do {
            outp(0x43,0xc2);
            status=inp(0x40);
            count=inp(0x40); count|=(unsigned)inp(0x40)<<8;
        } while(!count && --retry);
        outp(0x20,0x0a); pending=inp(0x20)&1;
        if((status&0x7f)==0x36) {
            phase=(unsigned)(0-count)>>1;
            if(!(status&0x80)) phase|=0x8000;
        } else if((status&0x7f)==0x34) phase=(unsigned)(0-count);
        else precise=0;
        /* IRQ0 pending while BIOS ticks still describe the preceding cycle.
           Near the latch/IRQ boundary prefer the pre-wrap sample. */
        if(pending && phase<0x8000) ++ticks;
    }
    if(ticks<last_ticks) day_bias+=0x00b00000UL;
    last_ticks=ticks;
    unlock_timer(flags);
    return ((ticks<<16)|phase)+day_bias;
}
#else
#include <time.h>
int timer_precise(void) { return 0; }
unsigned long timer_now(void)
{
    return (unsigned long)((double)clock()*(double)TIMER_HZ/CLOCKS_PER_SEC);
}
#endif
