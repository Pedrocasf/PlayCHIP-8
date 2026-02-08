#ifndef _CHIP8_JIT_H_
#define _CHIP8_JIT_H_
#include <stdint.h>
#include <sys/types.h>
#include "pd_api.h"
typedef enum InstructionOP{
    HALT,
    CLS,
    RET,
    JMP,
    CALL,
    SEKK,
    SNEKK,
    SE,
    LDXK,
    ADDXK,
    LDXY,
    ORXY,
    ANDXY,
    XORXY,
    ADDXY,
    SUBXY,
    SHRX,
    SUBNXY,
    SHLX,
    SNE,
    LDI,
    JPV0,
    RNDX,
    DRAW,
    SKP,
    SKNP,
    LDXDT,
    LDXKK,
    LDDTX,
    LDSTX,
    ADDIX,
    LDFX,
    LDBX,
    LDIX,
    LDXI
} InstructionOP;
typedef struct OperandsXYZ{
    uint8_t x;
    uint8_t y;
    uint8_t z;
} OperandsXYZ;
typedef union Operands{
    uint16_t nnn;
    OperandsXYZ xyz;
} Operands;
typedef struct Instruction{
    InstructionOP instr;
    Operands ops;
} Instruction;
typedef struct Chip8{
    uint8_t v[16];
    uint8_t mem[4096];
    uint16_t stack[16];
    uint8_t sp;
    uint8_t dt;
    uint8_t st;
    uint16_t pc;
    uint16_t i;
    uint8_t disp[256];
    bool fb[2048];
    Instruction *instrs;
} Chip8;

void init_from_file(char* file, Chip8**vm, PlaydateAPI* pd);
void init_from_header(Chip8**vm, PlaydateAPI* pd);
void run_frame(Chip8* vm, PlaydateAPI* pd);
#endif
