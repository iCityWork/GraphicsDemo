WSYNC    = $D40A          ; HSYNC wait — $D40A (NOT $D409 which is CHBASE!)
COLBK_HW = $D01A
NUM_BARS = 12

.import _dli_index
.import _vbi_offset
.import _color_table

.segment "CODE"
.export _dli_handler

.proc _dli_handler
    pha
    txa
    pha

    lda _dli_index
    cmp #NUM_BARS       ; carry clear when index < NUM_BARS
    bcc do_bar          ; indices 0-11 → draw a bar

    ; index >= NUM_BARS: reset COLBK to black, no bar color
    lda #$00
    sta WSYNC           ; wait for HSYNC
    sta COLBK_HW        ; black outer background
    inc _dli_index
    pla
    tax
    pla
    rti

do_bar:
    lda _vbi_offset
    clc
    adc _dli_index
    and #127
    tax
    lda _color_table,x
    sta WSYNC           ; wait for HSYNC
    sta COLBK_HW        ; set bar color
    inc _dli_index
    pla
    tax
    pla
    rti
.endproc