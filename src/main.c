#include <conio.h>
#include <peekpoke.h>

#include "colors.h"
#include "display.h"
#include "atari_hw.h"   /* pulls in <atari.h> + all NMIEN_* defines */

/* Assembly interrupt handlers (defined in dli.s and vbi.s) */
void dli_handler(void);
void vbi_handler(void);

int main(void)
{
    unsigned int  orig_vbi;
    unsigned char orig_colpf1, orig_colpf2, orig_colbk;

    /* ── Save OS state we will modify ──────────────────────────────── */
    orig_vbi    = PEEKW(0x0224);   /* deferred VBI vector              */
    orig_colpf1 = PEEK(COLPF1_SH);
    orig_colpf2 = PEEK(COLPF2_SH);
    orig_colbk  = PEEK(COLBK_SH);

    /* ── Initialise colour table, screen buffers, display list ─────── */
    colors_init();
    display_init();

    /* ── Install interrupt handlers ─────────────────────────────────── */
    NMIEN = NMIEN_NONE;                        /* silence all NMIs      */
    POKEW(0x0200, (unsigned int)dli_handler);  /* DLI vector at $0200   */
    POKEW(0x0224, (unsigned int)vbi_handler);  /* deferred VBI at $0224 */
    display_install();                         /* point ANTIC at our DL */
    NMIEN = NMIEN_BOTH;                        /* enable VBI + DLI      */

    /* ── Run until any key is pressed ──────────────────────────────── */
    cgetc();

    /* ── Restore OS state ────────────────────────────────────────────  */
    NMIEN = NMIEN_NONE;
    display_restore();
    POKEW(0x0224, orig_vbi);
    POKE(COLPF1_SH, orig_colpf1);
    POKE(COLPF2_SH, orig_colpf2);
    POKE(COLBK_SH,  orig_colbk);
    NMIEN = NMIEN_VBI;             /* VBI only — OS needs it            */

    clrscr();
    return 0;
}