"""dbg.py - Arranca el editor, escribe FIXME y vuelca el buffer por HTTP."""
import socket
import time
import urllib.request

u = lambda a, n: urllib.request.urlopen(
    "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (a, n)).read()

s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.5)
try:
    s.recv(65536)
except OSError:
    pass


def cmd(c, w=0.6):
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


def ver(tag):
    print(tag, repr(u(0x2E00, 24)))


cmd("r 0800\r", 3)
ver("inicio   ")
cmd("T.TXT\r", 1.5)
ver("tras nom.")
cmd("i", 0.8)
ver("tras i   ")
for ch in "ABCDE":
    cmd(ch, 0.35)
    ver("  " + ch + "      ")
cmd("\x1b", 1)
ver("tras ESC ")
s.close()
