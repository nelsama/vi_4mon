"""probe_seek.py - Ve la tabla MicroFS y el contenido real del fichero en la SD.

En lugar de ejecutar codigo 6502 nuevo, comprueba desde fuera:
  - el tamano declarado en la tabla,
  - los sectores reservados,
  - el contenido real en la imagen.

Eso basta para saber si la paginacion es viable con mfs_seek.

Uso: python tests/probe_seek.py
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402

SECTOR = 512


def main():
    contenido = "".join("<<%02d>>" % i + "." * 92 + "\n" for i in range(20))
    print("fichero de", len(contenido), "bytes")
    harness.hacer_imagen([("S.TXT", contenido)])
    data = open(os.path.join(harness.ROOT, "build", "test_sd.img"), "rb").read()

    print("\n-- tabla de ficheros --")
    for i in range(16):
        off = 16 + i * 32
        nombre = data[off:off + 12].split(b"\0")[0].decode(errors="replace")
        if not nombre:
            continue
        start, size, sect = struct.unpack_from("<HHH", data, off + 12)
        print("  %-12s sector=%d size=%d sectores=%d" % (nombre, start, size, sect))

    print("\n-- contenido real del sector del fichero --")
    off = 16 + 32               # segunda entrada (S.TXT si EDITOR va despues)
    for i in range(16):
        o = 16 + i * 32
        if data[o:o + 12].split(b"\0")[0].decode(errors="replace") == "S.TXT":
            off = o
            break
    start, size, sect = struct.unpack_from("<HHH", data, off + 12)
    bloque = data[start * SECTOR:start * SECTOR + 300]
    print("  primeros 200 B:", bloque[:200])
    print("\n  offset 100:", data[start * SECTOR + 100:start * SECTOR + 116])
    print("  offset 512:", data[start * SECTOR + 512:start * SECTOR + 528])
    print("\nvarios sectores contiguos:", "SI" if sect >= 2 else "no (fichero pequeno)")

    # ultimo sector
    ult = data[(start + sect - 1) * SECTOR:(start + sect - 1) * SECTOR + 16]
    print("  inicio del ultimo sector:", ult)


if __name__ == "__main__":
    main()
