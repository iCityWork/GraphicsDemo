#ifndef DISPLAY_H
#define DISPLAY_H

/*
 * NUM_BARS: number of DLI color bars.
 * Each bar = 8 scan lines (one Mode-2 text line).
 *
 * Scanline budget (NTSC ≈ 192 visible):
 *   24  blank top
 *   16  title (2 × 8)
 *    8  separator
 *    8  pre-bar blank+DLI
 *   96  bars (12 × 8)        ← right-size for NTSC
 *    8  separator
 *   16  footer (2 × 8)
 *  ───
 *  176  total  (leaves a clean 16-line margin at bottom)
 */
#define NUM_BARS      12

#define SCREEN_WIDTH  40

void display_init(void);
void display_install(void);
void display_restore(void);

#endif /* DISPLAY_H */