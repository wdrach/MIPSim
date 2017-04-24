#include <stdio.h>
#include "processor.h"

int main(int argc, char* argv[]) {
  if (argc != 6 && argc != 5) {
    printf("USAGE: ./run_sim <program_location> <i size> <d size> <block size> [--write_through/-wt]\n");
    return 0;
  }

  bool wt = false;
  if (argc == 6 && (strcmp(argv[5], "--write_through") == 0 || strcmp(argv[5], "-wt") == 0)) {
    wt = true;
  }
  init(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), wt);

  while(clock())
    ;
}
