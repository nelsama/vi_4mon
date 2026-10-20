"""probe9.py - Carga el binario y vuelca la RAM antes de ejecutarlo."""
import subprocess
import time

import run

run.matar_todos()
img = run.IMG
subprocess.run(["cp", run.EMU + "/tarjeta.img", img])
subprocess.run(["python", run.SD_TOOL, "inject", img,
                run.ROOT + "/output/editor.bin", "EDITOR.BIN"],
               capture_output=True)
run.arrancar()
s = run.conectar()
run.enviar(s, "load EDITOR.BIN 0800", 7)
print("cargado, ahora volcado")
s.close()

import urllib.request
u = lambda a, n: urllib.request.urlopen(
    "http://127.0.0.1:8080/ram.bin?start=%d&len=%d" % (a, n)).read()
b = open(run.ROOT + "/output/editor.bin", "rb").read()
r = u(0x0800, len(b))
diff = [i for i in range(len(b)) if b[i] != r[i]]
print("bytes del binario:", len(b), "distintos en RAM:", len(diff))
for i in diff[:10]:
    print("  $%04X bin=%02X ram=%02X" % (0x800 + i, b[i], r[i]))
# zona de pila y de estado
print("pila 1F0:", u(0x1F0, 16).hex(" "))
print("st 2AF1:", u(0x2AF1, 16).hex(" "))
