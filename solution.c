#include <stdio.h>
#include "processor.h"

int main() {
  read_file("./examples/loops.txt");
  init();
  while(step());
}
