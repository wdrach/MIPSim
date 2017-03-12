#include <stdio.h>
#include "processor.h"

int main() {
  read_file("./examples/test.txt");
  init();
  while(step());
}
