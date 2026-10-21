"""harness.py - Arranca un unico emulador con una imagen SD hecha a medida.

    from harness import Emu, hacer_imagen, extraer
    hacer_imagen([("T.TXT", "hola\n")])
    e = Emu()          # arranca, conecta y espera el prompt del monitor
    e.cargar_editor()  # load EDITOR.BIN 0800 + r 0800
    e.abrir("T.TXT")
    ...
    e.close()          # mata el emulador
"""
import os
import socket
import struct
import subprocess
import time
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EMU_DIR = "D:/Proyectos/EmuladorFpga6502"
IMG = os.path.join(ROOT, "build", "test_sd.img")

SECTOR = 512
MAX_FILES = 16
ENTRY_SIZE = 32
ENTRY_BASE = 16
IMG_SIZE = 16 * 1024 * 1024


def _tabla(data):
    out = []
    for i in range(MAX_FILES):
        off = ENTRY_BASE + i * ENTRY_SIZE
        nombre = bytes(data[off:off + 12]).split(b"\0")[0].decode("ascii", "replace")
        if nombre:
            start, size, _ = struct.unpack_from("<HHH", data, off + 12)
            out.append((nombre, start, size))
    return out


def hacer_imagen(ficheros, binario="editor/editor.bin"):
    """Crea build/test_sd.img desde cero.

    ficheros: lista de (nombre, bytes|str). EDITOR.BIN se anade siempre.
    """
    os.makedirs(os.path.dirname(IMG), exist_ok=True)
    data = bytearray(IMG_SIZE)
    data[0:2] = b"FM"

    todos = []
    for nombre, contenido in ficheros:
        if isinstance(contenido, str):
            contenido = contenido.encode()
        todos.append((nombre.upper(), contenido))
    # EDITOR.BIN va AL FINAL: asi el primer fichero empieza en el sector 1 y la
    # parte .GNU de MicroFS (sector 1) no queda tapada por datos. Con el
    # binario primero, cualquier fichero de mas de 512-32 bytes pisaba .GNU y
    # el editor leia tamanos basura del indice.
    todos.append(("EDITOR.BIN", open(os.path.join(ROOT, binario), "rb").read()))

    siguiente = 1
    for idx, (nombre, contenido) in enumerate(todos):
        off = ENTRY_BASE + idx * ENTRY_SIZE
        nb = nombre.encode("ascii")[:11]
        data[off:off + len(nb)] = nb
        sectores = (len(contenido) + SECTOR - 1) // SECTOR or 1
        struct.pack_into("<HHH", data, off + 12, siguiente, len(contenido), sectores)
        inicio = siguiente * SECTOR
        data[inicio:inicio + len(contenido)] = contenido
        siguiente += sectores

    struct.pack_into("<H", data, 6, siguiente)
    with open(IMG, "wb") as f:
        f.write(data)


def extraer(nombre):
    """Contenido del fichero `nombre` dentro de la imagen (None si no existe)."""
    data = open(IMG, "rb").read()
    for n, start, size in _tabla(data):
        if n.upper() == nombre.upper():
            return data[start * SECTOR:start * SECTOR + size]
    return None


def listar():
    return [n for n, _, _ in _tabla(open(IMG, "rb").read())]


def _matar():
    subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
    time.sleep(1.0)


class Emu:
    def __init__(self, espera=2.0):
        _matar()
        subprocess.run(["powershell", "-Command",
                        "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                        "'rom/monitor.bin','4000','%s','--debug=8080'"
                        " -WindowStyle Hidden" % IMG.replace("/", "\\")],
                       cwd=EMU_DIR)
        for _ in range(100):
            try:
                socket.create_connection(("127.0.0.1", 4000), timeout=1).close()
                urllib.request.urlopen("http://127.0.0.1:8080/", timeout=1).read()
                break
            except OSError:
                time.sleep(0.25)
        time.sleep(espera)
        self.s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
        self.s.settimeout(0.3)
        time.sleep(1.5)
        self.drain()

    def drain(self, seg=0.6):
        fin = time.time() + seg
        d = b""
        while time.time() < fin:
            try:
                c = self.s.recv(65536)
            except socket.timeout:
                continue
            if not c:
                break
            d += c
        return d

    def cmd(self, texto, seg=0.6):
        self.s.sendall(texto.encode() + b"\r")
        return self.drain(seg)

    def cargar_editor(self, seg=2.0):
        o = self.cmd("load EDITOR.BIN 0800", seg)
        o += self.cmd("r 0800", 1.5)
        return o

    def abrir(self, nombre, seg=2.0):
        self.s.sendall(nombre.encode() + b"\r")
        return self.drain(seg)

    def teclas(self, texto, seg=0.8):
        self.s.sendall(texto.encode())
        return self.drain(seg)

    def ram(self, addr, n):
        return urllib.request.urlopen(
            "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (addr, n),
            timeout=5).read()

    def close(self):
        try:
            self.s.close()
        finally:
            _matar()
