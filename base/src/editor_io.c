#include "editor.h"

/* ==========================================================================
   EDITOR_IO.C - Terminal ANSI por UART
   ==========================================================================
   Todas las escrituras pasan por io_putc(), que espera a la UART antes de
   enviar cada byte. Sin esa espera se perdian bytes sueltos (el ESC de una
   secuencia ANSI, el '1' de "[41;37m") y la barra de estado salia cortada.
   ========================================================================== */

#define ESC "\033"
#define CSI ESC "["

#define UART_STATUS 0xC021
#define UART_TX_RDY 0x01

static void uart_wait(void)
{
    volatile uint8_t d;
    while (!(*(volatile uint8_t *)UART_STATUS & UART_TX_RDY))
        ;
    for (d = 0; d < 8; d++)
        ;                       /* margen entre bytes */
}

void io_putc(char c)
{
    uart_wait();
    rom_uart_putc(c);
}

void io_puts(const char *s)
{
    while (*s) io_putc(*s++);
}

void term_init(void)
{
    io_puts(CSI "2J" CSI "H" CSI "?25l");
}

void term_clear(void)
{
    io_puts(CSI "2J" CSI "H");
}

void term_goto(uint8_t row, uint8_t col)
{
    io_putc(0x1B);
    io_putc('[');
    if (row >= 100) io_putc('0' + (row / 100));
    if (row >= 10)  io_putc('0' + ((row / 10) % 10));
    io_putc('0' + (row % 10));
    io_putc(';');
    if (col >= 100) io_putc('0' + (col / 100));
    if (col >= 10)  io_putc('0' + ((col / 10) % 10));
    io_putc('0' + (col % 10));
    io_putc('H');
}

void term_show_cursor(uint8_t show)
{
    io_puts(show ? CSI "?25h" : CSI "?25l");
}

void term_set_scroll(uint8_t top, uint8_t bottom)
{
    io_putc(0x1B);
    io_putc('[');
    if (top >= 10) io_putc('0' + (top / 10));
    io_putc('0' + (top % 10));
    io_putc(';');
    if (bottom >= 10) io_putc('0' + (bottom / 10));
    io_putc('0' + (bottom % 10));
    io_putc('r');
}

char io_getc(void)
{
    while (!rom_uart_rx_ready())
        ;
    return rom_uart_getc();
}

uint8_t io_char_ready(void)
{
    return rom_uart_rx_ready();
}

/* Lee una linea con eco hasta Enter (CR o LF). */
void io_gets(char *buf, uint8_t max)
{
    uint8_t i = 0;
    char c;

    while (1) {
        c = io_getc();
        if (c == '\r' || c == '\n') {
            io_puts("\r\n");
            break;
        }
        if (c == '\b' || c == 0x7F) {
            if (i > 0) {
                i--;
                io_puts("\b \b");
            }
            continue;
        }
        if (i < max - 1 && c >= 0x20) {
            buf[i++] = c;
            io_putc(c);
        }
    }
    buf[i] = '\0';
}
