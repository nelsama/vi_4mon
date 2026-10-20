/* ==========================================================================
   MAIN.C - Punto de entrada del Mini VI
   ==========================================================================
   Flujo:
     arranque -> pedir archivo -> cargar 1a pagina -> editor -> :q/:wq
     -> reiniciar el monitor con jmp $8000 (no hay retorno limpio).
   ========================================================================== */
#include "editor.h"

/* Sondas de compilacion (ver makefile): PROBE_MAIN solo imprime y vuelve;
   PROBE_MAIN_IO prueba ademas una llamada con argumento. */
void p8(uint8_t v);
void mkargs(void);

/* Sonda: se compila aparte con -DEDITOR_MARK=n. Deja huellas en la pila.
   El emulador no expone el registro SP, asi que se deduce del ultimo byte
   escrito en $0100-$01FF. */
#ifdef EDITOR_MARK
static void mark(uint8_t n)
{
    volatile uint8_t *pila = (volatile uint8_t *)0x01F0;
    pila[n] = (uint8_t)(0xA0 + n);
}
#endif

/* Sonda biseccion: con -DPROBE_FN=n se activa el tramo n de main. */
#ifndef PROBE_FN
#define PROBE_FN 0
#endif

#ifdef PROBE_MAIN

int main(void)
{
    volatile uint8_t *uart = (volatile uint8_t *)0xC020;
    volatile uint8_t *marca = (volatile uint8_t *)0x40;
    const char *s = "[main]";

    while (*s) *uart = (uint8_t)*s++;

#if PROBE_FN >= 1
    *marca = 1;                             /* store directo */
#endif
#if PROBE_FN >= 2
    *marca = *marca + 1;                    /* carga + suma + store */
#endif
#if PROBE_FN >= 3
    *marca = 13 << 2;                       /* shift constante */
#endif
#if PROBE_FN >= 4
    { uint8_t x = 13; *marca = x << 2; }    /* variable */
#endif
#if PROBE_FN >= 5
    p8(0);                                  /* llamada con argumento */
#endif
#if PROBE_FN >= 6
    { volatile uint16_t *b = (volatile uint16_t *)0x0200;
      uint8_t k;
      for (k = 0; k < 8; k++) { b[k] = k; *marca = k; } }
#endif
    return 0;
}

#else

/* Salta al monitor reinicializando la ROM ($8000). Hay que pasar por ASM:
   en -t none no existe la instruccion __asm__ en linea. */
extern void back_to_monitor_asm(void);

static void back_to_monitor(void)
{
    io_puts("\033[0m\033[2J\033[H\033[?25h");
    io_flush();
    back_to_monitor_asm();
}

int main(void)
{
    char name[MAX_FILENAME];
    uint16_t i;

#ifdef EDITOR_MARK
    mark(0);
#endif

    /* Dejar el estado en un punto conocido: el programa puede ejecutarse
       varias veces sin reiniciar la CPU. */
    for (i = 0; i < sizeof(st); i++) ((char *)&st)[i] = 0;

#ifdef EDITOR_MARK
    mark(1);
#endif
    io_init();
#ifdef EDITOR_MARK
    mark(2);
#endif
    io_puts("Mini VI " VERSION "\r\n");
    io_puts("Archivo: ");

    /* leer el nombre con eco */
    {
        uint8_t n = 0;
        char c;
        while (1) {
            c = io_getc();
            if (c == '\r') continue;
            if (c == '\n' || c == 0) break;
            if ((c == 8 || c == 0x7F) && n) {
                n--;
                io_puts("\b \b");
                continue;
            }
            if (n < MAX_FILENAME - 1 && c >= 32) {
                name[n++] = c;
                io_putc(c);
            }
        }
        name[n] = 0;
    }
    io_puts("\r\n");

    if (name[0]) {
        if (!txt_open(name)) {
            io_puts("No se pudo abrir\r\n");
            back_to_monitor();
        }
        vi_run();
    }

    back_to_monitor();
    return 0;
}
#endif
