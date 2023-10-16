#include "../include/vis.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>

#include "ws2811.h"

/*
    This file handles all the visualization of the LEDs.
    As part of the refactorization efforts, the audio visualization
    methods should be moved to the patterns file.
*/

// Create an LED object from ws2811 library.
ws2811_t ledstring = {
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
        {
            [0] =
                {
                    .gpionum = GPIO_PIN,
                    .invert = 0,
                    .count = NUM_LEDS,
                    .strip_type = STRIP_TYPE,
                    .brightness = 255,
                },
            [1] =
                {
                    .gpionum = 0,
                    .invert = 0,
                    .count = 0,
                    .brightness = 0,
                },
        },
};

// Constructor.
VisObject::VisObject() {
    ws2811_return_t ret;
    // Initialize ws2811 object.
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        std::cerr << "ws2811_init failed: " << ws2811_get_return_t_str(ret)
                  << std::endl;
    }
    // Pass the LED object to patterns.
    patterns.init(ledstring);
    // Proportion distribution for one of the audio visualization methods.
    distributeProportions();
    // Flags for main thread to check if it's running and if finished.
    finished = false;
    threadRunning = false;

    // Initialize an array of available LEDs for one of the audio visualization
    // methods.
    for (int i = 0; i < NUM_LEDS; i++) popLedsAvail.push_back(i);
}

// Destructor.
VisObject::~VisObject() {
    // Stop the main thread.
    finished = true;
    if (mainThread.get_id() != std::this_thread::get_id() &&
        mainThread.joinable())
        mainThread.join();
    ws2811_fini(&ledstring);  // Place after thread is done, otherwise wrong
                              // time possible.
    std::cout << "Vis destroyed" << std::endl;
}

// Set some single color for all the LEDs.
void VisObject::ledColorSet(unsigned long int color) {
    if (displayMode == 3) {
        for (int i = 0; i < NUM_LEDS; i++) {
            ledstring.channel[0].leds[i] = color;
        }
    }
    ws2811_render(&ledstring);  // Careful for race conditions, not sure if this
                                // function is thread safe.
}

// Public function to start visualization thread.
void VisObject::startVis() {
    finished = false;
    if (!threadRunning)
        // ledColorSet();
        mainThread = std::thread(&VisObject::mainVisThread, this);
    else
        std::cout << "LEDs already running" << std::endl;
}

// Main visualization thread function, probably not needed.
void VisObject::mainVisThread() {
    threadRunning = true;
    while (!finished) {
        mainDisplayRoutine();
    }
    threadRunning = false;
}

// Sets the visualization depending on the selected mode.
// The 10 millisecond delay is for the case of having not
// a lot of LEDs, so that the FPS would not be too high.
// In the case of more LEDs, FPS is limited by WS2812 timing.
void VisObject::mainDisplayRoutine() {
    switch (displayMode) {
        // Audio mode.
        case 0:
            setLeds();
            ws2811_render(&ledstring);
            decayLeds();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            break;
        // Pattern visualization mode.
        case 1:
            patterns.patternSet(0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            break;
        default:
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            break;
    }
}

// Function to set LEDs for audio visualization.
void VisObject::setLeds() {
    // Mutex for accessing audio data.
    std::unique_lock<std::mutex> lck(oct_lock);
    // Reset peak if peak reset button pressed.
    if (maxPeakReset) {
        maxPeak = 1;
        maxPeakReset = false;
    }
    // Update the max peak of audio data.
    for (int i = 0; i < OCT_N; i++) {
        if (oct_bins[i] > maxPeak) {
            maxPeak = oct_bins[i];
            std::cout << "Peak: " << maxPeak << std::endl;
        }
    }
    // Visualize certain audio mode (TO BE REFACTORED).
    switch (audioMode) {
        case 0:
            visAudioDoubleSpectrum();
            break;
        case 1:
            visAudioPops();
            break;
        default:
            break;
    }
    // Convert RGB to single 24 bit (variable is set to 32 bits but 24 used)
    // variable.
    for (int i = 0; i < NUM_LEDS; i++) {
        unsigned long int colors = 0;
        colors = (leds[i].b << 16) | (leds[i].g << 8) | leds[i].r;
        ledstring.channel[0].leds[i] = colors;
    }
}

// Function to periodically decay audio bins.
void VisObject::decayLeds() {
    std::unique_lock<std::mutex> lck(oct_lock);
    for (int i = 0; i < OCT_N; i++)  // this possible causes overflow, test it.
        oct_bins[i] = (oct_bins[i] < decayFun(oct_bins[i]) || oct_bins[i] < 1)
                          ? 0
                          : (oct_bins[i] - decayFun(oct_bins[i]));
}

// Function to stop the thread and in turn the LED visualization.
void VisObject::stopVis() {
    finished = true;
    if (mainThread.get_id() != std::this_thread::get_id() &&
        mainThread.joinable())
        mainThread.join();
    for (int i = 0; i < NUM_LEDS; i++) {
        ledstring.channel[0].leds[i] = 0;
    }
    ws2811_render(&ledstring);
}

// Helper function for decaying audio bins.
float VisObject::decayFun(float val) {
    // Decay mapped to a 2^x function. Makes decay look smooth.
    const float a = logf((float)decay_max) / (maxPeak * logf((float)2));
    return (powf(2, a * val) + decay_min - 1);
}

// Helper function for the spectrum audio visualization.
// It maps the audio bin magnitude to the width of lit up LEDs.
void VisObject::mapWidthx2(unsigned char widthArr[], unsigned int widthLen,
                           float input) {
    input = (input > maxPeak) ? maxPeak : input;
    // Map magnitude to a 2^x function.
    float a = logf(255 * (float)widthLen) / (maxPeak * logf(2));
    uint32_t width = (uint32_t)powf(2, a * input) - 1;

    for (uint16_t i = 0; i < widthLen; i++) {
        volatile float prop = (widthLen - (float)i) / widthLen;
        // Set the width array. prop^2 done because WS2812 brightness is weird.
        widthArr[i] = (width * prop > 255) ? 255 : width * prop * prop;
    }
}

// Helper function to distribute LEDs for the spectrum vis.
// It gives more LEDs to lower frequencies and less to higher.
void VisObject::distributeProportions() {
    unsigned int sum = 0;
    // Distribute LEDs equally.
    for (int i = 0; i < OCT_N; i++) ledProps[i] = NUM_LEDS / (4 * OCT_N);
    sum = sumP(ledProps);

    /*
     * LEDs can't always be equally distributed, so redistribute
     * first and last elements, to ensure distribution has the same
     * amount of LEDs as defined.
     */
    if (sum > NUM_LEDS / 2) {
        if ((NUM_LEDS / 2) % 2 ^ ledProps[0] % 2) {
            ledProps[OCT_N - 1] -= (sum - NUM_LEDS / 2) / 2;
            ledProps[0] -= 1;
        } else
            ledProps[OCT_N - 1] -= (sum - NUM_LEDS / 2) / 2;
    } else if (sum < NUM_LEDS / 2) {
        ledProps[0] += (NUM_LEDS / 2 - sum);
    }

    // Distribute LEDs again, giving largest significance to low frequencies.
    for (unsigned int i = 0; i < OCT_N - 1; i++) {
        unsigned int intm =
            (OCT_N / 2 - 1 + i * 2 > OCT_N - 1) ? OCT_N : OCT_N / 2 - 1 + i * 2;
        for (unsigned int j = i + 1; j < intm; j++) {
            if (!i && ledProps[j] > 1) {
                ledProps[i] += 2;
                ledProps[j] -= 1;
            } else if (ledProps[j] > 1) {
                ledProps[i] += 1;
                ledProps[j] -= 1;
            }
        }
    }
}

// Function which displays a symmetric spectrum of OCT_N frequency bins.
void VisObject::visAudioDoubleSpectrum() {
    unsigned char *widthArr = new unsigned char[ledProps[0]];
    // Map the magnitude of frequency to amount of LEDs to turn on and their
    // color.
    mapWidthx2(widthArr, ledProps[0], oct_bins[0]);
    // Values stored in abslt, which is then converted to RGB.
    for (unsigned int i = NUM_LEDS / 2 - 1;
         i > (NUM_LEDS / 2 - 1) - ledProps[0]; i--)
        leds[i].abslt = widthArr[(NUM_LEDS / 2 - 1) - i];
    // Do the same for the rest of the strip.
    int pos = (NUM_LEDS / 2 - 1) - ledProps[0];
    for (unsigned int i = 1; i < OCT_N; i++) {
        mapWidthx2(widthArr, ledProps[i], oct_bins[i]);
        for (unsigned int j = 0; j < ledProps[i]; j++) {
            if ((pos - ledProps[i] + 1 + j) >= 0)
                leds[pos - ledProps[i] + 1 + j].abslt = widthArr[j];
            if (pos - ledProps[i] - j >= 0)
                leds[pos - ledProps[i] - j].abslt = widthArr[j];
        }
        // Keep track of position where LEDs are turned on.
        pos = ((pos - ledProps[i] * 2) >= 0) ? pos - ledProps[i] * 2 : 0;
    }

    // if (col == 'M') state = !state;
    // if (state)
    // reverseOrder(leds);

    // Symmetrically show the FFT and convert abslt to RGB.
    for (uint16_t i = 0; i < NUM_LEDS / 2; i++)
        leds[NUM_LEDS - 1 - i].abslt = leds[i].abslt;
    abslToRGB();
    delete[] widthArr;
}

// Constantly check for a OCT_N time intervals and whatever bin the interval
// exceeds, the bin's LED lights up and the time interval restarts. Take care of
// possible overflows in this function.
void VisObject::visAudioPops() {
    unsigned int delays[OCT_N] = {MAX_POP_TIME};

    // Make static so they don't reinit every time function is called.
    static std::chrono::steady_clock::time_point startTimes[OCT_N] = {
        std::chrono::steady_clock::now()};
    static std::chrono::steady_clock::time_point endTimes[OCT_N] = {
        std::chrono::steady_clock::now()};

    // Decay lit up LEDs.
    popDecay();
    // See if any of the lit up LEDs are not lit up anymore and return them to
    // the vector of available LEDs
    for (int i = 0; i < popLedsBusy.size(); i++) {
        if (leds[popLedsBusy[i]].r == 0 && leds[popLedsBusy[i]].g == 0 &&
            leds[popLedsBusy[i]].b == 0) {
            popLedsAvail.push_back(popLedsBusy[i]);
            popLedsBusy.erase(popLedsBusy.begin() + i);
        }
    }

    // Map bin intensity to a delay value.
    mapDelays(delays);
    // Check if duration has passed, if it has, lit up corresponding color LED
    // and reset time.
    for (int i = 0; i < OCT_N; i++) {
        endTimes[i] = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTimes[i] - startTimes[i]);
        if (duration.count() > delays[i]) {
            if (oct_bins[i] > 0) popLedDist(i);
            startTimes[i] = std::chrono::steady_clock::now();
            endTimes[i] = std::chrono::steady_clock::now();
        }
    }
}

// Function to take an available LED from a vector and light it up.
void VisObject::popLedDist(int iter) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    for (int j = 0; j < NUM_LEDS_POP; j++) {
        std::uniform_int_distribution<> dis(0, popLedsAvail.size() - 1);
        int n = dis(gen);
        int ind = popLedsAvail[n];
        popLedsAvail.erase(popLedsAvail.begin() + n);
        popLedsBusy.push_back(ind);
        popLEDSet(iter, ind);
    }
}

// Map the delays on to a 2^x exponential.
void VisObject::mapDelays(unsigned int del[]) {
    const float a = log2f((float)MIN_POP_TIME / (float)MAX_POP_TIME) / maxPeak;
    const float b = log2f(MAX_POP_TIME);
    for (int i = 0; i < OCT_N; i++) {
        del[i] = powf(2, a * oct_bins[i] + b);
    }
}

// Function to set LED color for pop depending on bin frequency.
void VisObject::popLEDSet(int iter, int ind) {
    HsvColor hsv;

    hsv.h = 255 - ((255 - 150) / OCT_N) * iter;
    hsv.s = 180;
    hsv.v = 255;
    leds[ind] = HsvToRgb(hsv);
}

// Helper function to decay all LEDs.
void VisObject::popDecay() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].r = (leds[i].r < LED_DECAY) ? 0 : leds[i].r - LED_DECAY;
        leds[i].g = (leds[i].g < LED_DECAY) ? 0 : leds[i].g - LED_DECAY;
        leds[i].b = (leds[i].b < LED_DECAY) ? 0 : leds[i].b - LED_DECAY;
    }
}

// Helper function for the spectrum visualizer to convert absolute
// values to RGB. This is a bad implementation, really bad, needs to be
// revamped.
void VisObject::abslToRGB() {
    static float RCOEFF = 0.0F, GCOEFF = 0.99F, BCOEFF = 0.9F;

    // Set the color coefficients from Bluetooth received values
    switch (colorMode) {
        case 0:
            RCOEFF = 0;
            GCOEFF = mapLogCol(colorCoeff);
            BCOEFF = 1.7F - GCOEFF;
            break;
        case 1:
            RCOEFF = mapLogCol(colorCoeff);
            GCOEFF = 0;
            BCOEFF = 1.7F - RCOEFF;
            break;
        case 2:
            RCOEFF = mapLogCol(colorCoeff);
            GCOEFF = 1.7F - RCOEFF;
            BCOEFF = 0;
            break;
        default:
            break;
    }

    // Make sure main color turns on first and only then other colors follow.
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i].r =
            (leds[i].abslt > 255 * RCOEFF) ? leds[i].abslt - 255 * RCOEFF : 0;
        leds[i].g =
            (leds[i].abslt > 255 * GCOEFF) ? leds[i].abslt - 255 * GCOEFF : 0;
        leds[i].b =
            (leds[i].abslt > 255 * BCOEFF) ? leds[i].abslt - 255 * BCOEFF : 0;
    }
}

// Code found on stack overflow.
ledstruc VisObject::HsvToRgb(HsvColor hsv) {
    ledstruc rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0) {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6;

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.r = hsv.v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = hsv.v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = hsv.v;
            rgb.b = t;
            break;
        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v;
            rgb.g = p;
            rgb.b = q;
            break;
    }

    return rgb;
}

// Helper function for spectrum visualization to have a log
// scaling for the colors.
float VisObject::mapLogCol(unsigned int colCoeff) {
    const float a = 0.3F / logf(255);
    return a * logf(colCoeff + 1.0F) + 0.7F;
}

// Helper function for distribution of LEDs.
unsigned int VisObject::sumP(unsigned int arr[]) {
    unsigned int sum = 0;
    for (unsigned int i = 0; i < OCT_N; i++) {
        if (!i)
            sum += arr[i];
        else
            sum += arr[i] * 2;
    }

    return sum;
}