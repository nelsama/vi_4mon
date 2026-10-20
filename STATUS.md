# Estado de la depuración en el emulador

Fecha: 2026-10-19

## Resumen

Se ha montado un banco de pruebas sobre
`D:\Proyectos\EmuladorFpga6502` (ver `tests/README.md`) y se han corregido
varios fallos de fondo del editor. **Queda bloqueado el arranque**: con el
monitor v2.6.2 que usa el emulador, `r 0800` resetea el monitor en vez de
ejecutar el binario.

## Hallazgo bloqueante: el monitor no ejecuta lo cargado en `$0800`

Síntomas y pruebas:

- `load EDITOR.BIN 0800` → carga correcta; la RAM de `$0800` es idéntica al
  `.bin` (0 bytes distintos).
- `r 0800` → imprime `Ejecutando en $0800...` y **el monitor se reinicia**;
  nunca aparece `Retorno de $0800` ni la salida del editor.
- El `STARTUP` **no llega a ejecutarse**: se parchea su primer byte con
  `JMP $8000` y la CPU no lo alcanza; se parchean puntos intermedios de
  `STARTUP`, `main`, `mfs_init` y `term_init` y ninguno se ejecuta.
- La CPU emulada sí ejecuta RAM alta: un programa pequeño en `$1000`
  (`LDA #$41 / STA $1000 / RTS`) se ejecuta y retorna.
- Un binario grande cargado en `$2000` (`RIVER.BIN`) se queda colgado (sin
  respuesta), no resetea: el fallo depende de la dirección inicial.

Conclusión: el binario está bien; es la combinación **monitor v2.6.2 + `R`
sobre `$0800` + binarios lentos** la que provoca el reinicio. En la FPGA el
editor se arranca por auto-boot, que es el caso para el que está diseñado.

## Fallos corregidos

| # | Fallo | Corrección |
|---|---|---|
| 1 | `sp` de cc65 en `$3DFF` (un byte dentro del stack) → `zerobss` no limpiaba el BSS entre ejecuciones y el estado/buffer se heredaba de la sesión anterior | `sp = __STACKSTART__` (`$3E00`) desde el linker |
| 2 | `:q`/`:wq` y `monitor_reentry` usaban `__asm__` dentro de una función C: cc65 **no es inline**, emite un `jsr` con retorno por referencia y la CPU saltaba a mitad de la tabla de rutinas | `monitor_reentry` en `editor_util_asm.s` (`ldx #$FF / txs / jmp $8000`) |
| 3 | `main()` limpiaba el buffer, luego `vi_init()` lo anulaba (`SECTOR_VALID = 0`) | Se llama `vi_init()` **antes** de inicializar el buffer |
| 4 | `dd`/`dG` sobre la última línea dejaban `SECTOR_VALID = 0` (estado inválido) | `buf_delete_line` nunca deja 0 líneas; arreglado el `shift` de la última línea |
| 5 | `dG` no existía (sólo `dd` y `dw`) | Implementado `dG` y contadores en `dd` |
| 6 | Sin límite de longitud de línea → cursor en columna 102 y desbordamiento del buffer hasta corromper el código | Límite `MAX_LINE` en `buf_insert_char` + aviso `full` en la barra |
| 7 | Desbordamiento del buffer (`SECTOR_VALID` alcanzaba 1300 = tamaño del array) | Límite `EDIT_BUF_MAX` y reserva de byte para `\n` |
| 8 | `MAX_FILENAME 12` dejaba el nombre sin terminador NUL | `MAX_FILENAME 13`; `buf_save` copiaba 15 bytes de un nombre de 13 |
| 9 | El nombre no se copiaba cuando el archivo no existía (`buf_load` salía antes) | Se copia en `main()` |
| 10 | En modo NORMAL, una tecla desconocida redibujaba y "se comía" la siguiente | Se ignora sin redibujar |
| 11 | Código muerto: `buf_load_sector`/`buf_flush_sector`, `scr_*` declaradas y no implementadas, `ptr2` | Eliminado |
| 12 | Dirección de la estructura de estado repartida entre `$0660` (mapa) y `$06C0` (variables) | Una sola estructura `g_editor` en BSS (`$0400`) |
| 13 | `__g_editor` / `_g_editor` mal referenciado desde ASM | `import _g_editor` |

## Reparto de memoria tras los cambios

```
$0002-$001B  Zero Page cc65 (+ $FA-$FF temporales en STARTUP)
$0400-$044E  BSS: g_editor, g_filename
$0800-$3676  STARTUP + RODATA + CODE + DATA
$3800-$3DFF  edit_buf (1536 bytes, EDITS)
$3E00-$3FFF  pila de hardware (monitor + subrutinas)
$8000-$BFFF  ROM del monitor
```

## Pendiente

- Resolver el arranque con el monitor actual (`r 0800`) o documentar el
  arranque por auto-boot; mientras, la suite no puede ejecutarse.
- Aviso `Line full`/`Buffer full`: la barra se dibuja antes del rechazo, así
  que el aviso se ve en la siguiente pulsación.
- Rendimiento: ~180 ms por tecla (repintado completo). Falta redibujado por
  línea.
- `?` (búsqueda inversa) y `N` siguen sin implementar de verdad.
