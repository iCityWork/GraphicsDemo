/* ================================================================
   src/display.c  —  Color Storm ANTIC display list
   ================================================================
   KEY CHANGE: No separate footer_line variable.
   Footer data lives at title_line1+80 = first 40 bytes of dl_raw.
   Footer row uses DL_MODE2 natural scan — ANTIC scan counter is
   frozen at title_line1+80 after title_line2, so no LMS needed.
   ================================================================ */
#include <peekpoke.h>
#include "display.h"
#include "colors.h"
#include "atari_hw.h"

/* BSS declaration order is critical:
   title_line1[40]  <- LMS anchor
   title_line2[40]  <- natural scan from title_line1 LMS (= title_line1+40)
   dl_raw[256]      <- DL workspace; bytes 0..39 = FOOTER SCREEN DATA  */
static unsigned char title_line1[SCREEN_WIDTH];
static unsigned char title_line2[SCREEN_WIDTH];
static unsigned char dl_raw[256];

static unsigned int  orig_sdlst;
static unsigned char *dl_ptr;

/* ---------------------------------------------------------------
   compute_dl_ptr: 256-byte aligned address in dl_raw that does
   not cross a 1KB ANTIC boundary.
   --------------------------------------------------------------- */
static unsigned char *compute_dl_ptr(void)
{
    unsigned int addr = (unsigned int)dl_raw;
    if (addr & 0xFFu) {
        addr = (addr & 0xFF00u) + 0x0100u;
    }
    if ((addr & 0x03FFu) > (0x0400u - 64u)) {
        addr = (addr & 0xFC00u) + 0x0400u;
    }
    return (unsigned char *)addr;
}

/* ================================================================
   display_init
   ================================================================ */
void display_init(void)
{
    unsigned char *footer;
    unsigned char *dl;
    unsigned char  i;

    /* title_line1: "*** COLOR STORM ***"  cols 10-28 */
    for (i = 0; i < SCREEN_WIDTH; i++) title_line1[i] = 0;
    title_line1[10] = 10;   /* * */
    title_line1[11] = 10;   /* * */
    title_line1[12] = 10;   /* * */
    title_line1[14] = 35;   /* C */
    title_line1[15] = 47;   /* O */
    title_line1[16] = 44;   /* L */
    title_line1[17] = 47;   /* O */
    title_line1[18] = 50;   /* R */
    title_line1[20] = 51;   /* S */
    title_line1[21] = 52;   /* T */
    title_line1[22] = 47;   /* O */
    title_line1[23] = 50;   /* R */
    title_line1[24] = 45;   /* M */
    title_line1[26] = 10;   /* * */
    title_line1[27] = 10;   /* * */
    title_line1[28] = 10;   /* * */

    /* title_line2: "ANTIC DLI DEMO"  cols 13-26 */
    for (i = 0; i < SCREEN_WIDTH; i++) title_line2[i] = 0;
    title_line2[13] = 33;   /* A */
    title_line2[14] = 46;   /* N */
    title_line2[15] = 52;   /* T */
    title_line2[16] = 41;   /* I */
    title_line2[17] = 35;   /* C */
    title_line2[19] = 36;   /* D */
    title_line2[20] = 44;   /* L */
    title_line2[21] = 41;   /* I */
    title_line2[23] = 36;   /* D */
    title_line2[24] = 37;   /* E */
    title_line2[25] = 45;   /* M */
    title_line2[26] = 47;   /* O */

    /* footer: "PRESS ANY KEY TO EXIT"  cols 9-29
       Stored at title_line1+80 = title_line2+40 = &dl_raw[0].
       No separate C variable — pointer arithmetic from title_line1
       avoids any symbol-resolution issue.                          */
    footer = title_line1 + (2u * SCREEN_WIDTH);
    for (i = 0; i < SCREEN_WIDTH; i++) footer[i] = 0;
    footer[ 9] = 48;  /* P */
    footer[10] = 50;  /* R */
    footer[11] = 37;  /* E */
    footer[12] = 51;  /* S */
    footer[13] = 51;  /* S */
    footer[15] = 33;  /* A */
    footer[16] = 46;  /* N */
    footer[17] = 57;  /* Y */
    footer[19] = 43;  /* K */
    footer[20] = 37;  /* E */
    footer[21] = 57;  /* Y */
    footer[23] = 52;  /* T */
    footer[24] = 47;  /* O */
    footer[26] = 37;  /* E */
    footer[27] = 56;  /* X */
    footer[28] = 41;  /* I */
    footer[29] = 52;  /* T */

    /* Build display list */
    dl_ptr = compute_dl_ptr();
    dl     = dl_ptr;

    /* Top margin: 3x8 = 24 blank scanlines */
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;

    /* Title row 1: explicit LMS to title_line1 */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line1 & 0xFFu);
    *dl++ = (unsigned char)((unsigned int)title_line1 >> 8);

    /* Title row 2: natural scan -> title_line2 (= title_line1+40) */
    *dl++ = DL_MODE2;

    /* Separator */
    *dl++ = DL_BLANK8;

    /* 12 rainbow bars + 1 COLBK-reset DLI trigger */
    for (i = 0; i <= NUM_BARS; i++) {
        *dl++ = DL_BLANK8_DLI;
    }

    /* Post-bar spacer: 2x8 = 16 scanlines */
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;

    /* Footer row: NATURAL SCAN — no LMS.
       After title_line2, ANTIC scan counter = title_line1+80.
       All blank/DLI-blank lines freeze the counter there.
       This DL_MODE2 reads directly from our footer data.     */
    *dl++ = DL_MODE2;

    /* Trailing blanks: 2x8 = 16 scanlines */
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;

    /* Jump and Wait for Vertical Blank */
    *dl++ = DL_JVB;
    *dl++ = (unsigned char)((unsigned int)dl_ptr & 0xFFu);
    *dl++ = (unsigned char)((unsigned int)dl_ptr >> 8);

    /* Scanline audit: 24+8+8+8+104+16+8+16 = 192 (NTSC) */
}

/* ================================================================
   display_install
   ================================================================ */
void display_install(void)
{
    orig_sdlst = PEEKW(SDLSTL);
    POKE(CRSINH, 1u);
    POKE(COLPF1_SH, 0x0Fu);
    POKE(COLBK_SH,  0x00u);
    POKEW(SDLSTL, (unsigned int)dl_ptr);
    DLISTL = (unsigned char)((unsigned int)dl_ptr & 0xFFu);
    DLISTH = (unsigned char)((unsigned int)dl_ptr >> 8);
}

/* ================================================================
   display_restore
   ================================================================ */
void display_restore(void)
{
    POKEW(SDLSTL, orig_sdlst);
    DLISTL = (unsigned char)(orig_sdlst & 0xFFu);
    DLISTH = (unsigned char)(orig_sdlst >> 8);
    POKE(CRSINH, 0u);
}