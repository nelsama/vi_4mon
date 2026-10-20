/* ==========================================================================
   EDITOR_BUFFER.C - Buffer de edición con RAW sectors
   ==========================================================================
   EDIT_BUF es un array en C de 512 bytes (1 sector SD).
   El linker lo coloca automÃ¡ticamente en EDITS ($3900-$3AFF).
   ========================================================================== */
#include "editor.h"

#pragma bss-name("EDITBSS")
unsigned char edit_buf[EDIT_BUF_SIZE];
char g_filename[12];
#pragma bss-name("BSS")

static uint16_t* line_idx_ptr(void) {
    return (uint16_t*)LINE_IDX_ADDR;
}

/* -----------------------------------------------------------------------
   buf_rebuild_lines
   ----------------------------------------------------------------------- */
void buf_rebuild_lines(void) {
    uint8_t *buf = edit_buf;
    uint16_t *idx = line_idx_ptr();
    uint16_t pos;
    uint8_t line = 0;

    idx[0] = 0;
    line = 1;
    pos = 0;

    while (pos < STATE_SECTOR_VALID && line < LINE_IDX_MAX) {
        if (buf[pos] == '\n') {
            idx[line] = (uint16_t)(pos + 1);
            line++;
        }
        pos++;
    }

    g_state->lines_in_buf = line;
    if (line < LINE_IDX_MAX) idx[line] = 0xFFFF;
}

/* -----------------------------------------------------------------------
   buf_get_line, buf_line_len, buf_next_line_offset
   ----------------------------------------------------------------------- */
char* buf_get_line(uint8_t line_num) {
    uint16_t *idx = line_idx_ptr();
    if (line_num >= g_state->lines_in_buf) return 0;
    if (line_num >= LINE_IDX_MAX) return 0;
    return (char*)(edit_buf + idx[line_num]);
}

uint8_t buf_line_len(uint8_t line_num) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t start, pos;
    if (line_num >= g_state->lines_in_buf) return 0;
    start = idx[line_num];
    pos = start;
    while (pos < STATE_SECTOR_VALID) {
        if (buf[pos] == '\n') break;
        pos++;
    }
    return (uint8_t)(pos - start);
}

static uint16_t buf_next_line_offset(uint8_t line_num) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t pos;
    if (line_num >= g_state->lines_in_buf) return STATE_SECTOR_VALID;
    pos = idx[line_num];
    while (pos < STATE_SECTOR_VALID) {
        if (buf[pos] == '\n') return pos;
        pos++;
    }
    return STATE_SECTOR_VALID;
}

/* -----------------------------------------------------------------------
   buf_insert_char
   ----------------------------------------------------------------------- */
uint8_t buf_insert_char(uint8_t line, uint8_t col, char c) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t line_start, line_end, insert_pos, i;

    if (line >= g_state->lines_in_buf) return 0;
    line_start = idx[line];
    line_end = buf_next_line_offset(line);
    if (line_start + col > line_end) return 0;
    if (STATE_SECTOR_VALID >= EDIT_BUF_SIZE) return 0;

    insert_pos = line_start + col;
    for (i = STATE_SECTOR_VALID; i > insert_pos; i--) buf[i] = buf[i - 1];

    buf[insert_pos] = c;
    STATE_SECTOR_VALID++;
    STATE_EDIT_SIZE++;
    g_state->flags |= FLAG_MODIFIED | FLAG_SECTOR_DIRTY;
    buf_rebuild_lines();
    return 1;
}

/* -----------------------------------------------------------------------
   buf_delete_char
   ----------------------------------------------------------------------- */
uint8_t buf_delete_char(uint8_t line, uint8_t col) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t line_start, line_end, del_pos, i;

    if (line >= g_state->lines_in_buf) return 0;
    line_start = idx[line];
    line_end = buf_next_line_offset(line);
    del_pos = line_start + col;
    if (del_pos >= line_end) return 0;
    if (buf[del_pos] == '\n') return 0;

    for (i = del_pos; i < STATE_SECTOR_VALID - 1; i++) buf[i] = buf[i + 1];
    STATE_SECTOR_VALID--;
    STATE_EDIT_SIZE--;
    g_state->flags |= FLAG_MODIFIED | FLAG_SECTOR_DIRTY;
    buf_rebuild_lines();
    return 1;
}

/* -----------------------------------------------------------------------
   buf_split_line, buf_join_lines, buf_delete_line, buf_insert_line
   ----------------------------------------------------------------------- */
uint8_t buf_split_line(uint8_t line, uint8_t col) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t line_start, line_end, split_pos, i;

    if (line >= g_state->lines_in_buf) return 0;
    line_start = idx[line];
    line_end = buf_next_line_offset(line);
    split_pos = line_start + col;
    if (col > (uint8_t)(line_end - line_start)) return 0;
    if (STATE_SECTOR_VALID >= EDIT_BUF_SIZE) return 0;

    /* Partir al final de la linea (col == longitud): el '\n' ya esta ahi.
       Antes se insertaba otro y se duplicaba el salto de linea. */
    if (split_pos < STATE_SECTOR_VALID && buf[split_pos] == '\n') {
        buf_rebuild_lines();
        return 1;
    }

    for (i = STATE_SECTOR_VALID; i > split_pos; i--) buf[i] = buf[i - 1];
    buf[split_pos] = '\n';
    STATE_SECTOR_VALID++;
    STATE_EDIT_SIZE++;
    g_state->flags |= FLAG_MODIFIED | FLAG_SECTOR_DIRTY;
    buf_rebuild_lines();
    return 1;
}

uint8_t buf_join_lines(uint8_t line) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t line_end, i;

    if (line >= g_state->lines_in_buf - 1) return 0;
    line_end = buf_next_line_offset(line);
    if (buf[line_end] == '\n') {
        for (i = line_end; i < STATE_SECTOR_VALID - 1; i++) buf[i] = buf[i + 1];
        STATE_SECTOR_VALID--;
        STATE_EDIT_SIZE--;
        g_state->flags |= FLAG_MODIFIED | FLAG_SECTOR_DIRTY;
    }
    buf_rebuild_lines();
    return 1;
}

uint8_t buf_delete_line(uint8_t line) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t line_start, line_end, shift, i;

    if (line >= g_state->lines_in_buf) return 0;
    line_start = idx[line];
    line_end = buf_next_line_offset(line);

    {   /* copiar linea al yank */
        uint16_t len = line_end - line_start;
        uint8_t j;
        g_state->yank_offset = 0;
        g_state->yank_len = (len > 63) ? 63 : (uint8_t)len;
        for (j = 0; j < g_state->yank_len; j++)
            ((uint8_t*)YANK_BUF_ADDR)[j] = buf[line_start + j];
    }

    shift = line_end - line_start + 1;
    /* Cuanto se puede mover hacia arriba. Si la linea no termina en '\n'
       (ultima linea sin salto) shift se pasa uno: se recorta. */
    if (line_start + shift > STATE_SECTOR_VALID)
        shift = STATE_SECTOR_VALID - line_start;
    if (shift > STATE_SECTOR_VALID) shift = STATE_SECTOR_VALID;

    for (i = line_start; i + shift < STATE_SECTOR_VALID; i++)
        buf[i] = buf[i + shift];

    STATE_SECTOR_VALID -= shift;
    STATE_EDIT_SIZE -= shift;
    g_state->flags |= FLAG_MODIFIED | FLAG_SECTOR_DIRTY;
    buf_rebuild_lines();
    return 1;
}

uint8_t buf_insert_line(uint8_t line) {
    uint16_t *idx = line_idx_ptr();
    uint8_t *buf = edit_buf;
    uint16_t insert_pos, i;

    if (line >= g_state->lines_in_buf) return 0;
    if (STATE_SECTOR_VALID >= EDIT_BUF_SIZE) return 0;

    insert_pos = buf_next_line_offset(line);
    if (insert_pos < STATE_SECTOR_VALID && buf[insert_pos] == '\n') insert_pos++;

    for (i = STATE_SECTOR_VALID; i > insert_pos; i--) buf[i] = buf[i - 1];
    buf[insert_pos] = '\n';
    STATE_SECTOR_VALID++;
    STATE_EDIT_SIZE++;
    g_state->flags |= FLAG_MODIFIED | FLAG_SECTOR_DIRTY;
    buf_rebuild_lines();
    return 1;
}

/* -----------------------------------------------------------------------
   buf_load - Carga archivo desde SD
   ----------------------------------------------------------------------- */
uint8_t buf_load(const char *filename) {
    uint8_t *buf = edit_buf;
    uint16_t i;
    uint16_t file_size;
    uint16_t last_nl = 0;

    /* Copiar nombre a g_filename */
    for (i = 0; i < MAX_FILENAME - 1 && filename[i]; i++)
        g_filename[i] = filename[i];
    g_filename[i] = '\0';

    /* Limpiar buffer antes de cargar (evita basura) */
    for (i = 0; i < EDIT_BUF_SIZE; i++) edit_buf[i] = 0;

    /* Leer el fichero UNA vez y fiarse del buffer, no del valor devuelto por
       la ROM: rom_mfs_read_via_zp() devuelve $FFFF en este entorno para
       ficheros pequenos, y ese $FFFF acababa en STATE_SECTOR_VALID. Se lee
       a lo sumo EDIT_BUF_SIZE-1 y el tamano real se calcula DESPUES
       recorriendo el buffer (hasta el ultimo '\n' valido). */
    if (rom_mfs_open(filename) != MFS_OK) return 0;
    file_size = rom_mfs_get_size();
    if (file_size > EDIT_BUF_SIZE - 1) file_size = EDIT_BUF_SIZE - 1;
    if (file_size == 0) { rom_mfs_close(); return 0; }

    (void)rom_mfs_read_via_zp(buf, file_size);
    rom_mfs_close();

    /* Tamano real: hasta el ultimo '\n' del buffer (el fichero acaba en
       salto) o hasta el final de lo leido si no lo hay. */
    i = 0;
    while (i < file_size) {
        if (edit_buf[i] == '\n') last_nl = (uint16_t)(i + 1);
        i++;
    }
    if (last_nl == 0) {
        /* sin '\n': se queda como una sola linea, anadiendo el salto */
        edit_buf[file_size] = '\n';
        last_nl = (uint16_t)(file_size + 1);
    }

    /* Se escriben las macros ANTES de reconstruir el indice: con -O el
       compilador puede reordenar y buf_rebuild_lines() leia el valor viejo
       ($FFFF) y contaba lineas absurdas. */
    STATE_FILE_SIZE = last_nl;
    STATE_SECTOR_VALID = last_nl;
    STATE_EDIT_SIZE = last_nl;

    /* El sector de inicio del fichero NO se puede leer desde aqui: la tabla
       de MicroFS vive en $0264, que es la pila de hardware ($0100-$01FF usa
       hasta $0264 con el monitor). Leerla devolvia metadatos de la pila y el
       editor corrompia/repartia los archivos. No se usa para nada mas. */
    STATE_SECTOR_START = 0;

    g_state->file_sectors = 1;
    g_state->sector_idx = 0;
    g_state->sector_global_off = 0;
    buf_rebuild_lines();
    g_state->cur_line = 0;
    g_state->cur_col = 0;
    g_state->cur_global_line = 0;
    g_state->cur_global_offset = 0;
    g_state->scroll_line = 0;
    g_state->mode = MODE_NORMAL;
    g_state->flags = FLAG_FILE_OPEN;
    g_state->count = 0;
    g_state->count_digits = 0;
    g_state->search_active = 0;
    return 1;
}

/* -----------------------------------------------------------------------
   buf_save - Guarda EDIT_BUF a SD
   -----------------------------------------------------------------------
   MicroFS escribiendo: delete + create(tamano) + write + close.
   OJO: mfs_write() solo escribe mientras f_pos < f_size, asi que el total
   debe coincidir EXACTAMENTE con lo que se escribe. Se usa el tamano real
   del contenido (STATE_SECTOR_VALID), no el edit_size, que podia quedar
   desincronizado y dejaba ceros al final del fichero. */
uint8_t buf_save(void) {
    char local[16];
    uint16_t total, remain, offset, written, chunk;
    uint8_t li;

    /* No se puede guardar sin nombre de archivo */
    if (g_filename[0] == '\0') return 0;

    total = STATE_SECTOR_VALID;
    if (total == 0) return 0;
    if (total > EDIT_BUF_SIZE) total = EDIT_BUF_SIZE;

    for (li = 0; li < 15 && g_filename[li]; li++)
        local[li] = g_filename[li];
    local[li] = '\0';
    str_to_upper(local);            /* MicroFS usa nombres en mayusculas */

    rom_mfs_delete(local);
    if (rom_mfs_create_via_zp(local, total) != MFS_OK)
        return 0;

    offset = 0;
    remain = total;
    while (remain > 0) {
        chunk = (remain > 64) ? 64 : remain;
        written = rom_mfs_write_via_zp(edit_buf + offset, chunk);
        if (written != chunk) { rom_mfs_close(); return 0; }
        offset += chunk;
        remain -= chunk;
    }
    rom_mfs_close();

    /* Estado coherente tras guardar */
    STATE_EDIT_SIZE = total;
    STATE_FILE_SIZE = total;
    g_state->flags &= ~(FLAG_MODIFIED | FLAG_SECTOR_DIRTY);
    return 1;
}

/* -----------------------------------------------------------------------
   buf_load_sector, buf_flush_sector (no usados con EDIT_BUF fijo)
   ----------------------------------------------------------------------- */
uint8_t buf_load_sector(uint16_t sector_idx) { (void)sector_idx; return 0; }
uint8_t buf_flush_sector(void) { return 0; }
