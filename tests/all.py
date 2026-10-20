"""all.py - Compila, arranca el emulador y ejecuta la suite completa."""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    subprocess.run([sys.executable, os.path.join(HERE, "harness.py"), "start"],
                   check=True)
    r = subprocess.run([sys.executable, "-u", os.path.join(HERE, "suite.py")] + sys.argv[1:],
                       cwd=HERE)
    subprocess.run([sys.executable, os.path.join(HERE, "harness.py"), "stop"],
                   capture_output=True)
    raise SystemExit(r.returncode)


if __name__ == "__main__":
    main()
