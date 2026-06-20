;----------------------------------------------------------------------
; dli.s — Display List Interrupt handler
;
; Fires 13× per frame:
;   dli_index 0        : pre-bar — sets bar 1's color
;   dli_index 1-11     : bar N DLI — sets bar N+1's color
;   dli_index 12 (=NUM_BARS) : cleanup — restores OS shadow colors
;
; ── Shadow vs Hardware rule ──────────────────────────────────────────
; ONLY here (DLI) do we write hardware color registers directly.
; Shadows ($02Cxx) are updated once per frame at VBI — useless for
; per-scanline effects. DLI must hit hardware registers to take
; effect on the very next scanline.
;
; ── Why we also set COLPF1 ──────────────────────────────────────────
; In Mode 2 (text), character foreground pixels use COLPF1. By setting
; hardware COLPF1 = COLPF2 = bar color, any character pixels (from
; screen memory that isn't perfectly zeroed) become invisible against
; the background — the bars look solid regardless of screen content.
;----------------------------------------------------------------------

WSYNC       = $D409
COLPF1_HW   = $D017         ; Hardware: text pixel color (char foreground)
COLPF2_HW   = $D018         ; Hardware: text cell background
COLBK_HW    = $D01A         ; Hardware: border/background

; OS shadow registers — read in cleanup to restore title/footer colors
COLPF1_SH   = $02C5
COLPF2_SH   = $02C6
COLBK_SH    = $02C8

; MUST match NUM_BARS in display.h
NUM_BARS    = 12

.import _dli_index
.import _vbi_offset
.import _color_table

.segment "CODE"
.export _dli_handler

.proc _dli_handler

    pha                     ; save A
    txa
    pha                     ; save X (via A, costs 2 cycles vs TXA+PHA)

    ;------------------------------------------------------------------
    ; Is this the cleanup DLI (dli_index == NUM_BARS)?
    ;------------------------------------------------------------------
    lda _dli_index
    cmp #NUM_BARS
    bne do_bar

    ;------------------------------------------------------------------
    ; CLEANUP DLI
    ; Fires at the end of bar 12. After WSYNC we are at the first
    ; scanline of the post-bar separator. Restore all three hardware
    ; color registers from OS shadows so the footer is readable.
    ;
    ; We restore COLPF1 too — otherwise text pixel color stays as the
    ; last bar's color and footer characters appear wrong-colored.
    ;------------------------------------------------------------------
    lda COLPF2_SH           ; fetch OS shadow COLPF2
    sta WSYNC               ; stall to next scanline boundary
    sta COLPF2_HW           ; restore hardware COLPF2 for footer

    lda COLPF1_SH           ; fetch OS shadow COLPF1
    sta COLPF1_HW           ; restore hardware COLPF1 (white title/footer text)

    lda COLBK_SH            ; fetch OS shadow COLBK
    sta COLBK_HW            ; restore hardware COLBK

    inc _dli_index
    pla
    tax
    pla
    rti

    ;------------------------------------------------------------------
    ; BAR DLI
    ; Set COLPF1, COLPF2, and COLBK all to the same rainbow color.
    ;
    ; Setting COLPF1 = COLPF2 makes character foreground pixels the same
    ; color as the background — they become invisible. The bars look
    ; perfectly solid even if screen data isn't pristine.
    ;------------------------------------------------------------------
do_bar:
    lda _vbi_offset         ; animation phase (incremented each VBI)
    clc
    adc _dli_index          ; + which bar this is
    and #127                ; wrap to 0-127
    tax
    lda _color_table,x      ; look up rainbow color

    sta WSYNC               ; stall to next scanline boundary — same A used below
    sta COLPF2_HW           ; text background  = bar color
    sta COLBK_HW            ; border           = bar color
    sta COLPF1_HW           ; text foreground  = bar color (chars invisible)

    inc _dli_index

    pla
    tax
    pla
    rti

.endproc