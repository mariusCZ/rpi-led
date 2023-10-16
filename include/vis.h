#include <atomic>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "conf.h"
#include "patterns.h"

#define ARRAY_SIZE(stuff) (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ WS2811_TARGET_FREQ
#define GPIO_PIN 18
#define DMA 10
//#define STRIP_TYPE            WS2811_STRIP_RGB		//
// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE WS2811_STRIP_GBR  // WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW
//(NOT SK6812RGB)

// Decay rate for audio bins.
#define DECAY_RATE_MAX 1
#define DECAY_RATE_MIN 0.5

// Decay rate for pops.
// TODO: calculate these depending on amount of LEDs
#define MIN_POP_TIME 10
#define MAX_POP_TIME 2000
// Number of LEDs lit up per pop
#define NUM_LEDS_POP 4
// Pop LED decay rate.
#define LED_DECAY 5

class VisObject {
   public:
    VisObject();
    ~VisObject();

    // Public audio bin array and its mutex.
    float oct_bins[OCT_N] = {0};
    std::mutex oct_lock;
    // Thread finish atomic.
    std::atomic<bool> finished;

    // Audio bin decay atomics.
    std::atomic<float> decay_max{1.5};
    std::atomic<float> decay_min{1};

    // Mode atomics
    std::atomic<int> displayMode{0};
    std::atomic<int> audioMode{0};

    std::atomic<int> colorMode{0};
    std::atomic<int> colorCoeff{0};

    // Peak reset indicator atomic
    std::atomic<bool> maxPeakReset{false};

    void ledColorSet(unsigned long int color);
    void startVis();
    void stopVis();

   private:
    unsigned int maxPeak = 1;
    // Proportion array for spectrum.
    unsigned int ledProps[OCT_N];
    // Nicer RGB store for LEDs.
    ledstruc leds[NUM_LEDS];

    // Pattern object.
    Patterns patterns;

    // Thread running atomic.
    std::atomic<bool> threadRunning;

    std::thread mainThread;

    // LEDs available and busy for audio pops.
    std::vector<unsigned int> popLedsAvail;
    std::vector<unsigned int> popLedsBusy;

    void mainVisThread();
    void mainDisplayRoutine();

    float decayFun(float val);
    void mapWidthx2(unsigned char widthArr[], unsigned int widthLen,
                    float input);
    void mapDelays(unsigned int del[]);
    void popLEDSet(int iter, int ind);
    void popLedDist(int iter);
    void popDecay();
    void distributeProportions();
    void visAudioDoubleSpectrum();
    void visAudioPops();
    void abslToRGB();
    ledstruc HsvToRgb(HsvColor hsv);
    float mapLogCol(unsigned int colCoeff);
    unsigned int sumP(unsigned int arr[]);

    void setLeds();
    void decayLeds();
};