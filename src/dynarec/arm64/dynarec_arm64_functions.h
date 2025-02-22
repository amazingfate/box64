#ifndef __DYNAREC_ARM_FUNCTIONS_H__
#define __DYNAREC_ARM_FUNCTIONS_H__

#include "../dynarec_native_functions.h"

// Get an FPU scratch reg
int fpu_get_scratch(dynarec_arm_t* dyn);
// Reset scratch regs counter
void fpu_reset_scratch(dynarec_arm_t* dyn);
// Get an x87 double reg
int fpu_get_reg_x87(dynarec_arm_t* dyn, int t, int n);
// Get an MMX double reg
int fpu_get_reg_emm(dynarec_arm_t* dyn, int emm);
// Get an XMM quad reg
int fpu_get_reg_xmm(dynarec_arm_t* dyn, int t, int xmm);
// Free a FPU/MMX/XMM reg
void fpu_free_reg(dynarec_arm_t* dyn, int reg);
// Reset fpu regs counter
void fpu_reset_reg(dynarec_arm_t* dyn);

// ---- Neon cache functions
// Get type for STx
int neoncache_get_st(dynarec_arm_t* dyn, int ninst, int a);
// Get if STx is FLOAT or DOUBLE
int neoncache_get_st_f(dynarec_arm_t* dyn, int ninst, int a);
// Get actual type for STx
int neoncache_get_current_st(dynarec_arm_t* dyn, int ninst, int a);
// Get actual STx is FLOAT or DOUBLE
int neoncache_get_current_st_f(dynarec_arm_t* dyn, int a);
// Back-propagate a change float->double
void neoncache_promote_double(dynarec_arm_t* dyn, int ninst, int a);
// Combine and propagate if needed (pass 1 only)
int neoncache_combine_st(dynarec_arm_t* dyn, int ninst, int a, int b);  // with stack current dyn->n_stack*

// FPU Cache transformation (for loops) // Specific, need to be written par backend
int fpuCacheNeedsTransform(dynarec_arm_t* dyn, int ninst);

// Undo the changes of a neoncache to get the status before the instruction
void neoncacheUnwind(neoncache_t* cache);

// Get if ED will have the correct parity. Not emiting anything. Parity is 2 for DWORD or 3 for QWORD
int getedparity(dynarec_native_t* dyn, int ninst, uintptr_t addr, uint8_t nextop, int parity, int delta);

const char* getCacheName(int t, int n);

#endif //__DYNAREC_ARM_FUNCTIONS_H__
