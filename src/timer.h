#ifndef PIX86_TIMER_H
#define PIX86_TIMER_H
/* Unsigned differences, in nominal 1/1193182 s units; wraps after ~1 hour.
   Reads only: no timer programming or interrupt hooks. */
unsigned long timer_now(void);
int timer_precise(void);
#define TIMER_HZ 1193182UL
#define TIMER_MS(ticks) ((ticks)/1193UL)
#endif
