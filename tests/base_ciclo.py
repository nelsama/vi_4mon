"""base_ciclo.py - Ciclo completo sobre la BASE: abrir, editar, guardar, verificar.

Uso: python tests/base_ciclo.py [NOMBRE]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402


def main():
    nombre = sys.argv[1] if len(sys.argv) > 1 else "T.TXT"
    harness.hacer_imagen([(nombre, "linea uno\nlinea dos\n")],
                         binario="base/editor_base.bin")
    e = harness.Emu()
    print(e.cmd("load EDITOR.BIN 0800", 3.0).decode(errors="replace").strip().splitlines()[-1])
    print(e.cmd("r 0800", 2.0).decode(errors="replace").strip())
    out = e.teclas(nombre + "\r", 3.0).decode(errors="replace")
    print("--- tras abrir ---")
    print(out.replace("\r", ""))
    out = e.teclas("Go", 1.0).decode(errors="replace")
    out += e.teclas("nueva linea\r", 2.0).decode(errors="replace")
    out += e.teclas("\x1b", 1.0).decode(errors="replace")
    print("--- tras o+texto ---")
    print(out.replace("\r", "")[-400:])
    out = e.teclas(":wq\r", 4.0).decode(errors="replace")
    print("--- tras :wq ---")
    print(out.replace("\r", "")[-300:])
    e.close()
    data = harness.extraer(nombre)
    print("--- fichero en la SD ---")
    print(repr(data))
    return 0


if __name__ == "__main__":
    sys.exit(main())
