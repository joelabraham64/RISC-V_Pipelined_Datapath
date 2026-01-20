#ifndef __CORE_H__
#define __CORE_H__

#include "instruction_memory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define MEM_SIZE 1024       // Size of memory in bytes 
#define NUM_REGISTERS 32    // Size of register file 

typedef uint8_t byte_t;
typedef int64_t signal_t;
typedef int64_t register_t;
typedef uint64_t tick_t;
typedef uint64_t addr_t; // Change to match instruction.h

// Definition of the RISC-V core
typedef struct core_s {
    tick_t clk;                         // Core clock
    addr_t PC;                          // Program counter
    instruction_memory_t *instr_mem;    // Instruction memory 
    byte_t data_mem[MEM_SIZE];          // Data memory
    register_t reg_file[NUM_REGISTERS]; // Register file
    bool (*tick)(struct core_s *core);  // Simulate function pointer
} core_t;

// Definition of the various control signals
typedef struct control_signals_s {
    signal_t Branch;
    signal_t MemRead;
    signal_t MemtoReg;
    signal_t ALUOp;
    signal_t MemWrite;
    signal_t ALUSrc;
    signal_t RegWrite;
} control_signals_t;

// Function prototypes
core_t *init_core(instruction_memory_t *i_mem);
bool tick_func(core_t *core);
void print_core_state(core_t *core);
void print_data_memory(core_t *core, unsigned int start, unsigned int end);
void control_unit(signal_t input, control_signals_t *signals);
signal_t ALU_control_unit(signal_t ALUOp, signal_t funct7, signal_t funct3);
signal_t imm_gen(signal_t input);
void ALU(signal_t input_0, signal_t input_1, signal_t ALU_ctrl_signal, signal_t *ALU_result, signal_t *zero);
signal_t MUX(signal_t sel, signal_t input_0, signal_t input_1);
signal_t Add(signal_t input_0, signal_t input_1);
signal_t ShiftLeft1(signal_t input);

// Add function prototypes for missing functions
unsigned fetch_instruction(core_t *core);
void decode_instruction(unsigned instruction, core_t *core, control_signals_t *signals, signal_t *opcode, signal_t *funct3, signal_t *funct7, signal_t *imm, signal_t *rs1_val, signal_t *rs2_val);
void execute_stage(control_signals_t *signals, signal_t rs1_val, signal_t rs2_val, signal_t imm, signal_t *ALU_result, signal_t *zero);
void memory_access_stage(core_t *core, control_signals_t *signals, unsigned instruction, signal_t ALU_result, signal_t rs2_val);
void write_back_stage(core_t *core, control_signals_t *signals, unsigned instruction, signal_t ALU_result);
void update_pc_stage(core_t *core, control_signals_t *signals, signal_t imm, signal_t zero);

#endif
