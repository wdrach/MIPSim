#include <stdio.h>
#include "processor.h"

int main() {
  init("./examples/ExampleProgramFileUnOptimized.txt");
  while(clock())
    ;

    printf("Results:\nmem[6]: %d\nmem[7]: %d\nmem[8]: %d\nmem[9]: %d\n", memory[6], memory[7], memory[8], memory[9]);

  printf("Simulation results:\nTotal Clock Cycles: %lu\n", clock_cycles);
}
