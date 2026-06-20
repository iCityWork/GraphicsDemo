;----------------------------------------------------------------------
; vbi.s — Vertical Blank Interrupt handler
;
; Fires 60 times per second (NTSC), via VVBLKD (deferred VBI).
; The OS stage 1 VBI runs first (copies shadow regs to hardware),
; then calls us via JMP (VVBLKD).
;
; We run AFTER the OS copies shadows to hardware — so any hardware
; writes we make here override the OS's shadow values for this frame.
;
; EXIT: Must end with JMP $E462 (XITVBV). Using plain RTI leaves
;       NMI disabled and freezes the demo permanently.
;----------------------------------------------------------------------

COLPF2  = $D018         ; Hardware: playfield 2 color (text bg in GR.0)
COLBK   = $D01A         ; Hardware: background/border color
XITVBV  = $E462         ; OS VBI exit — always jump here

.import _dli_index
.import _vbi_offset
.import _color_table

.segment "CODE"

.export _vbi_handler

.proc _vbi_handler

    ;------------------------------------------------------------------
    ; Reset DLI bar counter for this new frame.
    ; Without this, dli_index keeps counting up into garbage table entries.
    ;------------------------------------------------------------------
    lda #0
    sta _dli_index

    ;------------------------------------------------------------------
    ; Advance the animation phase (makes the rainbow scroll each frame).
    ; +1 per frame at 60 fps = full 128-color cycle every ~2 seconds.
    ; AND #127 wraps within the 128-entry table with no division needed.
    ;------------------------------------------------------------------
    lda _vbi_offset
    clc
    adc #1
    and #127
    sta _vbi_offset

    ;------------------------------------------------------------------
    ; Set the initial background color for bar 1.
    ;
    ; The DLI fires at the END of each bar, setting up the NEXT bar's
    ; color. That means bar 1's color must be set here, before the
    ; display starts — nobody else will set it.
    ;
    ; We write directly to hardware registers because this VBI runs
    ; AFTER the OS has already copied shadows to hardware. Our writes
    ; take immediate effect for bar 1 before any scanlines are drawn.
    ;
    ; We set both COLBK (border) and COLPF2 (playfield background) to
    ; the same color so the entire screen width shows one solid color —
    ; no different-colored border strip at the edges.
    ;------------------------------------------------------------------
    tax                     ; X = vbi_offset = index for bar 1's color
    lda _color_table,x      ; look up bar 1's color
    sta COLBK               ; set hardware background
    sta COLPF2              ; set hardware playfield background

    ;------------------------------------------------------------------
    ; Exit — mandatory, do not change
    ;------------------------------------------------------------------
    jmp XITVBV

.endproc