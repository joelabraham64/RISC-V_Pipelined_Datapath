#include <sys/types.h>
#ifndef PARSER_H
#define PARSER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The GNU_SOURCE macro is already defined in the source file where you use the custom getline.
// It should be defined before including any standard headers, if needed.
#define _GNU_SOURCE

#include "instruction_memory.h"
#include "registers.h"

// Function prototypes
void load_instructions(instruction_memory_t *i_mem, const char *trace);
void parse_R_type(char *opr, instruction_t *instr);
void parse_I_type(char *opr, instruction_t *instr);
void parse_SB_type(char *opr, instruction_t *instr);
void parse_S_type(char *opr, instruction_t *instr);
int reg_index(char *reg);

#endif // PARSER_H
