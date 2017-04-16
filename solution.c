#include <stdio.h>
#include "processor.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("USAGE: ./run_sim <program_location>\n");
    return 0;
  }
  init(argv[1]);

  while(clock())
    ;

  printf("Simulation Results:\n\nReserved Memory:\n Location | Val (hex) | Val (dec) \n        6 |  %08x | %d\n        7 |  %08x | %d\n        8 |  %08x | %d\n        9 |  %08x | %d\n", memory[6], memory[6], memory[7], memory[7], memory[8], memory[8], memory[9], memory[9]);

  printf("\nTotal Clock Cycles: %lu\n", clock_cycles);
}
