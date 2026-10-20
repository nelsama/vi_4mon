"""probe3.py - Sustituir main() en RAM por JSR sonda + JMP monitor."""
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


cmd("load EDITOR.BIN 0800\r", 6)
# sonda en $1080: LDA #'A' / STA $C020 / RTS  (una vez)
for a, b in ((0x1080, 0xA9), (0x1081, 0x41), (0x1082, 0x8D),
             (0x1083, 0x20), (0x1084, 0xC0), (0x1085, 0x60)):
    cmd("W %04X %02X\r" % (a, b), 0.3)
# main = $0D3D -> JSR $1080 ; JMP $8000
for a, b in ((0x0D3D, 0x20), (0x0D3E, 0x80), (0x0D3F, 0x10),
             (0x0D40, 0x4C), (0x0D41, 0x00), (0x0D42, 0x80)):
    cmd("W %04X %02X\r" % (a, b), 0.3)
print("run:", repr(cmd("r 0800\r", 3)))
s.close()
