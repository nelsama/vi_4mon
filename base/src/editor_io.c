#include "editor.h"
#define ESC "\033"
#define CSI ESC "["
void term_init(void) {
    rom_uart_puts(CSI "2J"); rom_uart_puts(CSI "H"); rom_uart_puts(CSI "?25l");
}
void term_clear(void) {
    rom_uart_puts(CSI "2J"); rom_uart_puts(CSI "H");
}
void term_goto(uint8_t row, uint8_t col) {
    rom_uart_putc(0x1B); rom_uart_putc('[');
    if(row>=100) rom_uart_putc('0'+(row/100));
    if(row>=10) rom_uart_putc('0'+((row/10)%10));
    rom_uart_putc('0'+(row%10)); rom_uart_putc(';');
    if(col>=100) rom_uart_putc('0'+(col/100));
    if(col>=10) rom_uart_putc('0'+((col/10)%10));
    rom_uart_putc('0'+(col%10)); rom_uart_putc('H');
}
void term_show_cursor(uint8_t show) {
    if(show) rom_uart_puts(CSI "?25h"); else rom_uart_puts(CSI "?25l");
}
void term_set_scroll(uint8_t top, uint8_t bottom) {
    rom_uart_putc(0x1B); rom_uart_putc('[');
    if(top>=10) rom_uart_putc('0'+(top/10));
    rom_uart_putc('0'+(top%10)); rom_uart_putc(';');
    if(bottom>=10) rom_uart_putc('0'+(bottom/10));
    rom_uart_putc('0'+(bottom%10)); rom_uart_putc('r');
}
void io_putc(char c) { rom_uart_putc(c); }
void io_puts(const char *s) { rom_uart_puts(s); }
char io_getc(void) { while(!rom_uart_rx_ready()); return rom_uart_getc(); }
uint8_t io_char_ready(void) { return rom_uart_rx_ready(); }
void io_gets(char *buf, uint8_t max) {
    uint8_t i=0; char c;
    while(1){
        c=io_getc();
        if(c=='\r'||c=='\n'){io_puts("\r\n");break;}
        if(c=='\b'||c==0x7F){if(i>0){i--;io_puts("\b \b");}continue;}
        if(i<max-1){buf[i++]=c;io_putc(c);}
    }
    buf[i]='\0';
}
