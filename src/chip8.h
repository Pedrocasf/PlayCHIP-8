#ifndef _CHIP8_H_
#define _CHIP8_H_
#include <stdint.h>
#include "pd_api.h"
typedef struct Chip8{
    uint8_t v[16];
    uint8_t mem[4096];
    uint16_t stack[16];
    uint8_t sp;
    uint8_t d;
    uint8_t s;
    uint16_t pc;
    uint16_t i;
    uint8_t disp[256];
    bool fb[2048];

} Chip8;
void init_from_file(char* file, Chip8**vm, PlaydateAPI* pd);
void init_from_header(Chip8**vm, PlaydateAPI* pd);
void run_frame(Chip8* vm, PlaydateAPI* pd);
#endif
