"""test_casos.py - Bateria de casos de uso del editor (una sesion por caso).

Cada caso arranca un emulador limpio, hace una operacion y comprueba el
resultado objetivo (texto en pantalla o bytes del fichero en la SD).

Uso: python tests/test_casos.py [caso...]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402

BIN = "editor/editor.bin"


class Sesion:
    def __init__(self, ficheros, nombre=None):
        harness.hacer_imagen(ficheros, binario=BIN)
        self.e = harness.Emu()
        self.nombre = nombre
        self.log = ""
        self.cmd("load EDITOR.BIN 0800", 3.0)
        self.cmd("r 0800", 2.0)
        if nombre:
            self.raw(nombre + "\r", 3.0)

    def raw(self, texto, seg):
        out = self.e.teclas(texto, seg).decode(errors="replace")
        self.log += out
        return out

    def cmd(self, texto, seg):
        out = self.e.cmd(texto, seg).decode(errors="replace")
        self.log += out
        return out

    def pantalla(self):
        """Ultimo trozo de texto recibido (lo que se ve en pantalla)."""
        return self.log.replace("\r", "")

    def close(self):
        self.e.close()


def resultado(nombre, ok, detalle=""):
    print(("OK" if ok else "FALLO") + (
        ("   " + detalle) if detalle else ""), flush=True)
    return 0 if ok else 1


# ---------------------------------------------------------------------------

def c_abrir():
    s = Sesion([("T.TXT", "uno\ndos\ntres\n")], "T.TXT")
    t = s.pantalla()
    s.close()
    return resultado("abrir fichero existente", "uno" in t and "tres" in t)


def c_nuevo():
    # OJO: hay que entrar en modo INSERT con 'i'. Teclear directamente en modo
    # NORMAL interpreta cada letra como comando vi (la 'h' mueve a la
    # izquierda), que es justo como funciona vi de verdad.
    s = Sesion([], "NUEVO.TXT")
    s.raw("i", 0.6)
    s.raw("hola mundo\r", 1.5)
    s.raw("\x1b", 0.8)
    s.raw(":wq\r", 4.0)
    s.close()
    d = harness.extraer("NUEVO.TXT")
    return resultado("crear fichero nuevo", d == b"hola mundo\n", repr(d))


def c_insertar():
    s = Sesion([("T.TXT", "abc\n")], "T.TXT")
    s.raw("A", 0.5)
    s.raw("XYZ\r", 1.0)
    s.raw("\x1b", 0.5)
    s.raw(":wq\r", 4.0)
    s.close()
    d = harness.extraer("T.TXT")
    return resultado("insertar (A)", d == b"abcXYZ\n", repr(d))


def c_borrar_char():
    s = Sesion([("T.TXT", "abcd\n")], "T.TXT")
    s.raw("x", 0.5)
    s.raw(":wq\r", 4.0)
    s.close()
    d = harness.extraer("T.TXT")
    return resultado("borrar caracter (x)", d == b"bcd\n", repr(d))


def c_borrar_linea():
    s = Sesion([("T.TXT", "uno\ndos\ntres\n")], "T.TXT")
    s.raw("jdd", 1.5)
    s.raw(":wq\r", 4.0)
    s.close()
    d = harness.extraer("T.TXT")
    return resultado("borrar linea (dd)", d == b"uno\ntres\n", repr(d))


def c_yank_put():
    s = Sesion([("T.TXT", "uno\ndos\n")], "T.TXT")
    s.raw("yyp", 2.0)
    s.raw(":wq\r", 4.0)
    s.close()
    d = harness.extraer("T.TXT")
    return resultado("copiar y pegar (yy p)", d == b"uno\nuno\ndos\n", repr(d))


def c_mover():
    s = Sesion([("A.TXT", "AAA\nBBB\nCCC\n")], "A.TXT")
    s.raw("jjG", 1.5)
    s.raw(":wq\r", 4.0)
    s.close()
    return resultado("navegacion j G (no corrompe)", harness.extraer("A.TXT") == b"AAA\nBBB\nCCC\n")


def c_sin_guardar():
    s = Sesion([("T.TXT", "uno\n")], "T.TXT")
    s.raw("x", 0.5)
    s.raw("\x1b:q!\r", 2.0)
    s.close()
    d = harness.extraer("T.TXT")
    return resultado("salir sin guardar (:q!)", d == b"uno\n", repr(d))


def c_fichero_grande():
    """Fichero mayor que el buffer: NO debe destruirse ni mezclarse.
    (Paginacion fuera de alcance por ahora; lo que se exige es no perder el
    original: o se guarda entero o se rechaza el guardado.)"""
    texto = "".join("linea %03d\n" % i for i in range(150))   # ~1500 B > 1300
    s = Sesion([("G.TXT", texto)], "G.TXT")
    s.raw("A", 0.5)
    s.raw("X\r", 1.0)
    s.raw("\x1b", 0.5)
    s.raw(":wq\r", 5.0)
    s.close()
    d = harness.extraer("G.TXT")
    ok = d == texto.encode() + b"X\n"
    return resultado("fichero mayor que el buffer (~1500 B)", ok,
                     "tras guardar %d B de %d" % (len(d or b""), len(texto) + 2))


def c_multilinea():
    """Editar varias lineas y guardar sin perdidas."""
    s = Sesion([("M.TXT", "a\nb\nc\nd\n")], "M.TXT")
    s.raw("jjA", 0.6)
    s.raw("Z\r", 0.8)
    s.raw("\x1b", 0.5)
    s.raw(":wq\r", 4.0)
    s.close()
    d = harness.extraer("M.TXT")
    return resultado("multiples lineas", d == b"a\nb\ncZ\nd\n", repr(d))


CASOS = {
    "abrir": c_abrir,
    "nuevo": c_nuevo,
    "insertar": c_insertar,
    "borrar_char": c_borrar_char,
    "borrar_linea": c_borrar_linea,
    "yank_put": c_yank_put,
    "mover": c_mover,
    "sin_guardar": c_sin_guardar,
    "grande": c_fichero_grande,
    "multilinea": c_multilinea,
}


def main():
    nombres = sys.argv[1:] or list(CASOS)
    fallos = 0
    total = len(nombres)
    for i, n in enumerate(nombres, 1):
        if n not in CASOS:
            print("caso desconocido:", n, flush=True)
            continue
        print("[%d/%d] %s ..." % (i, total, n), end=" ", flush=True)
        r = CASOS[n]()
        fallos += r
        if r:
            print("  (seguimos con el siguiente)", flush=True)
    print()
    print("%d caso(s) fallido(s) de %d" % (fallos, total), flush=True)
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
