/* ==========================================================================
   IO.C - Pantalla ANSI por UART y lectura de teclado
   ==========================================================================
   Todo pasa por la ROM API del monitor ($BF00). Nada de librerias de cc65:
   el programa ocupa lo minimo y la ROM ya sabe hablar con la UART.
   ========================================================================== */
#include "editor.h"

#define ESC "\033"
#define CSI ESC "["

#define UART_DATA   0xC020
#define UART_STATUS 0xC021
#define UART_TX_RDY 0x01

/* A 115200 baudios un byte tarda ~87 us; con la CPU a 3.375 MHz son ~293
   ciclos. El software espera de verdad a que la UART este libre: asi el
   ritmo es el del hardware real (y el emulador, que no modela el BUSY, no
   mete 14 KB de golpe en el buffer TCP). Con ~100 ciclos por byte queda un
   margen del triple. Ajustar aqui si se cambia la velocidad de la UART. */
#define UART_BYTE_CYCLES 100

/* --- salida ------------------------------------------------------------- */

void io_init(void)
{
    io_puts(CSI "2J" CSI "H");      /* limpiar y cursor arriba */
}

void io_putc(char c)
{
    volatile uint8_t i = UART_BYTE_CYCLES;

    while (!(*((volatile uint8_t *)UART_STATUS) & UART_TX_RDY))
        ;
    rom_uart_putc(c);
    while (--i)
        ;
}

void io_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') io_putc('\r');
        io_putc(*s++);
    }
}

void io_flush(void)
{
    /* esperar a que el ultimo byte salga antes de saltar a la ROM */
    volatile uint16_t i;
    for (i = 0; i < 20000; i++)
        ;
}

/* cursor a (fila, col) 1-based: ESC [ fila ; col H */
void io_goto(uint8_t row, uint8_t col)
{
    io_putc(27); io_putc('[');
    if (row >= 10) io_putc('0' + row / 10);
    io_putc('0' + row % 10);
    io_putc(';');
    if (col >= 10) io_putc('0' + col / 10);
    io_putc('0' + col % 10);
    io_putc('H');
}

void io_clear_eol(void)
{
    io_puts(CSI "K");
}

void io_show_cursor(uint8_t on)
{
    if (on) io_puts(CSI "?25h");
    else    io_puts(CSI "?25l");
}

/* --- mensajes en la barra de estado ------------------------------------- */

void io_msg(const char *s)
{
    uint8_t i;
    io_puts(CSI "0m");
    io_goto(ROW_STATUS, 1);
    io_clear_eol();
    for (i = 0; s[i] && i < 60; i++) io_putc(s[i]);
}

void io_msg_num(const char *txt, uint16_t n)
{
    char buf[24];
    uint8_t i = 0;
    uint16_t d = 10000;

    while (txt[i] && i < 12) { buf[i] = txt[i]; i++; }
    while (d) {
        buf[i++] = '0' + (char)((n / d) % 10);
        d /= 10;
    }
    buf[i] = 0;
    io_msg(buf);
}

/* --- entrada ------------------------------------------------------------ */

char io_getc(void)
{
    while (!rom_uart_rx_ready())
        ;
    return rom_uart_getc();
}

/* Lee una linea mostrando "prompt" en la fila de comandos. */
void io_prompt(const char *p, char *buf, uint8_t max)
{
    uint8_t i = 0;
    char c;

    io_puts(CSI "0m");
    io_goto(ROW_CMD, 1);
    io_clear_eol();
    io_puts(p);

    while (1) {
        c = io_getc();
        if (c == '\r' || c == '\n') {
            io_puts("\r\n");
            break;
        }
        if (c == 8 || c == 0x7F) {
            if (i) {
                i--;
                io_puts("\b \b");
            }
            continue;
        }
        if (c == 27) {            /* ESC cancela */
            i = 0;
            io_puts("\r\n");
            break;
        }
        if (i < max - 1 && c >= 32) {
            buf[i++] = c;
            io_putc(c);
        }
    }
    buf[i] = 0;
}
