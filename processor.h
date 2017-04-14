#ifndef PROCESSOR_H_
#define PROCESSOR_H_
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMSIZE 1200

#define SP 29
#define FP 30
#define RA 31

int memory[MEMSIZE];
unsigned long clock_cycles;

typedef struct deconstructed_instruction {
  int opcode;
  int rs;
  int rt;
  int rd;
  int shamt;
  int funct;
  int immediate;
} inst;

typedef struct data {
  int mem;
  int rs;
  int rt;
  int rd;
  int immediate;
  int ALU_result;
} read_data;

typedef struct IFIDregister {
  int pc;
  int new_pc;
  int instruction;
} IFIDreg;

//IFID registers for the output of the IF stage
IFIDreg IFID;

typedef struct IDEXregister {
  int pc;
  int new_pc;

  inst instruction;
  read_data data;

  bool mem_read;
  bool mem_write;
  bool branch;
  bool reg_write;
  bool mem_to_reg;
  bool ALU_src;
} IDEXreg;

//Same deal as with IFID, but now with IDEX
IDEXreg IDEX;

typedef struct EXMEMregister {
  int pc;

  read_data data;
  inst instruction;

  bool mem_read;
  bool mem_write;
  bool branch;
  bool reg_write;
  bool mem_to_reg;
} EXMEMreg;

//and again!
EXMEMreg EXMEM

typedef struct MEMWBregister {
  int pc;

  read_data data;
  inst instruction;

  bool reg_write;
  bool mem_to_reg;
} MEMWBreg;

//AND ANOTHER ONE
MEMWBreg MEMWB

long ALU(read_data data, inst instruction);

void read_file(char* filename);

void hazard_detection();

bool clock();

void IF();
void ID();
void EX();
void MEM();
void WB();

#endif
