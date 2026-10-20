/* ==========================================================================
   TEXT.C - Buffer de edicion, paginacion y E/S con la SD
   ==========================================================================
   Modelo:
     - El fichero se divide en PAGINAS de TEXTO_DATA_SIZE bytes. Solo se tiene
       una pagina en RAM, asi un fichero de ~28 KB se edita con 4 KB de RAM.
     - st.page_off = offset de la pagina actual; st.page_line = numero de la
       primera linea (comandos G y ]] cuentan lineas absolutas).
     - st.page_dirty = 1 si la pagina se ha tocado y no esta en la SD.
     - Guardar:
         :w   una pagina  -> se reescribe el fichero (sin temporal).
         :w   varias      -> pagina -> temporal -> original. Si algo falla a
                             mitad, el original sigue intacto.
         :wq / :x         -> se guarda TODO el fichero pagina a pagina.
     - Al cargar se parte la linea a MAX_LINE: ninguna linea puede desbordar
       el buffer ni la pantalla.
   ========================================================================== */
#include "editor.h"

/* Estado del editor. El buffer de texto y el indice de lineas no son
   variables C: viven en direcciones fijas (ver editor.h). */
editor_state_t st;

char file_name[MAX_FILENAME];

static unsigned char *const buf = (unsigned char *)TEXTO_DATA_ADDR;
static uint16_t *const line_idx = (uint16_t *)LINE_IDX_ADDR;

/* ==========================================================================
   Cadenas y nombres (no hay libc con -t none)
   ========================================================================== */

char *txt_name(void)
{
    return file_name;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, unsigned int n)
{
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (!n) return 0;
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static void copy_name(char *dst, const char *src)
{
    uint8_t i;

    for (i = 0; i < MAX_FILENAME - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}

/* --------------------------------------------------------------------------
   Construccion de nombres MicroFS (maximo 11 caracteres + NUL).
   Un fichero largo se guarda en trozos: AAAAAAAA.E01, AAAAAAAA.E02...
   -------------------------------------------------------------------------- */
static char name_buf[2][13];

static void make_part(char *dst, const char *base, uint8_t part)
{
    uint8_t i, n = 0;

    for (i = 0; i < 8 && base[i] && base[i] != '.'; i++) dst[n++] = base[i];
    dst[n++] = '.';
    dst[n++] = 'E';
    if (part >= 10) dst[n++] = (char)('0' + part / 10);
    dst[n++] = (char)('0' + part % 10);
    dst[n] = 0;
}

/* Indice de la pagina en la cadena de trozos. */
static uint8_t page_part(uint16_t off)
{
    return (uint8_t)(off / TEXTO_DATA_SIZE + 1);
}

/* ==========================================================================
   Acceso a lineas
   ========================================================================== */

uint16_t txt_line_off(uint8_t line)
{
    if (line >= st.nlines) return st.buf_len;
    return line_idx[line];
}

uint8_t txt_line_len(uint8_t line)
{
    uint16_t off, end;

    if (line >= st.nlines) return 0;
    off = line_idx[line];
    end = off;
    while (end < st.buf_len && buf[end] != '\n') end++;
    return (uint8_t)(end - off);
}

char *txt_line(uint8_t line)
{
    return (char *)buf + txt_line_off(line);
}

/* Recalcula line_idx[] recorriendo el buffer una sola vez. */
void txt_index_lines(void)
{
    uint16_t pos = 0;
    uint8_t n = 0;

    while (pos < st.buf_len && n < PAGE_LINES - 1) {
        line_idx[n++] = pos;
        while (pos < st.buf_len && buf[pos] != '\n') pos++;
        if (pos < st.buf_len) pos++;      /* saltar el '\n' */
    }
    if (pos < st.buf_len && n < PAGE_LINES)
        line_idx[n++] = pos;
    if (!n) line_idx[n++] = 0;

    st.nlines = n;
}

uint8_t txt_page_full(void)
{
    return (st.buf_len + MAX_LINE + 1 >= TEXTO_DATA_SIZE ||
            st.nlines >= PAGE_LINES - 1);
}

/* ==========================================================================
   E/S con MicroFS
   ========================================================================== */

/* Lee del fichero abierto `len` bytes desde `off`, normaliza CRLF -> LF y
   parte las lineas de mas de MAX_LINE. Devuelve los bytes escritos. */
static uint16_t read_page(uint16_t off, uint16_t len)
{
    uint16_t i, n = 0;
    uint8_t col = 0, c;

    rom_mfs_seek(off);
    len = rom_mfs_read_via_zp(buf, len);
    if (len > TEXTO_DATA_SIZE - 1) len = TEXTO_DATA_SIZE - 1;

    for (i = 0; i < len && n < TEXTO_DATA_SIZE - 1; i++) {
        c = buf[i];
        if (c == '\r') {
            if (i + 1 < len && buf[i + 1] == '\n') continue;
            c = '\n';
        }
        if (c == '\n') {
            buf[n++] = '\n';
            col = 0;
        } else {
            if (col >= MAX_LINE) {
                buf[n++] = '\n';
                col = 0;
            }
            buf[n++] = c;
            col++;
        }
    }
    return n;
}

/* Carga la pagina que empieza en `off` dentro de un fichero de `size` bytes. */
static void load_page_from(uint16_t off, uint16_t size)
{
    uint16_t remain;

    if (off >= size) {
        buf[0] = '\n';
        st.buf_len = 1;
    } else {
        remain = size - off;
        if (remain > TEXTO_DATA_SIZE - 1) remain = TEXTO_DATA_SIZE - 1;
        st.buf_len = read_page(off, remain);

        /* si la pagina acaba a mitad de linea, se descarta: se leera entera
           en la pagina siguiente */
        if (off + st.buf_len < size) {
            uint16_t p = st.buf_len;
            while (p > 0 && buf[p - 1] != '\n') p--;
            st.buf_len = p;
        }
        if (st.buf_len >= TEXTO_DATA_SIZE - 1) st.buf_len = TEXTO_DATA_SIZE - 2;
        buf[st.buf_len++] = '\n';
    }

    st.page_off = off;
    txt_index_lines();
    st.cur_line = 0;
    st.cur_col = 0;
    st.want_col = 0;
    st.top_line = 0;
    st.page_dirty = 0;
}

uint8_t txt_load_page(void)
{
    if (rom_mfs_open(file_name) != MFS_OK) return 0;
    load_page_from(st.page_off, st.file_size);
    rom_mfs_close();
    return 1;
}

/* Cuenta las lineas de los `len` bytes que hay en el buffer (sin '\n' final). */
static uint16_t count_lines(uint16_t len)
{
    uint16_t i, n = 1;

    for (i = 0; i < len; i++) if (buf[i] == '\n') n++;
    return n;
}

/* Retrocede a la pagina anterior. Las paginas no se solapan, asi que basta
   con ir mirando hacia atras hasta dar con un corte que acabe en buf_len. */
void txt_goto_prev_page(void)
{
    uint16_t max = TEXTO_DATA_SIZE - 1;
    uint16_t target = st.page_off;
    uint16_t cand, p;

    if (!target || !st.file_size) return;
    if (target > st.file_size) target = st.file_size;

    cand = (target > max) ? target - max : 0;
    if (rom_mfs_open(file_name) != MFS_OK) return;

    for (;;) {
        if (!cand) break;
        load_page_from(cand, target);     /* `target` como tamano ficticio */
        if (cand + st.buf_len == target) break;
        p = cand + st.buf_len;            /* load_page_from dejo page_off=cand */
        if (!p || p >= target || p <= cand) { cand = 0; break; }
        cand = p;
    }
    rom_mfs_close();

    st.page_off = cand;
    if (!txt_load_page()) io_msg("ERROR al leer pagina");
    st.page_line = 0;
}

uint8_t txt_open(const char *name)
{
    copy_name(file_name, name);
    st.page_off = 0;
    st.page_line = 0;
    st.file_size = 0;
    st.page_dirty = 0;
    st.flags = FLAG_OPEN;

    if (rom_mfs_open(file_name) == MFS_OK) {
        st.file_size = rom_mfs_get_size();
        rom_mfs_close();
        return txt_load_page();
    }

    /* fichero nuevo */
    buf[0] = '\n';
    st.buf_len = 1;
    txt_index_lines();
    st.cur_line = 0;
    st.cur_col = 0;
    st.want_col = 0;
    st.top_line = 0;
    return 1;
}

/* Copia el fichero src completo a dst usando los `len` primeros bytes del
   buffer de texto como bloque: NO toca el contenido de la pagina (por eso se
   puede guardar mientras se edita). */
uint8_t txt_copy_file(const char *dst_name, const char *src_name, uint16_t len)
{
    uint16_t pos = 0, chunk, w;

    len &= 0xFFFE;                       /* queda dentro de una pagina */
    if (!len) return 1;

    rom_mfs_delete(dst_name);
    if (rom_mfs_create_via_zp(dst_name, len) != MFS_OK) return 0;

    while (pos < len) {
        if (rom_mfs_open(src_name) != MFS_OK) return 0;
        rom_mfs_seek(pos);
        chunk = len - pos;
        if (chunk > TEXTO_DATA_SIZE) chunk = TEXTO_DATA_SIZE;
        chunk = rom_mfs_read_via_zp(buf, chunk);
        rom_mfs_close();
        if (!chunk) return 0;

        rom_mfs_seek(pos);
        w = rom_mfs_write_via_zp(buf, chunk);
        if (w != chunk) return 0;
        pos += chunk;
    }
    rom_mfs_close();
    return 1;
}

/* Guarda la pagina actual en la SD.
   Si la pagina se ha estirado (texto anadido) y en el fichero le sigue otra
   pagina, no se puede escribir en sitio: se rehace el fichero con un temporal. */
uint8_t txt_save(void)
{
    uint16_t w;
    uint8_t same_size;

    /* sin cambios en la pagina no hay nada que escribir */
    if (!st.page_dirty) return 1;

    if (st.page_off == 0 && st.file_size <= st.buf_len) {
        /* una sola pagina: reescribir el fichero entero */
        rom_mfs_delete(file_name);
        if (rom_mfs_create_via_zp(file_name, st.buf_len) != MFS_OK)
            return 0;
        w = rom_mfs_write_via_zp(buf, st.buf_len);
        rom_mfs_close();
        if (w != st.buf_len) return 0;
        st.file_size = st.buf_len;
        st.page_dirty = 0;
        st.flags &= ~FLAG_MODIFIED;
        return 1;
    }

    same_size = (st.page_off + st.buf_len <= st.file_size);

    if (same_size) {
        /* escribir la pagina en su sitio */
        rom_mfs_open(file_name);
        rom_mfs_seek(st.page_off);
        w = rom_mfs_write_via_zp(buf, st.buf_len);
        rom_mfs_close();
        if (w != st.buf_len) return 0;
    } else {
        /* la pagina crecio y en el fichero le sigue otra: se rehace el fichero
           (principio + pagina nueva) con un temporal. Lo que habia despues de
           la pagina se pierde: el editor solo guarda hasta la pagina actual. */
        uint16_t newsize = st.page_off + st.buf_len;

        copy_name(name_buf[0], file_name);
        make_part(name_buf[1], file_name, 9);           /* AAAAAAAA.E09 */
        if (!txt_copy_file(name_buf[1], name_buf[0], st.page_off)) return 0;

        if (rom_mfs_open(name_buf[1]) != MFS_OK) return 0;
        rom_mfs_seek(st.page_off);
        w = rom_mfs_write_via_zp(buf, st.buf_len);
        rom_mfs_close();
        if (w != st.buf_len) return 0;

        rom_mfs_delete(name_buf[0]);
        if (rom_mfs_create_via_zp(name_buf[0], newsize) != MFS_OK) return 0;
        if (!txt_copy_file(name_buf[0], name_buf[1], newsize)) return 0;
        rom_mfs_delete(name_buf[1]);
        st.file_size = newsize;
    }

    st.page_dirty = 0;
    st.flags &= ~FLAG_MODIFIED;
    return 1;
}

/* Guarda el fichero completo. Como el editor solo puede modificar una pagina
   en RAM, basta con asegurar que esa pagina esta escrita en la SD; el resto
   del fichero ya esta tal cual. Con una sola pagina equivale a :w. */
uint8_t txt_save_all(void)
{
    return txt_save();
}

/* ==========================================================================
   Edicion
   ========================================================================== */

uint8_t txt_insert_char(uint8_t line, uint8_t col, char c)
{
    uint16_t pos, i;

    if (line >= st.nlines) return 0;
    if (txt_line_len(line) >= MAX_LINE) return 0;
    if (st.buf_len + 1 >= TEXTO_DATA_SIZE) return 0;

    pos = line_idx[line] + col;
    if (pos > st.buf_len) return 0;

    for (i = st.buf_len; i > pos; i--) buf[i] = buf[i - 1];
    buf[pos] = (unsigned char)c;
    st.buf_len++;
    st.flags |= FLAG_MODIFIED;
    st.page_dirty = 1;
    txt_index_lines();
    return 1;
}

uint8_t txt_del_char(uint8_t line, uint8_t col)
{
    uint16_t pos, i, end;

    if (line >= st.nlines) return 0;
    pos = line_idx[line] + col;
    end = line_idx[line] + txt_line_len(line);
    if (pos >= end) return 0;

    for (i = pos; i + 1 < st.buf_len; i++) buf[i] = buf[i + 1];
    st.buf_len--;
    st.flags |= FLAG_MODIFIED;
    st.page_dirty = 1;
    txt_index_lines();
    return 1;
}

uint8_t txt_split_line(uint8_t line, uint8_t col)
{
    uint16_t pos, i;

    if (line >= st.nlines) return 0;
    if (st.nlines >= PAGE_LINES - 1) return 0;
    if (st.buf_len + 1 >= TEXTO_DATA_SIZE) return 0;

    pos = line_idx[line] + col;
    for (i = st.buf_len; i > pos; i--) buf[i] = buf[i - 1];
    buf[pos] = '\n';
    st.buf_len++;
    st.flags |= FLAG_MODIFIED;
    st.page_dirty = 1;
    txt_index_lines();
    return 1;
}

uint8_t txt_join_lines(uint8_t line)
{
    uint16_t pos, i;

    if (line + 1 >= st.nlines) return 0;
    pos = line_idx[line] + txt_line_len(line);
    if (pos >= st.buf_len || buf[pos] != '\n') return 0;

    for (i = pos; i + 1 < st.buf_len; i++) buf[i] = buf[i + 1];
    st.buf_len--;
    st.flags |= FLAG_MODIFIED;
    st.page_dirty = 1;
    txt_index_lines();
    return 1;
}

uint8_t txt_del_line(uint8_t line)
{
    uint16_t start, end, i;

    if (line >= st.nlines) return 0;

    if (st.nlines <= 1) {                    /* nunca dejar 0 lineas */
        buf[0] = '\n';
        st.buf_len = 1;
        txt_index_lines();
        st.flags |= FLAG_MODIFIED;
        st.page_dirty = 1;
        return 1;
    }

    start = line_idx[line];
    end = (line + 1 < st.nlines) ? line_idx[line + 1] : st.buf_len;

    for (i = start; i + (end - start) < st.buf_len; i++)
        buf[i] = buf[i + (end - start)];
    st.buf_len -= (end - start);
    st.flags |= FLAG_MODIFIED;
    st.page_dirty = 1;
    txt_index_lines();
    return 1;
}

/* Inserta una linea en blanco encima de `line`. */
uint8_t txt_ins_line(uint8_t line)
{
    uint16_t pos, i;

    if (st.nlines >= PAGE_LINES - 1) return 0;
    if (st.buf_len + 1 >= TEXTO_DATA_SIZE) return 0;
    if (line >= st.nlines) line = st.nlines - 1;

    pos = line_idx[line];
    for (i = st.buf_len; i > pos; i--) buf[i] = buf[i - 1];
    buf[pos] = '\n';
    st.buf_len++;
    st.flags |= FLAG_MODIFIED;
    st.page_dirty = 1;
    txt_index_lines();
    return 1;
}

/* Lineas de la pagina actual (para numerar y para G). */
uint16_t txt_page_lines(void)
{
    return count_lines(st.buf_len);
}
