# VI-EDITOR para Tang Nano 9K / 6502

Editor de texto tipo VI para correr en una FPGA Tang Nano 9K con CPU 6502 a 3.375 MHz.

Compila con **CC65** y usa la **ROM API** del monitor para acceder a la SD Card mediante **MicroFS**.

## Características

### Modo Normal
| Comando | Acción |
|---------|--------|
| `h`/`j`/`k`/`l` | Navegación cursor |
| `0` / `$` | Inicio / Fin de línea |
| `w` / `b` | Siguiente / Anterior palabra |
| `G` | Ir a línea N |
| `i` / `a` / `I` / `A` | Modo inserción |
| `o` / `O` | Abrir línea abajo/arriba |
| `x` | Borrar carácter |
| `dd` / `D` / `dw` | Borrar línea / hasta fin / palabra |
| `yy` | Copiar línea |
| `p` / `P` | Pegar después / antes |
| `J` | Unir líneas |
| `/` / `?` | Búsqueda |
| `n` / `N` | Repetir búsqueda |
| `:` | Modo comando |

### Modo Comando (`:`)
| Comando | Función |
|---------|---------|
| `:w` | Guardar archivo |
| `:q` | Salir |
| `:q!` | Salir sin guardar |
| `:wq` / `:x` | Guardar y salir |
| `:e nombre` | Editar otro archivo |
| `:set number` / `:set nonumber` | Mostrar números de línea |
| `:help` | Ayuda |

## Mapa de Memoria

| Dirección | Tamaño | Uso |
|-----------|--------|-----|
| `$0002-$001B` | 26b | Zeropage CC65 |
| `$0200-$07FF` | ~1.5KB | Datos del monitor + editor |
| `$0660-$06A0` | 64b | Estado del editor (struct) |
| `$06A0-$06A7` | 8b | Macros (edit_size, sector, etc.) |
| `$06C8-$0747` | 128b | Índice de líneas (64 × uint16) |
| `$0748-$0787` | 64b | Yank buffer (copiar/pegar) |
| `$0800-$37FD` | ~12KB | **Código del editor** |
| `$3800-$3BE7` | **1000b** | **Buffer de edición** |
| `$3BE8-$3BF3` | 12b | Nombre del archivo actual |
| `$3C00-$3DFF` | 512b | Stack CC65 |
| `$8000-$FFFF` | 32KB | ROM del monitor |

## Compilar

### Requisitos
- CC65 (probado con v2.18)
- Windows con cmd.exe (o Linux/MSYS2)

### En Windows (cmd.exe)
```cmd
make
```

### En Linux / MSYS2
```bash
make -j
```

### Salida
```
output/editor.bin  →  Cargar en $0800 desde el monitor
```

## Instalación en la FPGA

1. Compilar: `make`
2. Copiar `output/editor.bin` a la SD Card
3. Desde el monitor: `L EDITOR.BIN` → `R`

## Estructura del proyecto

```
editor/
├── config/editor.cfg    → Linker config
├── include/             → Headers (editor.h, romapi.h)
├── src/                 → Código fuente
│   ├── startup.s        → Código de inicio 6502
│   ├── editor_main.c    → Punto de entrada + utilidades
│   ├── editor_buffer.c  → Buffer de edición
│   ├── editor_io.c      → I/O por UART + terminal ANSI
│   ├── editor_vi.c      → Máquina de modos vi
│   └── editor_util_asm.s → Utilidades en ASM
├── backup/              → Backup versión original
├── texts/               → Archivos ASM de prueba
├── monitor_6502_TN-9k_16k/ → Monitor de sistema
└── basic/               → Intérprete BASIC (hermano)
```

## Limitaciones

- Archivos de **hasta 1000 bytes** por el buffer limitado
- Sin soporte multi-sector (se puede agregar)
- La barra de estado se ve mejor si el terminal tiene ≥22 filas
- UTF-8 no es seguro (el cursor se mueve por bytes)
- Soportados saltos de línea `LF` (Unix) y `CR+LF` (Windows)

## Dependencias

- **Monitor 6502 v2.5.10+** (`monitor_6502_TN-9k_16k/`) — Proporciona la ROM API para SD, UART y MicroFS
- **CC65** — Compilador cruzado 6502

## Licencia

Ver `monitor_6502_TN-9k_16k/LICENSE`
