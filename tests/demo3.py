"""demo3.py - Sesion completa en UNA conexion (carga, ejecuta y edita)."""
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
    """Extrae el texto de la barra de estado (fila 21)."""
    t = d.decode(errors="replace")
    i = t.find("\x1b[21;1H")
    return t[i:i + 80] if i >= 0 else t[-80:]


print("load:", repr(cmd("load EDITOR.BIN 0800\r", 6)[-40:]))
print("run :", repr(cmd("r 0800\r", 4)[-60:]))
print("nom.:", repr(cmd("PRUEBA.TXT\r", 2)[-60:]))
print("i   :", repr(cmd("i", 1)[-40:]))
print("HOLA:", repr(cmd("HOLA", 1.2)[-40:]))
print("barra:", repr(barra(cmd("", 0.6))))
print("MUNDO:", repr(cmd(" MUNDO", 1.2)[-40:]))
print("barra:", repr(barra(cmd("", 0.6))))
print("ENTER:", repr(barra(cmd("\n", 1.2))))
print("linea2:", repr(barra(cmd("segunda", 1.2))))
print("ESC :", repr(barra(cmd("\x1b", 1.2))))
print("kk  :", repr(barra(cmd("k", 0.8))))
print("dd  :", repr(barra(cmd("dd", 1.2))))
print("u?  :", repr(barra(cmd(":w\n", 4))))
s.close()
