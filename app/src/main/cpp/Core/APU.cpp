#include "APU.hpp"
#include <algorithm>

namespace MedNES {

const u8 duty_seq[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 25% Negated
};

const u8 tri_seq[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

const u16 noise_period[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

// Length Counter Lookup Table
const u8 length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

APU::APU() {}

void APU::write(u16 addr, u8 data) {
    switch(addr) {
        // --- Pulse 1 ---
        case 0x4000: 
            p1.duty = (data >> 6) & 3; 
            p1.halt = data & 0x20; 
            p1.volume = data & 0xF; 
            break;
        case 0x4002: 
            p1.timerLoad = (p1.timerLoad & 0xFF00) | data; 
            break;
        case 0x4003: 
            p1.timerLoad = (p1.timerLoad & 0x00FF) | ((data & 7) << 8); 
            p1.dutyPos = 0; 
            if(p1.enabled) p1.lengthValue = length_table[data >> 3];
            break;
            
        // --- Pulse 2 ---
        case 0x4004: 
            p2.duty = (data >> 6) & 3; 
            p2.halt = data & 0x20; 
            p2.volume = data & 0xF; 
            break;
        case 0x4006: 
            p2.timerLoad = (p2.timerLoad & 0xFF00) | data; 
            break;
        case 0x4007: 
            p2.timerLoad = (p2.timerLoad & 0x00FF) | ((data & 7) << 8); 
            p2.dutyPos = 0;
            if(p2.enabled) p2.lengthValue = length_table[data >> 3];
            break;

        // --- Triangle ---
        case 0x4008: 
            tri.control = data & 0x80; 
            tri.linearReload = data & 0x7F; 
            break;
        case 0x400A: 
            tri.timerLoad = (tri.timerLoad & 0xFF00) | data; 
            break;
        case 0x400B: 
            tri.timerLoad = (tri.timerLoad & 0x00FF) | ((data & 7) << 8); 
            tri.reloadLinear = true;
            if(tri.enabled) tri.lengthValue = length_table[data >> 3];
            break;

        // --- Noise ---
        case 0x400C: 
            noise.halt = data & 0x20; 
            noise.volume = data & 0xF; 
            break;
        case 0x400E: 
            noise.mode = data & 0x80; 
            noise.timerPeriod = noise_period[data & 0xF]; 
            break;
        case 0x400F: 
            if(noise.enabled) noise.lengthValue = length_table[data >> 3]; 
            break;

        // --- Status / Control ---
        case 0x4015:
            p1.enabled = data & 1; if(!p1.enabled) p1.lengthValue = 0;
            p2.enabled = data & 2; if(!p2.enabled) p2.lengthValue = 0;
            tri.enabled = data & 4; if(!tri.enabled) tri.lengthValue = 0;
            noise.enabled = data & 8; if(!noise.enabled) noise.lengthValue = 0;
            break;
            
        case 0x4017:
            // Frame counter control (simplesmente reseta contagem para este exemplo)
            frameCounter = 0;
            break;
    }
}

u8 APU::read(u16 addr) {
    if (addr == 0x4015) {
        // Retorna status dos length counters
        return (p1.lengthValue > 0) | ((p2.lengthValue > 0) << 1) | 
               ((tri.lengthValue > 0) << 2) | ((noise.lengthValue > 0) << 3);
    }
    return 0;
}

// ================= Channels Output Logic =================

short APU::Pulse::output() const {
    if (lengthValue == 0 || timerLoad < 8) return 0;
    return duty_seq[duty][dutyPos] ? (volume * 100) : -(volume * 100);
}
void APU::Pulse::tickTimer() {
    if (timer > 0) {
        timer--;
    } else {
        timer = timerLoad;
        dutyPos = (dutyPos + 1) & 7;
    }
}

short APU::Triangle::output() const {
    // Triangle silencia se Linear ou Length forem zero
    if (lengthValue == 0 || linearCounter == 0 || timerLoad < 2) return 0;
    return (tri_seq[seqPos] - 7) * 50; 
}
void APU::Triangle::tickTimer() {
    if (timer > 0) {
        timer--;
    } else {
        timer = timerLoad;
        if (lengthValue > 0 && linearCounter > 0) {
            seqPos = (seqPos + 1) & 31;
        }
    }
}

short APU::Noise::output() const {
    if (lengthValue == 0 || (shiftReg & 1)) return 0;
    return (volume * 80);
}
void APU::Noise::tickTimer() {
    if (timer > 0) {
        timer--;
    } else {
        timer = timerPeriod;
        // Feedback bit calc
        u16 feedback = (shiftReg & 1) ^ ((shiftReg >> (mode ? 6 : 1)) & 1);
        shiftReg = (shiftReg >> 1) | (feedback << 14);
    }
}

void APU::clockFrameCounter() {
    // 240Hz clock signal (approx)
    // Atualiza Linear Counter (Tri) e Envelopes (Simplificado)
    if (tri.reloadLinear) {
        tri.linearCounter = tri.linearReload;
    } else if (tri.linearCounter > 0) {
        tri.linearCounter--;
    }
    
    if (!tri.control) tri.reloadLinear = false;

    // Length counters e Sweep são clockados na metade da taxa (120Hz approx)
    if (frameCounter % 2 == 0) {
        if (p1.lengthValue > 0 && !p1.halt) p1.lengthValue--;
        if (p2.lengthValue > 0 && !p2.halt) p2.lengthValue--;
        if (tri.lengthValue > 0 && !tri.control) tri.lengthValue--;
        if (noise.lengthValue > 0 && !noise.halt) noise.lengthValue--;
    }
}

void APU::tick() {
    // Timer Ticks (Frequência da CPU)
    p1.tickTimer();
    // Pulse 2 usa o mesmo clock
    if (p2.timer > 0) p2.timer--; else { p2.timer = p2.timerLoad; p2.dutyPos = (p2.dutyPos + 1) & 7; }
    tri.tickTimer();
    noise.tickTimer();

    // Frame Counter Approximation
    // CPU Clock ~1.789 MHz. Frame Counter 240Hz -> A cada ~7457 ciclos
    static int frameDiv = 0;
    if (++frameDiv >= 7457) {
        frameDiv = 0;
        frameCounter++;
        clockFrameCounter();
    }

    // Downsampling para saída de áudio (44.1kHz)
    // 1789773 / 44100 = ~40.58
    sampleAccumulator += 1.0f;
    if (sampleAccumulator >= 40.58f) {
        sampleAccumulator -= 40.58f;
        
        // Mixer simples
        short mix = p1.output() + p2.output() + tri.output() + noise.output();
        pushSample(mix);
    }
}

void APU::pushSample(short sample) {
    int next = (writeIndex + 1) % AUDIO_BUFFER_SIZE;
    if (next != readIndex) { 
        // Se buffer não está cheio, escreve
        buffer[writeIndex] = sample;
        writeIndex = next;
    }
}

int APU::getSamples(short* out, int maxLen) {
    int count = 0;
    int currentRead = readIndex.load();
    int currentWrite = writeIndex.load(); // Snapshot atômico

    while (count < maxLen && currentRead != currentWrite) {
        out[count++] = buffer[currentRead];
        currentRead = (currentRead + 1) % AUDIO_BUFFER_SIZE;
    }
    
    readIndex.store(currentRead);
    return count;
}

}
