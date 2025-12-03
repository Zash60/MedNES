#pragma once

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "Common/Typedefs.hpp"
#include "Controller.hpp"
#include "Mapper/Mapper.hpp"
#include "PPU.hpp"
#include "RAM.hpp"

namespace MedNES {

struct ExecutionState {
    u8 accumulator;
    u8 xRegister;
    u8 yRegister;
    u16 programCounter;
    u8 stackPointer;
    u8 statusRegister;
    int cycle;
};

class CPU6502 {
    enum MemoryAccessMode {
        READ,
        WRITE
    };

    enum StatusFlags {
        NEGATIVE = 7,
        OVERFLO = 6,
        BREAK5 = 5,
        BREAK4 = 4,
        DECIMAL = 3,
        INTERRUPT = 2,
        ZERO = 1,
        CARRY = 0
    };

   public:
    CPU6502(Mapper *mapper, PPU *ppu, Controller *controller) : mapper(mapper), ppu(ppu), controller(controller){};
    u8 fetchInstruction();
    void executeInstruction(u8 instruction);
    u8 memoryAccess(MemoryAccessMode mode, u16 address, u8 data);
    u8 read(u16 address);
    void write(u16 address, u8 data);
    void run();
    void step();
    void reset();
    void setProgramCounter(u16 pc);
    ExecutionState *getExecutionState();

   private:
    //Arithmetic
    u8 accumulator = 0;
    u8 xRegister = 0;
    u8 yRegister = 0;

    //Other
    u16 programCounter = 0;
    u8 stackPointer = 0xFD;
    u8 statusRegister = 0x24;

    int cycle = 7;

    //Devices
    RAM ram;
    Mapper *mapper;
    PPU *ppu;
    Controller *controller;

    std::stringstream execLog;

    inline void setSRFlag(StatusFlags, bool);
    inline void setNegative(bool);
    inline void setOverflow(bool);
    inline void setBreak4(bool);
    inline void setBreak5(bool);
    inline void setDecimal(bool);
    inline void setInterruptDisable(bool);
    inline void setZero(bool);
    inline void setCarry(bool);

    //vectors
    inline void irq();
    inline void NMI();

    inline void LOG_EXEC(u8 instr);
    inline void LOG_PC();
    inline void LOG_CPU_STATE();
    inline void PRINT_LOG();

    inline void tick();

    //stack
    void pushStack(u8);
    u8 popStack();

    //addressing - return address
    u16 immediate();
    u16 zeroPage();
    u16 zeroPageX();
    u16 zeroPageY();
    u16 absolute();
    u16 absoluteX(bool);
    u16 absoluteY(bool);
    u16 indirectX();
    u16 indirectY(bool);
    u16 relative();

    // Opcodes - Direct Values or Addresses
    void ADC(u8); 
    void AND(u8);
    
    // ASL
    u8 ASL_val(u8);

    // Branch Helpers
    void commonBranchLogic(bool, u16);
    void BCC(u16);
    void BCS(u16);
    void BEQ(u16);
    void BMI(u16);
    void BNE(u16);
    void BPL(u16);
    void BVC(u16);
    void BVS(u16);

    void BIT(u16); // Reads memory inside
    void BRK();
    
    void CLC();
    void CLD();
    void CLI();
    void CLV();

    void CMP(u8);
    void CPX(u8);
    void CPY(u8);

    u8 DEC_val(u8);
    void DEX();
    void DEY();

    void EOR(u8);

    u8 INC_val(u8);
    void INX();
    void INY();

    void JMP(u16); // Absolute
    void JMP_Indirect(); // Indirect logic inside

    void JSR(u16);

    void LDA(u8);
    void LDX(u8);
    void LDY(u8);

    u8 LSR_val(u8);

    void NOP(); // Standard NOP
    void ORA(u8);

    void PHA();
    void PHP();
    void PLA();
    void PLP();

    u8 ROL_val(u8);
    u8 ROR_val(u8);

    void RTI();
    void RTS();

    void SBC(u8);

    void SEC();
    void SED();
    void SEI();

    // Stores take address
    void STA(u16);
    void STX(u16);
    void STY(u16);

    void TAX();
    void TAY();
    void TSX();
    void TXA();
    void TXS();
    void TYA();

    // Unofficial Helpers
    void LAX(u16);
    void SAX(u16);
    void DCP(u16);
    void ISB(u16);
    void SLO(u16);
    void RLA(u16);
    void RRA(u16);
    void SRE(u16);

    void tickIfToNewPage(u16, u16);

    inline void pushPC();
};

};  //namespace MedNES
