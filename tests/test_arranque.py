"""test_arranque.py - Comprueba que el editor arranca desde $0800.

Carga output/editor.bin como lo haria el monitor y verifica que aparece el
banner y el prompt de fichero.

Uso: python tests/test_arranque.py [--no-build]
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

ROOT = harness.ROOT
BIN = os.path.join(ROOT, "output", "editor.bin")
MAP = os.path.join(ROOT, "output", "editor.map")

FUENTES = ["src/editor_main.c", "src/editor_buffer.c", "src/editor_io.c",
           "src/editor_vi.c", "src/startup.s", "src/editor_util_asm.s"]


def compilar():
    os.makedirs(os.path.dirname(BIN), exist_ok=True)
    r = subprocess.run(
        ["D:/cc65/bin/cl65", "-t", "none", "-O", "--cpu", "6502",
         "-I", "include", "-o", BIN, "-m", MAP,
         "-C", "config/editor.cfg"] + FUENTES +
        ["D:/cc65/lib/none.lib"],
        cwd=ROOT, capture_output=True)
    if r.returncode:
        sys.exit(r.stderr.decode())
    print("compilado:", BIN, os.path.getsize(BIN), "bytes")


def arranque():
    harness.hacer_imagen([("T.TXT", "uno\ndos\ntres\n")],
                         binario="output/editor.bin")
    e = harness.Emu()
    o = e.cmd("load EDITOR.BIN 0800", 3.0)
    print(o.decode(errors="replace").strip().splitlines()[-1])
    o = e.cmd("r 0800", 2.0)
    txt = o.decode(errors="replace")
    print("--- r 0800 ---")
    print(txt.replace("\r", ""))
    e.close()
    ok = "VI-EDITOR" in txt and "Archivo (nuevo si no existe)" in txt
    print("RESULTADO:", "OK" if ok else "FALLO")
    return 0 if ok else 1


def main():
    if "--no-build" not in sys.argv:
        compilar()
    return arranque()


if __name__ == "__main__":
    sys.exit(main())
