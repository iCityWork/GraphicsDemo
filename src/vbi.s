;----------------------------------------------------------------------
; vbi.s — Vertical Blank Interrupt handler (deferred, via VVBLKD $0224)
;
; Runs once per frame during vertical blank, before the display starts.
; Responsibilities:
;   1. Reset dli_index so bar 0 fires correctly on the next frame.
;   2. Advance vbi_offset to animate the colour palette each frame.
;   3. DEFEAT ATTRACT MODE — reset ATRACT ($004D) to 0 every frame.
;      The OS increments ATRACT each VBI. When it exceeds ~$80 (~2 sec),
;      stage-2 VBI starts colour-cycling the SHADOW registers to dim them.
;      The cleanup DLI reads those shadows for the footer — if COLPF1_SH
;      gets cycled to match COLPF2_SH the text disappears. Zeroing ATRACT
;      every frame prevents the counter from ever reaching the threshold.
;   4. Exit via XITVBV ($E462) — NOT RTI. XITVBV runs OS stage-2 which
;      copies shadows to hardware and then starts the active display.
;----------------------------------------------------------------------

XITVBV  = $E462
ATRACT  = $004D     ; OS attract-mode counter — zero each frame to disable

.import _dli_index
.import _vbi_offset

.segment "CODE"
.export _vbi_handler

.proc _vbi_handler

    ; Reset the DLI bar counter for the new frame.
    lda #0
    sta _dli_index

    ; Defeat attract mode — keeps counter at zero, threshold never reached.
    sta ATRACT          ; ATRACT = 0  (A already holds 0)

    ; Advance the palette animation phase (0-127, wraps).
    lda _vbi_offset
    clc
    adc #1
    and #127
    sta _vbi_offset

    ; Hand control to OS stage-2 VBI (copies shadows → hardware).
    jmp XITVBV

.endproc