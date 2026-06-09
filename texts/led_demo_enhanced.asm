; ============================================================================
; led_demo_enhanced.asm - Demo de LEDs mejorado para Tang Nano 9K
; Adaptado para AS65 (ensamblador residente)
;
; Efectos incluidos:
;   - Knight Rider (luz que va y viene)
;   - Blink (parpadeo de todos los LEDs)
;   - Chaser (persecucion: LED unico que recorre de izq. a der.)
;   - Heartbeat (doble parpadeo rapido + pausa, como latido)
;   - Random (secuencia pseudo-aleatoria con LFSR de 8 bits)
;
; Compilar con AS65 en la Tang Nano:
;   LOAD AS65.BIN 0800 346B
;   R 0800
;   > A
;   Archivo fuente: LED_DEMO_ENHANCED.ASM
;   Nombre del archivo de salida: LED_DEMO_ENHANCED.BIN
;
; Cargar y ejecutar:
;   LOAD LED_DEMO_ENHANCED.BIN 0400
;   R 0400
;
; Puerto de LEDs: $C001 (logica negativa: 0=encendido, 1=apagado)
; ============================================================================

.org $0800

; ============================================================================
; CONSTANTES
; ============================================================================
LEDS    = $C001         ; Puerto de LEDs (0=on, 1=off)

; ============================================================================
; VARIABLES EN ZERO PAGE
; ============================================================================
led     = $20           ; Patron de LED actual
temp    = $21           ; Variable temporal de uso general
count   = $22           ; Contador de repeticiones / vueltas
random  = $23           ; Valor pseudo-aleatorio para LFSR

; ============================================================================
; PUNTO DE ENTRADA
; ============================================================================
start:
    jsr init

main_loop:
    jsr effect_knight_rider
    jsr effect_blink
    jsr effect_chaser
    jsr effect_heartbeat
    jsr effect_random
    jmp main_loop

; ============================================================================
; INICIALIZACION
; ============================================================================
init:
    lda #$FF            ; Todos los LEDs apagados
    sta LEDS
    lda #$01            ; Semilla para LFSR (distinto de cero)
    sta random
    rts

; ============================================================================
; EFECTO: KNIGHT RIDER (luz que va y viene estilo KITT)
; ============================================================================
effect_knight_rider:
    lda #$01
    sta led

@go_left:
    lda led
    eor #$FF            ; Invertir para logica negativa
    sta LEDS
    jsr delay_long
    asl led             ; Desplazar a la izquierda
    lda led
    cmp #$40            ; Llego a $40? (LED 6 de 8)
    bne @go_left

    lda #$20
    sta led

@go_right:
    lda led
    eor #$FF
    sta LEDS
    jsr delay_long
    lsr led             ; Desplazar a la derecha
    lda led
    bne @go_right       ; Hasta que led = 0

    rts

; ============================================================================
; EFECTO: BLINK (parpadeo de todos los LEDs al mismo tiempo)
; ============================================================================
effect_blink:
    ldx #$06            ; 6 parpadeos

@loop:
    lda #$00            ; Todos encendidos
    sta LEDS
    jsr delay_long
    lda #$FF            ; Todos apagados
    sta LEDS
    jsr delay_long
    dex
    bne @loop

    rts

; ============================================================================
; EFECTO: CHASER (LED unico que recorre de izquierda a derecha)
; ============================================================================
effect_chaser:
    ldx #$03            ; 3 recorridos completos

@outer:
    stx count
    lda #$01
    sta led

@next:
    lda led
    eor #$FF
    sta LEDS
    jsr delay_long
    asl led             ; $01 -> $02 -> $04 -> ... -> $80 -> 0 + carry
    bcc @next           ; Si no hubo carry, seguir desplazando

    ldx count
    dex
    bne @outer          ; Repetir recorrido

    rts

; ============================================================================
; EFECTO: HEARTBEAT (doble parpadeo rapido + pausa, como latido)
; ============================================================================
effect_heartbeat:
    ldx #$04            ; 4 latidos completos

@beat:
    stx count

    ; ---- Primer parpadeo ----
    lda #$00
    sta LEDS            ; Todos encendidos
    ldy #$18            ; Delay corto inline
@fast1:
    dey
    bne @fast1

    lda #$FF
    sta LEDS            ; Todos apagados
    ldy #$18
@fast2:
    dey
    bne @fast2

    ; ---- Segundo parpadeo (doble latido) ----
    lda #$00
    sta LEDS
    ldy #$18
@fast3:
    dey
    bne @fast3

    lda #$FF
    sta LEDS
    ldy #$18
@fast4:
    dey
    bne @fast4

    ; ---- Pausa prolongada entre latidos ----
    jsr delay_long
    jsr delay_long

    ldx count
    dex
    bne @beat

    rts

; ============================================================================
; EFECTO: RANDOM (secuencia pseudo-aleatoria con LFSR de 8 bits)
; Polinomio: x^8 + x^4 + x^3 + x^2 + 1  (maxima longitud: 255 estados)
; ============================================================================
effect_random:
    ldx #$40            ; 64 patrones aleatorios

@loop:
    lda random
    eor #$FF            ; Invertir para logica negativa
    sta LEDS
    jsr delay_long

    ; ---- LFSR: desplazar e insertar bit de realimentacion ----
    lda random
    asl                 ; Desplazar a izquierda, bit 7 -> carry
    bcc @no_xor         ; Si bit 7 era 0, no hay realimentacion
    eor #$1D            ; Realimentacion: x^4+x^3+x^2+1 = %00011101
@no_xor:
    sta random

    dex
    bne @loop

    rts

; ============================================================================
; DELAY LARGO (~0.5 segundos a 3.375 MHz)
; ============================================================================
delay_long:
    ldx #$30
@outer:
    ldy #$00
@inner:
    nop
    nop
    nop
    nop
    dey
    bne @inner
    dex
    bne @outer
    rts
