/* ==========================================================================
   VI.C - Maquina de modos y comandos estilo vi
   ==========================================================================
   Subconjunto de vi pensado para 3.38 MHz y un terminal de 80x24:
     h j k l  0 $ ^ w b   i a I A o O   x X   dd dw D   yy p P   J   u
     G gg   /   n   :   ]] [[ :n :p :w :wq :q! :e :set nu
   Paginacion: el buffer solo tiene una pagina; con ]] y [[ se salta a la
   siguiente/anterior de forma transparente (guardando antes en la SD).
   ========================================================================== */
#include "editor.h"

/* ==========================================================================
   Pantalla
   ========================================================================== */

static void put_u16(uint16_t n)
{
    uint16_t d = 10000;
    uint8_t started = 0;

    while (d) {
        uint8_t v = (uint8_t)((n / d) % 10);
        if (v || started || d == 1) {
            io_putc('0' + v);
            started = 1;
        }
        d /= 10;
    }
}

/* Longitud que ocupa una linea en pantalla (MAX_LINE = 1 + 1 + 255 = 257,
   debe caber en 16 bits). */
static uint8_t disp_cols(uint8_t line)
{
    return st.flags & FLAG_SHOW_NUM ? INDENT_COLS + txt_line_len(line)
                                    : txt_line_len(line);
}

void scr_draw(void)
{
    uint8_t row, line, len, col, indent;
    char *p;

    indent = (st.flags & FLAG_SHOW_NUM) ? INDENT_COLS : 0;

    for (row = 0; row < SCR_ROWS; row++) {
        line = st.top_line + row;
        io_goto(row + 1, 1);
        io_clear_eol();
        if (line >= st.nlines) continue;

        if (indent) {
            put_u16((uint16_t)st.page_line + line + 1);
            io_puts(": ");
        }
        len = txt_line_len(line);
        p = txt_line(line);
        for (col = 0; col < len; col++) {
            char c = p[col];
            io_putc(c < 32 ? '.' : c);
        }
    }

    /* barra de estado */
    io_puts("\033[7m");
    io_goto(ROW_STATUS, 1);
    io_clear_eol();
    io_putc(' ');
    io_puts(txt_name());
    if (st.flags & FLAG_MODIFIED) io_puts(" [+]");
    io_putc(' ');
    put_u16((uint16_t)st.page_line + st.cur_line + 1);
    io_putc(',');
    put_u16(st.cur_col);
    io_puts("  ");
    put_u16(st.buf_len);
    io_putc('B');
    io_putc(' ');
    put_u16(st.page_off);
    io_putc('/');
    put_u16(st.file_size);
    if (st.page_dirty) io_puts(" pg*");
    if (st.mode == MODE_INSERT) io_puts("  -- INSERTAR --");
    io_puts("\033[0m");

    /* fila de comandos siempre limpia */
    io_goto(ROW_CMD, 1);
    io_clear_eol();
}

void scr_cursor(void)
{
    uint8_t row;
    uint8_t indent = (st.flags & FLAG_SHOW_NUM) ? INDENT_COLS : 0;

    if (st.cur_line < st.top_line) {
        st.top_line = st.cur_line;
        scr_draw();
    } else if (st.cur_line >= st.top_line + SCR_ROWS) {
        st.top_line = st.cur_line - SCR_ROWS + 1;
        scr_draw();
    }

    row = st.cur_line - st.top_line;
    io_goto(row + 1, st.cur_col + 1 + indent);
}

/* ==========================================================================
   Movimiento
   ========================================================================== */

static uint8_t is_word(char c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           c == '_';
}

static void clamp_col(void)
{
    uint8_t len = txt_line_len(st.cur_line);
    if (st.cur_col > len) st.cur_col = len;
}

static void set_col(uint8_t col)
{
    st.cur_col = col;
    clamp_col();
    st.want_col = st.cur_col;
}

static void move_l(uint8_t n)
{
    while (n-- && st.cur_col < txt_line_len(st.cur_line)) st.cur_col++;
    st.want_col = st.cur_col;
}

static void move_h(uint8_t n)
{
    if (n > st.cur_col) st.cur_col = 0;
    else st.cur_col -= n;
    st.want_col = st.cur_col;
}

static void move_j(uint8_t n)
{
    while (n-- && st.cur_line + 1 < st.nlines) {
        st.cur_line++;
        if (st.cur_line >= st.top_line + SCR_ROWS) st.top_line++;
        st.cur_col = st.want_col;
        clamp_col();
    }
}

static void move_k(uint8_t n)
{
    while (n-- && st.cur_line > 0) {
        st.cur_line--;
        if (st.cur_line < st.top_line) st.top_line--;
        st.cur_col = st.want_col;
        clamp_col();
    }
}

static void move_word_fwd(uint8_t n)
{
    uint8_t len;
    char *p;

    while (n--) {
        len = txt_line_len(st.cur_line);
        p = txt_line(st.cur_line);
        while (st.cur_col < len && is_word(p[st.cur_col])) st.cur_col++;
        while (st.cur_col < len && !is_word(p[st.cur_col])) st.cur_col++;
        if (st.cur_col >= len) {
            if (st.cur_line + 1 < st.nlines) {
                st.cur_line++;
                if (st.cur_line >= st.top_line + SCR_ROWS) st.top_line++;
                st.cur_col = 0;
            } else {
                st.cur_col = len;
            }
        }
    }
    st.want_col = st.cur_col;
}

static void move_word_back(uint8_t n)
{
    char *p;

    while (n--) {
        if (st.cur_col == 0) {
            if (st.cur_line > 0) {
                st.cur_line--;
                if (st.cur_line < st.top_line) st.top_line = st.cur_line;
                st.cur_col = txt_line_len(st.cur_line);
            }
            continue;
        }
        p = txt_line(st.cur_line);
        st.cur_col--;
        while (st.cur_col && !is_word(p[st.cur_col])) st.cur_col--;
        while (st.cur_col && is_word(p[st.cur_col - 1])) st.cur_col--;
    }
    st.want_col = st.cur_col;
}

static void goto_line(uint8_t line)
{
    if (line >= st.nlines) line = st.nlines ? st.nlines - 1 : 0;
    st.cur_line = line;
    st.cur_col = 0;
    st.want_col = 0;
    st.top_line = (line >= SCR_ROWS) ? line - SCR_ROWS + 1 : 0;
}

/* ==========================================================================
   Yank / put
   ========================================================================== */

static void yank_line(uint8_t line)
{
    uint8_t len = txt_line_len(line);
    uint8_t i;
    char *p = txt_line(line);

    if (len > sizeof(st.yank) - 1) len = sizeof(st.yank) - 1;
    for (i = 0; i < len; i++) st.yank[i] = p[i];
    st.yank_len = len;
}

/* Pega la linea copiada encima de `line` (deja el cursor en ella). */
static void paste_above(uint8_t line)
{
    uint8_t i;

    if (!st.yank_len) { io_msg("buffer de copia vacio"); return; }
    if (!txt_ins_line(line)) { io_msg("sin sitio para pegar"); return; }
    for (i = 0; i < st.yank_len; i++)
        txt_insert_char(line, i, st.yank[i]);
}

/* ==========================================================================
   Busqueda
   ========================================================================== */

static uint8_t up(char c)
{
    return (c >= 'a' && c <= 'z') ? (uint8_t)(c - 32) : (uint8_t)c;
}

static uint8_t find_in_line(char *s, const char *pat)
{
    uint8_t i, j;

    for (i = 0; s[i]; i++) {
        for (j = 0; pat[j]; j++) {
            if (up(s[i + j]) != up(pat[j])) break;
        }
        if (pat[j] == 0) return 1;
    }
    return 0;
}

static void search_run(uint8_t dir)
{
    uint8_t l;

    st.search_dir = dir;
    if (!st.search[0]) return;

    if (dir == 0) {
        for (l = st.cur_line + 1; l < st.nlines; l++)
            if (find_in_line(txt_line(l), st.search)) {
                goto_line(l);
                return;
            }
    } else {
        l = st.cur_line;
        while (l) {
            l--;
            if (find_in_line(txt_line(l), st.search)) {
                goto_line(l);
                return;
            }
        }
    }
    io_msg("no encontrado");
}

/* ==========================================================================
   Paginacion
   ========================================================================== */

/* Vuelca la pagina actual a la SD y carga la siguiente. Las paginas no se
   solapan: avanzan exactamente buf_len bytes. */
static void page_next(void)
{
    uint16_t start = st.page_off + st.buf_len;

    if (start >= st.file_size) {
        io_msg("ultima pagina");
        return;
    }
    if (st.page_dirty && !txt_save()) {
        io_msg("ERROR al guardar pagina");
        return;
    }

    st.page_line += txt_page_lines();
    st.page_off = start;
    if (!txt_load_page()) io_msg("ERROR al leer pagina");
    scr_draw();
}

/* Vuelve a la pagina anterior. */
static void page_prev(void)
{
    if (st.page_off == 0) {
        io_msg("primera pagina");
        return;
    }
    if (st.page_dirty && !txt_save()) {
        io_msg("ERROR al guardar pagina");
        return;
    }
    txt_goto_prev_page();
    scr_draw();
}

/* ==========================================================================
   Comandos ':'
   ========================================================================== */

static void cmd_save(void)
{
    if (txt_save()) {
        io_msg("Guardado");
    } else {
        io_msg("ERROR al guardar");
    }
}

static void do_command(void)
{
    char cmd[24];

    io_prompt(":", cmd, sizeof(cmd));

    if (!strcmp(cmd, "w")) {
        cmd_save();
    } else if (!strcmp(cmd, "q")) {
        if (st.flags & FLAG_MODIFIED) io_msg("Sin guardar: usa :q! o :wq");
        else st.flags &= ~FLAG_RUNNING;
    } else if (!strcmp(cmd, "q!")) {
        st.flags &= ~FLAG_RUNNING;
    } else if (!strcmp(cmd, "wq") || !strcmp(cmd, "x")) {
        if (txt_save_all()) st.flags &= ~FLAG_RUNNING;
        else io_msg("ERROR al guardar");
    } else if (!strcmp(cmd, "w!")) {
        cmd_save();
    } else if (!strcmp(cmd, "n")) {
        page_next();
    } else if (!strcmp(cmd, "p")) {
        page_prev();
    } else if (!strncmp(cmd, "set nu", 6)) {
        st.flags |= FLAG_SHOW_NUM;
    } else if (!strncmp(cmd, "set nonu", 8)) {
        st.flags &= ~FLAG_SHOW_NUM;
    } else if (!strcmp(cmd, "help")) {
        io_msg("hjkl w b 0$ iaI oO xX dd dw yy p J u / ]] [[ :w :wq :q!");
    } else if (cmd[0] == 0) {
        /* nada */
    } else {
        io_msg("Comando desconocido");
    }
}

/* ==========================================================================
   Modo INSERTAR
   ========================================================================== */

static void mode_insert(void)
{
    char c;

    if (st.insert_append) {
        if (st.cur_col < txt_line_len(st.cur_line)) st.cur_col++;
        st.insert_append = 0;
    }

    scr_draw();
    scr_cursor();

    while (1) {
        c = io_getc();

        if (c == 27) {                     /* ESC */
            if (st.cur_col) st.cur_col--;
            st.mode = MODE_NORMAL;
            return;
        }
        if (c == '\r') continue;           /* llega CRLF */

        if (c == 10) {                     /* Enter */
            if (!txt_split_line(st.cur_line, st.cur_col)) {
                io_msg("pagina llena: usa ]]");
                continue;
            }
            if (st.cur_line + 1 < st.nlines) {
                st.cur_line++;
                if (st.cur_line >= st.top_line + SCR_ROWS) st.top_line++;
            }
            st.cur_col = 0;
            st.want_col = 0;
            scr_draw();
            scr_cursor();
            continue;
        }

        if (c == 8 || c == 0x7F) {
            if (st.cur_col) {
                st.cur_col--;
                txt_del_char(st.cur_line, st.cur_col);
            } else if (st.cur_line) {
                uint8_t prev = st.cur_line - 1;
                uint8_t len = txt_line_len(prev);
                txt_join_lines(prev);
                st.cur_line = prev;
                st.cur_col = len;
                if (st.cur_line < st.top_line) st.top_line = st.cur_line;
            }
            scr_draw();
            scr_cursor();
            continue;
        }

        if (c == 9) c = ' ';

        if (c >= 32 && c < 127) {
            if (!txt_insert_char(st.cur_line, st.cur_col, c)) {
                io_msg("linea/buffer lleno");
            } else {
                st.cur_col++;
            }
            scr_draw();
            scr_cursor();
        }
    }
}

/* ==========================================================================
   Modo NORMAL
   ========================================================================== */

static uint8_t take_count(void)
{
    uint8_t n = st.count_digits ? st.count : 1;
    st.count = 0;
    st.count_digits = 0;
    return n;
}

/* Deshacer de una pagina: recarga la pagina desde la SD. */
static void undo_page(void)
{
    if (!st.page_dirty) {
        io_msg("nada que deshacer");
        return;
    }
    st.flags &= ~FLAG_MODIFIED;
    st.page_dirty = 0;
    if (st.file_size > st.buf_len || st.page_off) {
        if (!txt_load_page()) io_msg("ERROR al recargar");
    } else {
        /* pagina de un fichero nuevo: quedaria vacia */
        io_msg("nada que deshacer");
    }
    st.cur_line = 0;
    st.cur_col = 0;
    st.top_line = 0;
}

void vi_run(void)
{
    char c;
    uint8_t n;
    uint8_t pending = 0;        /* 0, 'd', 'y', 'g' */
    uint8_t done;

    st.mode = MODE_NORMAL;
    st.flags |= FLAG_RUNNING;
    st.top_line = 0;
    st.cur_line = 0;
    st.cur_col = 0;
    st.want_col = 0;

    io_show_cursor(1);
    scr_draw();
    scr_cursor();

    while (st.flags & FLAG_RUNNING) {

        c = io_getc();

        if (c >= '1' && c <= '9' && !pending) {
            st.count = st.count * 10 + (c - '0');
            if (st.count > 99) st.count = 99;
            st.count_digits++;
            continue;
        }
        if (c == '0' && st.count_digits && !pending) {
            st.count *= 10;
            if (st.count > 99) st.count = 99;
            st.count_digits++;
            continue;
        }

        if (pending) {
            char op = (char)pending;
            pending = 0;
            n = take_count();

            if (op == 'd') {
                if (c == 'd') {
                    while (n-- && st.nlines) {
                        yank_line(st.cur_line);
                        txt_del_line(st.cur_line);
                        if (st.cur_line >= st.nlines) st.cur_line = st.nlines - 1;
                    }
                    st.cur_col = 0;
                } else if (c == 'G') {
                    yank_line(st.cur_line);
                    while (st.nlines > 1)
                        txt_del_line(st.cur_line + 1);
                    st.cur_col = 0;
                } else if (c == 'w') {
                    uint8_t col = st.cur_col;
                    move_word_fwd(1);
                    while (txt_line_len(st.cur_line) > col)
                        txt_del_char(st.cur_line, col);
                    st.cur_col = col;
                }
            } else if (op == 'y') {
                if (c == 'y') {
                    yank_line(st.cur_line);
                    io_msg("linea copiada");
                }
            } else if (op == 'g') {
                if (c == 'g') goto_line(0);
            }
            clamp_col();
            scr_draw();
            scr_cursor();
            continue;
        }

        n = take_count();
        done = 0;

        switch (c) {
        case 'h': case 8:  move_h(n); done = 1; break;
        case 'j': case 10: move_j(n); done = 1; break;
        case 'k':          move_k(n); done = 1; break;
        case 'l': case ' ': move_l(n); done = 1; break;
        case 'w':          move_word_fwd(n); done = 1; break;
        case 'b':          move_word_back(n); done = 1; break;
        case '0': case '^': set_col(0); done = 1; break;
        case '$':          set_col(txt_line_len(st.cur_line)); done = 1; break;
        case 'G':
            goto_line(n > 1 ? (uint8_t)(n - 1) : (uint8_t)(st.nlines - 1));
            done = 1;
            break;
        case ']':
            page_next();
            done = 1;
            break;
        case '[':
            page_prev();
            done = 1;
            break;
        case 'u':
            undo_page();
            done = 1;
            break;
        case 'd':          pending = 'd'; break;
        case 'y':          pending = 'y'; break;
        case 'g':          pending = 'g'; break;
        case 'p':
            if (st.cur_line + 1 < st.nlines) st.cur_line++;
            paste_above(st.cur_line);
            st.cur_col = 0;
            done = 1;
            break;
        case 'P':
            paste_above(st.cur_line);
            st.cur_col = 0;
            done = 1;
            break;
        case 'x': {
            uint8_t k = n;
            while (k--) txt_del_char(st.cur_line, st.cur_col);
            clamp_col();
            done = 1;
            break;
        }
        case 'X':
            while (n-- && st.cur_col) {
                st.cur_col--;
                txt_del_char(st.cur_line, st.cur_col);
            }
            done = 1;
            break;
        case 'D':
            while (st.cur_col < txt_line_len(st.cur_line))
                txt_del_char(st.cur_line, st.cur_col);
            done = 1;
            break;
        case 'J':
            txt_join_lines(st.cur_line);
            done = 1;
            break;
        case 'i':
            st.mode = MODE_INSERT;
            st.insert_append = 0;
            mode_insert();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 'a':
            st.mode = MODE_INSERT;
            st.insert_append = 1;
            mode_insert();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 'I':
            st.cur_col = 0;
            st.mode = MODE_INSERT;
            st.insert_append = 0;
            mode_insert();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 'A':
            st.cur_col = txt_line_len(st.cur_line);
            st.mode = MODE_INSERT;
            st.insert_append = 0;
            mode_insert();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 'o':
            if (!txt_ins_line(st.cur_line)) {
                io_msg("pagina llena: usa ]]");
                break;
            }
            if (st.cur_line + 1 < st.nlines) st.cur_line++;
            if (st.cur_line >= st.top_line + SCR_ROWS) st.top_line++;
            st.cur_col = 0;
            st.mode = MODE_INSERT;
            st.insert_append = 0;
            mode_insert();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 'O':
            if (!txt_ins_line(st.cur_line)) {
                io_msg("pagina llena: usa ]]");
                break;
            }
            st.cur_col = 0;
            st.mode = MODE_INSERT;
            st.insert_append = 0;
            mode_insert();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 'n':
            search_run(st.search_dir);
            done = 1;
            break;
        case 'N':
            search_run(!st.search_dir);
            done = 1;
            break;
        case '/':
            st.mode = MODE_SEARCH;
            io_prompt("/", st.search, sizeof(st.search));
            st.mode = MODE_NORMAL;
            if (st.search[0]) search_run(0);
            done = 1;
            break;
        case ':':
            st.mode = MODE_COMMAND;
            do_command();
            st.mode = MODE_NORMAL;
            done = 1;
            break;
        case 27:
            break;
        default:
            break;
        }

        if (done) {
            st.want_col = st.cur_col;
            scr_draw();
            scr_cursor();
        }
    }
}
