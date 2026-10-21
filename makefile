# ============================================================================
# makefile - Mini VI para Monitor 6502 (Tang Nano 9K)
# ============================================================================
# El editor se construye desde base/ (la version que arranca).
# Uso:
#   make          -> base/editor_base.bin
#   make sd       -> inyecta el .bin en la tarjeta del emulador
#   make test     -> arranque + bateria de casos de uso
#   make clean
# ============================================================================

CC65_HOME ?= D:/cc65
CL65  = $(CC65_HOME)/bin/cl65
SDTOOL = python D:/Proyectos/EmuladorFpga6502/tools/sd_tool.py
TARJETA = D:/Proyectos/EmuladorFpga6502/tarjeta.img

BASE = base
BIN  = $(BASE)/editor_base.bin
MAP  = $(BASE)/editor_base.map

SRC = src/editor_main.c src/editor_buffer.c src/editor_io.c src/editor_vi.c \
      src/startup.s src/editor_util_asm.s

CFLAGS = -t none -O --cpu 6502 -I include

all: $(BIN)

$(BIN): $(addprefix $(BASE)/,$(SRC)) $(BASE)/include/editor.h $(BASE)/config/editor.cfg
	cd $(BASE) && $(CL65) $(CFLAGS) -o editor_base.bin -m editor_base.map \
	  -C config/editor.cfg $(SRC) $(CC65_HOME)/lib/none.lib
	@ls -l $(BIN) | sed 's/.* /binario: /'

# Copia el binario a la tarjeta que usa el emulador. Hacerlo siempre tras
# compilar: si no, se prueba un EDITOR.BIN viejo y los sintomas no cuadran.
sd: $(BIN)
	$(SDTOOL) inject $(TARJETA) $(BIN) EDITOR.BIN

test: $(BIN)
	python tests/base_test.py --no-build
	python tests/base_casos.py

clean:
	rm -f $(BIN) $(MAP)

.PHONY: all sd test clean
