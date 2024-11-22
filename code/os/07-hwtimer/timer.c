#include "os.h"

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
	/* each CPU has a separate source of timer interrupts. */
	int id = r_mhartid();
	
	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init()
{
	/*
	 * On reset, mtime is cleared to zero, but the mtimecmp registers 
	 * are not reset. So we have to init the mtimecmp manually.
	 */
	timer_load(TIMER_INTERVAL);

	/* enable machine-mode timer interrupts. */
	w_mie(r_mie() | MIE_MTIE);

	/* enable machine-mode global interrupts. */
	w_mstatus(r_mstatus() | MSTATUS_MIE);
}

void timer_handler() 
{
	_tick++;
	// printf("tick: %d\n", _tick);
    int hr  = _tick / 3600;
    int hr_hi = hr/10;
    int hr_lo = hr%10;
    int min = _tick / 60;
    int min_hi = min/10;
    int min_lo = min%10;
    int sec = _tick % 60;
    int sec_hi = sec/10;
    int sec_lo = sec%10;
    printf("%d%d:%d%d:%d%d\r",hr_hi,hr_lo,min_hi,min_lo,sec_hi,sec_lo);
	timer_load(TIMER_INTERVAL);
}
