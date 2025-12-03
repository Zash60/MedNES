#include "APU.hpp"
#include <cmath>

namespace MedNES {

// Tabela de Duty Cycle do NES
const u8 duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 25% neg
};

APU::APU() {
    sampleBuffer.reserve(4096);
}

void APU::write(u16 address, u8 data) {
    switch(address) {
        case 0x4000: // Pulse 1 Control
            p1_duty = (data >> 6) & 3;
            p1_halt = (data >> 5) & 1;
            p1_volume = data & 0x0F; // Envelope ignorado por simplicidade
            break;
        case 0x4002: // Pulse 1 Timer Low
            p1_timer_load = (p1_timer_load & 0xFF00) | data;
            break;
        case 0x4003: // Pulse 1 Timer High
            p1_timer_load = (p1_timer_load & 0x00FF) | ((data & 7) << 8);
            p1_duty_pos = 0; // Reset fase
            break;
        case 0x4015: // Status / Enable
            p1_enabled = (data & 1);
            if (!p1_enabled) p1_timer_load = 0; // Silencia
            break;
    }
}

u8 APU::read(u16 address) {
    if (address == 0x4015) {
        return (p1_timer_load > 0) ? 1 : 0; 
    }
    return 0;
}

void APU::tick() {
    // Pulse 1 Logic (Timer decrements every 2 CPU cycles effectively in NTSC)
    if (p1_timer > 0) {
        p1_timer--;
    } else {
        p1_timer = p1_timer_load;
        p1_duty_pos = (p1_duty_pos + 1) & 7;
    }

    // Downsampling: NES CPU ~1.78MHz -> Audio 44.1kHz
    // Ratio aprox 40.5 CPU cycles per Sample
    sampleAccumulator += 1.0f;
    if (sampleAccumulator >= 40.58f) {
        sampleAccumulator -= 40.58f;
        generateSample();
    }
}

void APU::generateSample() {
    short output = 0;

    // Se canal ativado e timer válido (> 8 evita frequências ultra altas inaudíveis)
    if (p1_enabled && p1_timer_load > 8) {
        if (duty_table[p1_duty][p1_duty_pos]) {
            // Volume 0-15 -> Amplitude
            output = (short)(p1_volume * 400); 
        } else {
            output = -(short)(p1_volume * 400);
        }
    }

    std::lock_guard<std::mutex> lock(bufferMutex);
    if (sampleBuffer.size() < 4096) { // Evita overflow se Android for lento
        sampleBuffer.push_back(output);
    }
}

int APU::getSamples(short* outBuffer, int maxLen) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    int count = 0;
    
    // Copia o que tiver disponível até maxLen
    int available = sampleBuffer.size();
    count = (available < maxLen) ? available : maxLen;

    for (int i = 0; i < count; i++) {
        outBuffer[i] = sampleBuffer[i];
    }
    
    // Remove os dados lidos (ineficiente para vector grande, mas ok para pequenas chunks)
    sampleBuffer.erase(sampleBuffer.begin(), sampleBuffer.begin() + count);
    
    return count;
}

}
