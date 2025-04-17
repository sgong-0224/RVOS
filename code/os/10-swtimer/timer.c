#include "os.h"

extern void schedule(void);

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

#define MAX_TIMER 10
// static struct timer timer_list[MAX_TIMER];
static struct timer* timer_list = NULL;

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
}

struct timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout)
{
	/* TBD: params should be checked more, but now we just simplify this */
	if (NULL == handler || 0 == timeout) {
		return NULL;
	}

	/* use lock to protect the shared timer_list between multiple tasks */
	spin_lock();

    struct timer* t = (struct timer*) malloc(sizeof(struct timer));
	t->func = handler;
	t->arg = arg;
	t->timeout_tick = _tick + timeout;
    t->next = NULL;
    if( timer_list == NULL ) {
        timer_list = t;
    } else {
        // 按照时间顺序插入
        for(struct timer* ptr = timer_list; ptr!=NULL;ptr = ptr->next){
            if( ptr->timeout_tick <= t->timeout_tick
            && ( ptr->next==NULL || t->timeout_tick <= ptr->next->timeout_tick ) ){
                t->next = ptr->next;
                ptr->next = t;   
                break;
            }
        }
    }

	spin_unlock();

	return t;
}

void timer_delete(struct timer *timer)
{
	spin_lock();

	struct timer* prev = NULL;
	for(struct timer *t = timer_list;t!=NULL;t=t->next){
		if (t == timer) {
			if(prev==NULL){
                timer_list = t->next;
            } else {
                prev->next = t->next;
                t->func = NULL;
                t->arg = NULL;
                t->next = NULL;
            }
            free(t);
            break;
		}
        prev = t;
	}

	spin_unlock();
}

/* this routine should be called in interrupt context (interrupt is disabled) */
static inline void timer_check()
{
    struct timer* prev = NULL;
	for (struct timer* t=timer_list; t!=NULL; t=t->next) {
		if (NULL != t->func) {
			if (_tick >= t->timeout_tick) {
				t->func(t->arg);

				/* once time, just delete it after timeout */
				if(prev==NULL){
                    timer_list = t->next;
                } else {
                    prev->next = t->next;
                    t->func = NULL;
                    t->arg = NULL;
                    t->next = NULL;
                }
                free(t);
                break;
			}
		}
        prev = t;
	}
}

void timer_handler() 
{
	_tick++;
	printf("tick: %d\n", _tick);

	timer_check();

	timer_load(TIMER_INTERVAL);

	schedule();
}
