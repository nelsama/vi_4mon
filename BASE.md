# Estado del editor

Comprobado con el emulador: `make test`.

## Casos de uso (ficheros < 1300 B)

| Caso | Estado |
|---|---|
| Abrir fichero existente | OK |
| Crear fichero nuevo | OK |
| Insertar (`A`) | OK |
| Borrar caracter (`x`) | OK |
| Borrar linea (`dd`) | OK |
| Copiar y pegar (`yy p`) | OK |
| Navegacion (`j G`) | OK |
| Salir sin guardar (`:q!`) | OK |
| Varias lineas (Enter) | OK |
| **Fichero > 1300 B** | **FALLA: se trunca al guardar** |

## Arreglos aplicados sobre el commit `f6ba43a`

| Bug | Sintoma | Arreglo |
|---|---|---|
| Macros de estado dentro del struct | `lines_in_buf` nunca se guardaba | `STATE_*` movidas a `$06A8` |
| Macros de estado dentro del indice | se borraba `SECTOR_VALID` | igual |
| `A`/`i` no movian el cursor | el texto iba a otro sitio | `editor_vi.c` |
| Enter no hacia nada en fichero nuevo | todo en una linea | regla de vi en `buf_split_line` |
| Enter metia linea vacia de mas | `cZ\n\nd` | igual |
| `dd` dejaba el resto duplicado | `uno/tres/res` | limite del desplazamiento |
| `p`/`P` pegaban en linea erronea | linea vacia intercalada | posicion y cursor |
| `buf_save` con tamano equivocado | ceros al final | usa `SECTOR_VALID` |
| Lectura de la tabla MicroFS en `$0264` | metadatos basura de la pila | eliminado `get_sector_start` |
| Estado sin inicializar (`$FFFF`) | ediciones fuera de rango | `zerodata` en `startup.s` |
| Bytes perdidos en la UART | barra de estado cortada | `io_putc` espera a TX_READY |
| Nombre del modo no se dibujaba | barra sin NORMAL/INSERT | elegido en C, no via ASM |
| `:help` fuera de pantalla | no se veia | filas 21-22 |
| Banner ambiguo | se tecleaba `:help` como nombre | texto + aviso |
| `tarjeta.img` desactualizada | se probaba un binario viejo | `make sd` lo instala |

## Pendiente

### 1. Ficheros grandes (paginacion)

Factible con la API actual, comprobado:

- `mfs_seek` + `mfs_read` permiten leer cualquier offset (probado).
- `mfs_create` acepta hasta 65535 B.
- Sectores de un fichero son contiguos.

Tres problemas:

1. La tabla de ficheros de MicroFS vive en `$0254-$0453`, encima de donde
   puede llegar la pila. Un desbordamiento corrompe el sistema de ficheros.
2. `mfs_create` no reutiliza el espacio de ficheros borrados: solo crece. Con
   16 entradas y guardados repetidos se fragmenta para siempre.
3. No hay `rename` ni `truncate`: guardar una pagina obliga a reescribir el
   fichero entero.

Presupuesto: `$0800-$3DFF` son 13,8 KB; el codigo ocupa 11,4 KB y el buffer
1,3 KB. Para paginar hacen falta ~4 KB de buffer, asi que hay que recortar
unos 2,4 KB de codigo.

### 2. Otras mejoras

- `u` (deshacer) no esta implementado.
- `N` (busqueda inversa) no esta implementado.
- `dw` solo borra hasta el final de la palabra actual.
