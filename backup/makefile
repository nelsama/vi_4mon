# ============================================================================
# makefile - Editor VI para Tang Nano 9K / 6502
# ============================================================================
CC65_HOME ?= D:/cc65
CC = $(CC65_HOME)/bin/cl65
CA65 = $(CC65_HOME)/bin/ca65
LD65 = $(CC65_HOME)/bin/ld65

CFLAGS = -t none -O --cpu 6502 -I include
ASFLAGS = -t none --cpu 6502

all: output/editor.bin

build:
	@mkdir -p build
output:
	@mkdir -p output

# ============================================================================
# Objetos del editor
# ============================================================================
EDITOR_OBJS = build/startup.o build/editor_main.o build/editor_buffer.o \
              build/editor_io.o build/editor_vi.o

output/editor.bin: $(EDITOR_OBJS) | build output
	$(LD65) -C config/editor.cfg -m output/editor.map -o $@ $^ $(CC65_HOME)/lib/none.lib
	@wc -c < $@ | awk '{printf "Binary: %d bytes = 0x%04X\nLOAD EDITOR.BIN 0800 %04X\n", $$1, $$1, $$1}'

build/startup.o: src/startup.s | build
	$(CA65) $(ASFLAGS) -o $@ $<

build/editor_main.o: src/editor_main.c include/editor.h include/romapi.h | build
	$(CC) -c $(CFLAGS) -o $@ $<

build/editor_buffer.o: src/editor_buffer.c include/editor.h include/romapi.h | build
	$(CC) -c $(CFLAGS) -o $@ $<

build/editor_io.o: src/editor_io.c include/editor.h include/romapi.h | build
	$(CC) -c $(CFLAGS) -o $@ $<

build/editor_vi.o: src/editor_vi.c include/editor.h include/romapi.h | build
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	-rm -rf build output
	@echo Clean.

.PHONY: all build output clean
