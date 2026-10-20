"""bisect.py - Biseccion del fallo de main() con el emulador.

Compila src/main.c con -DPROBE_MAIN -DPROBE_FN=n, lo carga y busca el
banner "[main]" y la marca en $40. Uso: python tests/bisect.py 1 2 3 ...
"""
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HOME = "D:/cc65/bin/"


def link(fn):
    subprocess.run([HOME + "cl65", "-c", "-t", "none", "-O", "--cpu", "6502",
                    "-I", "include", "-DPROBE_MAIN", "-DPROBE_FN=%d" % fn,
                    "-o", "build/bis.o", "src/main.c"], cwd=ROOT, check=True)
    out = os.path.join(ROOT, "build", "bis.bin")
    subprocess.run([HOME + "ld65", "-C", "config/editor.cfg", "-m",
                    "build/bis.map", "-o", out,
                    "build/bis.o", "build/io.o", "build/text.o", "build/vi.o",
                    "build/startup.o", "build/util.o", "build/mul.o",
                    "D:/cc65/lib/none.lib"], cwd=ROOT, check=True,
                   capture_output=True)
    return out


def main():
    for fn in sys.argv[1:]:
        link(int(fn))
        harness.hacer_imagen([], binario="build/bis.bin")
        e = harness.Emu()
        e.s.sendall(b"load EDITOR.BIN 0800\r")
        e.drain(3.0)
        e.s.sendall(b"r 0800\r")
        out = e.drain(3.0)
        marca = e.ram(0x40, 1)[0]
        e.close()
        ok = b"[main]" in out
        print("FN=%s banner=%s marca=$%02X" % (fn, "si" if ok else "NO", marca))


if __name__ == "__main__":
    main()
