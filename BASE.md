# Base congelada y experimental

## `base/` — punto de partida que funciona

Copia del editor del commit `f6ba43a` (el que arrancaba) **más los arreglos
verificados**. Es la referencia: si algo se rompe, se vuelve aquí.

```
base/src/          fuentes
base/include/      cabeceras
base/config/       mapa de memoria
base/editor_base.bin   binario de referencia (cargar en $0800)
```

Compilar y probar:

```
python tests/base_test.py     # recompila y comprueba que arranca
python tests/base_casos.py    # batería de casos de uso
```

### Arreglos aplicados sobre el commit original

| Bug | Síntoma | Arreglo |
|---|---|---|
| Macros de estado dentro del struct | `lines_in_buf` nunca se guardaba | `STATE_*` movidas a `$06D0-$06D7` |
| `A`/`i` no movían el cursor | el texto iba a otro sitio | corregido en `editor_vi.c` |
| `Enter` duplicaba el salto | `abc\n\n` | `buf_split_line` no inserta si ya hay `\n` |
| `dd` dejaba el resto duplicado | `uno/tres/res` | límite del desplazamiento corregido |
| `p`/`P` pegaban en línea errónea | línea vacía intercalada | posición y cursor corregidos |
| `buf_save` usaba el tamaño equivocado | ceros al final del fichero | usa `STATE_SECTOR_VALID` |
| Lectura de la tabla MicroFS en `$0264` | metadatos basura de la pila | eliminado `get_sector_start` |
| Estado sin inicializar (`$FFFF`) | ediciones fuera de rango | `zerodata` en `startup.s` |

### Fallos conocidos (pendientes)

- `STATE_SECTOR_VALID` sigue quedando en `$FFFF` al abrir: `mfs_read_via_zp`
  devuelve `$FFFF` en este entorno y el contador se toma tal cual.
- Crear fichero nuevo pierde el primer carácter.
- Ficheros mayores que el buffer (1300 B) se destruyen al guardar.

## `experimental/` — reescritura descartada

Reescritura completa con buffer de 4 KB y paginación. **Compila pero no
arranca**; se conserva solo por referencia. No forma parte del build.
