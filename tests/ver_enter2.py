"""ver_enter2.py - Compara el Enter con fichero NUEVO vs EXISTENTE.

Uso: python tests/ver_enter2.py [NOMBRE]
"""
import os
import socket
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

EMU = "D:/Proyectos/EmuladorFpga6502"
IMG = "build/test_sd.img"


def main():
    nombre = sys.argv[1] if len(sys.argv) > 1 else "NUEVO.TXT"
    harness.hacer_imagen([("EXIST.TXT", "uno\ndos\n")], binario="base/editor_base.bin")
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
    time.sleep(1.0)
    ruta = os.path.join(harness.ROOT, IMG).replace("/", "\\")
    subprocess.run(["powershell", "-Command",
                    "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                    "'rom/monitor.bin','4000','%s','--debug=8080'"
                    " -WindowStyle Hidden" % ruta], cwd=EMU)
    for _ in range(100):
        try:
            socket.create_connection(("127.0.0.1", 4000), timeout=1).close()
            break
        except OSError:
            time.sleep(0.25)
    time.sleep(2.0)
    s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
    s.settimeout(0.3)
    time.sleep(1.5)
    s.recv(65536)

    def env(d, espera=1.5):
        s.sendall(d)
        fin = time.time() + espera
        d2 = b""
        while time.time() < fin:
            try:
                d2 += s.recv(65536)
            except OSError:
                continue
        return d2

    env(b"load EDITOR.BIN 0800\r", 4.0)
    env(b"r 0800\r", 2.0)
    env(nombre.encode() + b"\r", 3.0)
    env(b"i", 0.8)
    env(b"AAA", 0.8)
    env(b"\r", 1.2)
    env(b"BBB", 1.0)
    out = env(b"\x1b", 1.0).decode(errors="replace")
    # barra de estado del ultimo frame
    import re
    m = re.findall(r"\((\d+),(\d+)\)", out)
    print("%-14s cursor (fila,col) = %s" % (nombre, m[-1] if m else "?"))
    s.close()
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)


if __name__ == "__main__":
    main()
