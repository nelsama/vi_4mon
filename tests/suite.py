"""suite.py - Bateria de pruebas del editor en el emulador FPGA 6502.

Requiere el emulador arrancado:  python tests/harness.py start

Uso:  python tests/suite.py [prueba ...]

Cada prueba escribe solo al principio del buffer y consulta la RAM por HTTP
(/ram.bin) con las direcciones reales de output/editor.map.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import emu


def check(name, cond, detail=""):
    print(("[OK]   " if cond else "[FALLO]") + " " + name +
          ("" if cond else f"   -> {detail}"))
    return cond


def session(name):
    """Arranca una sesion nueva del editor y devuelve (Emu, salida).

    Si el archivo ya existe en la SD, dG borra su contenido para que cada
    prueba empiece sobre un lienzo limpio. El nombre se teclea con pausas:
    io_gets() es lento y un burfer UART desbordado perderia caracteres.
    """
    e = emu.Emu()
    e.cmd("load EDITOR.BIN 0800", 6)
    e.cmd("r 0800", 2)
    o = b""
    for ch in name + "\r":
        o += e.send(ch, 0.12)
    e.drain()
    if emu.buf()[0] > 1:
        e.send("d", 0.3) + e.send("G", 0.4)
    return e, o


def t_status():
    e, o = session("STATUS.TXT")
    st = emu.status(o)
    check("nombre de archivo correcto", "STATUS.TXT" in st, st)
    check("contador arranca en 1b", "1b" in st, st)
    o = e.send("i", 0.5) + e.send("HOLA", 0.5)
    st = emu.status(o)
    check("insert 'HOLA' -> 5b", "5b" in st, st)
    n, d = emu.buf()
    check("buffer = 'HOLA' + LF", d.rstrip(b"\n") == b"HOLA", repr(d[-20:]))
    e.send("\x1b", 0.5)
    e.close()


def t_newline_cr():
    e, _ = session("NL.TXT")
    e.send("i", 0.5) + e.send("AA", 0.5) + e.send("\r", 0.8) + e.send("BB", 0.5)
    n, d = emu.buf()
    check("Enter (CR) crea linea", d.rstrip(b"\n").endswith(b"AA\nBB"),
          repr(d[-20:]))
    e.send("\x1b", 0.5)
    check("dos lineas logidas", d.count(b"\n") == 2, repr(d[-20:]))
    e.close()


def t_newline_lf():
    e, _ = session("NL2.TXT")
    e.send("i", 0.5) + e.send("AA", 0.5) + e.send("\n", 0.8) + e.send("BB", 0.5)
    n, d = emu.buf()
    check("Enter (LF) no duplica la linea",
          d.rstrip(b"\n").endswith(b"AA\nBB"), repr(d[-20:]))
    e.send("\x1b", 0.5)
    e.close()


def t_backspace():
    e, _ = session("BS.TXT")
    e.send("i", 0.5) + e.send("XYZ", 0.5) + e.send("\x7f", 0.5)
    n, d = emu.buf()
    check("Backspace borra el caracter", d.rstrip(b"\n").endswith(b"XY"),
          repr(d[-20:]))
    e.send("\x1b", 0.5)
    e.close()


def t_cursor_clamp():
    e, _ = session("CUR.TXT")
    e.send("i", 0.5)
    e.send("Q" * 80, 1.5)
    o = e.send("Q", 0.8)          # el 81.o se rechaza
    st = emu.status(o)
    check("cursor no pasa de columna 79", "(1,81)" not in st, st)
    check("avisa 'Line full'", "Line full" in st, st)
    n, d = emu.buf()
    check("linea limitada a 80 caracteres",
          d.count(b"Z") + d.count(b"Q") == 80 and b"\n" in d, str(n))
    e.send("\x1b", 0.5)
    e.close()


def t_buffer_full():
    e, _ = session("FULL.TXT")
    e.send("i", 0.5)
    o = b""
    for _ in range(13):
        o = e.send("Z" * 80, 0.8) + e.send("\r", 0.8)
    o += e.send("Z", 0.8)         # ya no cabe
    n = emu.u16(emu.SECTOR_VALID)
    check("el buffer no supera 1311", 0 < n <= 1311, str(n))
    st = emu.status(o)
    check("avisa 'Buffer full'", "Buffer full" in st, st)
    check("el editor sigue vivo", "FULL.TXT" in st, st)
    e.send("\x1b", 0.5)
    o = e.send(":", 0.5) + e.send("q!", 0.5) + e.send("\n", 4)
    check(":q! vuelve al monitor",
          "Monitor 6502" in o.decode(errors="replace"), repr(o[-40:]))
    e.close()


def t_esc():
    e, _ = session("ESC.TXT")
    e.send("i", 0.5) + e.send("AB", 0.5)
    st = emu.status(e.send("\x1b", 0.5))
    check("ESC -> modo NORMAL", "NORMAL" in st, st)
    n, d = emu.buf()
    check("sin caracteres basura tras ESC", d == b"AB\n", repr(d))
    e.close()


def t_quit():
    e, _ = session("Q.TXT")
    e.send("i", 0.5) + e.send("X", 0.5) + e.send("\x1b", 0.5)
    o = e.send(":", 0.5) + e.send("q!", 0.5) + e.send("\n", 3)
    check(":q! vuelve al monitor",
          "Monitor 6502" in o.decode(errors="replace"), repr(o[-40:]))
    e.close()


def t_relaunch():
    e, _ = session("R1.TXT")
    e.send("i", 0.5) + e.send("XYZ", 0.5) + e.send("\x1b", 0.5)
    e.send(":", 0.5) + e.send("q!", 0.5) + e.send("\n", 4)
    o = e.cmd("r 0800", 2)
    check("segundo lanzado arranca", "File:" in o.decode(errors="replace"),
          repr(o[-40:]))
    o = e.send("R2.TXT\r", 2)
    n, d = emu.buf()
    check("sesion nueva arranca limpia", d == b"\n", repr(d))
    check("barra muestra el nombre", "R2.TXT" in emu.status(o), emu.status(o))
    e.close()


def t_save_content():
    e, _ = session("SV.TXT")
    e.send("i", 0.5) + e.send("CONTENIDO", 0.5) + e.send("\x1b", 0.5)
    o = e.send(":", 0.5) + e.send("wq", 0.5) + e.send("\n", 5)
    check(":wq guarda y sale",
          "Monitor 6502" in o.decode(errors="replace"), repr(o[-40:]))
    e.close()


TESTS = {
    "status": t_status,
    "cr": t_newline_cr,
    "lf": t_newline_lf,
    "backspace": t_backspace,
    "cursor": t_cursor_clamp,
    "full": t_buffer_full,
    "esc": t_esc,
    "quit": t_quit,
    "relaunch": t_relaunch,
    "save": t_save_content,
}


def main():
    for name in (sys.argv[1:] or list(TESTS)):
        print(f"--- {name}")
        try:
            TESTS[name]()
        except Exception as ex:
            print(f"[FALLO] excepcion: {type(ex).__name__}: {ex}")
    print("fin")


if __name__ == "__main__":
    main()
