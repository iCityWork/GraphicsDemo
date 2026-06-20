/*
 * colors.c — Color table and shared interrupt state
 *
 * The color table is the heart of the visual effect. ANTIC's DLI
 * handler reads one entry per bar per frame. The VBI shifts the
 * starting point each frame, making the colors flow upward.
 *
 * Atari color byte format reminder:
 *   Bits 7-4 = Hue (0x0-0xF, 16 hues around the color wheel)
 *   Bits 3-1 = Luminance (0=black, 7=brightest)
 *   Bit    0 = Always 0
 *
 * We fill 128 entries with 8 sweeps through all 16 hues, alternating
 * between three luminance levels. This produces a pulsing rainbow that
 * cycles smoothly as vbi_offset increments each frame.
 */

#include "colors.h"

/* ── Shared interrupt state ──────────────────────────────────────────────
 *
 * These MUST be global (not static, not local) so the linker gives them
 * fixed addresses that the assembly interrupt handlers can import.
 *
 * The assembly files reference these as _dli_index and _vbi_offset
 * (CC65 prepends an underscore to all C global names in assembly).
 */
volatile unsigned char dli_index  = 0;
volatile unsigned char vbi_offset = 0;

/* ── Color table ─────────────────────────────────────────────────────────
 *
 * 128 entries = 8 sweeps × 16 hues.
 * Luminance alternates: 0xC (bright) → 0xE (brilliant) → 0xA (medium)
 * This gives the bars a subtle pulsing depth as the animation cycles.
 *
 * Hue mapping (approximate — varies slightly by TV/monitor):
 *   0x0 = grey/white   0x1 = gold       0x2 = orange-gold  0x3 = orange
 *   0x4 = yellow-orange 0x5 = yellow    0x6 = yellow-green  0x7 = green
 *   0x8 = teal-green   0x9 = teal       0xA = cyan-blue     0xB = blue
 *   0xC = blue-violet  0xD = violet     0xE = red-violet    0xF = red
 */
const unsigned char color_table[COLOR_TABLE_SIZE] = {
    /* Sweep 1: Bright (luminance 0xC = level 6) */
    0x0C, 0x1C, 0x2C, 0x3C, 0x4C, 0x5C, 0x6C, 0x7C,
    0x8C, 0x9C, 0xAC, 0xBC, 0xCC, 0xDC, 0xEC, 0xFC,
    /* Sweep 2: Brilliant (luminance 0xE = level 7, maximum) */
    0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E,
    0x8E, 0x9E, 0xAE, 0xBE, 0xCE, 0xDE, 0xEE, 0xFE,
    /* Sweep 3: Medium (luminance 0xA = level 5) */
    0x0A, 0x1A, 0x2A, 0x3A, 0x4A, 0x5A, 0x6A, 0x7A,
    0x8A, 0x9A, 0xAA, 0xBA, 0xCA, 0xDA, 0xEA, 0xFA,
    /* Sweep 4: Bright again — creates the "pulse" cycle */
    0x0C, 0x1C, 0x2C, 0x3C, 0x4C, 0x5C, 0x6C, 0x7C,
    0x8C, 0x9C, 0xAC, 0xBC, 0xCC, 0xDC, 0xEC, 0xFC,
    /* Repeat sweeps 1-4 for the second half of the table */
    /* (gives smooth wrapping when vbi_offset cycles through 0-127) */
    0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E,
    0x8E, 0x9E, 0xAE, 0xBE, 0xCE, 0xDE, 0xEE, 0xFE,
    0x0A, 0x1A, 0x2A, 0x3A, 0x4A, 0x5A, 0x6A, 0x7A,
    0x8A, 0x9A, 0xAA, 0xBA, 0xCA, 0xDA, 0xEA, 0xFA,
    0x0C, 0x1C, 0x2C, 0x3C, 0x4C, 0x5C, 0x6C, 0x7C,
    0x8C, 0x9C, 0xAC, 0xBC, 0xCC, 0xDC, 0xEC, 0xFC,
    0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E,
    0x8E, 0x9E, 0xAE, 0xBE, 0xCE, 0xDE, 0xEE, 0xFE,
};

/*
 * colors_init() — currently the table is compile-time initialized,
 * so this is a no-op. Reserved for future dynamic color generation
 * (plasma effects, sine-wave luminance curves, etc.)
 */
void colors_init(void)
{
    dli_index  = 0;
    vbi_offset = 0;
}