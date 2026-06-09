; ============================================================================
; hello_led.asm - Prueba mínima: enciende un LED
; ============================================================================
.org $0800

LEDS = $C001

start:
    lda #$00        ; 0 = encendido (lógica negativa)
    sta LEDS
loop:
    jmp loop        ; infinito
