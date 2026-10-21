"""ver_enter3.py - Mide el buffer tras Enter en fichero nuevo.

Uso: python tests/ver_enter3.py
"""
import os
import socket
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402
import urllib.request  # noqa: E402

EMU = "D:/Proyectos/EmuladorFpga6502"


def ram(a, n):
    return urllib.request.urlopen(
        "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (a, n), timeout=3).read()


def main():
    harness.hacer_imagen([("EXIST.TXT", "uno\ndos\n")], binario="base/editor_base.bin")
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
    time.sleep(1.0)
    ruta = os.path.join(harness.ROOT, "build/test_sd.img").replace("/", "\\")
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
        while time.time() < fin:
            try:
                s.recv(65536)
            except OSError:
                continue

    def estado(tag):
        st = ram(0x660, 12)
        mac = ram(0x6A8, 8)
        print("%-12s mode=%d cur=(%d,%d) lines=%d valid=%d buf=%s" % (
            tag, st[6], st[2], st[3], st[0],
            mac[2] | (mac[3] << 8), ram(0x36E0, 8).hex(" ")))

    env(b"load EDITOR.BIN 0800\r", 4.0)
    env(b"r 0800\r", 2.0)
    env(b"NUEVO.TXT\r", 3.0)
    estado("abierto")
    env(b"i", 0.8)
    estado("insert")
    env(b"AAA", 0.8)
    estado("AAA")
    env(b"\r", 1.5)
    estado("Enter")
    env(b"BBB", 1.0)
    estado("BBB")
    s.close()
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)


if __name__ == "__main__":
    main()
