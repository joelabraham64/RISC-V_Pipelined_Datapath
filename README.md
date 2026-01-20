# RISC-V Pipelined Datapath
## Requirements

To run this program, you will need to be in the directory with the following files:

- `main.c`
- `parser.c`
- `parser.h`
- `core.c`
- `core.h`
- `registers.c`
- `registers.h`
- `instruction_memory.h`
- `instruction.h`

## Installation

Run the following command in the terminal to compile the program:

```sh
gcc -o main main.c parser.c core.c registers.c -std=c99

After compiling, run the program with the following command:
./assembler trace_1
