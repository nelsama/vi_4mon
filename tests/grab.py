#!/usr/bin/env python3
"""grab.py - Cliente RAW de la UART del emulador (emula PuTTY en modo Raw).

Uso:
  python tests/grab.py "load EDITOR.BIN 0800" "r 0800" [--seg 3] [--puerto 4000]

Muestra todo lo que el monitor/editor envían por UART y guarda una copia en
tests/uart.log.
"""
import argparse
import socket
import sys
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("comandos", nargs="*", help="lineas a enviar (Enter al final)")
    ap.add_argument("--seg", type=float, default=3.0)
    ap.add_argument("--puerto", type=int, default=4000)
    ap.add_argument("--log", default="tests/uart.log")
    a = ap.parse_args()

    s = socket.create_connection(("127.0.0.1", a.puerto), timeout=5)
    todo = b""
    # leer el prompt inicial
    fin = time.time() + 2
    while time.time() < fin:
        s.settimeout(0.3)
        try:
            c = s.recv(65536)
        except OSError:
            continue
        if not c:
            break
        todo += c
    for cmd in a.comandos:
        s.sendall(cmd.encode() + b"\r")
        fin = time.time() + a.seg
        while time.time() < fin:
            s.settimeout(0.3)
            try:
                c = s.recv(65536)
            except OSError:
                continue
            if not c:
                break
            todo += c
    s.close()
    with open(a.log, "wb") as f:
        f.write(todo)
    sys.stdout.write(todo.decode(errors="replace"))


if __name__ == "__main__":
    main()
