extern void uart_init(void);
extern void uart_puts(char *s);
extern void uart_putc(char ch);
extern int  uart_getc(void);
void start_kernel(void)
{
	uart_init();
    uart_puts("Hello, RVOS!\n");

	while (1) {
        int a = 0;
        while((a=uart_getc())!=-1){
            if(a==0xd)
                uart_putc(0xa); // CR -> LF+CR
            uart_putc(a);
        }
    }; // stop here!
}

