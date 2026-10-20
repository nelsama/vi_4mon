"""check_mem.py - Valida el reparto de memoria del binario contra editor.h.

Reglas:
  - codigo + datos + BSS acaban antes del buffer de texto,
  - el buffer de texto no pisa $0000-$07FF (monitor) ni $8000+ (ROM),
  - el indice de lineas acaba antes de $8000,
  - la pila hardware/software ($3E00-$3FFF) puede quedar dentro del buffer:
    se reserva como zona prohibida.

Uso: python tests/check_mem.py [output/editor.map]
"""
import re
import sys

MONITOR_END = 0x0800
ROM_START = 0x8000
STACK = (0x3E00, 0x3FFF)


def leer_mapa(ruta):
    txt = open(ruta, encoding="latin-1").read()
    seg = txt.split("Segment list:")[1].split("Exports")[0]
    d = {}
    for m in re.finditer(r"^(\w+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})", seg, re.M):
        d[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return d


def leer_h():
    vals = {}
    for linea in open("include/editor.h", encoding="latin-1"):
        m = re.match(r"#define\s+(TEXTO_BUF_ADDR|TEXTO_BUF_SIZE|PAGE_LINES)\s+(\w+)", linea)
        if m:
            vals[m.group(1)] = int(m.group(2), 0)
    return vals


def main():
    ruta = sys.argv[1] if len(sys.argv) > 1 else "output/editor.map"
    seg = leer_mapa(ruta)
    h = leer_h()

    buf = h["TEXTO_BUF_ADDR"]
    size = h["TEXTO_BUF_SIZE"]
    idx_ini = buf + size
    idx_fin = idx_ini + h["PAGE_LINES"] * 2 - 1

    print("segmentos del binario:")
    for k, (a, b) in sorted(seg.items(), key=lambda kv: kv[1][0]):
        print(f"  {k:9s} {a:04X}-{b:04X}")
    print(f"  TEXT_BUF  {buf:04X}-{buf + size - 1:04X} ({size} B)")
    print(f"  LINE_IDX  {idx_ini:04X}-{idx_fin:04X} ({h['PAGE_LINES'] * 2} B)")
    print(f"  STACK     {STACK[0]:04X}-{STACK[1]:04X} (reservada dentro del buffer)")

    errores = []
    fin_datos = max(b for a, b in seg.values() if a < buf)
    if fin_datos >= buf:
        errores.append(f"datos hasta ${fin_datos:04X} chocan con TEXT_BUF ${buf:04X}")
    if buf < MONITOR_END:
        errores.append(f"TEXT_BUF empieza en ${buf:04X}, dentro de la zona del monitor")
    if idx_fin >= ROM_START:
        errores.append(f"LINE_IDX llega a ${idx_fin:04X} (limite ${ROM_START:04X})")

    if errores:
        for e in errores:
            print("FALLO:", e)
        return 1
    print("OK: reparto valido")
    return 0


if __name__ == "__main__":
    sys.exit(main())
