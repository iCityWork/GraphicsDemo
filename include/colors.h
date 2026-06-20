#ifndef COLORS_H
#define COLORS_H

/* Number of bars on screen — one DLI fires per bar */
#define NUM_BARS    20

/* Number of entries in the color table (must be power of 2) */
#define COLOR_TABLE_SIZE  128
#define COLOR_TABLE_MASK  127  /* Used for fast wrapping: index & MASK */

/*
 * color_table[] — 128 Atari color values cycling through the rainbow.
 * Defined in colors.c, accessed by the DLI handler in assembly.
 *
 * The 'extern' here tells C that the actual storage is in colors.c.
 * The assembly DLI/VBI handlers import this symbol via .import _color_table.
 */
extern const unsigned char color_table[COLOR_TABLE_SIZE];

/*
 * Shared state between the VBI and DLI handlers.
 * 'volatile' is required — these are written by interrupt handlers
 * and read by both interrupts and main code. Without volatile, the
 * compiler may cache them in registers and miss updates.
 *
 * dli_index  : which bar we're currently drawing (0..NUM_BARS-1)
 *              Reset to 0 by VBI at the start of each frame.
 *              Incremented by each DLI firing.
 *
 * vbi_offset : animation phase counter, incremented each VBI.
 *              Added to dli_index when looking up the color table,
 *              causing the rainbow to slide upward each frame.
 */
extern volatile unsigned char dli_index;
extern volatile unsigned char vbi_offset;

/*
 * colors_init() — fill in the color table with a rainbow gradient.
 * Called once from main() before enabling interrupts.
 */
void colors_init(void);

#endif /* COLORS_H */