"""demo.py - Sesion de uso real del editor (como un usuario con PuTTY)."""
import socket
import time

s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.5)
try:
    s.recv(65536)
except OSError:
    pass


def cmd(c, w=1.5):
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


print("LOAD:", repr(cmd("load EDITOR.BIN 0800\r", 6)[-60:]))
print("RUN :", repr(cmd("r 0800\r", 6)[-260:]))
print("NOMBRE:", repr(cmd("PRUEBA.TXT\r", 4)[-200:]))
print("i:", repr(cmd("i", 1)[-60:]))
print("TEXTO:", repr(cmd("HOLA MUNDO", 2)[-200:]))
print("ESC :", repr(cmd("\x1b", 1)[-160:]))
print(":", repr(cmd(":", 0.6)[-40:]))
print("wq:", repr(cmd("wq", 0.6) + cmd("\n", 5))[-300:])
s.close()
