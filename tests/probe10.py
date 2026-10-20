"""probe10.py - Diferencia de RAM entre "recien lanzado" y "tras leer el nombre"."""
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
antes = u(0x0800, 0x2B00)
out = cmd("T.TXT\r", 2)
print("salida:", repr(out[:120]))
despues = u(0x0800, 0x2B00)
for i in range(len(antes)):
    if antes[i] != despues[i]:
        a = 0x800 + i
        print("$%04X  %02X -> %02X" % (a, antes[i], despues[i]))
s.close()
