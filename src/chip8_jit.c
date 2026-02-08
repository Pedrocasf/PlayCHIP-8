#include "chip8_jit.h"
#include "roms/corax.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
void translate(Chip8**vm, PlaydateAPI* pd){

}
void init_from_file(char* file, Chip8**vm, PlaydateAPI* pd){
    uint8_t font[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                    0x20, 0x60, 0x20, 0x20, 0x70, // 1
                    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                    0xF0, 0x80, 0xF0, 0x80, 0x80  //
    };
    *vm = (Chip8*)pd->system->realloc(NULL, sizeof(Chip8));
    memcpy((*vm)->mem,font,80);
    SDFile* rom = pd->file->open(file,kFileReadData);
    pd->file->seek(rom, 0, SEEK_END);
    const int rom_sz = pd->file->tell(rom);
    pd->file->seek(rom, 0, SEEK_SET);
    int res = pd->file->read(rom,(*vm)->mem+0x200, rom_sz);
    pd->file->close(rom);
}
void init_from_header(Chip8**vm, PlaydateAPI* pd){
    uint8_t font[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                    0x20, 0x60, 0x20, 0x20, 0x70, // 1
                    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                    0xF0, 0x80, 0xF0, 0x80, 0x80  //
    };
    *vm = (Chip8*)pd->system->realloc(NULL, sizeof(Chip8));
    memcpy((*vm)->mem,font,80);
    memcpy((*vm)->mem+0x200, CORAX, 761);
    (*vm)->instrs = (Instruction*)pd->system->realloc(NULL, 2048 * sizeof(Instruction));
    (*vm)->pc = 0x200;
}
bool draw(Chip8*vm, uint8_t x, uint8_t y, uint8_t n, uint8_t* spr){
    bool rv = false;
    for(int yline = 0; yline< n; yline++){
        uint8_t pixels = spr[yline];
        for(int xline = 0; xline<8 ; xline++){
            if((pixels & (0x80>>xline)) != 0){
                if(vm->fb[((x+xline)%64)+(((y+yline)%32)*64)] == true){
                    rv = true;
                }
                vm->fb[((x+xline)%64)+(((y+yline)%32)*64)]^=true;
            }
        };
    };
    return rv;
}
void exec_op(Chip8*vm, uint16_t op, PlaydateAPI* pd){
    uint8_t op0 = ((op & 0xF000) >> 12);
    uint8_t op1 = ((op & 0x0F00) >> 8);
    uint8_t op2 = ((op & 0x00F0) >> 4);
    uint8_t op3 = ((op & 0x000F) >> 0);
    uint8_t x = op1;
    uint8_t y = op2;
    uint8_t vx = vm->v[x];
    uint8_t vy = vm->v[y];
    uint8_t n = op3;
    uint8_t kk = op & 0x00FF;
    uint16_t nnn = op & 0x0FFF;
    switch (op0) {
        case 0x0:
        switch (kk) {
            case 0xE0:
                memset(vm->fb, 0, 2048);
                vm->instrs[vm->pc].instr = CLS;
                pd->system->logToConsole("CLS addr:%x", vm->pc);
            break;
            case 0xEE:
                vm->sp -= 1;
                vm->instrs[vm->pc].instr = RET;
                vm->pc = vm->stack[vm->sp];
                pd->system->logToConsole("RET addr:%x", vm->pc);
            break;
            default:
                 pd->system->error("instr 0 %x at addr %x not implemented", kk , vm->pc);
            break;
        }
        break;
        case 0x1:
            pd->system->logToConsole("JMP target %x at addr:%x", nnn,vm->pc);
            vm->instrs[vm->pc].instr = JMP;
            vm->instrs[vm->pc].ops.nnn = nnn;
            vm->pc = nnn;
        break;
        case 0x2:
            pd->system->logToConsole("CALL target %x at addr:%x", nnn,vm->pc);
            vm->stack[vm->sp] = vm->pc;
            vm->sp++;
            vm->instrs[vm->pc].instr = CALL;
            vm->instrs[vm->pc].ops.nnn = nnn;
            vm->pc = nnn;
        break;
        case 0x3:
            vm->instrs[vm->pc].instr = SEKK;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = kk;
            if(vx == kk)
                vm->pc+=2;
            pd->system->logToConsole("SEKK addr:%x", vm->pc);
        break;
        case 0x4:
            vm->instrs[vm->pc].instr = SNEKK;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = kk;
            if(vx != kk)
                vm->pc+=2;
            pd->system->logToConsole("SNEKK addr:%x", vm->pc);
        break;
        case 0x5:
            vm->instrs[vm->pc].instr = SE;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = y;
            if (vx == vy)
                vm->pc+=2;
            pd->system->logToConsole("SE addr:%x", vm->pc);
        break;
        case 0x6:
            vm->instrs[vm->pc].instr = LDXK;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = kk;
            vm->v[x] = kk;
            pd->system->logToConsole("LDXK at addr:%x",vm->pc);
        break;
        case 0x7:
            vm->instrs[vm->pc].instr = ADDXK;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = kk;
            vm->v[x] += kk;
            pd->system->logToConsole("ADDXK at addr:%x",vm->pc);
        break;
        case 0x8:
            switch (op3) {
                case 0x0:
                    vm->instrs[vm->pc].instr = LDXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    vm->v[x] = vy;
                    pd->system->logToConsole("LDXY at addr:%x",vm->pc);
                break;
                case 0x1:
                    vm->instrs[vm->pc].instr = ORXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    vm->v[x] = vx | vy;
                    pd->system->logToConsole("ORXY at addr:%x",vm->pc);
                break;
                case 0x2:
                    vm->instrs[vm->pc].instr = ANDXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    vm->v[x] = vx & vy;
                    pd->system->logToConsole("ANDXY at addr:%x",vm->pc);
                break;
                case 0x3:
                    vm->instrs[vm->pc].instr = XORXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    vm->v[x] = vx ^ vy;
                    pd->system->logToConsole("XORXY at addr:%x",vm->pc);
                break;
                case 0x4:
                    vm->instrs[vm->pc].instr = ADDXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    uint16_t r = vx + vy;
                    vm->v[0xF] = r > 255;
                    vm->v[x] = r & 0xFF;
                    pd->system->logToConsole("ADDXY at addr:%x",vm->pc);
                break;
                case 0x5:
                    vm->instrs[vm->pc].instr = SUBXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    vm->v[0xF] = vx > vy;
                    vm->v[x] = vx - vy;
                    pd->system->logToConsole("SUBXY at addr:%x",vm->pc);
                break;
                case 0x6:
                    vm->instrs[vm->pc].instr = SHRX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->v[0xf] = vx & 0x01;
                    vm->v[x] = vx >> 1;
                    pd->system->logToConsole("SHRX at addr:%x",vm->pc);
                break;
                case 0x7:
                    vm->instrs[vm->pc].instr = SUBNXY;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->instrs[vm->pc].ops.xyz.y = y;
                    vm->v[0xF] = vy > vx;
                    vm->v[x] = vy - vx;
                    pd->system->logToConsole("SUBNXY at addr:%x",vm->pc);
                break;
                case 0xE:
                    vm->instrs[vm->pc].instr = SHLX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->v[0xF] = (vx & 0x80) == 0x80;
                    vm->v[x] = vx << 1;
                    pd->system->logToConsole("SHLX at addr:%x",vm->pc);
                break;
                default:
                    pd->system->error("instr 8 %x at addr %x not implemented", op3, vm->pc);
                break;
            }
        break;
        case 0x9:
            vm->instrs[vm->pc].instr = SNE;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = y;
            if (vx != vy)
                vm->pc+=2;
                pd->system->logToConsole("SNE addr:%x", vm->pc);
        break;
        case 0xA:
            vm->instrs[vm->pc].instr = LDI;
            vm->instrs[vm->pc].ops.nnn = nnn;
            vm->i = nnn;
            pd->system->logToConsole("LDI at addr:%x",vm->pc);
        break;
        case 0xB:
            vm->instrs[vm->pc].instr = JPV0;
            vm->instrs[vm->pc].ops.nnn = nnn;
            pd->system->logToConsole("JPV0 addr:%x", vm->pc);
            vm->pc = (nnn + vm->v[0]) & 0x0FFF;
        break;
        case 0xC:
            vm->instrs[vm->pc].instr = RNDX;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = kk;
            uint8_t r = rand() & kk;
            vm->v[x] = r;
            pd->system->logToConsole("RNDX at addr:%x",vm->pc);
        break;
        case 0xD:
            vm->instrs[vm->pc].instr = DRAW;
            vm->instrs[vm->pc].ops.xyz.x = x;
            vm->instrs[vm->pc].ops.xyz.y = y;
            vm->instrs[vm->pc].ops.xyz.z = n;
            vm->v[0xF] = draw(vm, vx, vy, n, &vm->mem[vm->i]);
            pd->system->logToConsole("DRW at addr:%x",vm->pc);
        break;
        case 0xE:
            switch (kk) {
                case 0x9E:
                    vm->instrs[vm->pc].instr = SKP;
                    vm->pc += 2;
                    pd->system->logToConsole("SKP addr:%x", vm->pc);
                break;
                case 0xA1:
                    vm->instrs[vm->pc].instr = SKNP;
                    vm->pc += 2;
                    pd->system->logToConsole("SKNP at addr:%x",vm->pc);
                break;
                default:
                    pd->system->error("instr E kk %x at addr %x not implemented", kk, vm->pc);
                break;
            }
        break;
        case 0xF:
            switch (kk) {
                case 0x07:
                    vm->instrs[vm->pc].instr = LDXDT;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->v[x] = vm->dt;
                    pd->system->logToConsole("LDXDT at addr:%x",vm->pc);
                break;
                case 0x0A:
                    vm->instrs[vm->pc].instr = LDXKK;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->v[x] = 0;
                    pd->system->logToConsole("LDXKK addr:%x", vm->pc);
                break;
                case 0x15:
                    vm->instrs[vm->pc].instr = LDDTX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->dt = vx;
                    pd->system->logToConsole("LDDTX at addr:%x",vm->pc);
                break;
                case 0x18:
                    vm->instrs[vm->pc].instr = LDSTX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->st = vx;
                    pd->system->logToConsole("LDSTX at addr:%x",vm->pc);
                break;
                case 0x1E:
                    vm->instrs[vm->pc].instr = ADDIX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->i += vx;
                    pd->system->logToConsole("ADDIX at addr:%x",vm->pc);
                break;
                case 0x29:
                    vm->instrs[vm->pc].instr = LDFX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    vm->i = vx * 5;
                    pd->system->logToConsole("LDFX at addr:%x",vm->pc);
                break;
                case 0x33:
                    vm->instrs[vm->pc].instr = LDBX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    uint16_t i = vm->i;
                    vm->mem[i] = vx / 100;
                    vm->mem[i+1] = (vx/10)%10;
                    vm->mem[i+2] = (vx%100)%10;
                    pd->system->logToConsole("LDBX at addr:%x",vm->pc);
                break;
                case 0x55:
                    vm->instrs[vm->pc].instr = LDIX;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    uint16_t i_reg = vm->i;
                    for(int i = 0;i<=x+1; i++)
                        vm->mem[i_reg+i] = vm->v[i];
                    vm->i+=x+1;
                    pd->system->logToConsole("LDIX at addr:%x",vm->pc);
                break;
                case 0x65:
                    vm->instrs[vm->pc].instr = LDXI;
                    vm->instrs[vm->pc].ops.xyz.x = x;
                    for(int i = 0;i<=x; i++){
                        uint16_t i_reg = vm->i;
                        vm->v[i] = vm->mem[i_reg+i];
                    }
                    vm->i+=x+1;
                    pd->system->logToConsole("LDXI at addr:%x",vm->pc);
                break;
                default:
                    pd->system->error("instr F kk %x at addr %x not implemented", kk, vm->pc);
                break;
            }
            break;
        break;
        default:
            pd->system->error("instr op0 %x at addr %x not implemented", op0, vm->pc);
            break;
    };
}
void run_instruction(Chip8* vm, PlaydateAPI* pd){
    uint16_t opcode = (((uint16_t)vm->mem[vm->pc]) << 8) | (((uint16_t)vm->mem[vm->pc+1]));
    vm->pc += 2;
    exec_op(vm, opcode, pd);
}
void run_frame(Chip8* vm, PlaydateAPI* pd){
    for (int i = 0; i < 50; i++) {
        run_instruction(vm, pd);
    };
    if(vm->dt > 0)
        vm->dt -= 1;
    if(vm->st > 0)
        vm->st -= 1;
    memset(vm->disp, 0, 256);
    for(int i = 0; i<256; i++){
        vm->disp[i] |= vm->fb[(i*8)+0]<<7;
        vm->disp[i] |= vm->fb[(i*8)+1]<<6;
        vm->disp[i] |= vm->fb[(i*8)+2]<<5;
        vm->disp[i] |= vm->fb[(i*8)+3]<<4;
        vm->disp[i] |= vm->fb[(i*8)+4]<<3;
        vm->disp[i] |= vm->fb[(i*8)+5]<<2;
        vm->disp[i] |= vm->fb[(i*8)+6]<<1;
        vm->disp[i] |= vm->fb[(i*8)+7]<<0;
    }
}
