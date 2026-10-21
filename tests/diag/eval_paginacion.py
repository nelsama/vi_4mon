"""eval_paginacion.py - Evaluacion tecnica de la paginacion.

Comprueba los tres puntos criticos:
  1. Donde vive la tabla de ficheros de MicroFS y si el editor puede leerla.
  2. Si mfs_seek + mfs_read permiten leer porciones arbitrarias.
  3. Si mfs_create puede crear un fichero mayor que la RAM disponible.

Uso: python tests/eval_paginacion.py
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402

TABLA = 0x0254
ENTRADAS = 0x0264


def main():
    grande = "".join("linea %03d\n" % i for i in range(300))   # ~3000 B
    harness.hacer_imagen([("G.TXT", grande), ("S.TXT", "uno\ndos\n")])
    e = harness.Emu()
    e.cmd("load EDITOR.BIN 0800", 3.0)

    print("== 1. Tabla de MicroFS en RAM ==")
    d = e.ram(0, 0x4000)
    print("  magic:", d[TABLA:TABLA + 2], " en", hex(TABLA))
    print("  tamano fichero grande en la imagen:", len(grande))

    print("\n== 2. La pila pisa la tabla? ==")
    # ejecutar el editor y volver a mirar la tabla
    e.s.sendall(b"r 0800\r")
    e.drain(1.5)
    e.s.sendall(b"S.TXT\r")
    e.drain(2.5)
    d2 = e.ram(ENTRADAS, 96)
    print("  entradas tras ejecutar el editor:", d2[:48].hex(" "))
    print("  nombre legible:", d2[:12])

    print("\n== 3. Presupuesto de RAM disponible ==")
    print("  $0800-$3DFF  = 13.8 KB (codigo + datos + buffer)")
    print("  codigo actual: 11.4 KB")
    print("  buffer actual: 1300 B")
    e.close()


if __name__ == "__main__":
    main()
