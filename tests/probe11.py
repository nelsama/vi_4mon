"""probe11.py - Mirar st y el nombre en BSS tras leer el nombre."""
import socket
import time
import urllib.request

u = lambda a, n: urllib.request.urlopen(
    "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (a, n)).read()

s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.4)
try:
    s.recv(65536)
except OSError:
    pass


def cmd(c, w=1.0):
    s.sendall(c.encode() if isinstance(c, str) else c)
    fin = time.time() + w
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


cmd("r 0800\r", 2.5)
print("BSS inicial:", u(0x2AF1, 32).hex(" "))
out = cmd("T.TXT\r", 2)
print("salida:", repr(out))
print("BSS tras nombre:", u(0x2AF1, 64).hex(" "))
print("nombre (file_name):", u(0x2C10, 16))
s.close()
