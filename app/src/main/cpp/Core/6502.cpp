#include "6502.hpp"

#include <assert.h>

namespace MedNES {

void CPU6502::step() {
    if (ppu->genNMI()) {
        NMI();
        cycle = 0;
    }

    u8 instruction = fetchInstruction();
    executeInstruction(instruction);
    programCounter++;
}

inline void CPU6502::tick() {
    ppu->tick();
    ppu->tick();
    ppu->tick();
    ++cycle;
}

ExecutionState *CPU6502::getExecutionState() {
    ExecutionState *execState = new ExecutionState();

    execState->accumulator = accumulator;
    execState->xRegister = xRegister;
    execState->yRegister = yRegister;
    execState->statusRegister = statusRegister;
    execState->programCounter = programCounter;
    execState->stackPointer = stackPointer;
    execState->cycle = cycle;

    return execState;
}

void CPU6502::setProgramCounter(u16 pc) {
    programCounter = pc;
}

inline void CPU6502::LOG_EXEC(u8 instr) {
    execLog << std::hex << static_cast<int>(instr) << " ";
}

inline void CPU6502::LOG_PC() {
    u8 lsb = programCounter & 0xFF;
    u8 msb = programCounter >> 8;
    u16 pc = msb * 256 + lsb;
    execLog << std::hex << static_cast<int>(pc) << " ";
}

inline void CPU6502::LOG_CPU_STATE() {
    execLog << "   A:" << std::hex << static_cast<int>(accumulator) << " X:" << std::hex << static_cast<int>(xRegister) << " Y:" << std::hex << static_cast<int>(yRegister) << " P:" << std::hex << static_cast<int>(statusRegister) << " SP:" << std::hex << static_cast<int>(stackPointer);
}

inline void CPU6502::PRINT_LOG() {
    std::cout << execLog.str() << "\n";
    execLog.str("");
}

u8 CPU6502::fetchInstruction() {
    return read(programCounter);
}

inline void CPU6502::pushPC() {
    u8 lsb = programCounter & 0xFF;
    u8 msb = programCounter >> 8;
    pushStack(msb);
    pushStack(lsb);
}

//Interupts
void CPU6502::reset() {
    //init program counter = $FFFC, $FFFD
    programCounter = read(0xFFFD) * 256 + read(0xFFFC);
}

inline void CPU6502::irq() {
    pushPC();
    pushStack(statusRegister);
    u8 lsb = read(0xFFFE);
    u8 msb = read(0xFFFF);
    programCounter = msb * 256 + lsb;
}

inline void CPU6502::NMI() {
    SEI();
    pushPC();
    pushStack(statusRegister);
    u8 lsb = read(0xFFFA);
    u8 msb = read(0xFFFB);
    programCounter = msb * 256 + lsb;
    tick();
}

u16 CPU6502::immediate() {
    return ++programCounter;
}

u16 CPU6502::zeroPage() {
    u8 zeroPage = read(++programCounter);
    return zeroPage % 256;
}

u16 CPU6502::zeroPageX() {
    tick();
    u8 zeroPage = read(++programCounter);
    return (zeroPage + xRegister) % 256;
}

u16 CPU6502::zeroPageY() {
    u8 zeroPage = read(++programCounter);
    return (zeroPage + yRegister) % 256;
}

u16 CPU6502::absolute() {
    u8 lsb = read(++programCounter);
    u8 msb = read(++programCounter);
    u16 address = msb * 256 + lsb;

    return address;
}

u16 CPU6502::absoluteY(bool extraTick) {
    u8 lsb = read(++programCounter);
    u8 msb = read(++programCounter);
    u16 address = msb * 256 + lsb;

    if (extraTick) {
        tickIfToNewPage(address, address + yRegister);
    }

    return address + yRegister;
}

u16 CPU6502::absoluteX(bool extraTick) {
    u8 lsb = read(++programCounter);
    u8 msb = read(++programCounter);
    u16 address = msb * 256 + lsb;

    if (extraTick) {
        tickIfToNewPage(address, address + xRegister);
    }

    return address + xRegister;
}

u16 CPU6502::indirectX() {
    tick();
    u16 operand = (read(++programCounter) + xRegister) % 256;
    u8 lsb = read(operand);
    u8 msb = read((operand + 1) % 256);
    u16 address = msb * 256 + lsb;

    return address;
}

u16 CPU6502::indirectY(bool extraTick) {
    u16 operand = read(++programCounter);
    u8 lsb = read(operand);
    u8 msb = read((operand + 1) % 256);
    u16 address = (msb * 256 + lsb);

    if (extraTick) {
        tickIfToNewPage(address, address + yRegister);
    }

    return address + yRegister;
}

u16 CPU6502::relative() {
    int8_t offset = read(++programCounter);
    return programCounter + offset;
}

void CPU6502::tickIfToNewPage(u16 pc, u16 newPc) {
    u16 newPcMSB = newPc >> 8;
    u16 oldPcMSB = pc >> 8;

    if (newPcMSB != oldPcMSB) {
        tick();
    }
}

void CPU6502::executeInstruction(u8 instruction) {
    u16 addr = 0;
    switch (instruction) {
        // ADC
        case 0x69: ADC(read(immediate())); break;
        case 0x65: ADC(read(zeroPage())); break;
        case 0x75: ADC(read(zeroPageX())); break;
        case 0x6D: ADC(read(absolute())); break;
        case 0x7D: ADC(read(absoluteX(true))); break;
        case 0x79: ADC(read(absoluteY(true))); break;
        case 0x61: ADC(read(indirectX())); break;
        case 0x71: ADC(read(indirectY(true))); break;

        // AND
        case 0x29: AND(read(immediate())); break;
        case 0x25: AND(read(zeroPage())); break;
        case 0x35: AND(read(zeroPageX())); break;
        case 0x2D: AND(read(absolute())); break;
        case 0x3D: AND(read(absoluteX(true))); break;
        case 0x39: AND(read(absoluteY(true))); break;
        case 0x21: AND(read(indirectX())); break;
        case 0x31: AND(read(indirectY(true))); break;

        // ASL
        case 0x0A: accumulator = ASL_val(accumulator); tick(); break;
        case 0x06: addr = zeroPage(); write(addr, ASL_val(read(addr))); tick(); break;
        case 0x16: addr = zeroPageX(); write(addr, ASL_val(read(addr))); tick(); break;
        case 0x0E: addr = absolute(); write(addr, ASL_val(read(addr))); tick(); break;
        case 0x1E: addr = absoluteX(false); tick(); write(addr, ASL_val(read(addr))); tick(); break;

        // BRANCHES
        case 0x90: BCC(relative()); break;
        case 0xB0: BCS(relative()); break;
        case 0xF0: BEQ(relative()); break;
        case 0x30: BMI(relative()); break;
        case 0xD0: BNE(relative()); break;
        case 0x10: BPL(relative()); break;
        case 0x50: BVC(relative()); break;
        case 0x70: BVS(relative()); break;

        // BIT
        case 0x24: BIT(zeroPage()); break;
        case 0x2C: BIT(absolute()); break;

        case 0x00: BRK(); break;

        // FLAGS
        case 0x18: CLC(); break;
        case 0xD8: CLD(); break;
        case 0x58: CLI(); break;
        case 0xB8: CLV(); break;

        // CMP
        case 0xC9: CMP(read(immediate())); break;
        case 0xC5: CMP(read(zeroPage())); break;
        case 0xD5: CMP(read(zeroPageX())); break;
        case 0xCD: CMP(read(absolute())); break;
        case 0xDD: CMP(read(absoluteX(true))); break;
        case 0xD9: CMP(read(absoluteY(true))); break;
        case 0xC1: CMP(read(indirectX())); break;
        case 0xD1: CMP(read(indirectY(true))); break;

        // CPX
        case 0xE0: CPX(read(immediate())); break;
        case 0xE4: CPX(read(zeroPage())); break;
        case 0xEC: CPX(read(absolute())); break;

        // CPY
        case 0xC0: CPY(read(immediate())); break;
        case 0xC4: CPY(read(zeroPage())); break;
        case 0xCC: CPY(read(absolute())); break;

        // DEC
        case 0xC6: addr = zeroPage(); write(addr, DEC_val(read(addr))); break;
        case 0xD6: addr = zeroPageX(); write(addr, DEC_val(read(addr))); break;
        case 0xCE: addr = absolute(); write(addr, DEC_val(read(addr))); break;
        case 0xDE: addr = absoluteX(false); tick(); write(addr, DEC_val(read(addr))); break;

        case 0xCA: DEX(); break;
        case 0x88: DEY(); break;

        // EOR
        case 0x49: EOR(read(immediate())); break;
        case 0x45: EOR(read(zeroPage())); break;
        case 0x55: EOR(read(zeroPageX())); break;
        case 0x4D: EOR(read(absolute())); break;
        case 0x5D: EOR(read(absoluteX(true))); break;
        case 0x59: EOR(read(absoluteY(true))); break;
        case 0x41: EOR(read(indirectX())); break;
        case 0x51: EOR(read(indirectY(true))); break;

        // INC
        case 0xE6: addr = zeroPage(); write(addr, INC_val(read(addr))); break;
        case 0xF6: addr = zeroPageX(); write(addr, INC_val(read(addr))); break;
        case 0xEE: addr = absolute(); write(addr, INC_val(read(addr))); break;
        case 0xFE: addr = absoluteX(false); tick(); write(addr, INC_val(read(addr))); break;

        case 0xE8: INX(); break;
        case 0xC8: INY(); break;

        // JMP
        case 0x4C: JMP(absolute()); break;
        case 0x6C: JMP_Indirect(); break;

        // JSR
        case 0x20: JSR(absolute()); break;

        // LDA
        case 0xA9: LDA(read(immediate())); break;
        case 0xA5: LDA(read(zeroPage())); break;
        case 0xB5: LDA(read(zeroPageX())); break;
        case 0xAD: LDA(read(absolute())); break;
        case 0xBD: LDA(read(absoluteX(true))); break;
        case 0xB9: LDA(read(absoluteY(true))); break;
        case 0xA1: LDA(read(indirectX())); break;
        case 0xB1: LDA(read(indirectY(true))); break;

        // LDX
        case 0xA2: LDX(read(immediate())); break;
        case 0xA6: LDX(read(zeroPage())); break;
        case 0xB6: LDX(read(zeroPageY())); tick(); break;
        case 0xAE: LDX(read(absolute())); break;
        case 0xBE: LDX(read(absoluteY(true))); break;

        // LDY
        case 0xA0: LDY(read(immediate())); break;
        case 0xA4: LDY(read(zeroPage())); break;
        case 0xB4: LDY(read(zeroPageX())); break;
        case 0xAC: LDY(read(absolute())); break;
        case 0xBC: LDY(read(absoluteX(true))); break;

        // LSR
        case 0x4A: accumulator = LSR_val(accumulator); tick(); break;
        case 0x46: addr = zeroPage(); write(addr, LSR_val(read(addr))); tick(); break;
        case 0x56: addr = zeroPageX(); write(addr, LSR_val(read(addr))); tick(); break;
        case 0x4E: addr = absolute(); write(addr, LSR_val(read(addr))); tick(); break;
        case 0x5E: addr = absoluteX(false); tick(); write(addr, LSR_val(read(addr))); tick(); break;

        case 0xEA: NOP(); break;

        // ORA
        case 0x09: ORA(read(immediate())); break;
        case 0x05: ORA(read(zeroPage())); break;
        case 0x15: ORA(read(zeroPageX())); break;
        case 0x0D: ORA(read(absolute())); break;
        case 0x1D: ORA(read(absoluteX(true))); break;
        case 0x19: ORA(read(absoluteY(true))); break;
        case 0x01: ORA(read(indirectX())); break;
        case 0x11: ORA(read(indirectY(true))); break;

        // PUSH/PULL
        case 0x48: PHA(); break;
        case 0x08: PHP(); break;
        case 0x68: PLA(); break;
        case 0x28: PLP(); break;

        // ROL
        case 0x2A: accumulator = ROL_val(accumulator); tick(); break;
        case 0x26: addr = zeroPage(); write(addr, ROL_val(read(addr))); tick(); break;
        case 0x36: addr = zeroPageX(); write(addr, ROL_val(read(addr))); tick(); break;
        case 0x2E: addr = absolute(); write(addr, ROL_val(read(addr))); tick(); break;
        case 0x3E: addr = absoluteX(false); tick(); write(addr, ROL_val(read(addr))); tick(); break;

        // ROR
        case 0x6A: accumulator = ROR_val(accumulator); tick(); break;
        case 0x66: addr = zeroPage(); write(addr, ROR_val(read(addr))); tick(); break;
        case 0x76: addr = zeroPageX(); write(addr, ROR_val(read(addr))); tick(); break;
        case 0x6E: addr = absolute(); write(addr, ROR_val(read(addr))); tick(); break;
        case 0x7E: addr = absoluteX(false); tick(); write(addr, ROR_val(read(addr))); tick(); break;

        case 0x40: RTI(); break;
        case 0x60: RTS(); break;

        // SBC
        case 0xE9: case 0xEB: SBC(read(immediate())); break;
        case 0xE5: SBC(read(zeroPage())); break;
        case 0xF5: SBC(read(zeroPageX())); break;
        case 0xED: SBC(read(absolute())); break;
        case 0xFD: SBC(read(absoluteX(true))); break;
        case 0xF9: SBC(read(absoluteY(true))); break;
        case 0xE1: SBC(read(indirectX())); break;
        case 0xF1: SBC(read(indirectY(true))); break;

        // SET FLAGS
        case 0x38: SEC(); break;
        case 0xF8: SED(); break;
        case 0x78: SEI(); break;

        // STA
        case 0x85: STA(zeroPage()); break;
        case 0x95: STA(zeroPageX()); break;
        case 0x8D: STA(absolute()); break;
        case 0x9D: STA(absoluteX(false)); tick(); break;
        case 0x99: STA(absoluteY(false)); tick(); break;
        case 0x81: STA(indirectX()); break;
        case 0x91: STA(indirectY(false)); tick(); break;

        // STX
        case 0x86: STX(zeroPage()); break;
        case 0x96: STX(zeroPageY()); tick(); break;
        case 0x8E: STX(absolute()); break;

        // STY
        case 0x84: STY(zeroPage()); break;
        case 0x94: STY(zeroPageX()); break;
        case 0x8C: STY(absolute()); break;

        // TRANSFERS
        case 0xAA: TAX(); break;
        case 0xA8: TAY(); break;
        case 0xBA: TSX(); break;
        case 0x8A: TXA(); break;
        case 0x9A: TXS(); break;
        case 0x98: TYA(); break;

        // UNOFFICIAL OPCODES
        case 0x04: case 0x44: case 0x64: NOP(); zeroPage(); tick(); break; // NOP ZeroPage
        case 0x0C: NOP(); absolute(); tick(); break; // NOP Absolute
        
        case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: NOP(); zeroPageX(); tick(); break; // NOP ZPX
        case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: NOP(); break; // NOP Implicit
        case 0x80: NOP(); immediate(); tick(); break; // NOP Immediate

        case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: NOP(); absoluteX(true); tick(); break; // NOP AbsX

        case 0xA3: LAX(indirectX()); break;
        case 0xA7: LAX(zeroPage()); break;
        case 0xAF: LAX(absolute()); break;
        case 0xB3: LAX(indirectY(true)); break;
        case 0xB7: LAX(zeroPageY()); tick(); break;
        case 0xBF: LAX(absoluteY(true)); break;

        case 0x83: SAX(indirectX()); break;
        case 0x87: SAX(zeroPage()); break;
        case 0x8F: SAX(absolute()); break;
        case 0x97: SAX(zeroPageY()); tick(); break;

        case 0xC3: DCP(indirectX()); break;
        case 0xC7: DCP(zeroPage()); break;
        case 0xCF: DCP(absolute()); break;
        case 0xD3: DCP(indirectY(true)); break;
        case 0xD7: DCP(zeroPageX()); break;
        case 0xDB: DCP(absoluteY(true)); break;
        case 0xDF: DCP(absoluteX(true)); break;

        case 0xE3: ISB(indirectX()); break;
        case 0xE7: ISB(zeroPage()); break;
        case 0xEF: ISB(absolute()); break;
        case 0xF3: ISB(indirectY(true)); break;
        case 0xF7: ISB(zeroPageX()); break;
        case 0xFB: ISB(absoluteY(true)); break;
        case 0xFF: ISB(absoluteX(true)); break;

        case 0x03: SLO(indirectX()); break;
        case 0x07: SLO(zeroPage()); break;
        case 0x0F: SLO(absolute()); break;
        case 0x13: SLO(indirectY(false)); tick(); break;
        case 0x17: SLO(zeroPageX()); break;
        case 0x1B: SLO(absoluteY(false)); tick(); break;
        case 0x1F: SLO(absoluteX(false)); tick(); break;

        case 0x23: RLA(indirectX()); break;
        case 0x27: RLA(zeroPage()); break;
        case 0x2F: RLA(absolute()); break;
        case 0x33: RLA(indirectY(false)); tick(); break;
        case 0x37: RLA(zeroPageX()); break;
        case 0x3B: RLA(absoluteY(false)); tick(); break;
        case 0x3F: RLA(absoluteX(false)); tick(); break;

        case 0x43: SRE(indirectX()); break;
        case 0x47: SRE(zeroPage()); break;
        case 0x4F: SRE(absolute()); break;
        case 0x53: SRE(indirectY(false)); tick(); break;
        case 0x57: SRE(zeroPageX()); break;
        case 0x5B: SRE(absoluteY(false)); tick(); break;
        case 0x5F: SRE(absoluteX(false)); tick(); break;

        case 0x63: RRA(indirectX()); break;
        case 0x67: RRA(zeroPage()); break;
        case 0x6F: RRA(absolute()); break;
        case 0x73: RRA(indirectY(false)); tick(); break;
        case 0x77: RRA(zeroPageX()); break;
        case 0x7B: RRA(absoluteY(false)); tick(); break;
        case 0x7F: RRA(absoluteX(false)); tick(); break;

        default:
            std::cout << "Unkown instruction " << instruction;
            programCounter++;
            break;
    }
}

u8 CPU6502::memoryAccess(MemoryAccessMode mode, u16 address, u8 data) {
    // Optimization: Order branches by most likely execution path
    // 1. RAM (0-2000)
    // 2. Cartridge (8000+)
    // 3. PPU (2000-4000)
    
    if (address < 0x2000) {
        if (mode == MemoryAccessMode::READ) {
            tick();
            return ram.read(address);
        } else {
            ram.write(address, data);
            tick();
            return 0;
        }
    } 
    else if (address >= 0x8000) {
         if (mode == MemoryAccessMode::READ) {
            tick();
            return mapper->read(address);
        } else {
            mapper->write(address, data);
            tick();
            return 0;
        }
    }
    else if (address >= 0x2000 && address < 0x4000) {
        if (mode == MemoryAccessMode::READ) {
            tick();
            return ppu->read(address);
        } else {
            ppu->write(address, data);
            tick();
            return 0;
        }
    } 
    else if (address >= 0x4000 && address < 0x4018) {
        //COPY OAM
        if (address == 0x4014) {
            if (mode == MemoryAccessMode::READ) {
                std::cout << "No read access at 0x4014";
            } else {
                ppu->write(address, data);

                for (int i = 0; i < 256; i++) {
                    tick();
                    ppu->copyOAM(read(data * 256 + i), i);
                }
            }
        } else {
            if (mode == MemoryAccessMode::READ) {
                tick();
                return controller->read(address);
            } else {
                controller->write(address, data);
                tick();
                return 0;
            }
        }
        //APU I/O registers
    } else if (address >= 0x4018 && address < 0x4020) {
        //CPU test mode
    } else if (address >= 0x6000 && address < 0x8000) {
        if (mode == MemoryAccessMode::READ) {
            tick();
            return mapper->read(address);
        } else {
            mapper->write(address, data);
            tick();
            return 0;
        }
    }

    tick();
    return 0;
}

u8 CPU6502::read(u16 address) {
    return memoryAccess(MemoryAccessMode::READ, address, 0);
}

void CPU6502::write(u16 address, u8 data) {
    memoryAccess(MemoryAccessMode::WRITE, address, data);
}

inline void CPU6502::setSRFlag(CPU6502::StatusFlags flag, bool val) {
    if (val) {
        statusRegister |= (1 << flag);
    } else {
        statusRegister &= ~(1 << flag);
    }
}

inline void CPU6502::setNegative(bool val) {
    setSRFlag(StatusFlags::NEGATIVE, val);
}

inline void CPU6502::setOverflow(bool val) {
    setSRFlag(StatusFlags::OVERFLO, val);
}

inline void CPU6502::setBreak4(bool val) {
    setSRFlag(StatusFlags::BREAK4, val);
}

inline void CPU6502::setBreak5(bool val) {
    setSRFlag(StatusFlags::BREAK5, val);
}

inline void CPU6502::setDecimal(bool val) {
    setSRFlag(StatusFlags::DECIMAL, val);
}

inline void CPU6502::setInterruptDisable(bool val) {
    setSRFlag(StatusFlags::INTERRUPT, val);
}

inline void CPU6502::setZero(bool val) {
    setSRFlag(StatusFlags::ZERO, val);
}

inline void CPU6502::setCarry(bool val) {
    setSRFlag(StatusFlags::CARRY, val);
}

void CPU6502::pushStack(u8 data) {
    write(stackPointer + 256, data);
    stackPointer--;
}

u8 CPU6502::popStack() {
    stackPointer++;
    return read(stackPointer + 256);
}

void CPU6502::ADC(u8 data) {
    u8 carry = statusRegister & 1;
    u16 sum = data + accumulator + carry;
    u8 overflow = (accumulator ^ sum) & (data ^ sum) & 0x80;
    setCarry(sum > 0xFF);
    accumulator = sum;
    setNegative(accumulator & 0x80);
    setZero(accumulator == 0);
    setOverflow(overflow);
}

void CPU6502::AND(u8 data) {
    accumulator &= data;
    setNegative(accumulator & 0x80);
    setZero(accumulator == 0);
}

u8 CPU6502::ASL_val(u8 data) {
    u8 bit7 = data & 0x80;
    data <<= 1;
    setCarry(bit7);
    setNegative(data & 0x80);
    setZero(data == 0);
    return data;
}

void CPU6502::commonBranchLogic(bool expr, u16 resolvePC) {
    if (expr) {
        tickIfToNewPage(programCounter + 1, resolvePC + 1);
        programCounter = resolvePC;
        tick();
    } else {
        programCounter++;
        tick();
    }
}

void CPU6502::BCC(u16 resolvePC) {
    u8 carry = statusRegister & 1;
    commonBranchLogic(!carry, resolvePC);
}

void CPU6502::BCS(u16 resolvePC) {
    u8 carry = statusRegister & 1;
    commonBranchLogic(carry, resolvePC);
}

void CPU6502::BEQ(u16 resolvePC) {
    u8 zero = (statusRegister >> 1) & 1;
    commonBranchLogic(zero, resolvePC);
}

void CPU6502::BMI(u16 resolvePC) {
    u8 neg = (statusRegister >> 7) & 1;
    commonBranchLogic(neg, resolvePC);
}

void CPU6502::BNE(u16 resolvePC) {
    u8 zero = (statusRegister >> 1) & 1;
    commonBranchLogic(!zero, resolvePC);
}

void CPU6502::BPL(u16 resolvePC) {
    u8 neg = (statusRegister >> 7) & 1;
    commonBranchLogic(!neg, resolvePC);
}

void CPU6502::BVC(u16 resolvePC) {
    u8 overflow = (statusRegister >> 6) & 1;
    commonBranchLogic(!overflow, resolvePC);
}

void CPU6502::BVS(u16 resolvePC) {
    u8 overflow = (statusRegister >> 6) & 1;
    commonBranchLogic(overflow, resolvePC);
}

void CPU6502::BIT(u16 addr) {
    u8 data = read(addr);
    u8 result = accumulator & data;
    u8 data_bit6 = (data >> 6) & 1;
    u8 data_bit7 = (data >> 7) & 1;
    setZero(result == 0);
    setOverflow(data_bit6);
    setNegative(data_bit7);
}

void CPU6502::BRK() {
    programCounter++;
    programCounter++;
    pushPC();
    u8 statusRegCpy = statusRegister;
    statusRegCpy |= (1 << 4);
    //statusRegCpy |= (1 << 5);
    pushStack(statusRegCpy);
    u8 lsb = read(0xFFFE);
    u8 msb = read(0xFFFF);
    programCounter = msb * 256 + lsb - 1;
    tick();
}

void CPU6502::CLC() {
    setCarry(0);
    tick();
}

void CPU6502::CLD() {
    setDecimal(0);
    tick();
}

void CPU6502::CLI() {
    setInterruptDisable(0);
    tick();
}

void CPU6502::CLV() {
    setOverflow(0);
    tick();
}

void CPU6502::CMP(u8 data) {
    u8 cmp = accumulator - data;
    setCarry(accumulator >= data);
    setZero(accumulator == data);
    setNegative(cmp & 0x80);
}

void CPU6502::CPX(u8 data) {
    u8 cmp = xRegister - data;
    setCarry(xRegister >= data);
    setZero(xRegister == data);
    setNegative(cmp & 0x80);
}

void CPU6502::CPY(u8 data) {
    u8 cmp = yRegister - data;
    setCarry(yRegister >= data);
    setZero(yRegister == data);
    setNegative(cmp & 0x80);
}

u8 CPU6502::DEC_val(u8 data) {
    data--;
    setZero(data == 0);
    setNegative(data & 0x80);
    tick();
    return data;
}

void CPU6502::DEX() {
    xRegister--;
    setZero(xRegister == 0);
    setNegative(xRegister & 0x80);
    tick();
}

void CPU6502::DEY() {
    yRegister--;
    setZero(yRegister == 0);
    setNegative(yRegister & 0x80);
    tick();
}

void CPU6502::EOR(u8 data) {
    accumulator ^= data;
    setZero(accumulator == 0);
    setNegative(accumulator & 0x80);
}

u8 CPU6502::INC_val(u8 data) {
    data = data + 1;
    setZero(data == 0);
    setNegative(data & 0x80);
    tick();
    return data;
}

void CPU6502::INX() {
    xRegister++;
    setZero(xRegister == 0);
    setNegative(xRegister & 0x80);
    tick();
}

void CPU6502::INY() {
    yRegister++;
    setZero(yRegister == 0);
    setNegative(yRegister & 0x80);
    tick();
}

void CPU6502::JMP(u16 addr) {
    programCounter = addr - 1;
}

void CPU6502::JMP_Indirect() {
    u8 lsb = read(programCounter + 1);
    u8 msb = read(programCounter + 2);
    u16 address = msb * 256 + lsb;
    u8 lsbt = read(address);
    u16 msbAddress = (address & 0xFF) == 0xFF ? address & 0xFF00 : address + 1;
    u8 msbt = read(msbAddress);
    programCounter = msbt * 256 + lsbt - 1;
}

void CPU6502::JSR(u16 jumpAddress) {
    u8 lsb = programCounter & 0xFF;
    u8 msb = programCounter >> 8;
    pushStack(msb);
    pushStack(lsb);
    programCounter = jumpAddress - 1;
    tick();
}

void CPU6502::LDA(u8 data) {
    accumulator = data;
    setZero(accumulator == 0);
    setNegative(accumulator & 0x80);
}

void CPU6502::LDX(u8 data) {
    xRegister = data;
    setZero(xRegister == 0);
    setNegative(xRegister & 0x80);
}

void CPU6502::LDY(u8 data) {
    yRegister = data;
    setZero(yRegister == 0);
    setNegative(yRegister & 0x80);
}

u8 CPU6502::LSR_val(u8 data) {
    u8 bit0 = data & 1;
    data >>= 1;
    setCarry(bit0);
    setNegative(data & 0x80);
    setZero(data == 0);
    return data;
}

void CPU6502::NOP() {
    tick();
}

void CPU6502::ORA(u8 data) {
    accumulator |= data;
    setZero(accumulator == 0);
    setNegative(accumulator & 0x80);
}

void CPU6502::PHA() {
    pushStack(accumulator);
    tick();
}

void CPU6502::PHP() {
    u8 statusRegCpy = statusRegister;
    statusRegCpy |= (1 << 4);
    statusRegCpy |= (1 << 5);
    pushStack(statusRegCpy);
    tick();
}

void CPU6502::PLA() {
    accumulator = popStack();
    setNegative(accumulator & 0x80);
    setZero(accumulator == 0);
    tick();
    tick();
}

void CPU6502::PLP() {
    statusRegister = popStack();
    setBreak4(0);
    setBreak5(1);
    tick();
    tick();
}

u8 CPU6502::ROL_val(u8 data) {
    u8 bit7 = data & 0x80;
    data = (data << 1) | (statusRegister & 1);
    setCarry(bit7);
    setZero(data == 0);
    setNegative(data & 0x80);
    return data;
}

u8 CPU6502::ROR_val(u8 data) {
    u8 bit0 = data & 1;
    data = (data >> 1) | ((statusRegister & 1) << 7);
    setCarry(bit0);
    setZero(data == 0);
    setNegative(data & 0x80);
    return data;
}

void CPU6502::RTI() {
    statusRegister = popStack();
    setBreak4(0);
    setBreak5(1);
    u8 pcLsb = popStack();
    u8 pcMsb = popStack();
    programCounter = pcMsb * 256 + pcLsb - 1;
    tick();
    tick();
}

void CPU6502::RTS() {
    u8 pcLsb = popStack();
    u8 pcMsb = popStack();
    programCounter = pcMsb * 256 + pcLsb;
    tick();
    tick();
    tick();
}

void CPU6502::SBC(u8 data) {
    ADC(data ^ 0xFF);
}

void CPU6502::SEC() {
    setCarry(1);
    tick();
}

void CPU6502::SED() {
    setDecimal(1);
    tick();
}

void CPU6502::SEI() {
    setInterruptDisable(1);
    tick();
}

void CPU6502::STA(u16 addr) {
    write(addr, accumulator);
}

void CPU6502::STX(u16 addr) {
    write(addr, xRegister);
}

void CPU6502::STY(u16 addr) {
    write(addr, yRegister);
}

void CPU6502::TAX() {
    xRegister = accumulator;
    setZero(xRegister == 0);
    setNegative(xRegister & 0x80);
    tick();
}

void CPU6502::TAY() {
    yRegister = accumulator;
    setZero(yRegister == 0);
    setNegative(yRegister & 0x80);
    tick();
}

void CPU6502::TSX() {
    xRegister = stackPointer;
    setZero(xRegister == 0);
    setNegative(xRegister & 0x80);
    tick();
}

void CPU6502::TXA() {
    accumulator = xRegister;
    setZero(accumulator == 0);
    setNegative(accumulator & 0x80);
    tick();
}

void CPU6502::TXS() {
    stackPointer = xRegister;
    tick();
}

void CPU6502::TYA() {
    accumulator = yRegister;
    setZero(accumulator == 0);
    setNegative(accumulator & 0x80);
    tick();
}

//UNOFFICIAL OPCODES
//LDA+LDX
void CPU6502::LAX(u16 addr) {
    u8 data = read(addr);
    LDA(data);
    LDX(data);
}

//STA+acc&x
void CPU6502::SAX(u16 addr) {
    write(addr, accumulator & xRegister);
}

//DEC+CMP
void CPU6502::DCP(u16 address) {
    u8 data = DEC_val(read(address));
    write(address, data);
    CMP(data);
}

//INC+SBC
void CPU6502::ISB(u16 address) {
    u8 data = INC_val(read(address));
    write(address, data);
    SBC(data);
}

//ASL+ORA
void CPU6502::SLO(u16 address) {
    u8 data = ASL_val(read(address));
    write(address, data);
    ORA(data);
    tick();
}

//ROL+AND
void CPU6502::RLA(u16 address) {
    u8 data = ROL_val(read(address));
    write(address, data);
    AND(data);
    tick();
}

//LSR+EOR
void CPU6502::SRE(u16 address) {
    u8 data = LSR_val(read(address));
    write(address, data);
    EOR(data);
    tick();
}

//ROR+ADC
void CPU6502::RRA(u16 address) {
    u8 data = ROR_val(read(address));
    write(address, data);
    ADC(data);
    tick();
}

}  //namespace MedNES
