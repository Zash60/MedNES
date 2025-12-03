#pragma once

#include "Common/Typedefs.hpp"
#include <vector>
#include <atomic>

namespace MedNES {

// 4096 samples é suficiente para ~90ms de buffer a 44.1kHz
constexpr int AUDIO_BUFFER_SIZE = 4096;

class APU {
public:
    APU();
    void write(u16 address, u8 data);
    u8 read(u16 address);
    void tick();
    
    // Método thread-safe para ler samples sem locks
    int getSamples(short* outBuffer, int maxLen);

private:
    // --- Ring Buffer (Lock-free para Single Producer/Single Consumer) ---
    short buffer[AUDIO_BUFFER_SIZE];
    std::atomic<int> writeIndex {0};
    std::atomic<int> readIndex {0};

    void pushSample(short sample);

    // --- Channels ---

    struct Pulse {
        bool enabled = false;
        bool halt = false;    // Length Counter Halt / Envelope Loop
        u8 volume = 0;        // Fixed volume or envelope period
        u8 duty = 0;
        u16 timer = 0;
        u16 timerLoad = 0;
        u8 dutyPos = 0;
        u8 lengthValue = 0;
        bool sweepEnabled = false;
        
        short output() const;
        void tickTimer();
    } p1, p2;

    struct Triangle {
        bool enabled = false;
        bool control = false; // Length Counter Halt / Linear Counter Control
        u16 timer = 0;
        u16 timerLoad = 0;
        u8 seqPos = 0;
        u8 lengthValue = 0;
        u8 linearCounter = 0;
        u8 linearReload = 0;
        bool reloadLinear = false;

        short output() const;
        void tickTimer();
    } tri;

    struct Noise {
        bool enabled = false;
        bool halt = false;
        u8 volume = 0;
        u16 timer = 0;
        u16 timerPeriod = 0;
        u16 shiftReg = 1;
        bool mode = false; // Loop Noise
        u8 lengthValue = 0;

        short output() const;
        void tickTimer();
    } noise;

    // --- Frame Counter & Globals ---
    u32 frameCounter = 0;
    float sampleAccumulator = 0;
    
    void clockFrameCounter();
};

}
