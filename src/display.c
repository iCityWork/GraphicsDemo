#include <peekpoke.h>
#include "display.h"
#include "colors.h"
#include "atari_hw.h"

#define TITLE1_ADDR  0x3000u
#define TITLE2_ADDR  0x3028u
#define FOOTER_ADDR  0x3050u
#define DL_ADDR      0x3100u

static unsigned int orig_sdlst;

void display_init(void)
{
    unsigned char *title1 = (unsigned char *)TITLE1_ADDR;
    unsigned char *title2 = (unsigned char *)TITLE2_ADDR;
    unsigned char *dl     = (unsigned char *)DL_ADDR;
    unsigned char  i;

    /* Clear all three screen rows */
    for (i = 0; i < SCREEN_WIDTH; i++) {
        title1[i] = 0;
        title2[i] = 0;
        POKE(FOOTER_ADDR + i, 0u);
    }

    /* Title row 1: *** COLOR STORM *** */
    title1[10]=10; title1[11]=10; title1[12]=10;
    title1[14]=35; title1[15]=47; title1[16]=44; title1[17]=47; title1[18]=50;
    title1[20]=51; title1[21]=52; title1[22]=47; title1[23]=50; title1[24]=45;
    title1[26]=10; title1[27]=10; title1[28]=10;

    /* Title row 2: ANTIC DLI DEMO */
    title2[13]=33; title2[14]=46; title2[15]=52; title2[16]=41; title2[17]=35;
    title2[19]=36; title2[20]=44; title2[21]=41;
    title2[23]=36; title2[24]=37; title2[25]=45; title2[26]=47;

    /* Footer: PRESS ANY KEY TO EXIT
       Written via explicit absolute POKEs — no pointer arithmetic,
       no array indexing, no possible compiler offset error.
       FOOTER_ADDR = 0x3050, so [9] = 0x3059, [10] = 0x305A, etc. */
    POKE(0x3059u, 48u);  /* P */
    POKE(0x305Au, 50u);  /* R */
    POKE(0x305Bu, 37u);  /* E */
    POKE(0x305Cu, 51u);  /* S */
    POKE(0x305Du, 51u);  /* S */
    /* [0x305E] = 0 (space) — already zeroed above */
    POKE(0x305Fu, 33u);  /* A */
    POKE(0x3060u, 46u);  /* N */
    POKE(0x3061u, 57u);  /* Y */
    /* [0x3062] = 0 (space) */
    POKE(0x3063u, 43u);  /* K */
    POKE(0x3064u, 37u);  /* E */
    POKE(0x3065u, 57u);  /* Y */
    /* [0x3066] = 0 (space) */
    POKE(0x3067u, 52u);  /* T */
    POKE(0x3068u, 47u);  /* O */
    /* [0x3069] = 0 (space) */
    POKE(0x306Au, 37u);  /* E */
    POKE(0x306Bu, 56u);  /* X */
    POKE(0x306Cu, 41u);  /* I */
    POKE(0x306Du, 52u);  /* T */

    /* Build display list at DL_ADDR = $3100 */
    *dl++ = DL_BLANK8; *dl++ = DL_BLANK8; *dl++ = DL_BLANK8;
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)(TITLE1_ADDR & 0xFFu);
    *dl++ = (unsigned char)(TITLE1_ADDR >> 8);
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)(TITLE2_ADDR & 0xFFu);
    *dl++ = (unsigned char)(TITLE2_ADDR >> 8);
    *dl++ = DL_BLANK8;
    for (i = 0; i <= NUM_BARS; i++) *dl++ = DL_BLANK8_DLI;
    *dl++ = DL_BLANK8; *dl++ = DL_BLANK8;
    *dl++ = DL_MODE2_LMS;
    *dl++ = (unsigned char)(FOOTER_ADDR & 0xFFu);
    *dl++ = (unsigned char)(FOOTER_ADDR >> 8);
    *dl++ = DL_BLANK8; *dl++ = DL_BLANK8;
    *dl++ = DL_JVB;
    *dl++ = (unsigned char)(DL_ADDR & 0xFFu);
    *dl++ = (unsigned char)(DL_ADDR >> 8);
}

void display_install(void)
{
    orig_sdlst = PEEKW(SDLSTL);
    POKE(CRSINH,    1u);
    POKE(COLPF2_SH, 0x94u);  /* blue text background */
    POKE(COLPF1_SH, 0x0Fu);  /* max luminance → bright text foreground */
    POKE(COLBK_SH,  0x00u);  /* black outer background */
    POKEW(SDLSTL, DL_ADDR);
    DLISTL = (unsigned char)(DL_ADDR & 0xFFu);
    DLISTH = (unsigned char)(DL_ADDR >> 8);
}

void display_restore(void)
{
    POKEW(SDLSTL, orig_sdlst);
    DLISTL = (unsigned char)(orig_sdlst & 0xFFu);
    DLISTH = (unsigned char)(orig_sdlst >> 8);
    POKE(CRSINH, 0u);
}