#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "registers.h"
#include "instruction.h"

// Rename your custom getline function to avoid conflict with standard libraries
ssize_t my_getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = NULL;
    size_t size;
    int c;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        return -1;
    }

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size_t new_size = size + 128;
            char *new_bufptr = realloc(bufptr, new_size);
            if (new_bufptr == NULL) {
                free(bufptr);  // Free original buffer on failure
                return -1;
            }
            p = new_bufptr + (p - bufptr);  // Update p after realloc
            bufptr = new_bufptr;
            size = new_size;
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }
    *p = '\0';  // Null-terminate the string
    *lineptr = bufptr;
    *n = size;

    return p - bufptr;
}

void load_instructions(instruction_memory_t *i_mem, const char *trace) {
    printf("Loading trace file: %s\n", trace);

    FILE *fd = fopen(trace, "r");
    if (fd == NULL) {
        perror("Cannot open trace file.\n");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    addr_t PC = 0;
    int IMEM_index = 0;

    while ((read = my_getline(&line, &len, fd)) != -1) {
        if (read == -1) {
            perror("Error reading trace file.\n");
            free(line);
            fclose(fd);
            exit(EXIT_FAILURE);
        }

        i_mem->instructions[IMEM_index].addr = PC;
        char *raw_instr = strtok(line, " ");
        if (raw_instr == NULL) {
            continue; // Skip empty lines or invalid instructions
        }

        // Parse different instruction types
        if (strcmp(raw_instr, "add") == 0 || strcmp(raw_instr, "sub") == 0 ||
            strcmp(raw_instr, "sll") == 0 || strcmp(raw_instr, "srl") == 0 ||
            strcmp(raw_instr, "xor") == 0 || strcmp(raw_instr, "or") == 0 ||
            strcmp(raw_instr, "and") == 0) {
            parse_R_type(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
        } else if (strcmp(raw_instr, "addi") == 0 || strcmp(raw_instr, "lw") == 0 ||
                   strcmp(raw_instr, "jalr") == 0 || strcmp(raw_instr, "slli") == 0 ||
                   strcmp(raw_instr, "ld") == 0) {
            parse_I_type(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
        } else if (strcmp(raw_instr, "beq") == 0 || strcmp(raw_instr, "bne") == 0) {
            parse_SB_type(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
        } else if (strcmp(raw_instr, "sd") == 0) {
            parse_S_type(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
        } else {
            printf("Unknown instruction: %s\n", raw_instr);
        }

        IMEM_index++;
        PC += 4;
    }

    free(line);  // Free allocated memory for line
    fclose(fd);
}


void parse_R_type(char *opr, instruction_t *instr) {
    instr->instruction = 0;
    unsigned opcode = 51;
    unsigned funct3 = 0;
    unsigned funct7 = 0;

    if (strcmp(opr, "add") == 0) {
        funct3 = 0;
        funct7 = 0;
    } else if (strcmp(opr, "sub") == 0) {
        funct3 = 0;
        funct7 = 32;
    } else if (strcmp(opr, "and") == 0) {
        funct3 = 7;
        funct7 = 0;
    } else if (strcmp(opr, "or") == 0) {
        funct3 = 6;
        funct7 = 0;
    } else if (strcmp(opr, "xor") == 0) {
        funct3 = 4;
        funct7 = 0;
    } else if (strcmp(opr, "sll") == 0) {
        funct3 = 1;
        funct7 = 0;
    } else if (strcmp(opr, "srl") == 0) {
        funct3 = 5;
        funct7 = 0;
    } else if (strcmp(opr, "sltu") == 0) {
        funct3 = 3;
        funct7 = 0;
    } else if (strcmp(opr, "sra") == 0) {
        funct3 = 5;
        funct7 = 32;
    }
    
    char *reg = strtok(NULL, ", ");
    unsigned rd = reg_index(reg);

    reg = strtok(NULL, ", ");
    unsigned rs_1 = reg_index(reg);

    reg = strtok(NULL, ", ");
    reg[strlen(reg) - 1] = '\0';
    unsigned rs_2 = reg_index(reg);

    // Construct instruction
    instr->instruction |= opcode;
    instr->instruction |= (rd << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
    instr->instruction |= (funct7 << (7 + 5 + 3 + 5 + 5));
}

void parse_I_type(char *opr, instruction_t *instr) {
    instr->instruction = 0;
    unsigned opcode = 0;
    unsigned funct3 = 0;
    int imm = 0;  // Immediate value as a signed integer

    if (strcmp(opr, "lb") == 0) {
        funct3 = 0;
        opcode = 3;
    } else if (strcmp(opr, "lh") == 0) {
        funct3 = 1;
        opcode = 3;
    } else if (strcmp(opr, "lw") == 0) {
        funct3 = 2;
        opcode = 3;
    } else if (strcmp(opr, "ld") == 0) {
        funct3 = 3;
        opcode = 3;
    } else if (strcmp(opr, "lbu") == 0) {
        funct3 = 4;
        opcode = 3;
    } else if (strcmp(opr, "lhu") == 0) {
        funct3 = 5;
        opcode = 3;
    } else if (strcmp(opr, "lwu") == 0) {
        funct3 = 6;
        opcode = 3;
    } else if (strcmp(opr, "addi") == 0) {
        funct3 = 0;
        opcode = 19;
    } else if (strcmp(opr, "slli") == 0) {
        funct3 = 1;
        opcode = 19;
    }

    char *reg = strtok(NULL, ", ");
    unsigned rd = reg_index(reg);

    if (opcode == 3) {  // Load-type instructions
        char *imm_str = strtok(NULL, "(");
        imm = atoi(imm_str);  // Get the immediate (offset) value

        reg = strtok(NULL, ")");
        unsigned rs_1 = reg_index(reg);  // Get the source register (rs1)

        instr->instruction |= (imm & 0xFFF) << 20;  // imm[11:0] -> bits [31:20]
        instr->instruction |= (rs_1 << 15);         // rs1 -> bits [19:15]
    } else if (opcode == 19) {  // Arithmetic-type instructions
        reg = strtok(NULL, ", ");
        unsigned rs_1 = reg_index(reg);  // Get the source register (rs1)

        reg = strtok(NULL, ", ");
        imm = atoi(reg);  // Get the immediate value

        instr->instruction |= (imm & 0xFFF) << 20;  // imm[11:0] -> bits [31:20]
        instr->instruction |= (rs_1 << 15);         // rs1 -> bits [19:15]
    }

    instr->instruction |= (funct3 << 12);     // funct3 -> bits [14:12]
    instr->instruction |= (rd << 7);          // rd -> bits [11:7]
    instr->instruction |= opcode;             // opcode -> bits [6:0]
}

void parse_SB_type(char *opr, instruction_t *instr) {
    instr->instruction = 0;
    unsigned opcode = 99;
    unsigned funct3 = 0;

    if (strcmp(opr, "beq") == 0) {
        funct3 = 0;
    } else if (strcmp(opr, "bne") == 0) {
        funct3 = 1;
    } else if (strcmp(opr, "blt") == 0) {
        funct3 = 4;
    } else if (strcmp(opr, "bge") == 0) {
        funct3 = 5;
    } else if (strcmp(opr, "bltu") == 0) {
        funct3 = 6;
    } else if (strcmp(opr, "bgeu") == 0) {
        funct3 = 7;
    }

    char *reg = strtok(NULL, ", ");
    unsigned rs1 = reg_index(reg);
    reg = strtok(NULL, ", ");
    unsigned rs2 = reg_index(reg);
    int imm = atoi(strtok(NULL, ", "));

    unsigned imm12 = (imm >> 12) & 0x1;
    unsigned imm11 = (imm >> 11) & 0x1;
    unsigned imm10_5 = (imm >> 5) & 0x3F;
    unsigned imm4_1 = imm & 0xF;

    instr->instruction |= (imm12 << 31);        // imm[12]
    instr->instruction |= (imm11 << 7);         // imm[11]
    instr->instruction |= (imm10_5 << 25);      // imm[10:5]
    instr->instruction |= (imm4_1 << 8);        // imm[4:1]
    instr->instruction |= rs2 << 20;            // rs2
    instr->instruction |= rs1 << 15;            // rs1
    instr->instruction |= funct3 << 12;         // funct3
    instr->instruction |= opcode;               // opcode
}

void parse_S_type(char *opr, instruction_t *instr) {
    instr->instruction = 0;
    unsigned opcode = 35;
    unsigned funct3 = 0;

    if (strcmp(opr, "sd") == 0) {
        funct3 = 3;
    } else if (strcmp(opr, "sw") == 0) {
        funct3 = 2;
    } else if (strcmp(opr, "sh") == 0) {
        funct3 = 1;
    } else if (strcmp(opr, "sb") == 0) {
        funct3 = 0;
    }

    char *reg = strtok(NULL, ", ");
    unsigned rs2 = reg_index(reg);

    char *imm_str = strtok(NULL, "(");
    int imm = atoi(imm_str);  // Get the immediate (offset) value

    reg = strtok(NULL, ")");
    unsigned rs1 = reg_index(reg);  // Get the source register (rs1)

    unsigned imm4_0 = imm & 0x1F;  // imm[4:0]
    unsigned imm11_5 = (imm >> 5) & 0x7F;  // imm[11:5]

    instr->instruction |= (imm11_5 << 25);  // imm[11:5] -> bits [31:25]
    instr->instruction |= (rs2 << 20);      // rs2 -> bits [24:20]
    instr->instruction |= (rs1 << 15);      // rs1 -> bits [19:15]
    instr->instruction |= (funct3 << 12);   // funct3 -> bits [14:12]
    instr->instruction |= (imm4_0 << 7);    // imm[4:0] -> bits [11:7]
    instr->instruction |= opcode;           // opcode -> bits [6:0]
}


int reg_index(char *reg)
{
    unsigned i = 0;
    for (i; i < NUM_OF_REGS; i++) {
        if (strcmp(REGISTER_NAME[i], reg) == 0)
            break;
    }

    return i;
}
