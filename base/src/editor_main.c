/* ==========================================================================
   EDITOR_MAIN.C - Punto de entrada y utilidades de string
   ========================================================================== */
#include "editor.h"

/* ==========================================================================
   strncmp_local - Comparación case-insensitive con límite
   ========================================================================== */
static uint8_t strncmp_local(const char *a, const char *b, uint8_t n) {
    uint8_t i;
    for (i = 0; i < n; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 1;
        if (ca == '\0') return 0;
    }
    return 0;
}

/* ==========================================================================
   strstr_local - Busca substring case-insensitive
   (usado en búsqueda /texto)
   ========================================================================== */
static const char* strstr_local(const char *haystack, const char *needle) {
    const char *h, *n;
    if (*needle == '\0') return haystack;
    while (*haystack) {
        h = haystack;
        n = needle;
        while (*h && *n) {
            char hc = *h;
            char nc = *n;
            if (hc >= 'a' && hc <= 'z') hc -= 32;
            if (nc >= 'a' && nc <= 'z') nc -= 32;
            if (hc != nc) break;
            h++; n++;
        }
        if (*n == '\0') return haystack;
        haystack++;
    }
    return 0;
}

/* Sobrescribir strstr/strncmp para que apunten a nuestras versiones */
/* (CC65 con -t none no tiene libc, así que definimos nuestros símbolos) */
int strncmp(const char *a, const char *b, unsigned int n) {
    return (int)strncmp_local(a, b, (uint8_t)n);
}

char* strstr(const char *haystack, const char *needle) {
    return (char*)strstr_local(haystack, needle);
}

/* ==========================================================================
   str_to_upper - Convierte string a mayúsculas (en editor_util_asm.s)
   ========================================================================== */
extern void __fastcall__ str_to_upper(char *s);

/* ==========================================================================
   main - Punto de entrada del editor
   ========================================================================== */
/* Flag global para indicar salida al monitor */
static volatile uint8_t g_quit_flag = 0;

/* ==========================================================================
   editor_quit - Vuelve al monitor $8000 via JMP (lo más confiable)
   Se llama desde vi_mode_command() cuando el usuario hace :q o :wq
   ========================================================================== */
void editor_quit(void) {
    void (* const monitor)(void) = (void (* const)(void))0x8000;

    term_clear();
    term_show_cursor(1);
    io_puts("EXIT\r\n");  /* "EXIT" para confirmar que llegó */

    /* Esperar a que termine TX antes de saltar */
    {
        volatile uint16_t w;
        for (w = 0; w < 30000; w++);
    }

    /* Llamar al monitor - JSR $8000 */
    monitor();

    /* Si vuelve (no debería), intentar JMP directo */
    /* Esto es un loop infinito para no volver al editor */
    while (1) {
        /* NOP */
    }
}

int main(void) {
    char fname[28];

    /* Inicializar terminal */
    term_init();

    /* Mostrar banner */
    io_puts("\r\nVI-EDITOR v" VERSION);
    io_puts(" for Tang Nano 9K/6502\r\n");

    /* Montar MicroFS (si falla, formatear y reintentar) */
    if (rom_mfs_mount() != MFS_OK) {
        io_puts("Format...\r\n");
        rom_mfs_format();
        rom_mfs_mount();
    }

    io_puts("Type :help for commands\r\n");

    /* Inicializar editor */
    vi_init();
    g_quit_flag = 0;

    /* Preguntar archivo a editar */
    io_puts("File: ");
    io_gets(fname, 28);

    if (fname[0] != '\0') {
        /* Convertir a mayúsculas (MicroFS) */
        str_to_upper(fname);

        /* Cargar archivo (buf_load obtiene sector_start internamente) */
        if (!buf_load(fname)) {
            /* Archivo no existe: crear nuevo vacío */
            uint8_t *buf = edit_buf;
            uint16_t i;

            STATE_FILE_SIZE = 0;
            STATE_EDIT_SIZE = 0;
            STATE_SECTOR_VALID = 0;
            g_state->sector_idx = 0;
            g_state->file_sectors = 1;
            STATE_SECTOR_START = 0;
            g_state->lines_in_buf = 1;
            g_state->flags = FLAG_FILE_OPEN;

            /* Limpiar buffer y poner una línea vacía */
            for (i = 0; i < EDIT_BUF_SIZE; i++) buf[i] = 0;
            buf[0] = '\n';
            STATE_SECTOR_VALID = 1;
            STATE_EDIT_SIZE = 1;

            /* Copiar nombre */
            for (i = 0; i < MAX_FILENAME - 1 && fname[i]; i++)
                g_filename[i] = fname[i];
            g_filename[i] = '\0';

            buf_rebuild_lines();
        }

        /* Entrar al editor */
        vi_run();
    }

    /* Si se solicitó salir con :q, saltar al monitor */
    if (g_quit_flag) {
        editor_quit();
    }

    /* Salir normal (por si algo falla) */
    term_clear();
    term_show_cursor(1);
    io_puts("Bye\r\n");

    return 0;
}
