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

    /* El prompt pide un NOMBRE DE FICHERO, no un comando. Antes ponia
       "Type :help for commands", que hacia teclear :help aqui y crear un
       fichero llamado ":HELP". */
    io_puts("Comandos vi: i=escribir  ESC=salir de escribir  :w=guardar  :q=salir\r\n");
    io_puts("            :help (ya dentro)    :wq (guardar y salir)\r\n");

    /* Inicializar editor */
    vi_init();
    g_quit_flag = 0;

    /* Preguntar archivo a editar */
    io_puts("Archivo (nuevo si no existe): ");
    io_gets(fname, 28);

    /* Aqui se escribe un NOMBRE DE FICHERO. Si el usuario teclea :help (o h)
       por costumbre, se le explica en vez de crear un fichero ":HELP". */
    if ((fname[0] == ':' && fname[1] == 'h') ||
        (fname[0] == 'h' && fname[1] == 'e' && fname[2] == 'l' && fname[3] == 'p')) {
        io_puts("Esto es el nombre de fichero, no un comando.\r\n");
        io_puts("Los comandos vi se usan DENTRO del editor (tras abrir un fichero).\r\n");
        io_puts("Escribe, por ejemplo, T.TXT y pulsa Enter.\r\n");
        editor_quit();
        return 0;
    }

    if (fname[0] != '\0') {
        /* Convertir a mayúsculas (MicroFS) */
        str_to_upper(fname);

        /* Intentar cargar. Antes de crear uno nuevo hay que asegurarse de que
           no existe: rom_mfs_open() sobre un fichero inexistente deja la ROM
           con el estado a medias y la PRIMERA tecla que se teclea despues se
           pierde (la UART queda desincronizada). */
        if (!buf_load(fname)) {
            /* Archivo no existe: crear nuevo vacío */
            uint8_t *buf = edit_buf;
            uint16_t i;

            /* Limpiar buffer y poner una línea vacía */
            for (i = 0; i < EDIT_BUF_SIZE; i++) buf[i] = 0;
            buf[0] = '\n';

            /* Copiar nombre ANTES de tocar las macros: se escriben una vez
               y en este orden, igual que en buf_load. */
            for (i = 0; i < MAX_FILENAME - 1 && fname[i]; i++)
                g_filename[i] = fname[i];
            g_filename[i] = '\0';

            g_state->sector_idx = 0;
            g_state->file_sectors = 1;
            g_state->sector_global_off = 0;
            g_state->flags = FLAG_FILE_OPEN;
            g_state->cur_line = 0;
            g_state->cur_col = 0;
            g_state->cur_global_line = 0;
            g_state->cur_global_offset = 0;

            STATE_SECTOR_START = 0;
            STATE_FILE_SIZE = 1;
            STATE_SECTOR_VALID = 1;
            STATE_EDIT_SIZE = 1;

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
