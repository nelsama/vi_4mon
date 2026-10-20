"""step.py - Localiza donde muere una rutina sin depurador.

Idea: se rellena la zona de pila de cc65 ($0300-$03FF, ver editor.h) con un
valor conocido y se ejecuta el programa. Si la rutina llega a RETIRAR del stack
(popax/pop1/...), la huella cambia: asi se sabe si el fallo es antes o
despues de la llamada. Tambien se mira la pila de hardware: si su cima
RETROCEDE es que alguien hizo PHA.

Uso: python tests/step.py [binario]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

SOFT_STACK = 0x03FD          # tope del stack de cc65 (crece hacia abajo)


def main():
    binario = sys.argv[1] if len(sys.argv) > 1 else "output/editor.bin"
    harness.hacer_imagen([("T.TXT", "uno\n")], binario=binario)
    e = harness.Emu()
    print(e.cmd("load EDITOR.BIN 0800", 3.0).decode(errors="replace").strip().splitlines()[-1])

    # huella en el stack de cc65
    for i in range(0x20):
        e.cmd("w %04X A5" % (SOFT_STACK - i), 0.12)
    print("huella puesta en $%04X-$%04X" % (SOFT_STACK - 0x1F, SOFT_STACK))

    e.s.sendall(b"r 0800\r")
    e.drain(1.0)
    d = e.ram(0, 0x4000)
    e.close()

    zona = d[SOFT_STACK - 0x20:SOFT_STACK + 1].hex(" ")
    print("stack cc65:", zona)
    pila = d[0x1F0:0x200]
    print("pila hw  :", pila.hex(" "))
    print("BSS st   :", d[0x3566:0x3586].hex(" "))


if __name__ == "__main__":
    main()
