/*
    Config file with defines and typedefs needed in different headers.
*/

#define NUM_LEDS 298
#define OCT_N 8

// Main button size def
#define MAIN_BTN_SIZE 200
// Bigger combo size def
#define COMBO_SIZE 70
// Smaller combo size def
#define COMBO_SIZE_SMALLER 40

#ifndef CONF_H_
#define CONF_H_

typedef struct ledstruc_s {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char abslt;
} ledstruc;

typedef struct HsvColor_s
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

#endif