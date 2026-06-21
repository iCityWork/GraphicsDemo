#ifndef ATARI_HW_H
#define ATARI_HW_H

#include <atari.h>

/* ── ANTIC write-only registers ──────────────────────────────────────────*/
#ifndef DLISTL
#define DLISTL  (*(volatile unsigned char*)0xD402)
#endif
#ifndef DLISTH
#define DLISTH  (*(volatile unsigned char*)0xD403)
#endif
#ifndef NMIEN
#define NMIEN   (*(volatile unsigned char*)0xD40E)
#endif

/* ── Colour shadow registers ──────────────────────────────────────────────
   OS VBI stage-2 copies these to hardware every frame.
   ALWAYS use shadows in setup code; only DLI handlers write hw directly.

     $02C8 = COLOR0 → COLPF0  ($D016)
     $02C9 = COLOR1 → COLPF1  ($D017)  text pixel foreground (Mode-2 ink)
     $02CA = COLOR2 → COLPF2  ($D018)  text cell background
     $02CB = COLOR3 → COLPF3  ($D019)
     $02CC = COLOR4 → COLBK   ($D01A)  border / background colour          */
#ifndef COLPF0_SH
#define COLPF0_SH   0x02C8
#endif
#ifndef COLPF1_SH
#define COLPF1_SH   0x02C9
#endif
#ifndef COLPF2_SH
#define COLPF2_SH   0x02CA
#endif
#ifndef COLPF3_SH
#define COLPF3_SH   0x02CB
#endif
#ifndef COLBK_SH
#define COLBK_SH    0x02CC
#endif

/* ── Display-list shadow ─────────────────────────────────────────────────
   Hardware $D402/$D403 are write-only; read the OS shadows instead.      */
#ifndef SDLSTL
#define SDLSTL      0x0230
#endif
#ifndef SDLSTH
#define SDLSTH      0x0231
#endif

/* ── Other OS shadows ────────────────────────────────────────────────────*/
#ifndef CRSINH
#define CRSINH      0x02F0
#endif
#ifndef ATRACT
#define ATRACT      0x004D
#endif

/* ── Hardware colour registers — DLI handlers ONLY ──────────────────────*/
#ifndef COLPF1_HW
#define COLPF1_HW   (*(volatile unsigned char*)0xD017)
#endif
#ifndef COLPF2_HW
#define COLPF2_HW   (*(volatile unsigned char*)0xD018)
#endif
#ifndef COLBK_HW
#define COLBK_HW    (*(volatile unsigned char*)0xD01A)
#endif

/* ── WSYNC ───────────────────────────────────────────────────────────────*/
#ifndef WSYNC
#define WSYNC       (*(volatile unsigned char*)0xD409)
#endif

/* ── NMI enable values ───────────────────────────────────────────────────*/
#ifndef NMIEN_NONE
#define NMIEN_NONE  0x00
#endif
#ifndef NMIEN_VBI
#define NMIEN_VBI   0x40
#endif
#ifndef NMIEN_DLI
#define NMIEN_DLI   0x80
#endif
#ifndef NMIEN_BOTH
#define NMIEN_BOTH  0xC0
#endif

/* ── Display-list instruction bytes ──────────────────────────────────────*/
#ifndef DL_BLANK8
#define DL_BLANK8       0x70
#endif
#ifndef DL_BLANK8_DLI
#define DL_BLANK8_DLI   0xF0
#endif
#ifndef DL_MODE2_LMS
#define DL_MODE2_LMS    0x42
#endif
#ifndef DL_MODE2
#define DL_MODE2        0x02   /* ANTIC Mode 2 text, no LMS, no DLI */
#endif
#ifndef DL_JVB
#define DL_JVB          0x41
#endif

#endif /* ATARI_HW_H */