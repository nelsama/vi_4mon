"""diag_barra.py - Comprueba la barra de estado: modo y ancho.

Uso: python tests/diag_barra.py
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402


def barra(out):
    """Ultima fila 22 emitida, sin secuencias ANSI."""
    m = re.findall(rb"\x1b\[22;1H(.*?)\x1b\[23", out, re.S)
    if not m:
        return "(sin barra)"
    t = m[-1].decode(errors="replace")
    return re.sub(r"\x1b\[[0-9;]*m", "", t)


def main():
    harness.hacer_imagen([("T.TXT", "uno\ndos\n")], binario="output/editor.bin")
    e = harness.Emu()
    e.cmd("load EDITOR.BIN 0800", 3.0)
    e.s.sendall(b"r 0800\r")
    e.drain(1.5)
    out = e.teclas("T.TXT\r", 2.5)
    print("NORMAL:", repr(barra(out)))
    out = e.teclas("i", 1.0)
    print("INSERT:", repr(barra(out)))
    out = e.teclas("\x1b", 1.0)
    print("NORMAL:", repr(barra(out)))
    out = e.teclas(":", 0.6)
    print("CMD   :", repr(barra(out)))
    e.close()


if __name__ == "__main__":
    main()
