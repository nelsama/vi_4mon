"""base_test.py - Verifica la BASE (editor que arranca) antes de cualquier cambio.

La base esta en base/ y se compila aparte, sin tocar src/ (que por ahora tiene
la reescritura rota). Estado esperado: arranca, muestra el banner, y permite
hacer :wq sin perder el fichero.

Uso: python tests/base_test.py
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

ROOT = harness.ROOT
BIN = os.path.join(ROOT, "base", "editor_base.bin")


def compilar():
    salida = os.path.join(ROOT, "base", "editor_base.bin")
    r = subprocess.run(
        ["D:/cc65/bin/cl65", "-t", "none", "-O", "--cpu", "6502", "-I", "include",
         "-o", salida, "-m", os.path.join(ROOT, "base", "editor_base.map"),
         "-C", "config/editor.cfg",
         "src/editor_main.c", "src/editor_buffer.c", "src/editor_io.c",
         "src/editor_vi.c", "src/startup.s", "src/editor_util_asm.s"],
        cwd=os.path.join(ROOT, "base"), capture_output=True)
    if r.returncode:
        sys.exit(r.stderr.decode())
    print("base compilada:", salida, os.path.getsize(salida), "bytes")


def arranque():
    harness.hacer_imagen([("T.TXT", "uno\ndos\ntres\n")], binario="base/editor_base.bin")
    e = harness.Emu()
    o = e.cmd("load EDITOR.BIN 0800", 3.0)
    print(o.decode(errors="replace").strip().splitlines()[-1])
    o = e.cmd("r 0800", 2.0)
    txt = o.decode(errors="replace")
    print("--- r 0800 ---")
    print(txt.replace("\r", ""))
    e.close()
    ok = "VI-EDITOR" in txt and "File:" in txt
    print("RESULTADO:", "OK" if ok else "FALLO")
    return 0 if ok else 1


def main():
    if "--no-build" not in sys.argv:
        compilar()
    return arranque()


if __name__ == "__main__":
    sys.exit(main())
