# ============================================================================
# makefile - Mini VI para Monitor 6502 (Tang Nano 9K)
# ============================================================================
# Todo el editor vive en base/ (fuentes, cabeceras, mapa de memoria y binario).
#
#   make         compila            -> base/editor_base.bin
#   make sd      compila e instala  -> sdcard/EDITOR.BIN del emulador
#   make test    arranque + bateria de casos de uso
#   make diag    diagnosticos (barra, help, enter, terminal)
#   make clean   borra binario y mapa
#
# IMPORTANTE: usa siempre `make sd` antes de probar en el emulador. Si solo
# haces `make`, la tarjeta sigue con el EDITOR.BIN anterior y se pierden horas
# depurando un binario que no es el que has compilado.
# ============================================================================

CC65_HOME ?= D:/cc65
CL65      = $(CC65_HOME)/bin/cl65
NONE_LIB  = $(CC65_HOME)/lib/none.lib

EMU_DIR   = D:/Proyectos/EmuladorFpga6502
SDTOOL    = python $(EMU_DIR)/tools/sd_tool.py
TARJETA   = $(EMU_DIR)/tarjeta.img
SDCARD    = $(EMU_DIR)/sdcard/EDITOR.BIN

BASE = base
BIN  = $(BASE)/editor_base.bin
MAP  = $(BASE)/editor_base.map

SRC = src/editor_main.c src/editor_buffer.c src/editor_io.c src/editor_vi.c \
      src/startup.s src/editor_util_asm.s

CFLAGS = -t none -O --cpu 6502 -I include

all: $(BIN)

$(BIN): $(addprefix $(BASE)/,$(SRC)) $(BASE)/include/editor.h \
        $(BASE)/include/romapi.h $(BASE)/config/editor.cfg
	cd $(BASE) && $(CL65) $(CFLAGS) -o editor_base.bin -m editor_base.map \
	  -C config/editor.cfg $(SRC) $(NONE_LIB)
	@echo "compilado: $(BIN) ($$(wc -c < $(BIN)) bytes)"

# Instala el binario donde el emulador lo espera:
#   - sdcard/EDITOR.BIN  (fuente de la tarjeta: run.bat reconstruye desde aqui)
#   - tarjeta.img        (la imagen que se monta ahora mismo)
sd: $(BIN)
	@mkdir -p $(dir $(SDCARD))
	cp $(BIN) $(SDCARD)
	@echo "instalado: $(SDCARD)"
	$(SDTOOL) inject $(TARJETA) $(BIN) EDITOR.BIN

test: $(BIN)
	python tests/base_test.py --no-build
	python tests/base_casos.py

diag: $(BIN)
	python tests/diag/diag_barra.py
	python tests/diag/diag_help.py
	python tests/diag/diag_terminal.py

clean:
	rm -f $(BIN) $(MAP)

.PHONY: all sd test diag clean
