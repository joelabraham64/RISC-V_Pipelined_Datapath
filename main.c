/* Simulator for the RISC-V single cycle datapath.
 *
 * Build as follows:
 *  $make clean && make
 *
 * Execute as follows: 
 *  $./RISCV_core <trace file>
 *
 * Modified by: Naga Kandasamy
 * Date: August 8, 2024
 *
 * Student name(s): Joel Abraham
 * Date: 08/15/2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "core.h"
#include "parser.h"

int main(int argc, const char **argv)
{   
    if (argc != 2) {
        printf("Usage: %s %s\n", argv[0], "<trace-file>");
        exit(EXIT_FAILURE);  // Change EXIT_SUCCESS to EXIT_FAILURE
    }

    // Translate assembly instructions into binary format; store binary instructions into instruction memory.
    instruction_memory_t instr_mem;
    instr_mem.last = NULL;
    load_instructions(&instr_mem, argv[1]);

    // Initialize core with the instruction memory
    core_t* core = init_core(&instr_mem);
    if (core == NULL) {
        perror("Failed to initialize the core.");
        exit(EXIT_FAILURE);
    }

    // Simulate core 
    while (core->tick(core));
    printf("Simulation complete.\n");

    // Print register file 
    print_core_state(core);

    // Print data memory in the address range [start, end). Start address is inclusive, end address is exclusive.
    unsigned int start = 0;
    unsigned int end = 32;
    
    print_data_memory(core, start, end);

    free(core);  // Free allocated memory for the core object   
    return 0;
}
