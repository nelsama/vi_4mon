"""traceboot.py - Traza filtrada del arranque con la sonda del emulador.

El emulador puede volcar por HTTP los ciclos de CPU (`/cycles.json`): se
obtiene la lista de PCs ejecutados desde `r 0800` hasta el cuelgue y se
recorre hacia atras para saber en que bucle/punto se queda.

Uso: python tests/traceboot.py [binario]
"""
import json
import os
import sys
import time
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402


def cycles(e, limite=200000):
    for ruta in ("/cycles.json?n=%d" % limite, "/cycles.json", "/cpu.json"):
        try:
            return json.loads(urllib.request.urlopen(
                "http://127.0.0.1:8080" + ruta, timeout=3).read())
        except Exception:
            continue
    return None


def main():
    binario = sys.argv[1] if len(sys.argv) > 1 else "output/editor.bin"
    harness.hacer_imagen([("T.TXT", "uno\n")], binario=binario)
    e = harness.Emu()
    print(e.cmd("load EDITOR.BIN 0800", 3.0).decode(errors="replace").strip().splitlines()[-1])
    e.s.sendall(b"r 0800\r")
    time.sleep(1.5)
    out = e.drain(0.2)
    print("uart:", repr(out.replace(b"\r\n", b"|"))[:200])
    d = e.ram(0, 0x4000)
    e.close()
    # volcado posterior al r 0800: mirar si main se ejecuto escribiendo en BSS
    print("pila:", d[0x1F0:0x200].hex(" "))
    print("st (BSS):", d[0x3566:0x3586].hex(" "))


if __name__ == "__main__":
    main()
