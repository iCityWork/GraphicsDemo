#ifndef DISPLAY_H
#define DISPLAY_H

/*
 * Number of color bar lines in the display list.
 * Each bar is one Mode 2 line = 8 scan lines tall.
 * 20 bars × 8 scan lines = 160 scan lines of animated color.
 */
#define NUM_BARS  20

/* Screen buffer width in characters (Atari GR.0 = 40 columns) */
#define SCREEN_WIDTH  40

/*
 * display_init() — build the ANTIC display list and fill screen buffers.
 * Must be called BEFORE pointing ANTIC at the new display list.
 *
 * Builds this layout:
 *   24 blank scan lines  (top margin)
 *    2 Mode 2 text lines (title)
 *    8 blank scan lines  (separator)
 *   20 Mode 2 lines      (color bars — each with DLI flag)
 *    8 blank scan lines  (separator)
 *    2 Mode 2 text lines (footer)
 *   JVB back to start
 */
void display_init(void);

/*
 * display_install() — point ANTIC at our display list.
 * Call this after display_init() has built it.
 */
void display_install(void);

/*
 * display_restore() — restore the OS display list.
 * Call this before exiting back to the OS.
 */
void display_restore(void);

#endif /* DISPLAY_H */