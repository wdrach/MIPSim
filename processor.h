#ifndef PROCESSOR_H_
#define PROCESSOR_H_
#include <stdbool.h>

#define MEMSIZE 1000

int memory[MEMSIZE] = {0};
int mem_start = 0;
int registers[32] = {0};
int pc = 0;

typedef struct IFIDregister {
  int new_pc;
  int instruction;
} IFID;

//IFID registers for the output of the IF stage
IFID IFo;
//and the input of the ID stage
IFID IDi;
//and an empty struct
IFID emptyIFID;

typedef struct IDEXregister {
  int new_pc;
  int instruction;
  int vala;
  int valb;
  bool memRead;
  bool memWrite;
  bool branch;
  bool RegWrite;
  bool memToReg;
  int dest;

  //not sure about these
  bool regDst;
  bool ALUOp;
  bool ALUSrc;
}

//Same deal as with IFID, but now with IDEX
IDEX IDo;
IDEX EXi;
IDEX emptyIDEX;

typedef struct EXMEMregister {
  int new_pc;
  int ALU_result;
  int valb;
  int dest;

  bool memRead;
  bool memWrite;
  bool RegWrite;
  bool memToReg;
}

//and again!
EXMEM EXo;
EXMEM MEMi;
EXMEM emptyEXMEM;

typedef struct MEMWBregister {
  int data;
  int ALU_result;
  int dest;

  bool RegWrite;
  bool memToReg;
}

//AND ANOTHER ONE
MEMWB MEMo;
MEMWB WBi;
MEMWB emptyMEMWB;

void init();

void clock();

void IF();
void ID();
void EX();
void MEM();
void WB();

#endif
