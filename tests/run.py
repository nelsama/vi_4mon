"""run.py - Lanza UN solo emulador, carga un binario y lo ejecuta.

Uso: python tests/run.py output/editor.bin [EDITOR.BIN] [--seg 4]

Garantiza que no queden emuladores viejos (puertos compartidos).
"""
import argparse
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EMU = "D:/Proyectos/EmuladorFpga6502"
SD_TOOL = EMU + "/tools/sd_tool.py"
IMG = os.path.join(ROOT, "editor_sd.img")


def matar_todos():
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"],
                   capture_output=True)
    for _ in range(20):
        r = subprocess.run(["tasklist", "/FI", "IMAGENAME eq emulador.exe"],
                           capture_output=True, text=True)
        if "emulador.exe" not in r.stdout:
            return
        time.sleep(0.25)
    raise SystemExit("no se pudo matar todos los emuladores")


def arrancar():
    matar_todos()
    subprocess.run(["powershell", "-Command",
                    "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                    "'rom/monitor.bin','4000','%s','--debug=8080'"
                    " -WindowStyle Hidden" % IMG.replace("/", "\\")],
                   cwd=EMU)
    for _ in range(60):
        try:
            socket.create_connection(("127.0.0.1", 4000), timeout=1).close()
            time.sleep(1.5)
            return
        except OSError:
            time.sleep(0.25)
    raise SystemExit("el emulador no arranco")


def conectar():
    s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
    s.settimeout(0.5)
    time.sleep(1.5)
    try:
        s.recv(65536)
    except OSError:
        pass
    return s


def enviar(s, texto, seg):
    s.sendall(texto.encode() + b"\r")
    fin = time.time() + seg
    d = b""
    while time.time() < fin:
        s.settimeout(0.3)
        try:
            x = s.recv(65536)
        except OSError:
            continue
        if not x:
            break
        d += x
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("binario")
    ap.add_argument("nombre", nargs="?", default="TEST.BIN")
    ap.add_argument("--seg", type=float, default=4.0)
    ap.add_argument("--no-load", action="store_true")
    a = ap.parse_args()

    binario = a.binario
    if not os.path.isabs(binario):
        binario = os.path.join(ROOT, binario)
    subprocess.run(["cp", EMU + "/tarjeta.img", IMG])
    out = subprocess.run(["python", SD_TOOL, "inject", IMG, binario, a.nombre],
                         capture_output=True, text=True).stdout
    print(out.strip())
    arrancar()
    s = conectar()
    if not a.no_load:
        print(enviar(s, "load %s 0800" % a.nombre, 6).decode(errors="replace").strip())
    print(enviar(s, "r 0800", a.seg).decode(errors="replace").strip())
    s.close()


if __name__ == "__main__":
    main()
