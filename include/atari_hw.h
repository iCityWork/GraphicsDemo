#ifndef ATARI_HW_H
#define ATARI_HW_H

#include <atari.h>

#ifndef DLISTL
#define DLISTL  (*(volatile unsigned char*)0xD402)
#endif
#ifndef DLISTH
#define DLISTH  (*(volatile unsigned char*)0xD403)
#endif
#ifndef NMIEN
#define NMIEN   (*(volatile unsigned char*)0xD40E)
#endif
/* $D409 = CHBASE (character set base) — do NOT redefine as WSYNC */
#ifndef WSYNC
#define WSYNC   (*(volatile unsigned char*)0xD40A)   /* $D40A = WSYNC */
#endif
#ifndef COLPF1_HW
#define COLPF1_HW (*(volatile unsigned char*)0xD017)
#endif
#ifndef COLPF2_HW
#define COLPF2_HW (*(volatile unsigned char*)0xD018)
#endif
#ifndef COLBK_HW
#define COLBK_HW  (*(volatile unsigned char*)0xD01A)
#endif

/* Shadow registers — Atari OS addresses */
#ifndef COLPF0_SH
#define COLPF0_SH  0x02C4
#endif
#ifndef COLPF1_SH
#define COLPF1_SH  0x02C5
#endif
#ifndef COLPF2_SH
#define COLPF2_SH  0x02C6
#endif
#ifndef COLPF3_SH
#define COLPF3_SH  0x02C7
#endif
#ifndef COLBK_SH
#define COLBK_SH   0x02C8
#endif
#ifndef SDLSTL
#define SDLSTL  0x0230
#endif
#ifndef SDLSTH
#define SDLSTH  0x0231
#endif
#ifndef CRSINH
#define CRSINH  0x02F0
#endif
#ifndef ATRACT
#define ATRACT  0x004D
#endif

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

#ifndef DL_BLANK8
#define DL_BLANK8     0x70
#endif
#ifndef DL_BLANK8_DLI
#define DL_BLANK8_DLI 0xF0
#endif
#ifndef DL_MODE2
#define DL_MODE2      0x02
#endif
#ifndef DL_MODE2_LMS
#define DL_MODE2_LMS  0x42
#endif
#ifndef DL_JVB
#define DL_JVB        0x41
#endif

#endif /* ATARI_HW_H */