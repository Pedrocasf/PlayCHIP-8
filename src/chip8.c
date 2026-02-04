#include "chip8.h"
#include "roms/maze.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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
    memcpy((*vm)->mem+0x200, MAZE, 34);
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
        case 0x01:
            vm->pc = nnn;
        break;
        case 0x03:
            if(vx == kk){
                vm->pc+=2;
            }
        break;
        case 0x06:
            vm->v[x] = kk;
        break;
        case 0x07:
            vm->v[x] += kk;
        break;
        case 0x0A:
            vm->i = nnn;
        break;
        case 0x0C:
            uint8_t r = rand() & kk;
            vm->v[x] = r;
        break;
        case 0x0D:
            vm->v[0xF] = draw(vm, vx, vy, n, &vm->mem[vm->i]);
        break;
        default:
            pd->system->error("instr op0 %x not implemented", op0);
            break;
    };
}
void run_instruction(Chip8* vm, PlaydateAPI* pd){
    uint16_t opcode = (((uint16_t)vm->mem[vm->pc]) << 8) | (((uint16_t)vm->mem[vm->pc+1]));
    vm->pc += 2;
    exec_op(vm, opcode, pd);
}
void run_frame(Chip8* vm, PlaydateAPI* pd){
    for (int i = 0; i < 10; i++) {
        run_instruction(vm, pd);
    };
    for(int i = 0; i<256; i++){
        vm->disp[i] |= vm->fb[(i*8)+0]<<0;
        vm->disp[i] |= vm->fb[(i*8)+1]<<1;
        vm->disp[i] |= vm->fb[(i*8)+2]<<2;
        vm->disp[i] |= vm->fb[(i*8)+3]<<3;
        vm->disp[i] |= vm->fb[(i*8)+4]<<4;
        vm->disp[i] |= vm->fb[(i*8)+5]<<5;
        vm->disp[i] |= vm->fb[(i*8)+6]<<6;
        vm->disp[i] |= vm->fb[(i*8)+7]<<7;
    }
}
