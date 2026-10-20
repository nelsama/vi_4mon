; ============================================================================
; startup.s - Arranque del Mini VI
; ============================================================================
; El monitor carga el .bin en $0800 y hace JSR (mon_execute): las RTS de este
; programa vuelven al monitor, igual que en el juego de Arkanoid.
;
; Diferencias importantes con un programa enlazado en ROM ($8000):
;   - El codigo se ejecuta EN RAM de $0800 a $3DFF (16 KB).
;   - La pila de hardware vive en $0100-$01FF: NO se pone en la zona alta
;     porque el hardware solo decodifica RAM hasta $3FFF.
;   - $3FFE-$3FFF los usa el autoboot del monitor: no se tocan.
;
; OJO CON LA ZP: sin libreria (none.lib) cc65 NO reserva la pagina cero; la
; ZP que queda en $20-$8F la reparte el linker entre las variables simples del
; programa y la del monitor que sigue en uso ($E-$F = registro de E/S, $FA-$FF
; = uart). Aqui se usa la ZP de usuario ($50-$8F), que ya es del programa.
; ============================================================================

.export _init
.export __STARTUP__ : absolute = 1
.import _main
.import __BSS_RUN__, __BSS_SIZE__

; ZP de usuario para zerobss (no se solapa con $E/$F ni con $FA-$FF)
PTR1    = $80
CNT     = $82

.segment "STARTUP"

_init:
    sei
    cld

    ldx #$FF
    txs

    jsr zerobss

    jsr _main

    ; main no deberia retornar; si lo hace, volver al monitor
    rts

.export _p8, _mkargs

.segment "CODE"
.proc _p8
    lda #0
    asl a
    sta $4F
    rts
.endproc

.proc _mkargs
    lda #13
    asl a
    asl a
    sta $4E
    rts
.endproc

.segment "STARTUP"

; ----------------------------------------------------------------------------
; zerobss - BSS a cero
; ----------------------------------------------------------------------------
zerobss:
    lda #<__BSS_SIZE__
    ora #>__BSS_SIZE__
    beq @done

    lda #<__BSS_RUN__
    sta PTR1
    lda #>__BSS_RUN__
    sta PTR1+1

    lda #<__BSS_SIZE__
    sta CNT
    lda #>__BSS_SIZE__
    sta CNT+1

    ldy #0
    lda #0
@loop:
    sta (PTR1),y
    inc PTR1
    bne @skip
    inc PTR1+1
@skip:
    lda CNT
    bne @dec_low
    dec CNT+1
@dec_low:
    dec CNT
    lda CNT
    ora CNT+1
    bne @loop
@done:
    rts
