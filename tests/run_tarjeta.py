"""run_tarjeta.py - Arranca el emulador con tarjeta.img y lanza el editor.

Uso: python tests/run_tarjeta.py [fichero_a_editar]

Carga EDITOR.BIN desde la tarjeta y entra al editor. Si se indica un fichero,
lo abre directamente.
"""
import os
import socket
import subprocess
import sys
import time
import urllib.request

EMU = "D:/Proyectos/EmuladorFpga6502"
IMG = EMU + "/tarjeta.img"


def main():
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
    time.sleep(1.0)
    subprocess.run(["powershell", "-Command",
                    "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                    "'rom/monitor.bin','4000','%s','--debug=8080'"
                    " -WindowStyle Hidden" % IMG.replace("/", "\\")],
                   cwd=EMU)
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

    def enviar(texto, seg=2.0):
        s.sendall(texto.encode())
        fin = time.time() + seg
        d = b""
        while time.time() < fin:
            try:
                d += s.recv(65536)
            except OSError:
                continue
        return d

    enviar("", 0.5)
    print(enviar("load EDITOR.BIN 0800\r", 4.0).decode(errors="replace").strip())
    print(enviar("r 0800\r", 2.5).decode(errors="replace").replace("\r", ""))
    if len(sys.argv) > 1:
        print(enviar(sys.argv[1] + "\r", 3.0).decode(errors="replace").replace("\r", ""))
        print("\nEditor listo. Conectate a localhost:4000 (PuTTY raw) para teclear.")
    s.close()


if __name__ == "__main__":
    main()
