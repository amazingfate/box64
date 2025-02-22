#ifndef __DYNAREC_H_
#define __DYNAREC_H_

typedef struct x64emu_s x64emu_t;

void DynaCall(x64emu_t* emu, uintptr_t addr); // try to use DynaRec... Fallback to EmuCall if no dynarec available

void x64test_init(x64emu_t* ref);
void x64test_check(x64emu_t* ref);

#endif // __DYNAREC_H_