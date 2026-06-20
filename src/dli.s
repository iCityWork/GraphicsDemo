;----------------------------------------------------------------------
; dli.s  —  DLI handler for Color Storm
;
; KEY DESIGN DECISION:
;   Bar DLIs write ONLY to COLBK_HW ($D01A).
;   COLPF1_HW ($D017) and COLPF2_HW ($D018) are NEVER touched.
;   Because those registers control Mode-2 text colour, the footer
;   always renders with the correct OS shadow values (white on blue)
;   without any cleanup needed.
;
; DLI sequence per frame (13 total):
;   dli_index  0     pre-bar  → WSYNC → COLBK = bar 1 colour
;   dli_index  1-11  bar N    → WSYNC → COLBK = bar N+1 colour
;   dli_index  12    bar 12   → WSYNC → COLBK = $02 (near-black)
;                              restores border for separator + footer
;----------------------------------------------------------------------

WSYNC       = $D409
COLBK_HW    = $D01A     ; ONLY register the bar DLIs touch

NUM_BARS    = 12        ; must match display.h

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
    cmp #NUM_BARS           ; bar 12 cleanup?
    bne do_bar

    ;------------------------------------------------------------------
    ; CLEANUP  (dli_index == 12, bar 12's DLI)
    ; WSYNC stalls to separator scanline 1.
    ; Restore COLBK to near-black for the separator and footer border.
    ; COLPF1 and COLPF2 need no restoration — they were never changed.
    ;------------------------------------------------------------------
    lda #$02                ; near-black border
    sta WSYNC               ; stall → separator scanline 1
    sta COLBK_HW

    inc _dli_index          ; 12 → 13  (VBI resets to 0 next frame)
    pla
    tax
    pla
    rti

    ;------------------------------------------------------------------
    ; BAR DLI  (dli_index 0-11)
    ; Sets the NEXT section's COLBK to a rainbow colour.
    ; COLPF1 and COLPF2 are deliberately left untouched.
    ;------------------------------------------------------------------
do_bar:
    lda _vbi_offset         ; cycles 0-127 each VBI → animation
    clc
    adc _dli_index          ; unique hue per bar
    and #127
    tax
    lda _color_table,x      ; rainbow colour byte

    sta WSYNC               ; stall → next section scanline 1
    sta COLBK_HW            ; set bar colour (border + blank scanline)

    inc _dli_index

    pla
    tax
    pla
    rti

.endproc