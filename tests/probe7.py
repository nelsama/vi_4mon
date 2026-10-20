"""probe7.py - Prueba variantes del STARTUP con un SD limpio y un solo emulador."""
import os
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run  # noqa: E402

ROOT = run.ROOT
CC65 = "D:/cc65/bin"

VAR = sys.argv[1]

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

open(os.path.join(ROOT, "tests/stub.s"), "w").write(BASE.format(extra=EXTRAS[VAR]))
subprocess.run([f"{CC65}/ca65.exe", "-t", "none", "--cpu", "6502",
                "-o", "build/startup.o", "tests/stub.s"], cwd=ROOT, check=True)
objs = ["build/startup.o"] + [f"build/{f}.o" for f in
        ("editor_util_asm", "editor_main", "editor_buffer",
         "editor_io", "editor_vi")]
subprocess.run([f"{CC65}/ld65.exe", "-C", "config/editor.cfg",
                "-o", "output/stub.bin"] + objs + ["D:/cc65/lib/none.lib"],
               cwd=ROOT, check=True)

subprocess.run(["cp", run.EMU + "/tarjeta.img", run.IMG])
subprocess.run(["python", run.SD_TOOL, "inject", run.IMG,
                os.path.join(ROOT, "output/stub.bin"), "TEST.BIN"],
               capture_output=True)
run.arrancar()
s = run.conectar()
run.enviar(s, "load TEST.BIN 0800", 6)
print(VAR, "->", repr(run.enviar(s, "r 0800", 3)))
s.close()
