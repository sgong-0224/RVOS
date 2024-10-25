#include "os.h"

#define DELAY 1000

void user_task0(void)
{
	uart_puts("Task 0: Created!\n");
    while (1) {
		uart_puts("Task 0: Running...\n");
		task_delay(DELAY);
		task_yield();
	}
}

void user_task2(void)
{
	uart_puts("Task 2: Created!\n");
	int i=0;
    while (i++<5) {
		uart_puts("Task 2: Running...\n");
		task_delay(DELAY);
		task_yield();
	}
    uart_puts("Task 2: Finished.\n");
    task_exit();
}

void user_task1(int a, int b, int c)
{
	uart_puts("Task 1: Created!\n");
    printf("Task1 params: %d %d %d\n",a,b,c);
	int i=0;
	while (i++<5) {
		uart_puts("Task 1: Running...\n");
		task_delay(DELAY);
		task_yield();
	}
    uart_puts("Task 1: Finished.\n");
    task_exit();
}

void user_task0_wrap(void* params)
{
    user_task0();
}

void user_task2_wrap(void* params)
{
    user_task2();
}

typedef struct{int a,b,c;} func1_params;
void user_task1_wrap(void* params)
{
    func1_params* p = (func1_params*) params;
    user_task1(p->a,p->b,p->c);
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
    static func1_params func1_param={3,4,5};
	task_create(user_task0_wrap,NULL,100);
	task_create(user_task2_wrap,NULL,1);
	task_create(user_task1_wrap,&func1_param,2);
	task_create(user_task2_wrap,NULL,3);
	task_create(user_task1_wrap,&func1_param,4);
}

