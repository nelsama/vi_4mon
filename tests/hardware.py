"""hardware.py - Pruebas contra el hardware real (Tang Nano 9K por UART).

El emulador no puede leer RAM una vez arrancado el programa (el HTTP solo mira
la RAM del propio emulador), y ademas no modela el BUSY de la UART. Por eso el
paso final se prueba en la placa con un puerto serie.

    python hardware.py COM5 arranque
    python hardware.py COM5 editar

Muestra todo lo que llega por el puerto y deja que el operador vea la pantalla.
"""
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("falta pyserial: pip install pyserial")


class Serie:
    def __init__(self, puerto, baud=115200):
        self.p = serial.Serial(puerto, baud, timeout=0.1)
        time.sleep(0.5)
        self.drain()

    def drain(self, seg=0.6):
        fin = time.time() + seg
        d = b""
        while time.time() < fin:
            d += self.p.read(4096)
        return d

    def cmd(self, texto, seg=1.0):
        self.p.write(texto.encode() + b"\r")
        return self.drain(seg)

    def teclas(self, texto, seg=0.8):
        self.p.write(texto.encode())
        return self.drain(seg)

    def close(self):
        self.p.close()


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    s = Serie(sys.argv[1])
    print(s.cmd("", 1.0).decode(errors="replace"))

    if len(sys.argv) > 2 and sys.argv[2] == "arranque":
        print(s.cmd("load EDITOR.BIN 0800", 8.0).decode(errors="replace"))
        print(s.cmd("r 0800", 3.0).decode(errors="replace"))
        name = input("nombre de fichero (ENTER para salir): ").strip()
        if name:
            print(s.teclas(name + "\r", 3.0).decode(errors="replace"))
        print(s.teclas(":q\r", 2.0).decode(errors="replace"))
    else:
        print("interactivo: escribe comandos (Ctrl+C para salir)")
        while True:
            linea = input()
            print(s.cmd(linea, 1.5).decode(errors="replace"), end="")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
