"""probe4.py - Cargar, ensuciar, ejecutar y ver si algo se limpia."""
import socket
import time
import urllib.request

g = lambda a, n: urllib.request.urlopen(
    "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (a, n)).read()

s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.5)
try:
    s.recv(65536)
except OSError:
    pass


def cmd(c, w=1.0):
    s.sendall(c.encode() if isinstance(c, str) else c)
    fin = time.time() + w
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


print(cmd("load EDITOR.BIN 0800\r", 6)[-40:])
cmd("W 040D 77\r", 0.5)
cmd("W 1000 55\r", 0.5)
print("antes : 040D=%s 1000=%s" % (g(0x40D, 1).hex(), g(0x1000, 1).hex()))
cmd("r 0800\r", 2)
print("despues: 040D=%s 1000=%s" % (g(0x40D, 1).hex(), g(0x1000, 1).hex()))
s.close()
