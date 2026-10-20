"""probe8.py - Donde acaba zerobss en el mapa y que hay en $0A20."""
import re

m = open("output/editor.map", encoding="latin-1").read()
sec = m.split("Exports list by name:")[-1]
d = {nm: int(a, 16) for nm, a in re.findall(r"_(\w+)\s+([0-9A-F]{6})", sec)}
print("zerobss exportado:", "zerobss" in d, d.get("zerobss"))
print("mfs_init:", hex(d["mfs_init"]))
# modulos que aportan CODE cerca de 0A20
mods = m.split("Modules list:")[1].split("Exports list")[0]
actual = None
for linea in mods.splitlines():
    if linea and not linea.startswith(" ") and linea.endswith(":"):
        actual = linea
    if "CODE" in linea and "Offs=" in linea:
        print(actual.strip(), linea.strip())
