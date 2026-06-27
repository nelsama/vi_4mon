/* ==========================================================================
   EDITOR.H - Editor tipo vi para Tang Nano 9K / 6502
   ========================================================================== */
#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>
#include "romapi.h"

#define VERSION         "VI-1.1"
#define MAX_LINE        80
#define MAX_LINES       256
#define MAX_FILENAME    12

/* EDIT_BUF es un array en C (segmento EDITBSS, $36E0-$3BFF) */
#define EDIT_BUF_SIZE   1300
#define EDIT_BUF_ADDR   0x36E0
#define EDIT_BUF       EDIT_BUF_ADDR
extern unsigned char edit_buf[EDIT_BUF_SIZE];
#define STATE_ADDR      0x0660
#define LINE_IDX_ADDR   0x06C8        /* 128 bytes - 64 entradas de uint16_t */
#define LINE_IDX_MAX    64
#define SCRATCH_ADDR    0x06C0        /* 56 bytes - temporal para MFS */
#define YANK_BUF_ADDR   0x0748        /* 64 bytes */
#define TEMP_LINE_ADDR  0x0788        /* 64 bytes */
#define STACK_AREA      0x07C8

#define MODE_NORMAL     0
#define MODE_INSERT     1
#define MODE_COMMAND    2
#define MODE_SEARCH     3
#define MODE_VISUAL     4

#define FLAG_MODIFIED   0x01
#define FLAG_SHOW_NUM   0x02
#define FLAG_SECTOR_DIRTY 0x04
#define FLAG_FILE_OPEN  0x08
#define FLAG_RUNNING    0x80

typedef struct {
    uint16_t sector_start;
    uint16_t file_size;
    uint16_t file_sectors;
    uint16_t edit_size;
    uint16_t sector_idx;
    uint16_t sector_global_off;
    uint16_t sector_valid;
    uint8_t  lines_in_buf;
    uint8_t  pad0;
    uint8_t  cur_line;
    uint8_t  cur_col;
    uint16_t cur_global_line;
    uint16_t cur_global_offset;
    uint8_t  scroll_line;
    uint8_t  screen_lines;
    uint8_t  mode;
    uint8_t  flags;
    uint8_t  count;
    uint8_t  count_digits;
    uint16_t yank_offset;
    uint8_t  yank_len;
    uint8_t  pad1;
    char     search_pat[16];
    uint8_t  search_dir;
    uint8_t  search_active;
    uint16_t search_line;
    char     filename[MAX_FILENAME];
    uint8_t  pad2;
} editor_state_t;

#define g_state     ((volatile editor_state_t*)STATE_ADDR)

/* Macros absolutas para campos críticos */
#define STATE_EDIT_SIZE      (*(volatile uint16_t*)0x06C0)
#define STATE_SECTOR_VALID   (*(volatile uint16_t*)0x06C2)
#define STATE_SECTOR_START   (*(volatile uint16_t*)0x06C4)
#define STATE_FILE_SIZE      (*(volatile uint16_t*)0x06C6)
extern char g_filename[12];
#define STATE_FILENAME       g_filename

/* Prototipos - editor_buffer.c */
uint8_t buf_load(const char *filename);
uint8_t buf_save(void);
uint8_t buf_load_sector(uint16_t sector_idx);
uint8_t buf_flush_sector(void);
void    buf_rebuild_lines(void);
char*   buf_get_line(uint8_t line_num);
uint8_t buf_line_len(uint8_t line_num);
uint8_t buf_insert_char(uint8_t line, uint8_t col, char c);
uint8_t buf_delete_char(uint8_t line, uint8_t col);
uint8_t buf_split_line(uint8_t line, uint8_t col);
uint8_t buf_join_lines(uint8_t line);
uint8_t buf_delete_line(uint8_t line);
uint8_t buf_insert_line(uint8_t line);

/* Prototipos - editor_io.c */
void    term_init(void);
void    term_clear(void);
void    term_goto(uint8_t row, uint8_t col);
void    term_show_cursor(uint8_t show);
void    term_set_scroll(uint8_t top, uint8_t bottom);
void    __fastcall__ io_putc(char c);
void    __fastcall__ io_puts(const char *s);
char    io_getc(void);
uint8_t io_char_ready(void);
void    io_gets(char *buf, uint8_t max);

/* Prototipos - editor_screen.c */
void    scr_draw(void);
void    scr_status_line(void);
void    scr_update_cursor(void);
void    scr_scroll_up(void);
void    scr_scroll_down(void);

/* Prototipos - editor_vi.c */
void    vi_init(void);
void    vi_run(void);
void    vi_mode_normal(void);
void    vi_mode_insert(void);
void    vi_mode_command(void);
void    vi_mode_search(void);

/* Prototipos de utilidades */
char*   strstr(const char *haystack, const char *needle);
int     strncmp(const char *a, const char *b, unsigned int n);
uint16_t get_sector_start(const char *name);
void     mfs_init(void);
void     editor_quit(void);

#endif /* EDITOR_H */
