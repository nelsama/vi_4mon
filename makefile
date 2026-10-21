# ============================================================================
# makefile - Mini VI para Monitor 6502 (Tang Nano 9K)
# ============================================================================
#   make         compila            -> output/editor.bin
#   make sd      compila e instala  -> sdcard/EDITOR.BIN del emulador
#   make test    arranque + bateria de casos de uso
#   make diag    diagnosticos (barra, help, terminal)
#   make clean   borra output/
#
# IMPORTANTE: usa `make sd` antes de probar en el emulador. Con solo `make`,
# la tarjeta se queda con el EDITOR.BIN anterior.
# ============================================================================

CC65_HOME ?= D:/cc65
CL65      = $(CC65_HOME)/bin/cl65
NONE_LIB  = $(CC65_HOME)/lib/none.lib

EMU_DIR   = D:/Proyectos/EmuladorFpga6502
SDTOOL    = python $(EMU_DIR)/tools/sd_tool.py
TARJETA   = $(EMU_DIR)/tarjeta.img
SDCARD    = $(EMU_DIR)/sdcard/EDITOR.BIN

OUT = output
BIN = $(OUT)/editor.bin
MAP = $(OUT)/editor.map

SRC = src/editor_main.c src/editor_buffer.c src/editor_io.c src/editor_vi.c \
      src/startup.s src/editor_util_asm.s

CFLAGS = -t none -O --cpu 6502 -I include

all: $(BIN)

$(OUT):
	mkdir -p $(OUT)

$(BIN): $(SRC) include/editor.h include/romapi.h config/editor.cfg | $(OUT)
	$(CL65) $(CFLAGS) -o $(BIN) -m $(MAP) -C config/editor.cfg $(SRC) $(NONE_LIB)
	@echo "compilado: $(BIN) ($$(wc -c < $(BIN)) bytes)"

# Instala el binario donde el emulador lo espera:
#   - sdcard/EDITOR.BIN  fuente de la tarjeta (run.bat reconstruye desde aqui)
#   - tarjeta.img        la imagen que se monta ahora mismo
sd: $(BIN)
	@mkdir -p $(dir $(SDCARD))
	cp $(BIN) $(SDCARD)
	@echo "instalado: $(SDCARD)"
	$(SDTOOL) inject $(TARJETA) $(BIN) EDITOR.BIN

test: $(BIN)
	python tests/test_arranque.py --no-build
	python tests/test_casos.py

diag: $(BIN)
	python tests/diag/diag_barra.py
	python tests/diag/diag_help.py
	python tests/diag/diag_terminal.py

clean:
	rm -rf $(OUT)

.PHONY: all sd test diag clean
