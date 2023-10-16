#include <vector>
#include <string>

#include "ws2811.h"
#include "conf.h"

class Patterns {
public:
    Patterns() {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i].r = 0;
            leds[i].g = 0;
            leds[i].b = 0;
            leds[i].abslt = 0;
        }
    }

    void init(ws2811_t led) {
        ledstring = led;
    }

    unsigned char fadeRate = 5;
    unsigned char popRate = 50;

    void patternSet(int index);
private:
    typedef void(Patterns::*Pattern)();

    void randomSingleColorPops();

    ws2811_t ledstring;
    Pattern pats[1] = {&Patterns::randomSingleColorPops};

    void popDecay();

    unsigned long int color;
    ledstruc leds[NUM_LEDS];
};