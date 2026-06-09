# ROM API - Guía de uso para guardar archivos en SD (MicroFS)

Basado en el análisis del `monitor_6502_TN-9k_16k` y el intérprete BASIC.

## Flujo correcto para guardar un archivo

```
DELETE → CREATE → WRITE → CLOSE
```

> **Nota**: `mfs_create` ya llama a `mfs_open` internamente.
> Si por alguna versión de ROM no funciona, añadir `OPEN` después de `CREATE`:
> `DELETE → CREATE → OPEN → WRITE → CLOSE`

### Desde C (con macros via_zp)

```c
#include "romapi.h"

// 1. Inicializar SD y montar FS (una vez al inicio)
rom_sd_init();          // $BF00
rom_mfs_mount();        // $BF03

// 2. Guardar archivo
char nombre[] = "TEST.TXT";
uint16_t tamano = 1234;

rom_mfs_delete(nombre);                     // $BF42 - eliminar si existe

if (rom_mfs_create_via_zp(nombre, tamano)   // $BF3C - crear (abre internamente)
    == MFS_OK) {
    uint16_t escrito;
    escrito = rom_mfs_write_via_zp(         // $BF3F - escribir datos
        &mis_datos[0], tamano);
    if (escrito == tamano) {
        // Éxito
    }
    rom_mfs_close();                         // $BF0C - cerrar
}
```

### Desde C (con nombre en struct)

Copiar el nombre a un buffer local ANTES de pasarlo a las macros:

```c
char local[16];
uint8_t i;
for (i = 0; i < 15 && g_state->filename[i]; i++)
    local[i] = g_state->filename[i];
local[i] = '\0';

rom_mfs_delete(local);
rom_mfs_create_via_zp(local, tamano);
rom_mfs_write_via_zp(mis_datos, tamano);
rom_mfs_close();
```

### Desde Ensamblador

```asm
    ; Crear archivo: name en AX, size en stack
    LDA #<nombre
    LDX #>nombre
    JSR push_rom_sp
    LDA #<tamano
    LDX #>tamano
    JSR $BF3C         ; mfs_create (abre internamente)

    ; Escribir: buffer en AX, len en stack
    LDA #<buffer
    LDX #>buffer
    JSR push_rom_sp
    LDA #<tamano
    LDX #>tamano
    JSR $BF3F         ; mfs_write

    JSR $BF0C         ; mfs_close
```

## Errores comunes

| Error | Causa |
|-------|-------|
| `create` devuelve error | Nombre no copiado a buffer local, o file table corrupta |
| `write` devuelve !== tamaño | Buffer o tamaño incorrecto en $F4-$F7 |
| Todo falla después de editar | EDIT_BUF solapa la file table (usar array de C) |

## Reglas importantes

1. **Siempre** hacer `delete` antes de `create` (por si el archivo ya existe)
2. **CREATE ya abre el archivo internamente.** No hace falta `OPEN` extra
3. **Siempre** hacer `close` después de `write`
4. **NUNCA** solapar EDIT_BUF con la file table ($0264-$0463)
5. Usar arrays de C (no direcciones fijas) para buffers grandes
6. Copiar el nombre a un buffer local antes de pasarlo a las macros

## Funciones disponibles

| Dir | Macro | Descripción |
|-----|-------|-------------|
| $BF00 | `rom_sd_init()` | Inicializar SD Card |
| $BF03 | `rom_mfs_mount()` | Montar MicroFS |
| $BF06 | `rom_mfs_open(name)` | Abrir archivo (name en $F4) |
| $BF09 | `rom_mfs_read_via_zp(buf, len)` | Leer datos (args en $F0-$F3) |
| $BF0C | `rom_mfs_close()` | Cerrar archivo |
| $BF0F | `rom_mfs_get_size()` | Obtener tamaño del archivo abierto |
| $BF3C | `rom_mfs_create_via_zp(name, size)` | Crear archivo (args en $F4-$F7) |
| $BF3F | `rom_mfs_write_via_zp(buf, len)` | Escribir datos (args en $F4-$F7) |
| $BF42 | `rom_mfs_delete(name)` | Eliminar archivo (name en $F4) |
| $BF45 | `rom_mfs_format()` | Formatear MicroFS |
