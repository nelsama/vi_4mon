"""emu.py - utilidades para probar VI-EDITOR en el emulador FPGA 6502.

Requiere el emulador arrancado (run_debug) en UART 4000 y debug 8080.
"""
import re
import socket
import sys
import time
import urllib.request

if sys.stdout:
    sys.stdout.reconfigure(line_buffering=True)

UART = 4000
DEBUG = 8080


class Emu:
    def __init__(self, port=UART, esperar_prompt=True):
        self.s = socket.create_connection(("127.0.0.1", port), timeout=5)
        self.s.settimeout(0.3)
        if esperar_prompt:
            self.esperar(b">")

    def esperar(self, marca, limite=25.0):
        """Lee hasta ver `marca` (p.ej. el prompt del monitor)."""
        datos = b""
        fin = time.time() + limite
        self.s.settimeout(0.3)
        while time.time() < fin:
            try:
                c = self.s.recv(65536)
            except socket.timeout:
                continue
            if not c:
                break
            datos += c
            if marca in datos:
                return datos
        return datos

    def drain(self):
        out = b""
        while True:
            try:
                c = self.s.recv(65536)
            except socket.timeout:
                break
            if not c:
                break
            out += c
        return out

    def send(self, data, wait=0.5):
        if isinstance(data, str):
            data = data.encode()
        self.s.sendall(data)
        time.sleep(wait)
        return self.drain()

    def cmd(self, text, wait=1.5):
        return self.send(text + "\r", wait)

    def close(self):
        self.s.close()


def ram(start, ln):
    return urllib.request.urlopen(
        f"http://127.0.0.1:{DEBUG}/ram.bin?start={start}&len={ln}").read()


def u16(addr):
    return int.from_bytes(ram(addr, 2), "little")


TXT_START = 0x0664


# Direcciones reales (ver tests/sym.py y output/editor.map)
SECTOR_VALID = 0x06C2
STATE_ADDR = 0x0654
EDIT_BUF = 0x36E0


def status(raw):
    """Devuelve el texto de la barra de estado (fila 22) del ultimo frame."""
    t = raw.decode(errors="replace")
    if not t:
        return "(sin barra)"
    if "DATA SAVE FAILED" in t:
        return "DATA SAVE FAILED"
    m = re.findall(r"\x1b\[22;1H(.*?)\x1b\[23", t, re.S)
    return m[-1] if m else "(sin barra)"


def buf():
    """Devuelve (longitud, contenido) del buffer de edición."""
    n = u16(SECTOR_VALID)
    if n > 1300:
        n = 1300
    return n, ram(EDIT_BUF, n)


def line_count():
    n = u16(SECTOR_VALID)
    if n > 1300:
        n = 1300
    return ram(EDIT_BUF, n).count(b"\n")


def start_editor(emu, name="A.TXT", out=None):
    """Carga y lanza el editor dejándolo pidiendo el nombre de archivo."""
    o = emu.cmd("load EDITOR.BIN 0800", 6)
    o += emu.cmd("r 0800", 3)
    if out is not None:
        out += o
    return o


def main():
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "start":
        e = Emu()
        print(start_editor(e).decode(errors="replace")[-300:])
        e.close()


if __name__ == "__main__":
    main()
