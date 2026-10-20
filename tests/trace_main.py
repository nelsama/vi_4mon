"""trace_main.py - Medir hasta donde llega main() leyendo g_editor (0400)."""
import socket, time, urllib.request
g=lambda a,n: urllib.request.urlopen("http://127.0.0.1:8080/ram.bin?start=%d&len=%d"%(a,n)).read()
s=socket.create_connection(("127.0.0.1",4000),timeout=5); time.sleep(1.5)
s.settimeout(0.5)
try: s.recv(65536)
except OSError: pass
def cmd(c,w=1.5):
    s.sendall(c.encode() if isinstance(c,str) else c)
    fin=time.time()+w; d=b""
    while time.time()<fin:
        s.settimeout(0.3)
        try: x=s.recv(65536)
        except OSError: continue
        if not x: break
        d+=x
    return d
cmd("load EDITOR.BIN 0800\r",6)
cmd("r 0800\r",2)
print("tras run:", "g_editor=",g(0x400,0x20).hex(" "), "sp=",g(2,2).hex(" "))
print("modo (0416):", g(0x416,1).hex())
print("uart:", repr(cmd("",0.5)))
s.close()
