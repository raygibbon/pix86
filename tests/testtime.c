#include <stdio.h>
#include <dos.h>
#include <time.h>
#include "timer.h"
int main(void)
{
    unsigned long i,a,b,delta,total,coarse;
    clock_t start;
    timer_precise(); start=clock(); a=timer_now(); total=0;
    for(i=0;i<200000UL;++i) {
        b=timer_now(); delta=b-a;
        if(delta>TIMER_HZ) {
            printf("FAIL: timer reversed %lu -> %lu at %lu\n",a,b,i);
            return 1;
        }
        total+=delta; a=b;
    }
    coarse=(unsigned long)(clock()-start)*1000UL/CLOCKS_PER_SEC;
    if(TIMER_MS(total)>coarse+110UL || coarse>TIMER_MS(total)+110UL) {
        printf("FAIL: PIT %lums disagrees with DOS clock %lums\n",TIMER_MS(total),coarse);
        return 1;
    }
    a=timer_now(); delay(100); b=timer_now();
    delta=TIMER_MS(b-a);
    if(delta<50 || delta>200) {
        printf("FAIL: 100ms delay measured %lu ms\n",delta); return 1;
    }
    printf("PASS: 200000 monotonic timer samples, precise=%d, delay=%lums, run=%lums\n",
           timer_precise(),delta,TIMER_MS(total));
    return 0;
}
