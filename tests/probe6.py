"""probe6.py - Prueba variantes del STARTUP con un SD limpio y un solo emulador."""
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CC65 = "D:/cc65/bin"
EMU = "D:/Proyectos/EmuladorFpga6502"

VARIANTE = sys.argv[1]

BASE = """
.export _init
.export __STARTUP__ : absolute = 1
.import _main
.import _mfs_init
.import zerobss
.importzp sp
.segment "STARTUP"
_init:
    sei
    cld
    ldx #$FF
    txs
    lda #<$3E00
    sta sp
    lda #>$3E00
    sta sp+1
    lda #'1'
    sta $C020
{extra}
    lda #'2'
    sta $C020
    jsr _main
    ldx #$FF
    txs
    jmp $8000
"""

EXTRAS = {
    "zm": "",
    "z": "    jsr zerobss\n",
    "m": "    jsr _mfs_init\n",
    "zmf": "    jsr zerobss\n    jsr _mfs_init\n",
}

open(os.path.join(ROOT, "tests/stub.s"), "w").write(BASE.format(extra=EXTRAS[VARIANTE]))
subprocess.run([f"{CC65}/ca65.exe", "-t", "none", "--cpu", "6502",
                "-o", "build/startup.o", "tests/stub.s"], cwd=ROOT, check=True)
objs = ["build/startup.o"] + [f"build/{f}.o" for f in
        ("editor_util_asm", "editor_main", "editor_buffer",
         "editor_io", "editor_vi")]
subprocess.run([f"{CC65}/ld65.exe", "-C", "config/editor.cfg",
                "-o", "output/stub.bin"] + objs + ["D:/cc65/lib/none.lib"],
               cwd=ROOT, check=True)

subprocess.run(["taskkill", "/F", "/IM", "emulador.exe"], capture_output=True)
time.sleep(2)
subprocess.run(["cp", EMU + "/tarjeta.img", "editor_sd.img"], cwd=ROOT)
subprocess.run(["python", EMU + "/tools/sd_tool.py", "inject", "editor_sd.img",
                os.path.join(ROOT, "output/stub.bin"), "TEST.BIN"],
               cwd=ROOT, capture_output=True)
subprocess.run(["powershell", "-Command",
                "Start-Process -FilePath 'emulador.exe' -ArgumentList "
                "'rom/monitor.bin','4000','D:/Proyectos/vi_4mon/editor_sd.img'"
                " -WindowStyle Hidden"], cwd=EMU)
time.sleep(8)
s = socket.create_connection(("127.0.0.1", 4000), timeout=5)
time.sleep(1.5)
s.settimeout(0.5)
try:
    s.recv(65536)
except OSError:
    pass


def cmd(c, w=2.0):
    s.sendall(c.encode() + b"\r")
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


cmd("load TEST.BIN 0800", 6)
print(VARIANTE, "->", repr(cmd("r 0800", 3)))
s.close()
