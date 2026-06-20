/*
 * display.c — ANTIC display list and screen buffers
 *
 * TWO CRITICAL ATARI HARDWARE RULES LEARNED THE HARD WAY:
 *
 * Rule 1 — DLISTL/DLISTH ($D402/$D403) are WRITE-ONLY.
 *   Reading them returns floating bus garbage, not the display list address.
 *   The OS keeps a readable copy at $0230/$0231 (shadow registers SDLSTL/SDLSTH).
 *   Always save/restore the shadow, never the hardware registers.
 *
 * Rule 2 — The OS VBI overwrites hardware registers from shadows every frame.
 *   If you write only $D402/$D403, the very next VBI copies $0230/$0231 back
 *   over them — your display list disappears in one frame.
 *   You MUST update both shadow AND hardware to make your display list stick.
 */

#include <string.h>
#include <peekpoke.h>
#include "display.h"
#include "atari_hw.h"

/*
 * OS shadow display list pointer — in page-2 RAM, READABLE.
 * The OS VBI stage 1 copies these to hardware DLISTL/DLISTH each frame.
 * Update these to make any display list change permanent across VBIs.
 */
#define SDLSTL  0x0230   /* Shadow for DLISTL — lo byte */
#define SDLSTH  0x0231   /* Shadow for DLISTH — hi byte */

/*
 * OS color shadow registers — copied to hardware by VBI each frame.
 * Set these to control the initial colors before the DLI sequence begins.
 */
#define COLPF1_SH  0x02C5   /* Shadow COLPF1: text pixel color in GR.0  */
#define COLPF2_SH  0x02C6   /* Shadow COLPF2: text background in GR.0   */
#define COLBK_SH   0x02C8   /* Shadow COLBK:  border color              */

/* Screen data: 40 bytes each, in Atari screen codes (= ASCII - 32) */
static unsigned char title_line1[SCREEN_WIDTH];   /* "** ATARI COLOR STORM **" */
static unsigned char title_line2[SCREEN_WIDTH];   /* "  CC65 + ANTIC DEMO     " */
static unsigned char blank_line[SCREEN_WIDTH];    /* all spaces — for color bars */
static unsigned char footer_line1[SCREEN_WIDTH];  /* "   PRESS ANY KEY TO EXIT  " */
static unsigned char footer_line2[SCREEN_WIDTH];  /* blank */

/* The ANTIC display list — must not cross a 1KB boundary.
 * Our list is ~80 bytes so this is very unlikely in practice. */
static unsigned char display_list[128];

/* Saved OS display list address (read from shadow on install) */
static unsigned int os_dlist;

/*
 * str_to_screen — convert ASCII string to Atari screen codes, centered.
 * Atari screen code = ASCII value - 32 (for printable chars 32-127).
 * Screen code 0 = space, 33 = 'A', 34 = 'B', etc.
 */
static void str_to_screen(const char *str, unsigned char *dest,
                           unsigned char maxlen)
{
    unsigned char len = 0;
    unsigned char start, i;
    const char *p;

    for (p = str; *p; ++p) ++len;
    if (len > maxlen) len = maxlen;

    memset(dest, 0, maxlen);            /* fill with spaces (code 0) */
    start = (maxlen - len) / 2;         /* center */
    for (i = 0; i < len; i++)
        dest[start + i] = (unsigned char)(str[i] - 32);
}

/* ── display_init ───────────────────────────────────────────────────── */

void display_init(void)
{
    unsigned char i;
    unsigned char *dl = display_list;

    /* Fill screen text buffers */
    str_to_screen("** ATARI COLOR STORM **", title_line1,  SCREEN_WIDTH);
    str_to_screen("  CC65  +  ANTIC  DLI  ", title_line2,  SCREEN_WIDTH);
    memset(blank_line,   0, SCREEN_WIDTH);   /* all spaces */
    str_to_screen(" PRESS  ANY  KEY  TO EXIT ", footer_line1, SCREEN_WIDTH);
    memset(footer_line2, 0, SCREEN_WIDTH);

    /*
     * Set OS color shadows so the title area looks correct.
     * The OS VBI copies these to hardware at the top of each frame,
     * before any DLIs fire. The DLI overwrites hardware COLPF2/COLBK
     * for the bar section, but the title/footer keep these values
     * because they appear before the first bar's DLI fires.
     */
    POKE(COLPF1_SH, 0x0E);   /* text pixels: white           */
    POKE(COLPF2_SH, 0x84);   /* text background: dark purple */
    POKE(COLBK_SH,  0x00);   /* border: black                */

    /* ── Build the ANTIC display list ── */

    /* 24 blank scan lines at top (3 × 8) */
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;

    /*
     * Title lines — NO DLI flag.
     * If we set DLI on these, the handler fires after the title line
     * and changes COLPF2 mid-screen, coloring the separator. Omitting
     * DLI here keeps the title section clean and stable.
     */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line1 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)title_line1 >> 8);

    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line2 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)title_line2 >> 8);

    /* 8 blank lines: separator between title and color bars */
    *dl++ = DL_BLANK8;

    /*
     * Color bar lines.
     * Each bar is Mode 2 (text) with all spaces in blank_line, so
     * the entire 8-scanline area fills with the current COLPF2/COLBK.
     * DLI fires at the END of each bar and sets the NEXT bar's color.
     * All bars share blank_line via LMS — only 40 bytes of screen data
     * are needed regardless of how many bars we have.
     */
    for (i = 0; i < NUM_BARS; i++) {
        *dl++ = DL_MODE2_DLILMS;
        *dl++ = (unsigned char)((unsigned int)blank_line & 0xFF);
        *dl++ = (unsigned char)((unsigned int)blank_line >> 8);
    }

    /* 8 blank lines: separator between bars and footer */
    *dl++ = DL_BLANK8;

    /* Footer lines — no DLI */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)footer_line1 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)footer_line1 >> 8);

    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)footer_line2 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)footer_line2 >> 8);

    /*
     * JVB — Jump and Vertical Blank.
     * Always the last instruction. ANTIC jumps back to our list
     * and signals the OS to run the VBI.
     */
    *dl++ = DL_JVB;
    *dl++ = (unsigned char)((unsigned int)display_list & 0xFF);
    *dl++ = (unsigned char)((unsigned int)display_list >> 8);
}

/* ── display_install ────────────────────────────────────────────────── */

void display_install(void)
{
    /*
     * Save the OS display list address.
     * READ FROM SHADOW ($0230) — NOT from hardware ($D402)!
     * Hardware DLISTL/DLISTH are write-only; reading them is undefined.
     */
    os_dlist = PEEKW(SDLSTL);

    /*
     * Install our display list in BOTH shadow AND hardware.
     *
     * Shadow first: The OS VBI copies shadow→hardware every frame.
     * If we skip the shadow, our hardware write is overwritten on the
     * very next VBI (within 1/60th of a second) — giving us one frame
     * of our demo followed by the OS screen forever.
     */
    POKEW(SDLSTL, (unsigned int)display_list);         /* shadow — persists across VBI */
    DLISTL = (unsigned char)((unsigned int)display_list & 0xFF); /* hardware — immediate */
    DLISTH = (unsigned char)((unsigned int)display_list >> 8);
}

/* ── display_restore ────────────────────────────────────────────────── */

void display_restore(void)
{
    /*
     * Restore OS display list to both shadow and hardware.
     * Shadow must be restored so the OS VBI doesn't keep
     * pointing ANTIC at our (now potentially invalid) buffer.
     */
    POKEW(SDLSTL, os_dlist);
    DLISTL = (unsigned char)(os_dlist & 0xFF);
    DLISTH = (unsigned char)(os_dlist >> 8);
}