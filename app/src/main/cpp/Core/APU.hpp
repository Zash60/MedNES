#pragma once

#include "Common/Typedefs.hpp"
#include <vector>
#include <mutex>

namespace MedNES {

class APU {
public:
    APU();
    void write(u16 address, u8 data);
    u8 read(u16 address); // Status register
    void tick(); // Chama a cada ciclo de CPU
    
    // Pega amostras acumuladas para enviar ao Android
    int getSamples(short* buffer, int maxLen);

private:
    // Audio Buffer
    std::vector<short> sampleBuffer;
    std::mutex bufferMutex;
    float sampleAccumulator = 0;
    
    // Pulse 1 Channel State
    bool p1_enabled = false;
    u8 p1_volume = 0;
    u8 p1_duty = 0;
    bool p1_halt = false;
    u16 p1_timer_load = 0;
    u16 p1_timer = 0;
    u8 p1_duty_pos = 0;
    
    // Frame Counter
    u32 cycleCount = 0;
    
    // Helper
    void generateSample();
};

}
