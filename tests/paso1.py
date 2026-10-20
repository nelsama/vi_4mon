"""paso1.py - Prueba del arranque del editor sin desbordar la UART.

Carga el binario por bloques de 64 bytes (un `load` por bloque, como el
monitor lee los ficheros) y ejecuta con `r 0800`. Comprueba:
  - que el binario se carga identico,
  - que zerobss deja el BSS a cero,
  - que `st` (primeros bytes del BSS) se inicializa desde main.

Uso: python tests/paso1.py [output/editor.bin] [--seg 4]
"""
import argparse
import os
import socket
import subprocess
import sys
import time
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EMU = "D:/Proyectos/EmuladorFpga6502"
SD_TOOL = EMU + "/tools/sd_tool.py"
IMG = os.path.join(ROOT, "editor_sd.img")
BASE = 0x0800
CHUNK = 64


def matar():
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
    time.sleep(1)


def arrancar():
    matar()
    subprocess.run(["powershell", "-Command",
                    "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                    "'rom/monitor.bin','4000','%s','--debug=8080'"
                    " -WindowStyle Hidden" % IMG.replace("/", "\\")],
                   cwd=EMU)
    for _ in range(80):
        try:
            socket.create_connection(("127.0.0.1", 4000), timeout=1).close()
            urllib.request.urlopen("http://127.0.0.1:8080/", timeout=1)
            time.sleep(2.0)
            return
        except OSError:
            time.sleep(0.25)
    raise SystemExit("el emulador no arranco")


def ram(addr, n):
    return urllib.request.urlopen(
        "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (addr, n),
        timeout=5).read()


def enviar(s, texto, seg):
    s.sendall(texto.encode() + b"\r")
    fin = time.time() + seg
    d = b""
    while time.time() < fin:
        s.settimeout(0.2)
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
    ap.add_argument("binario", nargs="?", default="output/editor.bin")
    ap.add_argument("--seg", type=float, default=4.0)
    a = ap.parse_args()

    binario = a.binario
    if not os.path.isabs(binario):
        binario = os.path.join(ROOT, binario)
    data = open(binario, "rb").read()
    print("binario: %d bytes ($%04X-$%04X)" %
          (len(data), BASE, BASE + len(data) - 1))

    subprocess.run(["cp", EMU + "/tarjeta.img", IMG], check=True)
    for n, off in enumerate(range(0, len(data), CHUNK)):
        trozo = data[off:off + CHUNK]
        nombre = "CH%03d.BIN" % n
        tmp = os.path.join(ROOT, "build", nombre)
        open(tmp, "wb").write(trozo)
        r = subprocess.run(["python", SD_TOOL, "inject", IMG, tmp, nombre],
                           capture_output=True, text=True)
        if r.returncode:
            print(r.stdout, r.stderr)

    arrancar()
    s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
    s.settimeout(0.5)
    time.sleep(1.5)
    try:
        s.recv(65536)
    except OSError:
        pass

    for n, off in enumerate(range(0, len(data), CHUNK)):
        enviar(s, "load CH%03d.BIN %04X" % (n, BASE + off), 0.35)

    cargado = ram(BASE, len(data))
    difs = [i for i in range(len(data)) if cargado[i] != data[i]]
    print("carga: %d diferencias" % len(difs))
    if difs:
        print("  primera dif. en $%04X" % (BASE + difs[0]))

    print("BSS antes de r  : " + ram(0x2AF1, 16).hex(" "))
    out = enviar(s, "r 0800", a.seg)
    print("--- salida:")
    print(out.decode(errors="replace").replace("\r", ""))
    print("BSS despues de r: " + ram(0x2AF1, 16).hex(" "))
    print("st (2AF1..2AFF): ")
    b = ram(0x2AF1, 40)
    print("  flags=%02X page=%02X/%02X ln=%02X cur=%02X,%02X" %
          (b[0], b[1], b[2], b[3], b[4], b[5]))
    s.close()


if __name__ == "__main__":
    main()
