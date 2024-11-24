#include "os.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 4
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t __attribute__((aligned(16))) task_stack[MAX_TASKS][STACK_SIZE];
// task_info
typedef struct {
    void (*start_routine)(void* param);
    uint32_t timeslice;
    uint32_t remaining_timeslice;
    uint16_t priority;
    struct context ctx;
} task_info;
task_info tasks[MAX_TASKS];
/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;

void sched_init()
{
	w_mscratch(0);
	/* enable machine-mode software interrupts. */
	w_mie(r_mie() | MIE_MSIE);
}
/*
 * implment a scheduler
 */
void schedule()
{
	if (_top < 0) {
        panic("Num of task should be greater than zero!");
        return;
    }
    
    uint8_t task_scheduled = 0;

    if(_current>=0) // _current执行了一个tick，更新剩余时间片
        tasks[_current].remaining_timeslice -= 1;

    for(uint16_t priority=0;priority<=255;++priority) {
        for (int i = 0; i < _top; ++i) {
            if (tasks[i].priority == priority && tasks[i].remaining_timeslice != 0 && i!=_current) {
                if ( _current<0 || (_current >= 0 && tasks[_current].priority > priority) ) {
                    // task[i]优先级更高或是首个任务，切换到该任务
                    tasks[i].remaining_timeslice = tasks[i].timeslice;
                    _current = i;
                    switch_to(&(tasks[i].ctx));
                    task_scheduled = 1;
                    break;
                } else if ( priority==tasks[_current].priority ) {
                    // 相同优先级任务轮转
                    tasks[i].remaining_timeslice = tasks[i].timeslice;
                    _current = i;
                    switch_to(&(tasks[i].ctx));
                    task_scheduled = 1;
                    break;
                }
            }
        }
        // 如果已经调度了一个任务，就退出循环
        if (task_scheduled)
            break;
    }

    if(!task_scheduled){
        // 如果没有找到任务，尝试继续调度_current; 如果_current时间片耗尽，等到下次调度时再执行
        if( tasks[_current].remaining_timeslice == 0 ){
            tasks[_current].remaining_timeslice  = tasks[_current].timeslice;
            for(uint16_t priority = tasks[_current].priority+1;priority<=255;++priority){
                for(int i=0;i<_top;++i){
                    if(tasks[i].priority==priority){
                        tasks[i].remaining_timeslice = tasks[i].timeslice;
                        _current = i;
                        switch_to(&(tasks[i].ctx));
                        task_scheduled = 1;
                        break;
                    }
                }
                if(task_scheduled)
                    break;
            }
        }
    }
}

/*
 * DESCRIPTION
 * 	Create a task.
 * 	- start_routin: task routine entry
 * RETURN VALUE
 * 	0: success
 * 	-1: if error occured
 */
int task_create(void (*start_routine)(void* param), void *param, uint8_t priority, uint32_t timeslice)
{
	if (_top < MAX_TASKS-1) {
        tasks[_top].start_routine = start_routine;
        tasks[_top].priority = priority;
        tasks[_top].timeslice = timeslice;
        tasks[_top].remaining_timeslice = timeslice;

        tasks[_top].ctx.sp = (reg_t) &task_stack[_top][STACK_SIZE-1];
        tasks[_top].ctx.pc = (reg_t) start_routine;
        tasks[_top].ctx.a0 = (reg_t) param;
		_top++;

        if( priority < tasks[_current].priority )
            task_yield();

		return 0;
	} else {
		return -1;
	}
}

/*
 *  增加任务退出接口 task_exit()，当前任务可以通过调用该接口退出执行，内核负责将该任务回收，并调度下⼀个可运行任务。
 *  void task_exit(void)
 */
void task_exit()
{
    tasks[_current].priority = 257;
    tasks[_current].timeslice = 0;
    tasks[_current].remaining_timeslice = 0;
    _current = -1;
    task_yield();
}

/*
 * DESCRIPTION
 * 	task_yield()  causes the calling task to relinquish the CPU and a new 
 * 	task gets to run.
 */
void task_yield()
{
	/* trigger a machine-level software interrupt */
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 1;
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}
