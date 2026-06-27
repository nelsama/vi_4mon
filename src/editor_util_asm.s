; ==========================================================================
; editor_util_asm.s
; ==========================================================================
.export _mode_name, _str_to_upper, _mfs_init
.importzp ptr1, ptr2

ROM_SD_INIT   = $BF00
ROM_MFS_MOUNT = $BF03
STATE_ADDR    = $0660

.segment "RODATA"
str_normal:  .byte "NORMAL",0
str_insert:  .byte "INSERT",0
str_command: .byte "COMMAND",0
str_search:  .byte "SEARCH",0
str_unknown: .byte "?",0

.segment "CODE"
_mode_name:
    lda STATE_ADDR+24
    beq @n
    cmp #1; beq @i
    cmp #2; beq @c
    cmp #3; beq @s
    lda #<str_unknown; ldx #>str_unknown; rts
@n: lda #<str_normal; ldx #>str_normal; rts
@i: lda #<str_insert; ldx #>str_insert; rts
@c: lda #<str_command; ldx #>str_command; rts
@s: lda #<str_search; ldx #>str_search; rts

_str_to_upper:
    sta ptr1; stx ptr1+1; ldy #0
@l: lda (ptr1),y; beq @d
    cmp #$61; bcc @x; cmp #$7B; bcs @x
    eor #$20; sta (ptr1),y
@x: iny; bne @l; inc ptr1+1; bne @l
@d: rts

_mfs_init:
    jsr ROM_SD_INIT; jmp ROM_MFS_MOUNT
