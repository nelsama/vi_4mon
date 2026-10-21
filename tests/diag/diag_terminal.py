"""diag_terminal.py - Renderiza la salida del editor como lo haria un terminal.

Simula una terminal ANSI de 80x24: interpreta ESC[...H (cursor), ESC[K
(borrar linea) y ESC[2J (limpiar), y muestra el resultado por filas.

Uso: python tests/diag_terminal.py
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402

ROWS, COLS = 24, 80


class Pantalla:
    def __init__(self):
        self.b = [[" "] * COLS for _ in range(ROWS)]
        self.r = self.c = 0

    def escribe(self, datos):
        i = 0
        while i < len(datos):
            ch = datos[i]
            if ch == "\x1b":
                m = re.match(r"\x1b\[([0-9;?]*)([A-Za-z])", datos[i:])
                if m:
                    self.csi(m.group(1), m.group(2))
                    i += m.end()
                    continue
                i += 1
                continue
            if ch == "\r":
                self.c = 0
            elif ch == "\n":
                self.r = min(self.r + 1, ROWS - 1)
            elif ch >= " ":
                if self.c < COLS and self.r < ROWS:
                    self.b[self.r][self.c] = ch
                self.c += 1
            i += 1

    def csi(self, params, cmd):
        if cmd == "H":
            p = [x for x in params.split(";") if x.isdigit()] or ["1", "1"]
            self.r = min(int(p[0]) - 1, ROWS - 1)
            self.c = min(int(p[1]) - 1, COLS - 1) if len(p) > 1 else 0
        elif cmd == "J":
            self.b = [[" "] * COLS for _ in range(ROWS)]
        elif cmd == "K":
            if self.r < ROWS:
                for x in range(self.c, COLS):
                    self.b[self.r][x] = " "

    def dump(self):
        for n, fila in enumerate(self.b, 1):
            print("%02d|%s|" % (n, "".join(fila).rstrip()))


def main():
    harness.hacer_imagen([("T.TXT", "uno\ndos\n")], binario="base/editor_base.bin")
    e = harness.Emu()
    p = Pantalla()
    p.escribe(e.cmd("load EDITOR.BIN 0800", 3.0).decode(errors="replace"))
    p.escribe(e.cmd("r 0800", 1.5).decode(errors="replace"))
    p.escribe(e.teclas("T.TXT\r", 2.5).decode(errors="replace"))
    print("=== al abrir ===")
    p.dump()
    p.escribe(e.teclas("i", 0.8).decode(errors="replace"))
    p.escribe(e.teclas("HOLA\r", 1.5).decode(errors="replace"))
    print("=== tras i HOLA Enter ===")
    p.dump()
    e.close()


if __name__ == "__main__":
    main()
