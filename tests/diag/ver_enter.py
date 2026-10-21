"""ver_enter.py - Reproduce el fallo del Enter y captura la terminal virtual.

Arranca el emulador con tarjeta.img, carga EDITOR.BIN, abre un fichero,
entra en INSERT, escribe y pulsa Enter. Guarda capturas reales del terminal
virtual del emulador (/console.png) en build/.

Uso: python tests/ver_enter.py
"""
import os
import socket
import subprocess
import sys
import time
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import harness  # noqa: E402

EMU = "D:/Proyectos/EmuladorFpga6502"
IMG = EMU + "/tarjeta.img"
OUT = os.path.join(harness.ROOT, "build")


def pedir(ruta, nombre):
    try:
        d = urllib.request.urlopen("http://127.0.0.1:8080" + ruta, timeout=3).read()
        open(os.path.join(OUT, nombre), "wb").write(d)
        return True
    except Exception as ex:
        print("  no se pudo leer %s: %s" % (ruta, ex))
        return False


def texto():
    try:
        return urllib.request.urlopen(
            "http://127.0.0.1:8080/console.txt", timeout=3).read().decode(
                errors="replace")
    except Exception:
        return ""


def main():
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
    time.sleep(1.0)
    subprocess.run(["powershell", "-Command",
                    "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                    "'rom/monitor.bin','4000','%s','--debug=8080'"
                    " -WindowStyle Hidden" % IMG.replace("/", "\\")], cwd=EMU)
    for _ in range(100):
        try:
            socket.create_connection(("127.0.0.1", 4000), timeout=1).close()
            urllib.request.urlopen("http://127.0.0.1:8080/", timeout=1).read()
            break
        except OSError:
            time.sleep(0.25)
    time.sleep(2.0)

    s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
    s.settimeout(0.3)
    time.sleep(1.5)
    s.recv(65536)

    def env(datos, espera=2.0):
        s.sendall(datos)
        fin = time.time() + espera
        while time.time() < fin:
            try:
                s.recv(65536)
            except OSError:
                continue

    env(b"load EDITOR.BIN 0800\r", 4.0)
    env(b"r 0800\r", 2.0)
    env(b"T.TXT\r", 3.0)
    pedir("/console.png", "term1_abierto.png")
    print("--- tras abrir ---")
    print(texto()[-1200:])

    env(b"i", 0.8)
    env(b"HOLA", 1.0)
    pedir("/console.png", "term2_escrito.png")
    print("--- tras escribir HOLA ---")
    print(texto()[-800:])

    env(b"\r", 1.2)
    pedir("/console.png", "term3_enter.png")
    print("--- tras Enter ---")
    print(texto()[-800:])

    env(b"MUNDO", 1.0)
    pedir("/console.png", "term4_mundo.png")
    print("--- tras escribir MUNDO ---")
    print(texto()[-800:])
    s.close()


if __name__ == "__main__":
    main()
