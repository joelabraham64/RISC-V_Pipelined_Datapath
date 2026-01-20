#include "core.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> // Include for memset if needed

// Initialize core function
core_t *init_core(instruction_memory_t *i_mem) {
    core_t *core = (core_t *)malloc(sizeof(core_t));
    if (core == NULL)
        return NULL;

    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tick_func;

    // Initialize data memory and register file
    memset(core->data_mem, 0, MEM_SIZE * sizeof(uint8_t));
    memset(core->reg_file, 0, NUM_REGISTERS * sizeof(signal_t));

    return core;
}

// Define tick function to manage core execution
bool tick_func(core_t *core) {
    // Step 1: Fetch
    unsigned instruction = fetch_instruction(core);

    // Step 2: Decode
    signal_t opcode = instruction & 0x7F;
    signal_t funct3 = (instruction >> 12) & 0x7;
    signal_t funct7 = (instruction >> 25) & 0x7F;

    control_signals_t signals;
    control_unit(opcode, &signals);

    //printf("Decoded opcode: 0x%02x, funct3: 0x%x, funct7: 0x%x\n", opcode, funct3, funct7);
    //printf("Control signals - ALUSrc: %d, MemtoReg: %d, RegWrite: %d, MemRead: %d, MemWrite: %d, Branch: %d, ALUOp: %d\n", 
        //signals.ALUSrc, signals.MemtoReg, signals.RegWrite, signals.MemRead, signals.MemWrite, signals.Branch, signals.ALUOp);

    // Generate immediate
    signal_t imm = imm_gen(instruction);
    //printf("Generated immediate: 0x%08x\n", imm);

    // Register values
    signal_t rs1_val = core->reg_file[(instruction >> 15) & 0x1F];
    signal_t rs2_val = core->reg_file[(instruction >> 20) & 0x1F];

    //printf("Register rs1 value: %lld, rs2 value: %lld\n", rs1_val, rs2_val);

    // Step 3: Execute
    signal_t ALU_input_1 = rs1_val;
    signal_t ALU_input_2 = MUX(signals.ALUSrc, rs2_val, imm);
    //printf("ALU inputs - input_1: %lld, input_2: %lld\n", ALU_input_1, ALU_input_2);

    // Determine ALU control signal explicitly
    signal_t ALU_ctrl_signal = ALU_control_unit(signals.ALUOp, funct7, funct3);
    //printf("ALU control signal: %d\n", ALU_ctrl_signal);

    // Execute ALU operation
    signal_t ALU_result, zero;
    ALU(ALU_input_1, ALU_input_2, ALU_ctrl_signal, &ALU_result, &zero);
    //printf("ALU result: %lld, Zero flag: %d\n", ALU_result, zero);

    // Step 4: Memory Access
    memory_access_stage(core, &signals, instruction, ALU_result, rs2_val);

    // Step 5: Write Back
    write_back_stage(core, &signals, instruction, ALU_result);

    // Step 6: PC Update
    update_pc_stage(core, &signals, imm, zero);

    // Step 7: Clock increment and halt condition check
    ++core->clk;

    // Halting condition: if the PC is beyond the last address of instruction memory
    if (core->PC > core->instr_mem->last->addr) {
        return false;
    }

    return true;
}

// Function to fetch the instruction from memory
unsigned fetch_instruction(core_t *core) {
    unsigned instruction = core->instr_mem->instructions[core->PC / 4].instruction;
    return instruction;
}

// Function to handle memory access
void memory_access_stage(core_t *core, control_signals_t *signals, unsigned instruction, signal_t ALU_result, signal_t rs2_val) {
    if (signals->MemWrite) {
        for (int i = 0; i < 8; i++) { // Handle 64-bit store
            core->data_mem[ALU_result + i] = (rs2_val >> (i * 8)) & 0xFF;
        }
    }

    if (signals->MemtoReg) {
        core->reg_file[(instruction >> 7) & 0x1F] = core->data_mem[ALU_result];
    }
}

// Function to write back to register file
void write_back_stage(core_t *core, control_signals_t *signals, unsigned instruction, signal_t ALU_result) {
    if (signals->RegWrite && !signals->MemtoReg) {
        core->reg_file[(instruction >> 7) & 0x1F] = ALU_result;
    }
}

// Function to handle branch and PC update
void update_pc_stage(core_t *core, control_signals_t *signals, signal_t imm, signal_t zero) {
    if (signals->Branch && zero) {
        core->PC += ShiftLeft1(imm);
    } else {
        core->PC += 4;
    }
}

// Control unit function to set control signals based on opcode
void control_unit(signal_t input, control_signals_t *signals) {
    if (input == 51) { // R-type
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
    } else if (input == 3) { // ld
        signals->ALUSrc = 1;
        signals->MemtoReg = 1;
        signals->RegWrite = 1;
        signals->MemRead = 1;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 19) { // addi and slli (I-type)
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
        //printf("Control signals set for I-type (addi or slli)\n");
    } else if (input == 35) { // sd (S-type)
        signals->ALUSrc = 1;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 1;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 99) { // beq (SB-type)
        signals->ALUSrc = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 1;
        signals->ALUOp = 1;
    }
}

// ALU control unit function to determine the operation based on ALUOp, Funct7, and Funct3
signal_t ALU_control_unit(signal_t ALUOp, signal_t Funct7, signal_t Funct3) {
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 0) {
        return 2;  // add
    }
    if (ALUOp == 2 && Funct7 == 32 && Funct3 == 0) {
        return 6;  // sub
    }
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 7) {
        return 0;  // and
    }
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 6) {
        return 1;  // or
    }
    if (ALUOp == 2 && Funct3 == 1) {
        return 3;  // slli
    }

    if (ALUOp == 0) {
        return 2;  // addition for ld/sd
    }

    return 0;
}

// Immediate generation function based on the opcode
signal_t imm_gen(signal_t input) {
    signal_t opcode = input & 0x7F;
    signal_t imm = 0;

    if (opcode == 3 || opcode == 19) { // I-type
        imm = (input >> 20) & 0xFFF;
        if (imm & 0x800) {
            imm |= 0xFFFFF000; // Sign extend to 32 bits
        }
        //printf("imm_gen (I-type) - Immediate: 0x%08x\n", imm);
    } else if (opcode == 35) { // S-type
        signal_t imm4_0 = (input >> 7) & 0x1F;
        signal_t imm11_5 = (input >> 25) & 0x7F;
        imm = (imm11_5 << 5) | imm4_0;
        if (imm & 0x800) {
            imm |= 0xFFFFF000; // Sign extend to 32 bits
        }
        //printf("imm_gen (S-type) - Immediate: 0x%08x\n", imm);
    } else if (opcode == 99) { // SB-type
        signal_t imm11 = (input >> 7) & 0x1;
        signal_t imm4_1 = (input >> 8) & 0xF;
        signal_t imm10_5 = (input >> 25) & 0x3F;
        signal_t imm12 = (input >> 31) & 0x1;
        imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);
        if (imm & 0x1000) {
            imm |= 0xFFFFE000; // Sign extend to 32 bits
        }
        //printf("imm_gen (SB-type) - Immediate: 0x%08x\n", imm);
    }

    return imm;
}

// ALU function to perform the operation based on the control signal
void ALU(signal_t input_0, signal_t input_1, signal_t ALU_ctrl_signal, signal_t *ALU_result, signal_t *zero) {
    if (ALU_ctrl_signal == 2) {
        *ALU_result = (input_0 + input_1);
        *zero = (*ALU_result == 0);
    }
    if (ALU_ctrl_signal == 6) {
        *ALU_result = (input_0 - input_1);
        *zero = (*ALU_result == 0);
    }
    if (ALU_ctrl_signal == 0) {
        *ALU_result = (input_0 & input_1);
        *zero = (*ALU_result == 0);
    }
    if (ALU_ctrl_signal == 1) {
        *ALU_result = (input_0 || input_1);
        *zero = (*ALU_result == 0);
    }
    if (ALU_ctrl_signal == 3) {  // 3 is for SLLI
        *ALU_result = input_0 << input_1;
        *zero = (*ALU_result == 0);
    }
}

// Multiplexer function for selecting between two inputs
signal_t MUX(signal_t sel, signal_t input_0, signal_t input_1) {
    return sel == 0 ? input_0 : input_1;
}

// Function for left shift operation
signal_t ShiftLeft1(signal_t input) {
    return input << 1;
}

// Function to print the state of the core registers
void print_core_state(core_t *core) {
    printf("Register file\n");
    for (int i = 0; i < NUM_REGISTERS; i++)
        printf("x%d \t: %lld\n", i, core->reg_file[i]);
}

// Function to print data memory for debugging
void print_data_memory(core_t *core, unsigned int start, unsigned int end) {
    if ((start >= MEM_SIZE) || (end > MEM_SIZE)) {
        printf("Address range [%d, %d) is invalid\n", start, end);
        return;
    }

    printf("Data memory: bytes (in hex) within address range [%d, %d)\n", start, end);
    for (int i = start; i < end; i++)
        printf("%d: \t %02x\n", i, core->data_mem[i]);
}

// Main simulator function
int simulator_main(instruction_memory_t *instr_mem) {
    core_t *core = init_core(instr_mem);
    if (core == NULL) {
        printf("Failed to initialize core\n");
        return -1;
    }

    while (tick_func(core)) {
        print_core_state(core);
        printf("Clock Cycle: %d\n", core->clk);
    }

    printf("Simulation completed\n");
    print_data_memory(core, 0, MEM_SIZE);

    // Free allocated memory
    free(core);

    return 0;
}
