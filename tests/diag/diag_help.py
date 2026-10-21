"""diag_help.py - Comprueba que :help se ve dentro de la pantalla del editor.

Uso: python tests/diag_help.py
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402


def main():
    harness.hacer_imagen([("T.TXT", "uno\ndos\n")], binario="editor/editor.bin")
    e = harness.Emu()
    e.cmd("load EDITOR.BIN 0800", 3.0)
    e.s.sendall(b"r 0800\r")
    e.drain(1.2)
    e.s.sendall(b"T.TXT\r")
    e.drain(3.0)
    e.s.sendall(b":help\r")
    out = e.drain(2.0)
    e.close()

    # secuencias ESC[fila;colH emitidas por :help
    filas = re.findall(rb"\x1b\[(\d+);1H", out[out.rfind(b":help"):])
    print("filas usadas por :help:", [int(f) for f in filas])
    ok = True
    for f in filas:
        if int(f) > 23:
            print("FALLO: escribe en la fila %s (max 23)" % f.decode())
            ok = False
    texto = out[out.rfind(b":help"):].decode(errors="replace")
    for linea in re.findall(r"\x1b\[\d+;1H([^\x1b]*)", texto):
        if len(linea) > 80:
            print("FALLO: linea de %d columnas: %r" % (len(linea), linea))
            ok = False
    print("RESULTADO:", "OK" if ok else "FALLO")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
