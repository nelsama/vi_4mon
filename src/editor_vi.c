/* ==========================================================================
   EDITOR_VI.C - Máquina de modos y comandos vi
   ==========================================================================
   Implementa el loop principal del editor con los 3 modos:
   - NORMAL: navegación y comandos (h,j,k,l,i,a,dd,yy,:,/,etc.)
   - INSERT: escritura de texto directo
   - COMMAND: comandos ":w", ":q", etc.
   - SEARCH: búsqueda "/texto"
   ========================================================================== */
#include "editor.h"

/* ==========================================================================
   HELPERS INTERNOS
   ========================================================================== */

/* Línea temporal para construir la línea a mostrar */
#define LINE_TEMP ((char*)TEMP_LINE_ADDR)

/* Obtiene la línea de estado para el modo (en editor_util_asm.s) */
extern const char* mode_name(void);

/* Dibuja toda la pantalla */
static void draw_screen(void) {
    uint8_t line, screen_line;
    uint8_t *idx = (uint8_t*)LINE_IDX_ADDR;
    uint8_t *buf = (uint8_t*)EDIT_BUF;
    char *line_ptr;
    uint8_t col;
    uint8_t draw_lines;
    uint8_t start_line;

    /* Calcular cuántas líneas caben en pantalla (asumimos 20) */
    draw_lines = 20;
    if (draw_lines > g_state->lines_in_buf)
        draw_lines = g_state->lines_in_buf;

    start_line = g_state->scroll_line;

    for (screen_line = 0; screen_line < draw_lines; screen_line++) {
        line = start_line + screen_line;
        if (line >= g_state->lines_in_buf) break;

        /* Posicionar cursor */
        term_goto(screen_line + 1, 1);

        /* Número de línea (opcional) */
        if (g_state->flags & FLAG_SHOW_NUM) {
            uint8_t n = line + 1;
            io_putc(' ');
            if (n >= 100) io_putc('0' + (n / 100));
            if (n >= 10)  io_putc('0' + ((n / 10) % 10));
            io_putc('0' + (n % 10));
            io_putc(' ');
        }

        /* Obtener línea y dibujarla */
        line_ptr = buf_get_line(line);
        if (line_ptr) {
            for (col = 0; col < MAX_LINE; col++) {
                char c = line_ptr[col];
                if (c == '\n' || c == '\0') break;
                if (c < 32) c = '.';  /* caracteres no imprimibles */
                io_putc(c);
            }
        }

        /* Limpiar resto de la línea (borrar residuos) */
        io_puts("\033[K");
    }

    /* Limpiar líneas restantes de la pantalla */
    for (; screen_line < 22; screen_line++) {
        term_goto(screen_line + 1, 1);
        io_puts("\033[K");
    }
}

/* Dibuja la línea de estado (últimas 2 líneas) */
static void draw_status(void) {
    /* Línea de estado: archivo, modo, cursor, modified */
    term_goto(23, 1);
    io_puts("\033[7m");  /* Invertido */
    io_puts("\033[K");  /* Limpiar linea */
    io_puts(" ");

    /* Nombre archivo */
    if (g_filename[0]) {
        io_puts(g_filename);
    } else {
        io_puts("[No file]");
    }
    io_putc(' ');

    /* Modified flag */
    if (g_state->flags & FLAG_MODIFIED) {
        io_puts("[+] ");
    }

    /* Posición cursor */
    io_putc('(');
    {
        uint8_t n = g_state->cur_global_line + 1;
        if (n >= 100) io_putc('0' + (n / 100));
        if (n >= 10)  io_putc('0' + ((n / 10) % 10));
        io_putc('0' + (n % 10));
    }
    io_putc(',');
    {
        uint8_t n = g_state->cur_col + 1;
        if (n >= 100) io_putc('0' + (n / 100));
        if (n >= 10)  io_putc('0' + ((n / 10) % 10));
        io_putc('0' + (n % 10));
    }
    io_putc(')');

    io_puts("  ");
    {
        uint16_t n = STATE_EDIT_SIZE;
        if (n >= 10000) io_putc('0' + (n / 10000));
        if (n >= 1000)  io_putc('0' + ((n / 1000) % 10));
        if (n >= 100)   io_putc('0' + ((n / 100) % 10));
        if (n >= 10)    io_putc('0' + ((n / 10) % 10));
        io_putc('0' + (n % 10));
    }
    io_puts("b  ");
    io_puts(mode_name());

    io_puts("\033[0m");   /* Reset */

    /* Línea de comandos (24) */
    term_goto(24, 1);
    io_puts("\033[K");   /* Limpiar */
}

/* ==========================================================================
   scr_update_cursor - Posiciona cursor en pantalla según cur_line/cur_col
   ========================================================================== */
void scr_update_cursor(void) {
    uint8_t screen_row;
    int16_t rel;

    /* Calcular fila relativa en pantalla */
    rel = (int16_t)g_state->cur_line - (int16_t)g_state->scroll_line;
    if (rel < 0) rel = 0;
    if (rel > 19) rel = 19;

    screen_row = (uint8_t)rel + 1;  /* 1-based */
    term_goto(screen_row, g_state->cur_col + 1);
}

/* ==========================================================================
   vi_init - Inicializa editor
   ========================================================================== */
void vi_init(void) {
    g_state->mode = MODE_NORMAL;
    g_state->flags = 0;
    g_state->cur_line = 0;
    g_state->cur_col = 0;
    g_state->cur_global_line = 0;
    g_state->cur_global_offset = 0;
    g_state->scroll_line = 0;
    g_state->screen_lines = 20;
    g_state->count = 0;
    g_state->count_digits = 0;
    g_state->search_active = 0;
    g_state->lines_in_buf = 1;
    STATE_EDIT_SIZE = 0;
    STATE_FILE_SIZE = 0;
    g_filename[0] = '\0';
}

/* ==========================================================================
   vi_mode_normal - Loop de modo NORMAL
   ========================================================================== */
void vi_mode_normal(void) {
    char c;
    uint8_t done = 0;

    g_state->mode = MODE_NORMAL;

    while (!done) {
        draw_screen();
        draw_status();
        scr_update_cursor();

        c = io_getc();

        /* Acumular conteo numérico */
        if (c >= '0' && c <= '9' && g_state->count_digits < 3) {
            g_state->count = g_state->count * 10 + (c - '0');
            g_state->count_digits++;
            continue;
        }

        /* Procesar comando */
        switch (c) {
            /* Navegación */
            case 'h':
            case 0x08:  /* Backspace */
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    if (g_state->cur_col > 0) g_state->cur_col--;
                    g_state->count--;
                }
                break;

            case 'j':
            case 0x0A:  /* Line feed */
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    if (g_state->cur_line < g_state->lines_in_buf - 1) {
                        g_state->cur_line++;
                        g_state->cur_global_line++;
                        /* Scroll si es necesario */
                        if (g_state->cur_line >= g_state->scroll_line + 20) {
                            g_state->scroll_line++;
                        }
                    }
                    g_state->count--;
                }
                break;

            case 'k':
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    if (g_state->cur_line > 0) {
                        g_state->cur_line--;
                        if (g_state->cur_global_line > 0)
                            g_state->cur_global_line--;
                        /* Scroll up */
                        if (g_state->cur_line < g_state->scroll_line) {
                            if (g_state->scroll_line > 0)
                                g_state->scroll_line--;
                        }
                    }
                    g_state->count--;
                }
                /* Ajustar columna si la línea nueva es más corta */
                {
                    uint8_t len = buf_line_len(g_state->cur_line);
                    if (g_state->cur_col >= len) g_state->cur_col = len;
                }
                break;

            case 'l':
            case ' ':
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    uint8_t len = buf_line_len(g_state->cur_line);
                    if (g_state->cur_col < len) g_state->cur_col++;
                    g_state->count--;
                }
                break;

            case '0':
            case '^':
                g_state->cur_col = 0;
                break;

            case '$':
                g_state->cur_col = buf_line_len(g_state->cur_line);
                break;

            case 'w':
                /* Avanzar al inicio de la siguiente palabra */
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    char *line = buf_get_line(g_state->cur_line);
                    uint8_t len = buf_line_len(g_state->cur_line);
                    uint8_t pos = g_state->cur_col;
                    uint8_t found = 0;

                    /* Buscar fin de palabra actual (espacio) */
                    while (pos < len && line[pos] != ' ' && line[pos] != '\t') pos++;
                    /* Saltar espacios */
                    while (pos < len && (line[pos] == ' ' || line[pos] == '\t')) pos++;
                    /* Buscar inicio de siguiente palabra */
                    while (pos < len) {
                        if (line[pos] != ' ' && line[pos] != '\t') {
                            g_state->cur_col = pos;
                            found = 1;
                            break;
                        }
                        pos++;
                    }
                    if (!found) {
                        /* Ir a siguiente línea */
                        if (g_state->cur_line < g_state->lines_in_buf - 1) {
                            g_state->cur_line++;
                            g_state->cur_global_line++;
                            g_state->cur_col = 0;
                            if (g_state->cur_line >= g_state->scroll_line + 20)
                                g_state->scroll_line++;
                        }
                    }
                    g_state->count--;
                }
                break;

            case 'b':
                /* Retroceder al inicio de palabra anterior */
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    char *line = buf_get_line(g_state->cur_line);
                    int16_t pos = (int16_t)g_state->cur_col - 1;

                    /* Saltar espacios hacia atrás */
                    while (pos >= 0 && (line[pos] == ' ' || line[pos] == '\t')) pos--;
                    /* Buscar inicio de palabra */
                    while (pos >= 0 && line[pos] != ' ' && line[pos] != '\t') pos--;
                    if (pos >= 0) {
                        g_state->cur_col = (uint8_t)(pos + 1);
                    } else {
                        /* Ir a línea anterior */
                        if (g_state->cur_line > 0) {
                            g_state->cur_line--;
                            g_state->cur_global_line--;
                            if (g_state->cur_line < g_state->scroll_line &&
                                g_state->scroll_line > 0)
                                g_state->scroll_line--;
                            g_state->cur_col = buf_line_len(g_state->cur_line);
                        }
                    }
                    g_state->count--;
                }
                break;

            case 'G':
                /* G: ir a línea count, o última línea si sin count */
                if (g_state->count > 0 && g_state->count <= g_state->lines_in_buf) {
                    g_state->cur_line = g_state->count - 1;
                    g_state->cur_global_line = g_state->cur_line;
                } else {
                    g_state->cur_line = g_state->lines_in_buf - 1;
                    g_state->cur_global_line = g_state->cur_line;
                }
                /* Ajustar scroll */
                if (g_state->cur_line >= 10)
                    g_state->scroll_line = g_state->cur_line - 5;
                else
                    g_state->scroll_line = 0;
                g_state->cur_col = 0;
                break;

            /* Inserción */
            case 'i':
                g_state->mode = MODE_INSERT;
                done = 1;
                break;

            case 'a':
                if (g_state->cur_col < buf_line_len(g_state->cur_line))
                    g_state->cur_col++;
                g_state->mode = MODE_INSERT;
                done = 1;
                break;

            case 'I':
                g_state->cur_col = 0;
                g_state->mode = MODE_INSERT;
                done = 1;
                break;

            case 'A':
                g_state->cur_col = buf_line_len(g_state->cur_line);
                g_state->mode = MODE_INSERT;
                done = 1;
                break;

            case 'o':
                buf_insert_line(g_state->cur_line);
                g_state->cur_line++;
                g_state->cur_global_line++;
                g_state->cur_col = 0;
                g_state->mode = MODE_INSERT;
                done = 1;
                break;

            case 'O':
                buf_insert_line(g_state->cur_line - 1);
                g_state->cur_col = 0;
                g_state->mode = MODE_INSERT;
                done = 1;
                break;

            /* Borrado */
            case 'x':
                if (g_state->count == 0) g_state->count = 1;
                while (g_state->count > 0) {
                    buf_delete_char(g_state->cur_line, g_state->cur_col);
                    g_state->count--;
                }
                break;

            case 'd':
                /* dd: borrar línea */
                c = io_getc();
                if (c == 'd') {
                    uint8_t n = (g_state->count == 0) ? 1 : g_state->count;
                    while (n > 0) {
                        buf_delete_line(g_state->cur_line);
                        n--;
                    }
                    if (g_state->lines_in_buf == 0) {
                        /* última línea: mantener una vacía */
                        uint8_t *b = edit_buf;
                        uint16_t j;
                        for (j = 0; j < EDIT_BUF_SIZE; j++) b[j] = 0;
                        b[0] = '\n';
                        STATE_SECTOR_VALID = 1;
                        STATE_EDIT_SIZE = 1;
                        buf_rebuild_lines();
                    }
                    if (g_state->cur_line >= g_state->lines_in_buf &&
                        g_state->lines_in_buf > 0) {
                        g_state->cur_line = g_state->lines_in_buf - 1;
                    }
                    g_state->cur_col = 0;
                } else if (c == 'w') {
                    /* dw: borrar palabra */
                    if (g_state->count == 0) g_state->count = 1;
                    while (g_state->count > 0) {
                        char *line = buf_get_line(g_state->cur_line);
                        uint8_t len = buf_line_len(g_state->cur_line);
                        uint8_t pos = g_state->cur_col;
                        uint8_t end = pos;

                        /* Buscar fin de palabra */
                        while (end < len && line[end] != ' ' && line[end] != '\t') end++;
                        while (end < len && (line[end] == ' ' || line[end] == '\t')) end++;

                        /* Borrar de pos a end */
                        while (pos < end) {
                            buf_delete_char(g_state->cur_line, g_state->cur_col);
                            end--;
                        }
                        g_state->count--;
                    }
                }
                break;

            case 'D':
                /* Borrar hasta fin de línea */
                {
                    uint8_t len = buf_line_len(g_state->cur_line);
                    while (g_state->cur_col < len) {
                        buf_delete_char(g_state->cur_line, g_state->cur_col);
                        len--;
                    }
                }
                break;

            /* Yank/Paste */
            case 'y':
                c = io_getc();
                if (c == 'y') {
                    /* yy: yank (copiar) línea */
                    char *line = buf_get_line(g_state->cur_line);
                    uint8_t *yank = (uint8_t*)YANK_BUF_ADDR;
                    uint8_t len = buf_line_len(g_state->cur_line);
                    uint8_t i;

                    g_state->yank_len = (len > 63) ? 63 : len;
                    g_state->yank_offset = 0;

                    for (i = 0; i < g_state->yank_len; i++) {
                        yank[i] = line[i];
                    }
                }
                break;

            case 'p':
                /* Pegar después */
                if (g_state->yank_len > 0) {
                    uint8_t *yank = (uint8_t*)YANK_BUF_ADDR;
                    uint8_t i;

                    /* Insertar línea nueva */
                    buf_insert_line(g_state->cur_line);

                    /* Insertar caracteres del yank */
                    for (i = 0; i < g_state->yank_len; i++) {
                        buf_insert_char(g_state->cur_line + 1, i, (char)yank[i]);
                    }
                }
                break;

            case 'P':
                /* Pegar antes */
                if (g_state->yank_len > 0) {
                    uint8_t *yank = (uint8_t*)YANK_BUF_ADDR;
                    uint8_t i;

                    buf_insert_line(g_state->cur_line - 1);

                    for (i = 0; i < g_state->yank_len; i++) {
                        buf_insert_char(g_state->cur_line, i, (char)yank[i]);
                    }
                }
                break;

            /* Unir líneas */
            case 'J':
                buf_join_lines(g_state->cur_line);
                break;

            /* Búsqueda */
            case '/':
                g_state->search_dir = 0;  /* forward */
                g_state->mode = MODE_SEARCH;
                done = 1;
                break;

            case '?':
                g_state->search_dir = 1;  /* backward */
                g_state->mode = MODE_SEARCH;
                done = 1;
                break;

            case 'n':
                /* Repetir búsqueda */
                if (g_state->search_active) {
                    uint8_t start = g_state->cur_line;
                    uint8_t l = g_state->search_dir ? start : start + 1;
                    uint8_t found = 0;
                    uint8_t max_lines = g_state->lines_in_buf;

                    while (l < max_lines) {
                        char *line = buf_get_line(l);
                        if (line) {
                            if (strstr(line, g_state->search_pat)) {
                                g_state->cur_line = l;
                                g_state->cur_global_line = l;
                                found = 1;
                                break;
                            }
                        }
                        l = g_state->search_dir ? (l - 1) : (l + 1);
                    }
                    if (!found) {
                        /* Mostrar error rápido */
                        term_goto(24, 1);
                        io_puts("Pattern not found");
                    }
                }
                break;

            case 'N':
                /* Repetir búsqueda inversa */
                if (g_state->search_active) {
                    uint8_t temp = g_state->search_dir;
                    g_state->search_dir = !temp;
                    /* Simular n */
                    /* ... simplificado: re-asignar dir y re-procesar */
                    g_state->search_dir = temp;
                }
                break;

            /* Comandos : */
            case ':':
                g_state->mode = MODE_COMMAND;
                done = 1;
                break;

            /* Salir rápido */
            case 0x06:  /* Ctrl+F: página siguiente (siguiente sector) */
                if (g_state->file_sectors > 1 &&
                    g_state->sector_idx < g_state->file_sectors - 1) {
                    buf_load_sector(g_state->sector_idx + 1);
                    g_state->cur_line = 0;
                    g_state->cur_global_line = g_state->sector_idx * 512 / 20;
                    g_state->scroll_line = 0;
                }
                break;

            case 0x02:  /* Ctrl+B: página anterior (sector previo) */
                if (g_state->sector_idx > 0) {
                    buf_load_sector(g_state->sector_idx - 1);
                    g_state->cur_line = 0;
                    g_state->cur_global_line = g_state->sector_idx * 512 / 20;
                    g_state->scroll_line = 0;
                }
                break;

            case 0x1B:  /* ESC */
                /* Ya estamos en modo normal */
                break;

            default:
                /* Ignorar comandos no reconocidos */
                break;
        }

        /* Reset conteo */
        g_state->count = 0;
        g_state->count_digits = 0;
    }
}

/* ==========================================================================
   vi_mode_insert - Loop de modo INSERT
   ========================================================================== */
void vi_mode_insert(void) {
    char c;
    uint8_t done = 0;

    g_state->mode = MODE_INSERT;

    /* Redibujar pantalla completa al entrar a INSERT */
    draw_screen();
    draw_status();
    scr_update_cursor();

    while (!done) {
        c = io_getc();

        switch (c) {
            case 0x1B:  /* ESC → volver a modo normal */
                done = 1;
                g_state->mode = MODE_NORMAL;
                break;

            case '\r':
            case '\n':
                buf_split_line(g_state->cur_line, g_state->cur_col);
                g_state->cur_line++;
                g_state->cur_global_line++;
                g_state->cur_col = 0;
                if (g_state->cur_line >= g_state->scroll_line + 20) {
                    g_state->scroll_line++;
                }
                draw_screen();
                draw_status();
                scr_update_cursor();
                continue;

            case '\b':
            case 0x7F:
                if (g_state->cur_col > 0) {
                    buf_delete_char(g_state->cur_line, g_state->cur_col - 1);
                    g_state->cur_col--;
                } else if (g_state->cur_line > 0) {
                    uint8_t prev_len = buf_line_len(g_state->cur_line - 1);
                    buf_join_lines(g_state->cur_line - 1);
                    g_state->cur_line--;
                    g_state->cur_global_line--;
                    g_state->cur_col = prev_len;
                    if (g_state->cur_line < g_state->scroll_line &&
                        g_state->scroll_line > 0)
                        g_state->scroll_line--;
                    draw_screen();
                    draw_status();
                    scr_update_cursor();
                    continue;
                }
                draw_screen();
                draw_status();
                scr_update_cursor();
                continue;

            case '\t':
                buf_insert_char(g_state->cur_line, g_state->cur_col, ' ');
                g_state->cur_col++;
                buf_insert_char(g_state->cur_line, g_state->cur_col, ' ');
                g_state->cur_col++;
                buf_insert_char(g_state->cur_line, g_state->cur_col, ' ');
                g_state->cur_col++;
                buf_insert_char(g_state->cur_line, g_state->cur_col, ' ');
                g_state->cur_col++;
                draw_screen();
                draw_status();
                scr_update_cursor();
                continue;

            default:
                if (c >= 32 && c < 127) {
                    if (buf_insert_char(g_state->cur_line, g_state->cur_col, c)) {
                        g_state->cur_col++;
                    }
                }
                draw_screen();
                draw_status();
                scr_update_cursor();
                continue;
        }
    }
}

/* ==========================================================================
   vi_mode_search - Captura patrón de búsqueda
   ========================================================================== */
void vi_mode_search(void) {
    char pat[16];
    uint8_t i;
    char c;

    g_state->mode = MODE_SEARCH;

    /* Mostrar prompt */
    if (g_state->search_dir == 0) {
        pat[0] = '/';
    } else {
        pat[0] = '?';
    }

    /* Mostrar en línea de comandos */
    term_goto(24, 1);
    io_putc(pat[0]);

    i = 1;
    while (1) {
        c = io_getc();

        if (c == '\r' || c == '\n') break;
        if (c == 0x1B) { /* ESC cancela */
            pat[0] = '\0';
            break;
        }
        if (c == '\b' || c == 0x7F) {
            if (i > 1) {
                i--;
                io_puts("\b \b");
            }
            continue;
        }
        if (i < 15) {
            pat[i++] = c;
            io_putc(c);
        }
    }
    pat[i] = '\0';

    /* Si hay patrón, buscar */
    if (pat[0] && i > 1) {
        uint8_t l;

        /* Copiar patrón (sin el / o ?) */
        for (l = 0; l < 15 && pat[l + 1]; l++) {
            g_state->search_pat[l] = pat[l + 1];
        }
        g_state->search_pat[l] = '\0';
        g_state->search_active = 1;

        /* Buscar desde línea actual */
        {
            uint8_t start = g_state->cur_line;
            uint8_t found = 0;
            char *line;

            l = (g_state->search_dir == 0) ? start + 1 : (start > 0 ? start - 1 : 0);

            while (1) {
                if (l >= g_state->lines_in_buf) break;
                if (g_state->search_dir && l > start) break;

                line = buf_get_line(l);
                if (line) {
                    if (strstr(line, g_state->search_pat)) {
                        g_state->cur_line = l;
                        g_state->cur_global_line = l;
                        found = 1;
                        break;
                    }
                }
                l = (g_state->search_dir == 0) ? (l + 1) : (l - 1);
            }

            if (!found) {
                term_goto(24, 1);
                io_puts("Pattern not found");
            }
        }
    }

    g_state->mode = MODE_NORMAL;
}

/* ==========================================================================
   vi_mode_command - Ejecuta comandos ":"
   ========================================================================== */
void vi_mode_command(void) {
    char cmd_buf[32];
    uint8_t i = 0;
    char c;

    g_state->mode = MODE_COMMAND;

    term_goto(24, 1);
    io_putc(':');

    while (1) {
        c = io_getc();

        if (c == '\r' || c == '\n') break;
        if (c == 0x1B) { /* ESC cancela */
            cmd_buf[0] = '\0';
            break;
        }
        if (c == '\b' || c == 0x7F) {
            if (i > 0) {
                i--;
                io_puts("\b \b");
            }
            continue;
        }
        if (i < 30) {
            cmd_buf[i++] = c;
            io_putc(c);
        }
    }
    cmd_buf[i] = '\0';

    /* Ejecutar comando */
    if (cmd_buf[0]) {
        /* :w */
        if (cmd_buf[0] == 'w' && cmd_buf[1] == '\0') {
            if (buf_save()) {
                term_goto(24, 1);
                io_puts("Written");
            } else {
                term_goto(24, 1);
                io_puts("ERR: Write failed");
            }
        }
        /* :q */
        else if (cmd_buf[0] == 'q' && cmd_buf[1] == '\0') {
            if (g_state->flags & FLAG_MODIFIED) {
                term_goto(24, 1);
                io_puts("ERR: No write (:q! to force)");
                /* Esperar una tecla para que vea el error */
                io_getc();
            } else {
                editor_quit();
            }
        }
        /* :q! */
        else if (cmd_buf[0] == 'q' && cmd_buf[1] == '!' && cmd_buf[2] == '\0') {
            editor_quit();
        }
        /* :wq / :x */
        else if ((cmd_buf[0] == 'w' && cmd_buf[1] == 'q' && cmd_buf[2] == '\0') ||
                 (cmd_buf[0] == 'x' && cmd_buf[1] == '\0')) {
            term_goto(24, 1);
            io_puts("Saving ");
            {
                /* Mostrar tamaño a guardar */
                uint16_t n = STATE_EDIT_SIZE;
                if (n >= 10000) io_putc('0' + (n / 10000));
                if (n >= 1000)  io_putc('0' + ((n / 1000) % 10));
                if (n >= 100)   io_putc('0' + ((n / 100) % 10));
                if (n >= 10)    io_putc('0' + ((n / 10) % 10));
                io_putc('0' + (n % 10));
                io_puts(" bytes...");
            }
            if (buf_save()) {
                io_puts("OK");
                {
                    volatile uint16_t d;
                    for (d = 0; d < 10000; d++);
                }
                editor_quit();
            } else {
                io_puts("FAIL");
                io_getc();
            }
        }
        /* :e nombre */
        else if (cmd_buf[0] == 'e' && cmd_buf[1] == ' ') {
            const char *name = cmd_buf + 2;
            if (g_state->flags & FLAG_MODIFIED) {
                term_goto(24, 1);
                io_puts("ERR: No write (:e! to force)");
            } else {
                /* Re-montar MFS para recargar file table en RAM */
                rom_mfs_mount();
                STATE_SECTOR_START = get_sector_start(name);
                if (buf_load(name)) {
                    g_state->mode = MODE_NORMAL;
                } else {
                    term_goto(24, 1);
                    io_puts("ERR: Cannot open file");
                }
            }
        }
        /* :e! nombre */
        else if (cmd_buf[0] == 'e' && cmd_buf[1] == '!' && cmd_buf[2] == ' ') {
            const char *name = cmd_buf + 3;
            /* Re-montar MFS para recargar file table */
            rom_mfs_mount();
            STATE_SECTOR_START = get_sector_start(name);
            if (buf_load(name)) {
                g_state->mode = MODE_NORMAL;
            } else {
                term_goto(24, 1);
                io_puts("ERR: Cannot open file");
            }
        }
        /* :set number */
        else if (strncmp(cmd_buf, "set number", 10) == 0) {
            g_state->flags |= FLAG_SHOW_NUM;
        }
        /* :set nonumber */
        else if (strncmp(cmd_buf, "set nonumber", 12) == 0) {
            g_state->flags &= ~FLAG_SHOW_NUM;
        }
        /* :help */
        else if (cmd_buf[0] == 'h' && cmd_buf[1] == 'e' && cmd_buf[2] == 'l' &&
                 cmd_buf[3] == 'p') {
            term_goto(23, 1);
            io_puts("i=insert o=abajo O=arriba x=borrar");
            term_goto(24, 1);
            io_puts("dd=linea D=resto yy=copiar p=pegar");
            io_puts(" [OK]");
            io_getc();
        }
        /* :version */
        else if (strncmp(cmd_buf, "version", 7) == 0) {
            term_goto(24, 1);
            io_puts("VI-editor " VERSION " for Tang Nano 9K/6502");
        }
        else {
            term_goto(24, 1);
            io_puts("Unknown command");
        }
    }

    g_state->mode = MODE_NORMAL;
}

/* ==========================================================================
   vi_run - Bucle principal del editor
   ========================================================================== */
void vi_run(void) {
    /* Activar flag de ejecución */
    g_state->flags |= FLAG_RUNNING;

    /* Mostrar cursor durante la edición */
    term_show_cursor(1);

    while (g_state->flags & FLAG_RUNNING) {
        switch (g_state->mode) {
            case MODE_NORMAL:
                vi_mode_normal();
                break;
            case MODE_INSERT:
                vi_mode_insert();
                break;
            case MODE_SEARCH:
                vi_mode_search();
                break;
            case MODE_COMMAND:
                vi_mode_command();
                break;
            default:
                g_state->mode = MODE_NORMAL;
                break;
        }
    }
}
