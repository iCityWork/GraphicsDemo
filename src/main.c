/*
 * main.c — ATARI COLOR STORM demo entry point
 */

#include <conio.h>
#include <peekpoke.h>
#include "atari_hw.h"
#include "display.h"
#include "colors.h"

extern void dli_handler(void);
extern void vbi_handler(void);

/* OS shadow color registers — restore on exit */
#define COLPF1_SH  0x02C5
#define COLPF2_SH  0x02C6
#define COLBK_SH   0x02C8

static unsigned int orig_vbi;

/* Save initial OS color shadows so we can restore them on exit */
static unsigned char orig_colpf1;
static unsigned char orig_colpf2;
static unsigned char orig_colbk;

int main(void)
{
    /* ── Step 1: Save OS state we'll be changing ── */
    orig_vbi    = PEEKW(0x0224);          /* VVBLKD — in OS RAM, readable */
    orig_colpf1 = PEEK(COLPF1_SH);
    orig_colpf2 = PEEK(COLPF2_SH);
    orig_colbk  = PEEK(COLBK_SH);

    /* ── Step 2: Build display list and color table ── */
    colors_init();
    display_init();

    /* ── Step 3: Install everything with NMI fully disabled ──
     *
     * Keep NMI off for the entire setup sequence. This prevents the VBI
     * from firing between our writes and seeing a half-configured state
     * (e.g., DLI installed but display list not yet switched, or vice versa).
     * The window is only a few microseconds — negligible to the OS.
     */
    NMIEN = NMIEN_NONE;

    POKEW(0x0200, (unsigned int)dli_handler);   /* DLI vector (VDSLST)  */
    POKEW(0x0224, (unsigned int)vbi_handler);   /* VBI vector (VVBLKD)  */

    display_install();  /* switch ANTIC to our display list (shadow + hw) */

    /* Demo is live — enable both VBI and DLI */
    NMIEN = NMIEN_BOTH;

    /* ── Step 4: Main loop — demo runs entirely in interrupts ──
     *
     * The 6502 sits here doing nothing. All animation happens in
     * dli_handler (20×/frame) and vbi_handler (1×/frame).
     */
    cgetc();   /* block until any key pressed */

    /* ── Step 5: Tear down in reverse order ──
     *
     * Disable everything first, then restore. Never restore the display
     * list while interrupts are live — a DLI could fire mid-restore and
     * try to read COLBK while ANTIC is in an unknown state.
     */
    NMIEN = NMIEN_NONE;

    display_restore();                    /* restore OS display list   */
    POKEW(0x0224, orig_vbi);              /* restore OS VBI vector     */

    /* Restore OS color shadows (VBI will copy them to hardware) */
    POKE(COLPF1_SH, orig_colpf1);
    POKE(COLPF2_SH, orig_colpf2);
    POKE(COLBK_SH,  orig_colbk);

    NMIEN = NMIEN_VBI;   /* re-enable OS VBI only */

    clrscr();
    return 0;
}