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
    for (i = line_start; i + shift < STATE_SECTOR_VALID; i++) buf[i] = buf[i + shift];

    if (shift <= STATE_SECTOR_VALID) {
        STATE_SECTOR_VALID -= shift;
        STATE_EDIT_SIZE -= shift;
    } else {
        STATE_SECTOR_VALID = 0;
        STATE_EDIT_SIZE = 0;
    }
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

    /* Copiar nombre a g_filename */
    for (i = 0; i < MAX_FILENAME - 1 && filename[i]; i++)
        g_filename[i] = filename[i];
    g_filename[i] = '\0';

    /* Usar MicroFS para cargar */
    if (rom_mfs_open(filename) != MFS_OK) return 0;
    STATE_FILE_SIZE = rom_mfs_get_size();
    if (STATE_FILE_SIZE > EDIT_BUF_SIZE) { rom_mfs_close(); return 0; }

    i = rom_mfs_read_via_zp(buf, STATE_FILE_SIZE);
    if (i > STATE_FILE_SIZE) i = STATE_FILE_SIZE;
    if (i >= EDIT_BUF_SIZE) i = EDIT_BUF_SIZE - 1;
    STATE_SECTOR_VALID = i;
    STATE_EDIT_SIZE = i;
    rom_mfs_close();

    /* Obtener sector de inicio (para RAW save si el tamaño no cambia) */
    STATE_SECTOR_START = get_sector_start(filename);

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
   ----------------------------------------------------------------------- */
uint8_t buf_save(void) {
    /* No se puede guardar sin nombre de archivo */
    if (g_filename[0] == '\0') return 0;

    if (STATE_SECTOR_START != 0 &&
        STATE_EDIT_SIZE == STATE_FILE_SIZE) {
        /* RAW: escribir sector directo (mÃ¡s rÃ¡pido) */
        rom_sd_write_sector_via_zp(STATE_SECTOR_START,
            (void*)(uint16_t)edit_buf);
        STATE_FILE_SIZE = STATE_EDIT_SIZE;
        g_state->flags &= ~(FLAG_MODIFIED | FLAG_SECTOR_DIRTY);
        return 1;
    }

    /* MicroFS: delete + create + open + write + close */
    {
        uint8_t *buf = edit_buf;
        uint8_t *tmp = (uint8_t*)0x06C8;
        char local[16];
        uint16_t remain = STATE_EDIT_SIZE;
        uint16_t offset = 0, i, written, chunk;
        uint8_t li;

        for (li = 0; li < 15 && g_filename[li]; li++)
            local[li] = g_filename[li];
        local[li] = '\0';

        rom_mfs_delete(local);
        if (rom_mfs_create_via_zp(local, STATE_EDIT_SIZE) != MFS_OK)
            return 0;
        if (rom_mfs_open(local) != MFS_OK) return 0;

        while (remain > 0) {
            chunk = (remain > 32) ? 32 : remain;
            for (i = 0; i < chunk; i++) tmp[i] = buf[offset + i];
            written = rom_mfs_write_via_zp(tmp, chunk);
            if (written != chunk) { rom_mfs_close(); return 0; }
            offset += chunk;
            remain -= chunk;
        }
        rom_mfs_close();
    }

    STATE_FILE_SIZE = STATE_EDIT_SIZE;
    g_state->flags &= ~(FLAG_MODIFIED | FLAG_SECTOR_DIRTY);
    return 1;
}

/* -----------------------------------------------------------------------
   buf_load_sector, buf_flush_sector (no usados con EDIT_BUF fijo)
   ----------------------------------------------------------------------- */
uint8_t buf_load_sector(uint16_t sector_idx) { (void)sector_idx; return 0; }
uint8_t buf_flush_sector(void) { return 0; }
