extern void uart_init(void);
extern void uart_puts(char *s);
extern void uart_putc(char ch);
extern int  uart_getc(void);
void start_kernel(void)
{
	uart_init();
    int a = uart_getc();
    if(a!=-1)
        uart_putc(a);
	
    uart_putc('u');
	uart_putc('s');
	uart_putc('t');
	uart_putc('c');
	uart_putc(':');
	uart_putc(' ');
    uart_puts("Hello, RVOS!\n");

	while (1) {}; // stop here!
}

