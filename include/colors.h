#ifndef COLORS_H
#define COLORS_H

#ifndef COLOR_TABLE_SIZE
#define COLOR_TABLE_SIZE  128
#endif

#ifndef COLOR_TABLE_MASK
#define COLOR_TABLE_MASK  127
#endif

extern const unsigned char color_table[COLOR_TABLE_SIZE];
extern volatile unsigned char dli_index;
extern volatile unsigned char vbi_offset;

void colors_init(void);

#endif /* COLORS_H */