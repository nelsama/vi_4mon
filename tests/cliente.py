"""cliente.py - Cliente UART sencillo para hablar con el monitor como un usuario.

Uso: python tests/cliente.py "load EDITOR.BIN 0800" "r 0800"
"""
import socket
import sys
import time


def abrir(puerto=4000):
    s = socket.create_connection(("127.0.0.1", puerto), timeout=5)
    time.sleep(1.5)
    s.settimeout(0.5)
    try:
        prompt = s.recv(65536)
    except OSError:
        prompt = b""
    return s, prompt


def enviar(s, texto, espera=3.0):
    """Envia una linea y devuelve toda la respuesta."""
    s.sendall(texto.encode() + b"\r")
    fin = time.time() + espera
    datos = b""
    while time.time() < fin:
        s.settimeout(0.3)
        try:
            c = s.recv(65536)
        except OSError:
            continue
        if not c:
            break
        datos += c
    return datos


def cerrar(s):
    s.close()


if __name__ == "__main__":
    s, prompt = abrir()
    for linea in sys.argv[1:]:
        print(f"$ {linea}")
        out = enviar(s, linea, 4.0)
        print(out.decode(errors="replace"))
    cerrar(s)
