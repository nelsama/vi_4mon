; ============================================
; startup.s - Codigo de inicio para EDITOR VI
; ============================================
; Inicializa el runtime CC65
; ZP: sp en $0002 (como en el monitor)
; ============================================

.export _init
.export __STARTUP__ : absolute = 1

.import _main
.import _mfs_init
.import __BSS_RUN__, __BSS_SIZE__
.importzp sp

; ZP temporal para zerobss (no interfiere con sp en $0002)
PTR1    = $FA
PTR2    = $FC
CNT     = $FE

.segment "STARTUP"

_init:
    sei
    cld

    ; Inicializar stack pointer del 6502
    ldx #$FF
    txs

    ; Inicializar stack pointer de CC65 (software stack)
    lda #<$3DFF
    sta sp
    lda #>$3DFF
    sta sp+1

    ; Inicializar BSS a ceros
    jsr zerobss

    ; Inicializar las variables absolutas del editor ($0660-$06D7)
    jsr zerodata

    ; Inicializar SD y montar MicroFS
    jsr _mfs_init

    ; Llamar a main del editor
    jsr _main

    ; Volver al monitor
    jmp $8000

; ============================================
; zerobss - Inicializa BSS a ceros
; Usa ZP $FA-$FF para no interferir con sp
; ============================================
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

; ============================================================================
; zerodata - Pone a cero las variables absolutas del editor ($0660-$06D7)
; ============================================================================
; El BSS de C no las cubre: el estado del editor y los contadores de tamano
; son macros a direcciones fijas. Sin esto, STATE_SECTOR_VALID arranca en
; $FFFF (la RAM viene con FF tras la carga) y todas las ediciones usan
; indices absurdos.
; ============================================================================
M_STATE_LO = $60
M_STATE_HI = $06
M_STATE_LEN = $78          ; $0660-$06D7

zerodata:
    ldx #0
    lda #0
@clr:
    sta M_STATE_LO,x
    inx
    cpx #M_STATE_LEN
    bne @clr
    rts
