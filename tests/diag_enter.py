"""diag_enter.py - Comprueba que Enter (CR y LF) hace salto en modo INSERT.

Uso: python tests/diag_enter.py
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402


def estado(e, tag):
    st = e.ram(0x660, 8)
    print("%-16s cur=(%d,%d) lines=%d buf=%s" %
          (tag, st[2], st[3], st[0], e.ram(0x36E0, 10).hex(" ")))


def main():
    harness.hacer_imagen([("T.TXT", "uno\n")], binario="base/editor_base.bin")
    e = harness.Emu()
    e.cmd("load EDITOR.BIN 0800", 3.0)
    e.s.sendall(b"r 0800\r")
    e.drain(1.5)
    e.s.sendall(b"T.TXT\r")
    e.drain(2.5)
    estado(e, "abierto")
    e.s.sendall(b"i")
    e.drain(0.6)
    for tecla, nombre in ((b"\r", "x+CR"), (b"\n", "x+LF")):
        e.s.sendall(b"x" + tecla)
        e.drain(1.0)
        estado(e, nombre)
    e.close()


if __name__ == "__main__":
    main()
