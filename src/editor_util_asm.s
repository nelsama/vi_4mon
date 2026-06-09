; ==========================================================================
; editor_util_asm.s - Funciones utilitarias en assembler
; ==========================================================================
; mode_name, str_to_upper, mfs_init
; ==========================================================================

.export _mode_name, _str_to_upper, _mfs_init
.importzp ptr1, ptr2

ROM_SD_INIT      = $BF00
ROM_MFS_MOUNT    = $BF03

STATE_ADDR       = $0660
MODE_OFFSET      = 24

.segment "RODATA"

str_normal:  .byte "NORMAL", 0
str_insert:  .byte "INSERT", 0
str_command: .byte "COMMAND", 0
str_search:  .byte "SEARCH", 0
str_unknown: .byte "?", 0

.segment "CODE"

; const char* mode_name(void)
_mode_name:
    lda     STATE_ADDR + MODE_OFFSET
    beq     @normal
    cmp     #1
    beq     @insert
    cmp     #2
    beq     @command
    cmp     #3
    beq     @search
    lda     #<str_unknown
    ldx     #>str_unknown
    rts
@normal:
    lda     #<str_normal
    ldx     #>str_normal
    rts
@insert:
    lda     #<str_insert
    ldx     #>str_insert
    rts
@command:
    lda     #<str_command
    ldx     #>str_command
    rts
@search:
    lda     #<str_search
    ldx     #>str_search
    rts

; void str_to_upper(char *s)  -- __fastcall__, s en AX
_str_to_upper:
    sta     ptr1
    stx     ptr1+1
    ldy     #0
@loop:
    lda     (ptr1),y
    beq     @done
    cmp     #$61
    bcc     @next
    cmp     #$7B
    bcs     @next
    eor     #$20        ; flip case: 'a'^$20 = 'A'
    sta     (ptr1),y
@next:
    iny
    bne     @loop
    inc     ptr1+1
    bne     @loop
@done:
    rts

; void mfs_init(void)
_mfs_init:
    jsr     ROM_SD_INIT
    jmp     ROM_MFS_MOUNT
