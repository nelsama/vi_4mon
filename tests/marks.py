"""marks.py - Huellas para saber donde muere el programa.

Compila el proyecto entero con -DEDITOR_MARK=n (en un solo paso, igual que el
makefile), lo carga en el emulador y ensena la pila y la ZP.
Uso: python tests/marks.py [n]
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HOME = "D:/cc65/bin/"


def compilar(marca):
    out = os.path.join(ROOT, "build", "mk.bin")
    r = subprocess.run([HOME + "cl65", "-t", "none", "-O", "--cpu", "6502",
                        "-I", "include", "-DEDITOR_MARK=%d" % marca,
                        "-o", out, "-m", "build/mk.map",
                        "-C", "config/editor.cfg",
                        "src/main.c", "src/io.c", "src/text.c", "src/vi.c",
                        "src/startup.s", "src/util.s", "src/mul.s"],
                       cwd=ROOT, capture_output=True)
    if r.returncode:
        sys.exit(r.stderr.decode())
    return out


def main():
    marca = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    compilar(marca)
    harness.hacer_imagen([], binario="build/mk.bin")
    e = harness.Emu()
    o = e.cmd("load EDITOR.BIN 0800", 3.0)
    print(o.decode(errors="replace").strip().splitlines()[-1])
    e.s.sendall(b"r 0800\r")
    out = e.drain(1.5)
    print("uart:", repr(out.replace(b"\r\n", b"|"))[:300])
    d = e.ram(0, 0x4000)
    e.close()
    print("pila $1F0-$1FF:", d[0x1F0:0x200].hex(" "))
    print("ZP 00-0F:", d[0:16].hex(" "))
    print("ZP 20-3F:", d[0x20:0x40].hex(" "))


if __name__ == "__main__":
    main()
