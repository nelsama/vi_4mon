# Mini VI para Tang Nano 9K / 6502

Editor de texto tipo vi que se carga desde el monitor del sistema en `$0800`
y trabaja sobre ficheros de la SD (MicroFS).

## Estructura

```
base/                 el editor (fuente unica)
  src/                editor_main.c, editor_buffer.c, editor_io.c,
                      editor_vi.c, startup.s, editor_util_asm.s
  include/            editor.h, romapi.h
  config/             editor.cfg (mapa de memoria)
  editor_base.bin     binario compilado (se versiona como referencia)

tests/                pruebas contra el emulador
  base_test.py        arranque: carga el .bin y comprueba el banner
  base_casos.py       bateria de casos de uso (ficheros < 1300 B)
  harness.py          arranca UN emulador y monta una SD a medida
  run_tarjeta.py      arranca el emulador con la tarjeta real
  diag/               diagnosticos puntuales (barra, help, Enter, terminal)

build/                todo lo generado (no se versiona)
```

## Compilar e instalar

```
make            # compila -> base/editor_base.bin
make sd         # compila e instala en el emulador (sdcard/ y tarjeta.img)
make test       # arranque + bateria de casos
make diag       # diagnosticos
```

> Usa siempre `make sd`. Si solo compilas, la tarjeta se queda con el
> `EDITOR.BIN` anterior y se depura un binario que no es el actual.

## Uso en el emulador

Arranca el emulador (`run_debug.bat` en el proyecto del emulador) y conéctate a
la UART (PuTTY en modo Raw, `localhost:4000`):

```
load EDITOR.BIN 0800
r 0800
Archivo (nuevo si no existe): T.TXT
```

## Comandos

| Modo normal | |
|---|---|
| `h` `j` `k` `l` | mover cursor |
| `0` `$` | principio / fin de linea |
| `w` `b` | palabra siguiente / anterior |
| `i` `a` `I` `A` | insertar / anadir / insertar al inicio / al final |
| `o` `O` | abrir linea debajo / encima |
| `x` `X` | borrar caracter bajo el cursor / a la izquierda |
| `dd` `D` | borrar linea / hasta fin de linea |
| `yy` `p` `P` | copiar linea / pegar despues / antes |
| `J` | unir lineas |
| `G` | ir a la ultima linea |
| `/` `?` `n` `N` | buscar |
| `u` | (no implementado) |

| Comandos `:` | |
|---|---|
| `:w` | guardar |
| `:q` | salir (avisa si hay cambios) |
| `:q!` | salir sin guardar |
| `:wq` / `:x` | guardar y salir |
| `:set number` / `:set nonumber` | numeros de linea |
| `:help` | ayuda en las filas 21-22 |
| `:version` | version |

## Mapa de memoria

```
$0002-$001F  ZP del monitor
$0020-$00FF  ZP del programa
$0100-$01FF  pila de hardware
$0254-$0453  tabla de ficheros de MicroFS (ojo: solapa con la pila si crece)
$0660-$06A7  estado del editor (editor_state_t)
$06A8-$06AF  contadores: EDIT_SIZE, SECTOR_VALID, SECTOR_START, FILE_SIZE
$06C0-$06C7  scratch MicroFS
$06C8-$0747  indice de lineas (64 x uint16)
$0748-$0787  buffer de copia (yank)
$0800-$3522  codigo
$36E0-$3BFF  buffer de texto (1300 B)
$3E00-$3FFF  pila de cc65
$8000-$BFFF  ROM del monitor
```

## Estado

Ver [BASE.md](BASE.md). Resumen: 9 de 10 casos de uso pasan. Los ficheros de
mas de 1300 B se truncan al guardar (necesita paginacion).

## Dependencias

- [cc65](https://cc65.github.io/) en `D:/cc65`
- Monitor 6502 v2.6.x (`monitor_6502_TN-9k_16k`)
- Emulador FPGA 6502 (`EmuladorFpga6502`)
- Python 3 (pruebas)
