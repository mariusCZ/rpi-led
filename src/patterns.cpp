#include "../include/patterns.h"

#include <random>
#include <chrono>
#include <iostream>

void Patterns::patternSet(int index) {
    (this->*pats[index])();
    for (int i = 0; i < NUM_LEDS; i++) {
        unsigned long int colors = 0;
        colors = (leds[i].b << 16) | (leds[i].g << 8) | leds[i].r;
        ledstring.channel[0].leds[i] = colors;
    }
    ws2811_render(&ledstring);
}

void Patterns::randomSingleColorPops() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, NUM_LEDS - 1);

    static auto startTime = std::chrono::steady_clock::now();
    auto endTime = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    popDecay();

    if (duration.count() >= popRate) {
        //std::cout << "Test\n";
        int n = dis(gen);
        leds[n].r = 255;
        startTime = std::chrono::steady_clock::now();
    }
}

void Patterns::popDecay() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = (leds[i].r < fadeRate) ? 0 : leds[i].r - fadeRate;
        leds[i].g = (leds[i].g < fadeRate) ? 0 : leds[i].g - fadeRate;
        leds[i].b = (leds[i].b < fadeRate) ? 0 : leds[i].b - fadeRate;
    }
}