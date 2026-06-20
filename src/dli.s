;----------------------------------------------------------------------
; dli.s — Display List Interrupt handler
;
; Fires once per color bar (20 times per frame).
; Changes the background color (COLBK) for the next bar.
;
; TIMING BUDGET: ~114 machine cycles per scan line (NTSC).
; This handler uses approximately 40 cycles — well within budget.
;
; REGISTER PRESERVATION: A DLI is an NMI (Non-Maskable Interrupt).
; The CPU pushes PC and SR automatically. We must save and restore
; any other registers we use. Failing to do so corrupts the main
; program's state and causes crashes.
;
; WHY ASSEMBLY: CC65's C output for even a simple function is 60+
; cycles with unpredictable timing. Assembly lets us count every
; cycle and guarantee we finish before the next scanline begins.
;
; WSYNC: Writing any value to $D409 (WSYNC) stalls the 6502 until
; the start of the next scanline's horizontal blank. We do this
; BEFORE writing COLBK so the color change takes effect cleanly
; at the scanline boundary — no color tearing mid-line.
;----------------------------------------------------------------------

; Hardware register addresses
WSYNC  = $D409          ; Write: stall CPU until next scanline
COLBK  = $D01A          ; Write: background color (hardware register)
COLPF2 = $D018          ; Write: playfield 2 color (text background)

; Import shared variables from colors.c
; CC65 prepends underscore to all C global names
.import _dli_index      ; which bar we're on this frame (unsigned char)
.import _vbi_offset     ; animation phase from VBI     (unsigned char)
.import _color_table    ; 128-entry color table        (unsigned char[])

.segment "CODE"

;----------------------------------------------------------------------
; _dli_handler — exported so C code can take its address
;----------------------------------------------------------------------
.export _dli_handler

.proc _dli_handler

    ; Save registers — the CPU saves PC and P (flags) automatically
    ; on NMI, but we must save A, X, Y ourselves.
    pha                     ; save A  (3 cycles)
    txa                     ; copy X to A
    pha                     ; save X  (3 cycles)

    ;------------------------------------------------------------------
    ; Compute color table index:
    ;   index = (vbi_offset + dli_index) & COLOR_TABLE_MASK (127)
    ;
    ; Adding vbi_offset to dli_index is what animates the effect.
    ; Each frame, vbi_offset increases by 1, so the entire rainbow
    ; shifts up by one entry — the bars appear to scroll upward.
    ;------------------------------------------------------------------
    lda _vbi_offset         ; (4) load animation phase
    clc
    adc _dli_index          ; (4) add current bar number
    and #127                ; (2) wrap to 0-127 (fast — power of 2 mask)
    tax                     ; (2) use as index into color_table

    ;------------------------------------------------------------------
    ; Look up the color for this bar
    ;------------------------------------------------------------------
    lda _color_table,x      ; (5) fetch color byte

    ;------------------------------------------------------------------
    ; Wait for the scanline boundary, then set the new color.
    ;
    ; STA WSYNC halts the CPU until the start of horizontal blank.
    ; The SAME accumulator value (our color) is written to both:
    ;   WSYNC  — triggers the wait (value written is ignored)
    ;   COLBK  — sets the new background color at the clean boundary
    ;
    ; Using the same value for both avoids needing to re-load A.
    ;------------------------------------------------------------------
    sta WSYNC               ; (4) wait for scanline — A still = color
    sta COLBK               ; (4) set background color for next bar
    sta COLPF2              ; (4) match text background (for title/footer)

    ;------------------------------------------------------------------
    ; Advance dli_index for the next DLI firing this frame
    ;------------------------------------------------------------------
    inc _dli_index          ; (6) next bar's color will be +1 in table

    ; Restore registers
    pla                     ; restore X  (4)
    tax
    pla                     ; restore A  (4)

    rti                     ; return from NMI interrupt  (6)
                            ; (CPU automatically restores PC and flags)

.endproc
; Total cycle count: ~55 cycles — 59 cycles to spare per scanline.