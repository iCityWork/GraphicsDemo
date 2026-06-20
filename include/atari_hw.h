#ifndef ATARI_HW_H
#define ATARI_HW_H

/*
 * atari_hw.h — Atari 8-bit hardware register map
 *
 * All #defines are wrapped in #ifndef so this header coexists safely
 * with CC65's own atari.h (included transitively via <conio.h>).
 * CC65 defines some of these without 'volatile'; we prefer volatile,
 * but we defer to whatever is already defined rather than conflict.
 *
 * Hardware registers are memory-mapped I/O. The 'volatile' keyword
 * tells the compiler these can change at any time (DMA, ANTIC chip,
 * interrupt handlers) and must never be cached in a CPU register.
 */

/* ── ANTIC Registers ($D400-$D40F) ──────────────────────────────────────
 *
 * ANTIC is Atari's display coprocessor. It runs independently of the
 * 6502 and generates the video signal from the display list program.
 */
#ifndef DMACTL
#define DMACTL  (*(volatile unsigned char*)0xD400) /* DMA control           */
#endif
#ifndef CHACTL
#define CHACTL  (*(volatile unsigned char*)0xD401) /* Character control      */
#endif
#ifndef DLISTL
#define DLISTL  (*(volatile unsigned char*)0xD402) /* Display list ptr lo    */
#endif
#ifndef DLISTH
#define DLISTH  (*(volatile unsigned char*)0xD403) /* Display list ptr hi    */
#endif
#ifndef HSCROL
#define HSCROL  (*(volatile unsigned char*)0xD404) /* Horizontal fine scroll */
#endif
#ifndef VSCROL
#define VSCROL  (*(volatile unsigned char*)0xD405) /* Vertical fine scroll   */
#endif
#ifndef PMBASE
#define PMBASE  (*(volatile unsigned char*)0xD406) /* Player/Missile base    */
#endif
#ifndef CHBASE
#define CHBASE  (*(volatile unsigned char*)0xD408) /* Character set base     */
#endif

/*
 * WSYNC — Write any value here to stall the 6502 until the start of
 * the next scanline's horizontal blank. Used in DLI handlers to ensure
 * color register writes take effect at a clean scanline boundary.
 */
#ifndef WSYNC
#define WSYNC   (*(volatile unsigned char*)0xD409) /* Wait for sync (WRITE)  */
#endif
#ifndef VCOUNT
#define VCOUNT  (*(volatile unsigned char*)0xD40A) /* Scanline counter (READ)*/
#endif
#ifndef NMIEN
#define NMIEN   (*(volatile unsigned char*)0xD40E) /* NMI enable             */
#endif
#ifndef NMIRES
#define NMIRES  (*(volatile unsigned char*)0xD40F) /* NMI reset (READ)       */
#endif

/* ── GTIA Color Registers ($D012-$D01A) ─────────────────────────────────
 *
 * GTIA takes ANTIC's display data and produces the final color output.
 * These are the registers we write in our DLI handler to change colors.
 *
 * Atari color byte format:
 *   Bits 7-4 = Hue  (0x0-0xF, 16 hues around the color wheel)
 *   Bits 3-1 = Luminance (0=black, 7=brightest)
 *   Bit    0 = Always 0
 *
 * IMPORTANT: In a DLI handler, always write to these HARDWARE registers
 * directly — NOT the OS shadow registers ($02C4-$02C8). The OS copies
 * shadows to hardware only once per frame during VBI. That is far too
 * slow for per-scanline color effects.
 */
#ifndef COLPM0
#define COLPM0  (*(volatile unsigned char*)0xD012) /* Player 0 color         */
#endif
#ifndef COLPM1
#define COLPM1  (*(volatile unsigned char*)0xD013) /* Player 1 color         */
#endif
#ifndef COLPM2
#define COLPM2  (*(volatile unsigned char*)0xD014) /* Player 2 color         */
#endif
#ifndef COLPM3
#define COLPM3  (*(volatile unsigned char*)0xD015) /* Player 3 color         */
#endif
#ifndef COLPF0
#define COLPF0  (*(volatile unsigned char*)0xD016) /* Playfield 0 color      */
#endif
#ifndef COLPF1
#define COLPF1  (*(volatile unsigned char*)0xD017) /* Playfield 1 color      */
#endif
#ifndef COLPF2
#define COLPF2  (*(volatile unsigned char*)0xD018) /* Playfield 2 / GR.0 bg  */
#endif
#ifndef COLPF3
#define COLPF3  (*(volatile unsigned char*)0xD019) /* Playfield 3 color      */
#endif
#ifndef COLBK
#define COLBK   (*(volatile unsigned char*)0xD01A) /* Background color       */
#endif

/* ── NMIEN values ───────────────────────────────────────────────────────
 *
 * NMIEN controls which events generate a Non-Maskable Interrupt.
 * At power-on the Atari OS sets NMIEN = $40 (VBI only).
 * We set it to $C0 to enable both VBI and our DLI.
 */
#ifndef NMIEN_VBI
#define NMIEN_VBI   0x40   /* Enable Vertical Blank Interrupt only  */
#endif
#ifndef NMIEN_DLI
#define NMIEN_DLI   0x80   /* Enable Display List Interrupt only    */
#endif
#ifndef NMIEN_BOTH
#define NMIEN_BOTH  0xC0   /* Enable both VBI and DLI               */
#endif
#ifndef NMIEN_NONE
#define NMIEN_NONE  0x00   /* Disable all NMI (use briefly only!)   */
#endif

/* ── OS Interrupt Vectors (Atari OS page 2) ─────────────────────────────
 *
 * These 16-bit pointers tell the Atari where your interrupt handlers
 * live. Always disable NMI (NMIEN = 0) while writing a vector to prevent
 * the interrupt firing mid-write with a half-updated address.
 */
#ifndef VDSLST
#define VDSLST  (*(unsigned int*)0x0200)  /* DLI vector (2 bytes)          */
#endif
#ifndef VVBLKD
#define VVBLKD  (*(unsigned int*)0x0224)  /* Deferred VBI vector (2 bytes) */
#endif

/* ── OS Routines ────────────────────────────────────────────────────────
 *
 * XITVBV: The OS exit point for VBI handlers installed via VVBLKD.
 * Your VBI handler MUST end with JMP $E462. It re-enables NMI and
 * performs the RTI that was pending from the original VBI entry.
 * Never exit a VBI with plain RTI — it leaves NMI permanently disabled.
 */
#ifndef XITVBV
#define XITVBV  0xE462  /* VBI exit address — all VBI handlers end here */
#endif

/*
 * OS colour shadow registers — written by your code, copied to hardware
 * by the OS VBI stage-2 once per frame.  These are the PLAYFIELD shadows,
 * not the player/missile shadows at $02C4-$02C7.
 */
#ifndef COLPF1_SH
#define COLPF1_SH  0x02C9   /* COLOR1 → COLPF1 ($D017) text char pixel  */
#endif
#ifndef COLPF2_SH
#define COLPF2_SH  0x02CA   /* COLOR2 → COLPF2 ($D018) text cell bg      */
#endif
#ifndef COLPF3_SH
#define COLPF3_SH  0x02CB   /* COLOR3 → COLPF3 ($D019) (unused here)     */
#endif
#ifndef COLBK_SH
#define COLBK_SH   0x02CC   /* COLOR4 → COLBK  ($D01A) border/background */
#endif

/* ── Display List Instruction Bytes ─────────────────────────────────────
 *
 * ANTIC display list byte format:
 *
 *   Bit 7 (0x80): DLI  — fire a Display List Interrupt after this line
 *   Bit 6 (0x40): LMS  — Load Memory Scan: next 2 bytes = screen data addr
 *   Bit 5 (0x20): VSCROL — enable vertical fine scroll on this line
 *   Bit 4 (0x10): HSCROL — enable horizontal fine scroll on this line
 *   Bits 3-0:     Mode  (0=blank/jump, 2=GR.0 text, F=GR.8 bitmap, etc.)
 *
 * CC65's atari.h defines DL_JVB and possibly others — wrap all of them.
 */
#ifndef DL_BLANK8
#define DL_BLANK8        0x70  /* 8 blank scan lines                        */
#endif
#ifndef DL_MODE2
#define DL_MODE2         0x02  /* ANTIC mode 2 (GR.0 text, 8 scan lines/row)*/
#endif
#ifndef DL_MODE2_DLI
#define DL_MODE2_DLI     0x82  /* Mode 2 + DLI flag                         */
#endif
#ifndef DL_MODE2_LMS
#define DL_MODE2_LMS     0x42  /* Mode 2 + LMS (followed by 2-byte address) */
#endif
#ifndef DL_MODE2_DLILMS
#define DL_MODE2_DLILMS  0xC2  /* Mode 2 + DLI + LMS                        */
#endif
#ifndef DL_JVB
#define DL_JVB           0x41  /* Jump + VBI: end of display list            */
#endif                         /* (followed by 2-byte address of list start) */
#ifndef DL_BLANK8_DLI
#define DL_BLANK8_DLI    0xF0  /* 8 blank scan lines + DLI flag ($70|$80)    */
#endif
#endif /* ATARI_HW_H */