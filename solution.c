#include <stdio.h>
#include "processor.h"

int main() {
  read_file("./examples/simple.txt");
  init();
  while(step());
}
