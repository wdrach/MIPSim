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
}
