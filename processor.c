#include "processor.h"
//TODO: opcodes 0x18-0x2B not implemented in ALU

int memory[MEMSIZE] = {0};
int mem_start = 0;
int registers[32] = {0};
int pc = 1;
unsigned long clock_cycles = 0;

//bools for stalling
bool stall_IF = false;
bool stall_ID = false;
bool stall_EX = false;
bool stall_MEM = false;
bool stall_WB = false;

void read_file(char* filename) {
  FILE* fp = fopen(filename, "r");
  char buf[100];
  char* temp;

  //start strtok
  fgets(buf, 100, fp);
  temp = strtok(buf, ", ");
  int instruction = strtol(temp, NULL, 16);
  memory[0] = instruction;

  int i = 1;
  while(fgets(buf, 100, fp) != NULL) {
    temp = strtok(buf, ",");
    instruction = strtol(temp, NULL, 16);
    memory[i] = instruction;
    i++;
  }

  //set SP to end of memory
  registers[SP] = memory[0];
  registers[FP] = memory[1];
  pc = memory[5];
}

bool clock() {
  //run through the cycles in reverse order, so the registers
  //don't overwrite before they are used
  WB();
  MEM();
  EX();
  ID();
  IF();

  //reset all of the stalls
  stall_IF = false;
  stall_ID = false;
  stall_EX = false;
  stall_MEM = false;
  stall_WB = false;

  hazard_detection();

  clock_cycles++;

  if (pc <= 1) return false;
  return true;
}

bool step() {
  printf("\n\nStepping through code, clock running.\n");
  bool ret = clock();
  printf("\nPrinting Registers:");
  print_registers();
  printf("Press any key to step.\n");
  getchar();
  return ret;
}

void hazard_detection() {
}

void IF() {
  if (stall_IF) return;

  IFID.pc = pc;
  IFID.instruction = memory[pc];

  //increment PC
  pc++;
  IFID.new_pc = pc;
}

void ID() {
  if (stall_ID) return;

  //simple forwarding
  IDEX.pc = IFID.pc;
  IDEX.new_pc = IFID.new_pc;

  //Instruction Formats:
  //  R: opcode (6) | rs (5) | rt (5) | rd (5) | shamt (5) | funct (6)
  //  I: opcode (6) | rs (5) | rt (5) | IMM (16)
  //  J: opcode (6) | Pseudo-Address (26)

  int instruction = IDi.instruction;
}

void EX() {
  if (stall_EX) return;

  //simple copies
  EXMEM.pc = IDEX.pc;
  EXMEM.instruction = IDEX.instruction;
  EXMEM.mem_read = IDEX.mem_read;
  EXMEM.mem_write = IDEX.mem_write;
  EXMEM.branch = IDEX.branch;
  EXMEM.reg_write = IDEX.reg_write;
  EXMEM.mem_to_reg = IDEX.mem_to_reg;

}

void MEM() {
  if (stall_MEM) return;

  //simple copies
  //TODO: switch this to the new stuff

  int opcode = MEMi.instruction.opcode;

  //load instructions
  if (MEMi.memRead) {
    int full_word = memory[(MEMi.ALU_result - mem_start)/4];
#ifdef DEBUG
  printf("Loading from memory.\n");
#endif

    switch (opcode) {
      case 0x20:
        //load byte
      case 0x24: {
        //load unsigned byte
        int data = 0;

        switch (MEMi.ALU_result%4) {
          case 0:
            data = 0x000000FF & full_word;
            break;
          case 1:
            data = (0x0000FF00 & full_word)>>8;
            break;
          case 2:
            data = (0x00FF0000 & full_word)>>16;
            break;
          case 3:
            data = (0xFF000000 & full_word)>>24;
            break;
        }
        //sign extend if needed
        if (opcode == 0x20) data = data&0x80 ? data|0xFFFFFF00 : data;
        break;
      }
      case 0x21:
        //Load unsigned halfword
      case 0x25: {
        //Load halfword
        //very similar to byte, just different adjustments
        int data;
        switch(MEMi.ALU_result%2) {
          case 0:
            data = 0x0000FFFF & full_word;
            break;
          case 1:
            data = (0xFFFF0000 & full_word)>>16;
            break;
        }

        //sign extend
        if (opcode == 0x21) data = data&0x8000 ? data|0xFFFF0000 : data;
        break;
      }
      case 0x22:
        //Load word right (C division will do this for us)
      case 0x23:
        //Load word
        MEMo.data = full_word;
        break;
      case 0x26: {
        //Load word left
        int addr = (MEMi.ALU_result - mem_start)/4;
        addr += MEMi.ALU_result%4 == 0 ? 0 : 1;
        MEMo.data = memory[addr];
        break;
      }
    }
  }
  else MEMo.data = 0;

  //similar to all of the read stuff, just with write
  if (MEMi.memWrite) {
    int addr = (MEMi.ALU_result - mem_start)/4;
    switch (opcode) {
      case 0x28:
        //store byte
        switch (MEMi.ALU_result%4) {
          case 0:
            memory[addr] &= 0xFFFFFF00;
            memory[addr] += MEMi.valb;
            break;
          case 1:
            memory[addr] &= 0xFFFF00FF;
            memory[addr] += MEMi.valb<<8;
            break;
          case 2:
            memory[addr] &= 0xFF00FFFF;
            memory[addr] += MEMi.valb<<16;
            break;
          case 3:
            memory[addr] &= 0x00FFFFFF;
            memory[addr] += MEMi.valb<<24;
            break;
        }
        break;
      case 0x29:
        //store halfword
        switch (MEMi.ALU_result%8) {
          case 0:
            memory[addr] &= 0xFFFF0000;
            memory[addr] += MEMi.valb;
            break;
          case 1:
            memory[addr] &= 0x0000FFFF;
            memory[addr] += MEMi.valb;
            break;
        }
        break;
      case 0x2A:
        //store word left
        addr += MEMi.ALU_result%4 == 0 ? 0 : 1;
        memory[addr] = MEMi.valb;
        break;
      case 0x2B:
        //store word
      case 0x2e:
        //store word right
      case 0x38:
        //store conditional
        //TODO: properly implement this?
        memory[addr] = MEMi.valb;
        break;
    }
  }
}

void WB() {
  if (stall_WB) return;

  //if we need to write, write!
  if (WBi.RegWrite && WBi.dest != 0) registers[WBi.dest] = WBi.memToReg ? WBi.data : WBi.ALU_result;
}

long ALU(read_data data, inst instruction);
