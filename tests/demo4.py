"""demo4.py - Sesion completa en UNA conexion, con los tiempos bien."""
import socket
import time

s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.4)
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


def barra(d):
    t = d.decode(errors="replace")
    i = t.rfind("\x1b[21;1H")
    if i < 0:
        return "(sin barra)"
    j = t.find("\x1b[0m", i)
    return t[i:j if j > 0 else i + 90]


print("load  :", repr(cmd("load EDITOR.BIN 0800\r", 7)[-40:]))
print("run   :", repr(cmd("r 0800\r", 3)[-40:]))
print("nom   :", repr(cmd("DEMO.TXT\r", 4)[-40:]))
print("i     :", repr(cmd("i", 2)[-30:]))
print("HOLA  :", repr(barra(cmd("HOLA", 2))))
print("MUNDO :", repr(barra(cmd(" MUNDO", 2))))
print("ENTER :", repr(barra(cmd("\n", 2))))
print("linea2:", repr(barra(cmd("segunda", 2))))
print("ESC   :", repr(barra(cmd("\x1b", 2))))
print("wq    :", repr(cmd(":wq\n", 8)[-80:]))
s.close()
