#include <string.h>
#include <peekpoke.h>
#include "display.h"
#include "atari_hw.h"

#define SDLSTL  0x0230
#define CRSINH  0x02F0

static unsigned char title_line1[SCREEN_WIDTH];
static unsigned char title_line2[SCREEN_WIDTH];
static unsigned char footer_line1[SCREEN_WIDTH];
static unsigned char footer_line2[SCREEN_WIDTH];

static unsigned char  dl_raw[256];
static unsigned char *display_list;
static unsigned int   os_dlist;

#define DL_MAX_SIZE 64

static void compute_dl_ptr(void)
{
    unsigned int addr    = (unsigned int)dl_raw;
    unsigned int in_page = addr & 0x03FFu;
    if (in_page + DL_MAX_SIZE > 0x0400u)
        display_list = dl_raw + (0x0400u - in_page);
    else
        display_list = dl_raw;
}

static void str_to_screen(const char *str, unsigned char *dest,
                           unsigned char maxlen)
{
    unsigned char len = 0, start, i;
    const char *p;
    for (p = str; *p; ++p) ++len;
    if (len > maxlen) len = maxlen;
    memset(dest, 0, maxlen);
    start = (maxlen - len) / 2;
    for (i = 0; i < len; i++)
        dest[start + i] = (unsigned char)(str[i] - 32);
}

void display_init(void)
{
    unsigned char  i;
    unsigned char *dl;

    compute_dl_ptr();
    dl = display_list;

    str_to_screen("** ATARI COLOR STORM **",    title_line1,  SCREEN_WIDTH);
    str_to_screen("  CC65  +  ANTIC  DLI  ",    title_line2,  SCREEN_WIDTH);
    str_to_screen(" PRESS  ANY  KEY  TO EXIT ", footer_line1, SCREEN_WIDTH);
    memset(footer_line2, 0, SCREEN_WIDTH);

    /* Shadow registers — OS VBI copies these to hardware each frame.
       COLPF1 and COLPF2 control Mode-2 text colours.
       The bar DLIs only touch COLBK, so these values persist intact
       through the entire frame — title and footer both get them. */
    POKE(COLPF1_SH, 0x0E);   /* white  — character pixels     */
    POKE(COLPF2_SH, 0x84);   /* dark blue — character bg      */
    POKE(COLBK_SH,  0x02);   /* near-black — border           */
    POKE(CRSINH,    1);

    /* ── Display list ──────────────────────────────────────────────── */

    /* 24 blank lines (top margin) */
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;

    /* Title — rendered before any DLI fires, uses OS shadow colours */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line1 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)title_line1 >> 8);

    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line2 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)title_line2 >> 8);

    /* Gap + pre-bar DLI ─────────────────────────────────────────────
       DL_BLANK8_DLI entries show their 8 scanlines in COLBK colour.
       The DLI fires at the END and WSYNC stalls to the NEXT section,
       so each entry's DLI sets the colour of the FOLLOWING bar.

       Pre-bar (dli_index=0): fires → WSYNC → bar 1 COLBK = colour 0.
       The 8 pre-bar scanlines display COLBK=$02 (near-black) because
       no DLI has changed it yet — acts as a dark gap before the bars. */
    *dl++ = DL_BLANK8_DLI;   /* pre-bar, dli_index = 0 */

    /* Bars 1-12 — all blank lines, DLI only touches COLBK.
       COLPF1 and COLPF2 are NEVER written by bar DLIs, so the
       Mode-2 footer always has the correct text colours.
       dli_index 1-11  → do_bar (sets next bar COLBK via WSYNC)
       dli_index 12    → cleanup (sets COLBK=$02 for separator+footer) */
    for (i = 0; i < NUM_BARS; i++) {
        *dl++ = DL_BLANK8_DLI;
    }

    /* Separator — 8 plain blank lines.
       COLBK=$02 (near-black) was restored by the cleanup DLI above. */
    *dl++ = DL_BLANK8;

    /* Footer — Mode-2 text.
       COLPF1=$0E (white) and COLPF2=$84 (dark blue) were never
       changed by any DLI, so text is always visible here. */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)footer_line1 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)footer_line1 >> 8);

    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)footer_line2 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)footer_line2 >> 8);

    /* Jump and Wait for VBL */
    *dl++ = DL_JVB;
    *dl++ = (unsigned char)((unsigned int)display_list & 0xFF);
    *dl++ = (unsigned char)((unsigned int)display_list >> 8);
}

void display_install(void)
{
    os_dlist = PEEKW(SDLSTL);
    POKEW(SDLSTL, (unsigned int)display_list);
    DLISTL = (unsigned char)((unsigned int)display_list & 0xFF);
    DLISTH = (unsigned char)((unsigned int)display_list >> 8);
}

void display_restore(void)
{
    POKEW(SDLSTL, os_dlist);
    DLISTL = (unsigned char)(os_dlist & 0xFF);
    DLISTH = (unsigned char)(os_dlist >> 8);
    POKE(CRSINH, 0);
}