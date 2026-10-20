"""probe12.py - Quien escribe en BSS: ejecutar el programa varias veces y comparar."""
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


print("antes de cargar, BSS:", u(0x2AF1, 16).hex(" "))
print(cmd("load EDITOR.BIN 0800\r", 7)[-40:])
print("tras cargar,  BSS:", u(0x2AF1, 16).hex(" "))
out = cmd("r 0800\r", 3)
print("salida run:", repr(out[:80]))
print("tras run,     BSS:", u(0x2AF1, 16).hex(" "))
s.close()
