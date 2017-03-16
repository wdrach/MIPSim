#include <stdio.h>
#include "processor.h"

int main() {
  read_file("./examples/super_simple_loops.txt");
  init();
  while(step());
}
