"""probe5.py - Llega el editor a main()? (comprueba el banner por UART)."""
import socket
import time

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


print("load:", repr(cmd("load EDITOR.BIN 0800\r", 6)[-30:]))
print("run :", repr(cmd("r 0800\r", 4)))
print("más :", repr(cmd("", 2)))
s.close()
