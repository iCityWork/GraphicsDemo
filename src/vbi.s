COLPF1_HW = $D017
COLPF2_HW = $D018
XITVBV    = $E462
ATRACT    = $004D

.import _dli_index
.import _vbi_offset

.segment "CODE"
.export _vbi_handler

.proc _vbi_handler
    lda #0
    sta _dli_index
    sta ATRACT

    ; Override color hardware directly, after OS shadow copy
    lda #$94
    sta COLPF2_HW       ; blue text background
    lda #$0F
    sta COLPF1_HW       ; max luminance → bright white text on blue

    lda _vbi_offset
    clc
    adc #1
    and #127
    sta _vbi_offset

    jmp XITVBV
.endproc