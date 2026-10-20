# Pruebas en el emulador FPGA 6502

Pruebas automatizadas de VI-EDITOR sobre
[`EmuladorFpga6502`](../../EmuladorFpga6502). El emulador se controla por
**UART (TCP 4000)** y su estado se inspecciona por HTTP (`/ram.bin`).

## Requisitos

- CC65 en `D:/cc65` (editable en `harness.py`)
- Emulador en `D:/Proyectos/EmuladorFpga6502` (editable en `harness.py`)
- Python 3 (sólo biblioteca estándar)

## Uso

```bash
python tests/harness.py build     # compila output/editor.bin
python tests/harness.py start     # compila + SD + arranca emulador
python tests/harness.py stop      # detiene el emulador
python tests/suite.py             # todas las pruebas
python tests/suite.py cr lf       # sólo algunas
```

`all.py` hace todo seguido (compila, arranca, prueba y para).

## Cómo se arranca el editor

El editor se carga en `$0800` (`load EDITOR.BIN 0800`) y se ejecuta con
`r 0800`.

> **Limitación del monitor v2.6.2 actual**: con esa ROM, `r 0800` provoca un
> reset del monitor en lugar de ejecutar el binario (ni siquiera ejecuta el
> `STARTUP`). Está diagnosticado en `STARTUP`: el código de `$0800` se carga
> correctamente, pero no se ejecuta. El editor está pensado para el arranque
> normal del sistema (auto-boot de la FPGA).

## Direcciones que usa la suite

`emu.py` lee las variables del editor por HTTP. Si cambia el mapa de memoria,
actualiza estas direcciones (y las de `sym.py`, que las saca de
`output/editor.map`):

| Símbolo | Dirección |
|---|---|
| `g_editor` (BSS en LORAM) | `$0400` |
| `edit_buf` (EDITBSS) | `$3800` |
| `STATE_SECTOR_VALID` | `$0666` |
| `LINE_IDX` | `$06C8` |
