/*
 * display.c — ANTIC display list and screen buffers
 *
 * ── Why the display list alignment matters ───────────────────────────
 * ANTIC reads the display list with a 10-bit internal page counter.
 * When that counter reaches $400 it wraps to $000 — the START of the
 * same 1KB page. If our display list spans a 1KB ($0400) boundary,
 * ANTIC continues reading from the wrong address and interprets random
 * bytes as mode instructions, producing garbled bars mid-screen.
 *
 * Fix: allocate a 256-byte raw buffer and pick a starting offset that
 * keeps the entire 60-byte display list inside one 1KB page. The
 * largest skip we ever need is <60 bytes, so a 256-byte pool is ample.
 *
 * ── Shadow register rule ─────────────────────────────────────────────
 * All color setup uses OS shadow registers ($02C4-$02C8). The OS VBI
 * copies those to hardware once per frame — enough for title/footer.
 * Only DLI handlers write hardware color registers directly, because
 * shadows are updated just once per frame and we need per-scanline
 * color changes during the bar section.
 *
 * ── Hardware register rule ───────────────────────────────────────────
 * DLISTL/DLISTH ($D402/$D403): WRITE-ONLY. Never read.
 * OS shadows at SDLSTL/SDLSTH ($0230/$0231) are the readable copies.
 * Write BOTH shadow and hardware when changing the display list, or
 * the next OS VBI reverts to the old address.
 */

#include <string.h>
#include <peekpoke.h>
#include "display.h"
#include "atari_hw.h"

/* OS shadow display list registers — readable, written by VBI each frame */
#define SDLSTL  0x0230
#define SDLSTH  0x0231   /* not used directly, POKEW handles both */

/* Cursor inhibit — set to 1 to stop OS from drawing cursor into screen RAM */
#define CRSINH     0x02F0

/* Screen data buffers: 40 bytes each, in Atari screen codes (= ASCII - 32) */
static unsigned char title_line1[SCREEN_WIDTH];
static unsigned char title_line2[SCREEN_WIDTH];
static unsigned char blank_line[SCREEN_WIDTH];    /* all spaces — color bars */
static unsigned char footer_line1[SCREEN_WIDTH];
static unsigned char footer_line2[SCREEN_WIDTH];

/*
 * Display list alignment pool.
 *
 * We allocate 256 bytes and compute the starting offset at runtime so
 * our ~60-byte display list always sits within a single 1KB ANTIC page.
 *
 * Worst case: dl_raw starts 59 bytes before a 1KB boundary.
 * Skip needed = 59 bytes.  Remaining in buffer = 256 - 59 = 197 bytes.
 * Our DL is ~60 bytes.  197 > 60.  Always fits.
 */
static unsigned char  dl_raw[256];
static unsigned char *display_list;   /* points into dl_raw at safe offset */

/* Saved OS display list pointer (from shadow, not hardware) */
static unsigned int os_dlist;

/* Convert centered ASCII string to Atari screen codes (= ASCII - 32) */
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

/*
 * compute_dl_ptr — find a safe starting address within dl_raw.
 *
 * "Safe" means the entire DL fits in one 1KB ANTIC page.
 * We need at most DL_MAX_SIZE bytes clear before the next $400 boundary.
 */
#define DL_MAX_SIZE 80   /* conservative upper bound on our DL byte count */

static void compute_dl_ptr(void)
{
    unsigned int addr    = (unsigned int)dl_raw;
    unsigned int in_page = addr & 0x03FFu;          /* offset within 1KB page */

    if (in_page + DL_MAX_SIZE > 0x0400u) {
        /*
         * dl_raw is too close to the end of this 1KB page.
         * Advance display_list to the start of the NEXT 1KB page.
         * The skip is at most DL_MAX_SIZE-1 bytes, which always fits
         * inside our 256-byte dl_raw pool.
         */
        display_list = dl_raw + (0x0400u - in_page);
    } else {
        display_list = dl_raw;
    }
}

/* ── display_init ───────────────────────────────────────────────────── */

void display_init(void)
{
    unsigned char i;
    unsigned char *dl;

    /* ── Find a page-aligned start for the display list ── */
    compute_dl_ptr();
    dl = display_list;

    /* ── Fill screen text buffers ── */
    str_to_screen("** ATARI COLOR STORM **", title_line1,  SCREEN_WIDTH);
    str_to_screen("  CC65  +  ANTIC  DLI  ", title_line2,  SCREEN_WIDTH);
    memset(blank_line,   0, SCREEN_WIDTH);   /* all spaces — no char pixels */
    str_to_screen(" PRESS  ANY  KEY  TO EXIT ", footer_line1, SCREEN_WIDTH);
    memset(footer_line2, 0, SCREEN_WIDTH);

    /*
     * Set title/footer colors via OS SHADOW registers.
     * Never write hardware color registers here — the OS VBI copies
     * these shadows to hardware once per frame, which is all we need
     * for sections that stay one color per frame.
     */
    POKE(COLPF1_SH, 0x0E);   /* text pixels: white         */
    POKE(COLPF2_SH, 0x84);   /* text background: dark blue */
    POKE(COLBK_SH,  0x02);   /* border: near black         */

    /*
     * Disable the OS cursor.
     * The OS draws the cursor by writing a character into screen RAM
     * at the current cursor position. If blank_line happens to sit at
     * the same physical address as that cursor cell, the cursor char
     * would appear in our color bars. CRSINH=1 prevents this entirely.
     */
    POKE(CRSINH, 1);

    /* ── Build ANTIC display list ── */

    /* 24 blank lines at top (3 × 8) */
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;
    *dl++ = DL_BLANK8;

    /*
     * Title lines — NO DLI flag.
     * These use the OS shadow colors set above. Adding DLI here would
     * fire the handler mid-title and turn the second line into a bar.
     */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line1 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)title_line1 >> 8);

    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)title_line2 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)title_line2 >> 8);

    /* 8 blank lines: separator between title and bars */
    *dl++ = DL_BLANK8;

    /*
     * Pre-bar setup line: 8 blank scanlines WITH DLI ($F0 = $70|$80).
     *
     * This fires the first DLI (dli_index=0) at the end of these blank
     * lines. After WSYNC the handler writes bar 1's rainbow color so
     * bar 1 gets a proper hue instead of the OS default.
     */
    *dl++ = DL_BLANK8_DLI;   /* $F0 */

    /*
     * Color bar lines.
     * Each is Mode 2 (text mode) with blank_line as screen data, so the
     * entire 8-scanline area fills with the current hardware COLPF2/COLBK.
     *
     * DLI fires at the end of each bar and sets the NEXT bar's color.
     * Bar NUM_BARS's DLI (dli_index == NUM_BARS in the handler) is the
     * cleanup DLI — it restores OS shadow colors for the footer section.
     *
     * All bars reuse the same 40-byte blank_line buffer via LMS.
     */
    for (i = 0; i < NUM_BARS; i++) {
        *dl++ = DL_MODE2_DLILMS;
        *dl++ = (unsigned char)((unsigned int)blank_line & 0xFF);
        *dl++ = (unsigned char)((unsigned int)blank_line >> 8);
    }

    /* 8 blank lines: separator after bars */
    *dl++ = DL_BLANK8;

    /* Footer lines — no DLI (cleanup DLI already restored colors) */
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)footer_line1 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)footer_line1 >> 8);

    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)((unsigned int)footer_line2 & 0xFF);
    *dl++ = (unsigned char)((unsigned int)footer_line2 >> 8);

    /* JVB — jump back to start and signal VBI. Always the last byte. */
    *dl++ = DL_JVB;
    *dl++ = (unsigned char)((unsigned int)display_list & 0xFF);
    *dl++ = (unsigned char)((unsigned int)display_list >> 8);
}

/* ── display_install ────────────────────────────────────────────────── */

void display_install(void)
{
    /* Read OS DL from shadow — hardware DLISTL/DLISTH are write-only */
    os_dlist = PEEKW(SDLSTL);

    /* Write BOTH shadow and hardware — shadow must match or OS VBI reverts */
    POKEW(SDLSTL, (unsigned int)display_list);
    DLISTL = (unsigned char)((unsigned int)display_list & 0xFF);
    DLISTH = (unsigned char)((unsigned int)display_list >> 8);
}

/* ── display_restore ────────────────────────────────────────────────── */

void display_restore(void)
{
    POKEW(SDLSTL, os_dlist);
    DLISTL = (unsigned char)(os_dlist & 0xFF);
    DLISTH = (unsigned char)(os_dlist >> 8);

    /* Re-enable cursor */
    POKE(CRSINH, 0);
}