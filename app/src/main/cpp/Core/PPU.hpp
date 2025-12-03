#pragma once

#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "Common/Typedefs.hpp"
#include "INESBus.hpp"
#include "Mapper/Mapper.hpp"

namespace MedNES {

struct Sprite {
    u8 y;
    u8 tileNum;
    u8 attr;
    u8 x;
    u8 id;
};

struct SpriteRenderEntity {
    u8 lo;
    u8 hi;
    u8 attr;
    u8 counter;
    u8 id;
    bool flipHorizontally;
    bool flipVertically;
    int shifted = 0;

    void shift() {
        if (shifted == 8) {
            return;
        }

        if (flipHorizontally) {
            lo >>= 1;
            hi >>= 1;
        } else {
            lo <<= 1;
            hi <<= 1;
        }

        shifted++;
    }
};

class PPU : public INESBus {
   public:
    PPU(Mapper *mapper) : mapper(mapper){
        // Reserva memória para evitar realocações durante a emulação
        spriteRenderEntities.reserve(8);
    };

    //cpu address space
    u8 read(u16 address);
    void write(u16 address, u8 data);

    //ppu address space
    u8 ppuread(u16 address);
    void ppuwrite(u16 address, u8 data);

    void tick();
    void copyOAM(u8, int);
    u8 readOAM(int);
    bool genNMI();
    bool generateFrame;
    
    // Buffer público para o JNI acessar diretamente
    uint32_t buffer[256 * 240] = {0};

   private:
    //Registers

    //$2000 PPUCTRL
    union {
        struct
        {
            unsigned baseNametableAddress : 2;
            unsigned vramAddressIncrement : 1;
            unsigned spritePatternTableAddress : 1;
            unsigned bgPatternTableAddress : 1;
            unsigned spriteSize : 1;
            unsigned ppuMasterSlaveSelect : 1;
            unsigned generateNMI : 1;
        };

        u8 val;
    } ppuctrl;

    //$2001 PPUMASK
    union {
        struct
        {
            unsigned greyScale : 1;
            unsigned showBgLeftmost8 : 1;
            unsigned showSpritesLeftmost8 : 1;
            unsigned showBg : 1;
            unsigned showSprites : 1;
            unsigned emphasizeRed : 1;
            unsigned emphasizeGreen : 1;
            unsigned emphasizeBlue : 1;
        };

        u8 val;
    } ppumask;

    //$2002 PPUSTATUS
    union {
        struct
        {
            unsigned leastSignificantBits : 5;
            unsigned spriteOverflow : 1;
            unsigned spriteZeroHit : 1;
            unsigned vBlank : 1;
        };

        u8 val;
    } ppustatus;

    u8 ppustatus_cpy = 0;
    u8 oamaddr = 0;    //$2003
    u8 oamdata = 0;    //$2004
    u8 ppuscroll = 0;  //$2005
    u8 ppu_read_buffer = 0;
    u8 ppu_read_buffer_cpy = 0;

    // Paleta NES NTSC padrão convertida para 0xAABBGGRR (Android Bitmap Little Endian)
    // Isso permite memcpy direto no JNI.
    u32 palette[64] = {
        0xFF7C7C7C, 0xFFFC0000, 0xFFBC0000, 0xFFBC2844, 0xFF840094, 0xFF2000A8, 0xFF0010A8, 0xFF001488,
        0xFF003050, 0xFF007800, 0xFF006800, 0xFF005800, 0xFF584000, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFBCBCBC, 0xFFF87800, 0xFFF85800, 0xFFE44468, 0xFFCC0094, 0xFF5800E4, 0xFF0038F8, 0xFF105CE4,
        0xFF007CAC, 0xFF00B800, 0xFF00A800, 0xFF44A800, 0xFF888800, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFF8F8F8, 0xFFFCBC3C, 0xFFFC8868, 0xFFF87898, 0xFFF878F8, 0xFF9858F8, 0xFF5878F8, 0xFF44A0FC,
        0xFF00B8F8, 0xFF18F8B8, 0xFF54D858, 0xFF98F858, 0xFFD8E800, 0xFF787878, 0xFF000000, 0xFF000000,
        0xFFFCFCFC, 0xFFFCE4A4, 0xFFF8B8B8, 0xFFF8A8D8, 0xFFF8A8F8, 0xFFC0A4F8, 0xFFB0D0F8, 0xFFA8E0FC,
        0xFF78D8F8, 0xFF78F8D8, 0xFFB8F8B8, 0xFFD8F8B8, 0xFFFCE0B0, 0xFFC0C0C0, 0xFF000000, 0xFF000000
    };

    //BG
    u8 bg_palette[16] = {0};
    u8 vram[2048] = {0};
    u16 v = 0, t = 0;
    u8 x = 0;
    int w = 0;
    u8 ntbyte, attrbyte, patternlow, patternhigh;
    u16 bgShiftRegLo;
    u16 bgShiftRegHi;
    u16 attrShiftReg1;
    u16 attrShiftReg2;
    u8 quadrant_num;

    //Sprites
    u8 sprite_palette[16] = {0};
    u16 spritePatternLowAddr, spritePatternHighAddr;
    int primaryOAMCursor = 0;
    int secondaryOAMCursor = 0;
    Sprite primaryOAM[64];
    Sprite secondaryOAM[8];
    Sprite tmpOAM;
    bool inRange = false;
    int inRangeCycles = 8;
    int spriteHeight = 8;

    std::vector<SpriteRenderEntity> spriteRenderEntities;
    SpriteRenderEntity out;

    Mapper *mapper;

    int scanLine = 0;
    int dot = 0;
    int pixelIndex = 0;
    bool odd = false;
    bool nmiOccured = false;

    //methods
    inline void copyHorizontalBits();
    inline void copyVerticalBits();
    inline bool isRenderingDisabled();
    inline void emitPixel();
    inline void fetchTiles();
    inline void xIncrement();
    inline void yIncrement();
    inline void reloadShiftersAndShift();
    inline void decrementSpriteCounters();
    u16 getSpritePatternAddress(const Sprite &, bool);
    void evalSprites();
    bool inYRange(const Sprite &);
    bool isUninit(const Sprite &);
};

};  //namespace MedNES
