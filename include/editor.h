/* ==========================================================================
   EDITOR.H - Mini editor vi para Monitor 6502 (Tang Nano 9K)

   Objetivos de diseno (impuestos por el hardware de 16 KB de RAM):
   - Cargar en $0800 como cualquier programa del monitor (no tocar $0000-$07FF).
   - Perfil vi: modos, h j k l, w b, 0 $, i a A I o O, x X, dd dw dG, yy p P,
     J, /, G, n, :w :q :q! :wq :e :set nu :help.
   - Buffer de trabajo de 16 KB (el archivo puede ser mayor: se pagina).
   - Ficheros de hasta 28 KB mediante pagina (lista de lineas en RAM) y
     lectura de SD con mfs_seek, sin cargar el archivo completo.
   ========================================================================== */
#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>
#include "romapi.h"

#define VERSION          "VI-2.0"
#define MAX_FILENAME     13     /* 12 + NUL (limite MicroFS) */

/* --- Geometria de pantalla (terminal ANSI 80x24) --- */
#define SCR_ROWS         21     /* filas de texto (1..21) */
#define SCR_COLS         80
#define ROW_STATUS       22     /* barra de estado (inversa) */
#define ROW_CMD          23     /* prompt : o / */
#define MAX_LINE         255    /* longitud maxima de una linea (255 + '\n') */
#define INDENT_COLS      3      /* "nn: " del :set nu */

/* --- Tamanos del buffer --- */
/* 4 KB de texto caben con el codigo en los 14 KB de RAM ($0800-$3DFF).
   El archivo puede ser mayor: se edita por paginas de TEXTO_BUF_SIZE bytes.
   RAM libre que sobra: ~8 KB (se puede subir a 6-8 KB sin tocar nada mas). */
#define TEXTO_BUF_SIZE   4096
#define PAGE_LINES       128    /* lineas maximas por pagina */

/* --- Direcciones --- */
#define TEXTO_BUF_ADDR   0x2A00       /* final de las 4 KB de texto     */
#define LINE_IDX_ADDR    0x2A00       /* indice de lineas: 128 x 2 B    */
#define TEXTO_DATA_ADDR  0x2B00       /* inicio de los datos del texto  */
#define TEXTO_DATA_SIZE  (TEXTO_BUF_ADDR + TEXTO_BUF_SIZE - TEXTO_DATA_ADDR)

/* Estructura de estado (en BSS) */
typedef struct {
    uint8_t  mode;              /* MODE_* */
    uint8_t  flags;             /* FLAG_* */
    uint8_t  cur_line;          /* linea del cursor dentro de la pagina */
    uint8_t  cur_col;           /* columna (0..len) */
    uint8_t  want_col;          /* columna deseada al subir/bajar */
    uint8_t  top_line;          /* primera linea dibujada */
    uint8_t  nlines;            /* lineas cargadas en la pagina */
    uint8_t  count;             /* contador previo (3dd, 2p...) */
    uint8_t  count_digits;
    uint8_t  prev_cmd;          /* ultimo comando de dos letras (d, y, g) */
    uint8_t  insert_append;     /* a / A: insertar tras el cursor */
    uint8_t  yank_len;
    uint8_t  search_dir;        /* 0 = adelante, 1 = atras */
    uint16_t buf_len;           /* bytes usados en el buffer de texto */
    uint16_t page_off;          /* offset de la pagina en el archivo */
    uint16_t file_size;         /* tamanio total del archivo */
    uint16_t page_line;         /* numero de la 1a linea de la pagina (0-based) */
    uint8_t  page_dirty;        /* 1 = la pagina se ha modificado */
    char     yank[64];
    char     search[32];
    char     msg[40];
} editor_state_t;

extern editor_state_t st;

#define MODE_NORMAL      0
#define MODE_INSERT      1
#define MODE_COMMAND     2
#define MODE_SEARCH      3

#define FLAG_MODIFIED    0x01
#define FLAG_RUNNING     0x02
#define FLAG_SHOW_NUM    0x04
#define FLAG_OPEN        0x08

/* --- Pantalla / teclado (io.c) --- */
void     io_init(void);
void     io_putc(char c);
void     io_puts(const char *s);
char     io_getc(void);
void     io_goto(uint8_t row, uint8_t col);
void     io_clear_eol(void);
void     io_show_cursor(uint8_t on);
void     io_msg(const char *s);
void     io_msg_num(const char *txt, uint16_t n);
void     io_prompt(const char *p, char *buf, uint8_t max);
void     io_flush(void);

/* --- Buffer de texto (text.c) --- */
uint8_t  txt_open(const char *name);         /* abre/crea y carga la 1a pagina */
uint8_t  txt_save(void);                     /* guarda la pagina a la SD */
uint8_t  txt_save_all(void);                 /* guarda el fichero completo */
uint8_t  txt_load_page(void);                /* recarga la pagina actual */
uint8_t  txt_copy_file(const char *dst_name, const char *src_name, uint16_t len);
void     txt_goto_prev_page(void);           /* retrocede a la pagina anterior */
uint8_t  txt_page_full(void);
void     txt_index_lines(void);              /* recalcula el indice de lineas */
uint16_t txt_line_off(uint8_t line);
uint8_t  txt_line_len(uint8_t line);
char    *txt_line(uint8_t line);
char    *txt_name(void);
uint8_t  txt_insert_char(uint8_t line, uint8_t col, char c);
uint8_t  txt_del_char(uint8_t line, uint8_t col);
uint8_t  txt_split_line(uint8_t line, uint8_t col);
uint8_t  txt_join_lines(uint8_t line);
uint8_t  txt_del_line(uint8_t line);
uint8_t  txt_ins_line(uint8_t line);
uint16_t txt_page_lines(void);               /* lineas de la pagina actual */

/* --- Cadenas (text.c; no hay libc con -t none) --- */
int      strcmp(const char *a, const char *b);
int      strncmp(const char *a, const char *b, unsigned int n);

/* --- Editor (vi.c) --- */
void     vi_run(void);
void     scr_draw(void);
void     scr_cursor(void);

/* --- Multiplicacion/division (mul.s) --- */
uint16_t _umul8(uint8_t a, uint8_t b);
uint8_t  _udiv8(uint8_t a, uint8_t b);
uint8_t  _umod8(uint8_t a, uint8_t b);
uint16_t _half16(uint16_t n);

#endif /* EDITOR_H */
