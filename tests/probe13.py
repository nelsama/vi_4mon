"""probe13.py - Mapa de BSS por modulo y solape con otro objeto."""
import re
import subprocess

MAP = "output/editor.map"
m = open(MAP, encoding="latin-1").read()
mods = m.split("Modules list:")[1].split("Segment list:")[0]

actual = None
print("BSS por modulo:")
for linea in mods.splitlines():
    if linea and not linea.startswith(" ") and linea.rstrip().endswith(":"):
        actual = linea.strip()
    if "BSS" in linea and "Offs=" in linea:
        print(" ", actual, linea.strip())

print()
seg = m.split("Segment list:")[1].split("Exports")[0]
for linea in seg.splitlines():
    if linea.startswith(("BSS", "DATA", "CODE", "RODATA", "STARTUP", "ZEROPAGE")):
        print(linea)

print()
r = subprocess.run(["D:/cc65/bin/od65.exe", "--dump-exports",
                    "build/text.o"], capture_output=True, text=True)
for linea in r.stdout.splitlines():
    if "Name" in linea or "Value" in linea:
        print(linea)
