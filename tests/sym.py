"""sym.py - Lee las direcciones de los simbolos desde output/editor.map."""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAP = os.path.join(ROOT, "output", "editor.map")


def load():
    """Devuelve {simbolo: direccion} de la lista de exports."""
    syms = {}
    with open(MAP, encoding="latin-1") as f:
        txt = f.read()
    sec = txt.split("Exports list by name:")[-1]
    for m in re.finditer(r"_(\w+)\s+([0-9A-F]{6})\s", sec):
        syms[m.group(1)] = int(m.group(2), 16)
    return syms


if __name__ == "__main__":
    for k, v in sorted(load().items(), key=lambda kv: kv[1]):
        print(f"{k:24s} ${v:04X}")
