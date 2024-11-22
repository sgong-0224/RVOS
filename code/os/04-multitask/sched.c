#include "os.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 1024
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t __attribute__((aligned(16))) task_stack[MAX_TASKS][STACK_SIZE];

// TODO: task_info
typedef struct {
    struct context ctx;
    void (*start_routine)(void* param);
    uint8_t priority;
    uint8_t exited;
} task_info;

task_info tasks[MAX_TASKS];

/*
 * _top is used to mark the max available position of tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;
static int current_priority = 0;

static void w_mscratch(reg_t x)
{
	asm volatile("csrw mscratch, %0" : : "r" (x));
}

/*
 * implment a simple cycle FIFO schedular
 * 修改任务调度算法，在原先简单轮转的基础上⽀持按照优先级排序，优先选择优先级⾼的任务运⾏，同⼀级多个任务再轮转。
 */
#define SCHED_PRIORITY 254
task_info scheduler;
void schedule()
{
    if (_top < 0) {
        panic("Num of task should be greater than zero!");
        return;
    }
    
    int priority = 0;

    while ( priority!=255 ) {
        for (int i = 0; i < _top; ++i) {
            if ( tasks[i].priority == priority && !tasks[i].exited ) {
                // 找最高优先级的任务
                _current = i;
                current_priority = tasks[_current].priority;
                switch_to(&(tasks[_current].ctx));
                return;
            }
        }
        // 尝试更低的优先级
        ++priority; 
    }
    _current = -1;
    switch_to(&scheduler.ctx);
}

// 内核调度任务
void sched_task()
{
    while(1){
        schedule();
    }
}

void sched_init()
{
	w_mscratch(0);

    // scheduler初始化
    scheduler.start_routine = &sched_task;
    scheduler.priority = SCHED_PRIORITY;
    scheduler.ctx.sp = (reg_t) &task_stack[MAX_TASKS-1][STACK_SIZE-1];
    scheduler.ctx.ra = (reg_t) &sched_task;
    scheduler.ctx.a0 = 0;
    scheduler.exited = 0;
}


/*
 * DESCRIPTION
 * 	Create a task.
 * 	- start_routin: task routine entry
 * RETURN VALUE
 * 	0: success
 * 	-1: if error occured
 *
 *  param ⽤于在创建任务执⾏函数时可带入参数，如果没有参数则传入 NULL。
 *  priority ⽤于指定任务的优先级，⽬前要求最多⽀持 256 级，0 最⾼，依次类推。
 *  
 */
int task_create(void (*start_routine)(void* param), void *param, uint8_t priority)
{
    if ( _top < MAX_TASKS-1 ) {
        // 初始化任务信息
        tasks[_top].start_routine = start_routine;
        tasks[_top].priority = priority;

        // 设置上下文
        tasks[_top].ctx.sp = (reg_t) &task_stack[_top][STACK_SIZE-1];
        tasks[_top].ctx.ra = (reg_t) start_routine;
        tasks[_top].ctx.a0 = (reg_t) param;
        _top++;
        return 0;
    } else {
        return -1;
    }
}

/*
 *  增加任务退出接⼝ task_exit()，当前任务可以通过调⽤该接⼝退出执⾏，内核负责将该任务回收，并调度下⼀个可运⾏任务。
 *  void task_exit(void)
 */
void task_exit()
{
    tasks[_current].exited = 1;
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
	switch_to(&scheduler.ctx);
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

