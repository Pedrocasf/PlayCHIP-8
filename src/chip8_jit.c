#include "chip8_jit.h"
#include "roms/corax.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
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
    (*vm)->instr_blk = (InstructionBlock*)pd->system->realloc(NULL, 2048 * sizeof(InstructionBlock*));
    for(int i = 0x200;i<2048;i++){
        (*vm)->instr_blk[i].begin_addr = 0;
        (*vm)->instr_blk[i].end_addr = 0;
        (*vm)->instr_blk[i].instrs = (Instruction*)pd->system->realloc(NULL, sizeof(Instruction*));
    }
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
Instruction translate_op(Chip8*vm, uint16_t op, PlaydateAPI* pd, uint16_t idx){
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
    Instruction instr = {
        .opcode  = HALT,
        .ops = 0,
    };
    switch (op0) {
        case 0x0:
        switch (kk) {
            case 0xE0:
                //memset(vm->fb, 0, 2048);
                pd->system->logToConsole("CLS addr:%x", vm->pc);
                instr.opcode = CLS;
            break;
            case 0xEE:
                pd->system->logToConsole("RET addr:%x", vm->pc);
                instr.opcode = RET;
                /*
                vm->sp -= 1;
                vm->pc = vm->stack[vm->sp];
                */
            break;
            default:
                pd->system->error("instr 0 %x at addr %x not implemented", kk , vm->pc);
            break;
        }
        break;
        case 0x1:
            pd->system->logToConsole("JMP target %x at addr:%x", nnn,vm->pc);
            instr.opcode = JMP;
            instr.ops.nnn = nnn;
            //vm->pc = nnn;
        break;
        case 0x2:
            pd->system->logToConsole("CALL target %x at addr:%x", nnn,vm->pc);
            instr.opcode = CALL;
            instr.ops.nnn = nnn;
            /*
            vm->stack[vm->sp] = vm->pc;
            vm->sp++;
            vm->pc = nnn;
            */
        break;
        case 0x3:
            /*
            if(vx == kk)
                vm->pc+=2;
            */
            pd->system->logToConsole("SEKK addr:%x", vm->pc);
            instr.opcode = SEKK;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = kk;
        break;
        case 0x4:
            pd->system->logToConsole("SNEKK addr:%x", vm->pc);
            instr.opcode = SNEKK;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = kk;
            /*
            if(vx != kk)
                vm->pc+=2;
            */
        break;
        case 0x5:
            pd->system->logToConsole("SE addr:%x", vm->pc);
            instr.opcode = SE;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = y;
            /*
            if (vx == vy)
                vm->pc+=2;
            */
        break;
        case 0x6:
            //vm->v[x] = kk;
            pd->system->logToConsole("LDXK at addr:%x",vm->pc);
            instr.opcode = LDXK;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = kk;
        break;
        case 0x7:
            //vm->v[x] += kk;
            pd->system->logToConsole("ADDXK at addr:%x",vm->pc);
            instr.opcode = ADDXK;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = kk;
        break;
        case 0x8:
            switch (op3) {
                case 0x0:
                    //vm->v[x] = vy;
                    pd->system->logToConsole("LDXY at addr:%x",vm->pc);
                    instr.opcode = LDXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0x1:
                    //vm->v[x] = vx | vy;
                    pd->system->logToConsole("ORXY at addr:%x",vm->pc);
                    instr.opcode = ORXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0x2:
                    //vm->v[x] = vx & vy;
                    pd->system->logToConsole("ANDXY at addr:%x",vm->pc);
                    instr.opcode = ANDXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0x3:
                    //vm->v[x] = vx ^ vy;
                    pd->system->logToConsole("XORXY at addr:%x",vm->pc);
                    instr.opcode = XORXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0x4:
                    /*
                    uint16_t r = vx + vy;
                    vm->v[0xF] = r > 255;
                    vm->v[x] = r & 0xFF;
                    */
                    pd->system->logToConsole("ADDXY at addr:%x",vm->pc);
                    instr.opcode = ADDXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0x5:
                    /*
                    vm->v[0xF] = vx > vy;
                    vm->v[x] = vx - vy;
                    */
                    pd->system->logToConsole("SUBXY at addr:%x",vm->pc);
                    instr.opcode = SUBXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0x6:
                    /*
                    vm->v[0xf] = vx & 0x01;
                    vm->v[x] = vx >> 1;
                    */
                    pd->system->logToConsole("SHRX at addr:%x",vm->pc);
                    instr.opcode = SHRX;
                    instr.ops.xyz.x= x;
                break;
                case 0x7:
                    /*
                    vm->v[0xF] = vy > vx;
                    vm->v[x] = vy - vx;
                    */
                    pd->system->logToConsole("SUBNXY at addr:%x",vm->pc);
                    instr.opcode = SUBNXY;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                case 0xE:
                    /*
                    vm->v[0xF] = (vx & 0x80) == 0x80;
                    vm->v[x] = vx << 1;
                    */
                    pd->system->logToConsole("SHLX at addr:%x",vm->pc);
                    instr.opcode = SHLX;
                    instr.ops.xyz.x= x;
                    instr.ops.xyz.y = y;
                break;
                default:
                    pd->system->error("instr 8 %x at addr %x not implemented", op3, vm->pc);
                break;
            }
        break;
        case 0x9:
            /*
             if (vx != vy)
                vm->pc+=2;
            */
            pd->system->logToConsole("SNE addr:%x", vm->pc);
            instr.opcode = SNE;
            instr.ops.xyz.z = x;
            instr.ops.xyz.y = y;
        break;
        case 0xA:
            //vm->i = nnn;
            pd->system->logToConsole("LDI at addr:%x",vm->pc);
            instr.opcode = LDI;
            instr.ops.nnn = nnn;
        break;
        case 0xB:
            pd->system->logToConsole("JPV0 addr:%x", vm->pc);
            //vm->pc = (nnn + vm->v[0]) & 0x0FFF;
            instr.opcode = JPV0;
            instr.ops.nnn = nnn;
        break;
        case 0xC:
            /*
            uint8_t r = rand() & kk;
            vm->v[x] = r;
            */
            pd->system->logToConsole("RNDX at addr:%x",vm->pc);
            instr.opcode = RNDX;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = kk;
        break;
        case 0xD:
            //vm->v[0xF] = draw(vm, vx, vy, n, &vm->mem[vm->i]);
            pd->system->logToConsole("DRW at addr:%x",vm->pc);
            instr.opcode = DRAW;
            instr.ops.xyz.x= x;
            instr.ops.xyz.y = y;
            instr.ops.xyz.z = n;
        break;
        case 0xE:
            switch (kk) {
                case 0x9E:
                    //vm->pc += 2;
                    pd->system->logToConsole("SKP addr:%x", vm->pc);
                    instr.opcode = SKP;
                break;
                case 0xA1:
                    //vm->pc += 2;
                    pd->system->logToConsole("SKNP at addr:%x",vm->pc);
                    instr.opcode = SKNP;
                break;
                default:
                    pd->system->error("instr E kk %x at addr %x not implemented", kk, vm->pc);
                break;
            }
        break;
        case 0xF:
            switch (kk) {
                case 0x07:
                    /*
                    vm->v[x] = vm->dt;
                    */
                    pd->system->logToConsole("LDXDT at addr:%x",vm->pc);
                    instr.opcode = LDXDT;
                    instr.ops.xyz.x = x;

                break;
                case 0x0A:
                    /*
                    vm->v[x] = 0;
                    */
                    pd->system->logToConsole("LDXKK addr:%x", vm->pc);
                    instr.opcode = LDXKK;
                    instr.ops.xyz.x = x;
                break;
                case 0x15:
                    //vm->dt = vx;
                    pd->system->logToConsole("LDDTX at addr:%x",vm->pc);
                    instr.opcode = LDDTX;
                    instr.ops.xyz.x = x;
                break;
                case 0x18:
                    //vm->st = vx;
                    pd->system->logToConsole("LDSTX at addr:%x",vm->pc);
                    instr.opcode = LDSTX;
                    instr.ops.xyz.x = x;
                break;
                case 0x1E:
                    //vm->i += vx;
                    pd->system->logToConsole("ADDIX at addr:%x",vm->pc);
                    instr.opcode = ADDIX;
                    instr.ops.xyz.x = x;
                break;
                case 0x29:
                    //vm->i = vx * 5;
                    pd->system->logToConsole("LDFX at addr:%x",vm->pc);
                    instr.opcode = LDFX;
                    instr.ops.xyz.x = x;
                break;
                case 0x33:
                    /*
                    uint16_t i = vm->i;
                    vm->mem[i] = vx / 100;
                    vm->mem[i+1] = (vx/10)%10;
                    vm->mem[i+2] = (vx%100)%10;
                    */
                    pd->system->logToConsole("LDBX at addr:%x",vm->pc);
                    instr.opcode = LDBX;
                    instr.ops.xyz.x = x;
                break;
                case 0x55:
                    /*
                    uint16_t i_reg = vm->i;
                    for(int i = 0;i<=x+1; i++)
                        vm->mem[i_reg+i] = vm->v[i];
                    vm->i+=x+1;
                    */
                    pd->system->logToConsole("LDIX at addr:%x",vm->pc);
                    instr.opcode = LDIX;
                    instr.ops.xyz.x = x;
                break;
                case 0x65:
                    /*
                    for(int i = 0;i<=x; i++){
                        uint16_t i_reg = vm->i;
                        vm->v[i] = vm->mem[i_reg+i];
                    }
                    vm->i+=x+1;
                    */
                    pd->system->logToConsole("LDXI at addr:%x",vm->pc);
                    instr.opcode = LDXI;
                    instr.ops.xyz.x = x;
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
bool test_branch(InstructionOP op){
    switch(op){
        case SKP:
        case SKNP:
        case JPV0:
        case SNE:
        case SE:
        case SNEKK:
        case SEKK:
        case CALL:
        case JMP:
        case RET:
        return true;
        default:
        return false;
    }
}
void translate_program(Chip8* vm, PlaydateAPI* pd){
    uint16_t block = 0;
    uint16_t block_size = 1;
    uint16_t idx = 0;
    vm->instr_blk[block].instrs = (Instruction*)pd->system->realloc((vm->instr_blk[block].instrs), block_size*sizeof(Instruction));
    for(int i = 0x200;i<4096;i+=2){
        uint16_t opcode = (((uint16_t)vm->mem[i]) << 8) | (((uint16_t)vm->mem[i+1]));
        vm->pc = i;
        pd->system->logToConsole("translating instr:%x upper addr:%x",opcode, i);
        Instruction instr = translate_op(vm, opcode, pd, idx);
        bool is_branch = test_branch(instr.opcode);
        if(is_branch){
            block += 1;
            idx = 0;
            block_size = 1;
            pd->system->logToConsole("translating branch addr:%x",i);
        }else {
            vm->instr_blk[block].instrs = (Instruction*)pd->system->realloc((&(vm)->instr_blk[block].instrs), block_size*sizeof(Instruction));
            pd->system->logToConsole("translating block addr:%x",i);
            block_size++;
            idx++;
        }
        pd->system->logToConsole("translating low addr:%x",i);
    }

}
void run_instruction(Chip8* vm, PlaydateAPI* pd){
    uint16_t opcode = (((uint16_t)vm->mem[vm->pc]) << 8) | (((uint16_t)vm->mem[vm->pc+1]));
    vm->pc += 2;
    //exec_op(vm, opcode, pd);
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
