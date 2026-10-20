"""test_emu.py - Pruebas del editor contra el emulador FPGA 6502.

    python tests/test_emu.py arranque|editar|guardar|paginas|all

Cada caso arranca un unico emulador con su propia imagen SD (tests/harness.py)
y comprueba el resultado leyendo la uart y la RAM.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from harness import Emu, hacer_imagen, extraer, listar  # noqa: E402

STATE_BSS = None   # se rellena desde output/editor.map


def dir_estado():
    """Direccion de `st` (primer simbolo de BSS) segun el mapa del linker."""
    if STATE_BSS is None:
        raise RuntimeError("llama a cargar_mapa() primero")
    return STATE_BSS


def cargar_mapa():
    global STATE_BSS
    for linea in open(os.path.join(ROOT, "output", "editor.map"), encoding="latin-1"):
        partes = linea.replace("_", " _").split()
        # las exportaciones vienen en pares: "_sym addr tipo _sym addr tipo"
        for i in range(0, len(partes) - 2, 3):
            if partes[i] == "_st":
                STATE_BSS = int(partes[i + 1], 16)
                return
    raise RuntimeError("no encuentro _st en output/editor.map")


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def estado(e):
    s = e.ram(dir_estado(), 0x30)
    return {
        "mode": s[0], "flags": s[1], "cur": (s[2], s[3]),
        "top": s[5], "nlines": s[6],
        "buf_len": s[13] | (s[14] << 8),
        "page_off": s[15] | (s[16] << 8),
        "file_size": s[17] | (s[18] << 8),
        "page_line": s[19] | (s[20] << 8),
        "dirty": s[21],
    }


def fallo(msg):
    print("  FALLO:", msg)
    return 1


# ---------------------------------------------------------------------------

def t_arranque():
    print("[arranque]")
    hacer_imagen([("T.TXT", "uno\ndos\ntres\n")])
    e = Emu()
    out = e.cargar_editor()
    out += e.abrir("T.TXT")
    txt = out.decode(errors="replace")
    st = estado(e)
    e.teclas(":q\r", 1.5)
    e.close()
    print("  estado:", st)
    if "Mini VI" not in txt:
        return fallo("sin banner: %r" % txt[:200])
    if "uno" not in txt:
        return fallo("no dibuja el texto: %r" % txt[-400:])
    if st["nlines"] != 3 or st["file_size"] != 13:
        return fallo("nlines=%d file_size=%d" % (st["nlines"], st["file_size"]))
    return 0


def t_editar():
    print("[editar]")
    hacer_imagen([("T.TXT", "alfa\nbeta\ngamma\n")])
    e = Emu()
    e.cargar_editor()
    e.abrir("T.TXT")

    e.teclas("o", 0.5)
    e.teclas("nueva\r", 1.0)
    e.teclas("\x1b", 0.6)
    a = estado(e)
    e.teclas("x", 0.6)
    b = estado(e)
    e.teclas("dd", 1.0)
    c = estado(e)
    e.teclas("p", 1.0)
    d = estado(e)
    e.teclas("u", 2.5)
    f = estado(e)
    e.teclas(":q!\r", 1.0)
    e.close()

    print("  tras o+texto:", a)
    print("  tras x      :", b)
    print("  tras dd     :", c)
    print("  tras p      :", d)
    print("  tras u      :", f)
    if a["nlines"] != 4:
        return fallo("o+texto -> nlines=%d (esperado 4)" % a["nlines"])
    if c["nlines"] != 3:
        return fallo("dd -> nlines=%d (esperado 3)" % c["nlines"])
    if d["nlines"] != 4:
        return fallo("p -> nlines=%d (esperado 4)" % d["nlines"])
    if f["nlines"] != 3:
        return fallo("u -> nlines=%d (esperado 3)" % f["nlines"])
    return 0


def t_guardar():
    print("[guardar]")
    hacer_imagen([("T.TXT", "alfa\n")])
    e = Emu()
    e.cargar_editor()
    e.abrir("T.TXT")
    e.teclas("A", 0.4)
    e.teclas("beta\r", 1.0)
    e.teclas("\x1b", 0.6)
    e.teclas(":w\r", 3.0)
    st = estado(e)
    e.teclas(":q\r", 1.0)
    e.close()
    data = extraer("T.TXT")
    print("  estado:", st)
    print("  fichero:", data)
    if data != b"alfabeta\n":
        return fallo("guardado %r (esperado b'alfabeta\\n')" % (data,))
    return 0


def t_paginas():
    print("[paginas]")
    texto = "".join("L%07d\n" % i for i in range(1500))      # ~12 KB
    hacer_imagen([("G.TXT", texto)])
    e = Emu()
    e.cargar_editor()
    e.abrir("G.TXT", 3.0)
    p0 = estado(e)
    e.teclas("]]", 2.5)
    p1 = estado(e)
    e.teclas("]]", 2.5)
    e.teclas("]]", 2.5)
    p3 = estado(e)
    e.teclas("[[", 2.5)
    p2 = estado(e)
    e.teclas("]]", 2.5)
    e.teclas("A", 0.4)
    e.teclas("X\r", 1.0)
    e.teclas("\x1b", 0.6)
    e.teclas(":wq\r", 5.0)
    e.close()
    for nombre, st in (("p0", p0), ("p1", p1), ("p3", p3), ("p2", p2)):
        print("  %s: off=%d line=%d nlines=%d dirty=%d" %
              (nombre, st["page_off"], st["page_line"], st["nlines"], st["dirty"]))
    data = extraer("G.TXT")
    print("  ficheros en la SD:", listar())
    if data is None:
        return fallo("G.TXT desaparecio")
    print("  tamano guardado:", len(data), "esperado:", len(texto) + 1)
    if p0["page_off"] != 0 or p1["page_off"] <= p0["page_off"]:
        return fallo("la paginacion no avanza")
    if len(data) != len(texto) + 1:
        return fallo("tamano guardado %d" % len(data))
    if data.count(b"\n") != 1501:
        return fallo("lineas=%d (esperado 1501)" % data.count(b"\n"))
    if not data.endswith(b"X\n"):
        return fallo("falta la linea X al final: %r" % data[-40:])
    return 0


CASOS = {
    "arranque": t_arranque,
    "editar": t_editar,
    "guardar": t_guardar,
    "paginas": t_paginas,
}


def main():
    cargar_mapa()
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    nombres = list(CASOS) if sys.argv[1] == "all" else sys.argv[1:]
    fallos = 0
    for n in nombres:
        if n not in CASOS:
            print("caso desconocido:", n)
            return 2
        fallos += CASOS[n]() or 0
    print("OK" if not fallos else "%d fallos" % fallos)
    return 0 if not fallos else 1


if __name__ == "__main__":
    sys.exit(main())
