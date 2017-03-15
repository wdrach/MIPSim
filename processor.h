#ifndef PROCESSOR_H_
#define PROCESSOR_H_
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMSIZE 1000

#define RA 31
#define SP 29

typedef struct deconstructed_instruction {
  int opcode;
  int rs;
  int rt;
  int rd;
  int shamt;
  int funct;
  int immediate;
} inst;

inst empty_inst;

typedef struct IFIDregister {
  int pc;
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
  int pc;
  int new_pc;
  inst instruction;
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
} IDEX;

//Same deal as with IFID, but now with IDEX
IDEX IDo;
IDEX EXi;
IDEX emptyIDEX;

typedef struct EXMEMregister {
  int pc;
  int new_pc;
  int ALU_result;
  int valb;
  int dest;
  inst instruction;

  bool memRead;
  bool memWrite;
  bool RegWrite;
  bool memToReg;
} EXMEM;

//and again!
EXMEM EXo;
EXMEM MEMi;
EXMEM emptyEXMEM;

typedef struct MEMWBregister {
  int pc;
  int data;
  int ALU_result;
  int dest;

  bool RegWrite;
  bool memToReg;
} MEMWB;

//AND ANOTHER ONE
MEMWB MEMo;
MEMWB WBi;
MEMWB emptyMEMWB;

long ALU(int vala, int valb, int valc, inst instruction);

void init();

void read_file(char* filename);

void print_registers();

bool clock();

bool step();

void IF();
void ID();
void EX();
void MEM();
void WB();

#endif
