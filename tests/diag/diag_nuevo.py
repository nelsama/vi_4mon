"""diag_nuevo.py - Diagnostico del caso 'fichero nuevo'.

Uso: python tests/diag_nuevo.py [con_i]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402


def main():
    con_i = "con_i" in sys.argv
    harness.hacer_imagen([], binario="base/editor_base.bin")
    e = harness.Emu()
    e.cmd("load EDITOR.BIN 0800", 3.0)
    e.s.sendall(b"r 0800\r")
    e.drain(1.2)
    e.s.sendall(b"NEW.TXT\r")
    e.drain(3.0)
    st = e.ram(0x660, 8)
    print("al abrir: mode=%d lines=%d buf=%s" %
          (st[6], st[0], e.ram(0x36E0, 4).hex(" ")))
    if con_i:
        e.s.sendall(b"i")
        e.drain(0.6)
    e.s.sendall(b"hola mundo\r")
    e.drain(1.5)
    print("buffer  : %s" % e.ram(0x36E0, 14).hex(" "))
    e.s.sendall(b"\x1b")
    e.drain(0.8)
    e.s.sendall(b":wq\r")
    e.drain(3.0)
    e.close()
    print("SD      : %r" % (harness.extraer("NEW.TXT"),))


if __name__ == "__main__":
    main()
