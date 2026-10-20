"""demo2.py - Sesion de uso real del Mini VI."""
import socket
import time

s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.5)
try:
    s.recv(65536)
except OSError:
    pass


def cmd(c, w=0.8):
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


print("run:", repr(cmd("r 0800\r", 4)[-60:]))
print("archivo:", repr(cmd("PRUEBA.TXT\r", 2)[-60:]))
print("i:", repr(cmd("i", 1)[-40:]))
print("texto:", repr(cmd("HOLA MUNDO", 1.5)[-120:]))
print("enter:", repr(cmd("\n", 1.2)[-120:]))
print("linea2:", repr(cmd("segunda linea", 1.5)[-120:]))
print("esc:", repr(cmd("\x1b", 1.2)[-160:]))
print("k:", repr(cmd("k", 1)[-160:]))
print("w:", repr(cmd("w", 1)[-160:]))
print("dd:", repr(cmd("dd", 1.5)[-160:]))
print("guardar:", repr(cmd(":w\n", 3)[-200:]))
s.close()
