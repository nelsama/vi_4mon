"""link_tool.py - Enlaza con ld65 y avisa si alguna llamada cae fuera del binario."""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CC65 = "D:/cc65/bin"
LIB = "D:/cc65/lib/none.lib"
OBJS = ["build/startup.o", "build/editor_util_asm.o", "build/editor_main.o",
        "build/editor_buffer.o", "build/editor_io.o", "build/editor_vi.o"]


def enlazar(salida="output/editor.bin", mapa="output/editor.map"):
    r = subprocess.run([f"{CC65}/ld65.exe", "-C", "config/editor.cfg",
                        "-m", mapa, "-o", salida] + OBJS + [LIB],
                       cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout, r.stderr)
        raise SystemExit("fallo de enlace")
    return os.path.join(ROOT, salida), os.path.join(ROOT, mapa)


def comprobar(binpath, mapapath):
    datos = open(binpath, "rb").read()
    ini, fin = 0x800, 0x800 + len(datos)
    mapa = open(mapapath, encoding="latin-1").read()
    seg = mapa.split("Segment list:")[1].split("Exports")[0]
    limites = {}
    for m in re.finditer(r"^(\w+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})", seg, re.M):
        limites[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    problemas = []
    i = 0
    while i < len(datos) - 2:
        op = datos[i]
        if op in (0x20, 0x4C):  # JSR / JMP absoluto
            destino = datos[i + 1] | (datos[i + 2] << 8)
            if not (ini <= destino < fin):
                problemas.append((ini + i, destino))
            i += 3
            continue
        i += 1
    print("segmentos:", {k: "%04X-%04X" % v for k, v in limites.items()})
    print("binario: %04X-%04X (%d bytes)" % (ini, fin - 1, len(datos)))
    if problemas:
        print("LLAMADAS FUERA DEL BINARIO:")
        for origen, destino in problemas[:20]:
            print("  $%04X -> $%04X" % (origen, destino))
    else:
        print("sin llamadas fuera del binario")
    return problemas


if __name__ == "__main__":
    b, m = enlazar()
    comprobar(b, m)
