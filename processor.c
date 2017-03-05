#include "processor.h"

int memory[MEMSIZE] = {0};
int mem_start = 0;
int registers[32] = {0};
int pc = 0;
bool branch_taken = false;

void init() {
  //zero out EVERYTHING

  emptyIFID.new_pc = 0;
  emptyIFID.instruction = 0;
  IFo = emptyIFID;
  IDi = emptyIFID;

  emptyIDEX.new_pc = 0;
  emptyIDEX.instruction = 0;
  emptyIDEX.vala = 0;
  emptyIDEX.valb = 0;
  emptyIDEX.memRead = false;
  emptyIDEX.memWrite = false;
  emptyIDEX.branch = false;
  emptyIDEX.memToReg = false;
  emptyIDEX.dest = 0;
  emptyIDEX.regDst = false;
  emptyIDEX.ALUOp = false;
  emptyIDEX.ALUSrc = false;
  IDo = emptyIDEX;
  EXi = emptyIDEX;

  emptyEXMEM.new_pc = 0;
  emptyEXMEM.ALU_result = 0;
  emptyEXMEM.valb = 0;
  emptyEXMEM.dest = 0;
  emptyEXMEM.memRead = false;
  emptyEXMEM.memWrite = false;
  emptyEXMEM.memToReg = false;
  EXo = emptyEXMEM;
  MEMi = emptyEXMEM;

  emptyMEMWB.data = 0;
  emptyMEMWB.dest = 0;
  emptyMEMWB.RegWrite = false;
  emptyMEMWB.memToReg = false;
  MEMo = emptyMEMWB;
  WBi = emptyMEMWB;
}

void clock() {
  //update registers
  IDi = IFo;
  EXi = IDo;
  MEMi = EXo;
  WBi = MEMo;

  //run through the cycles
  IF();
  ID();
  EX();
  if (!branch_taken) {
    MEM();
    WB();
  }

  branch_taken = false;
};

void IF() {
  IFo.new_pc = pc + 1;
  IFo.instruction = memory[pc];
}
void ID() {
  //the simple forwarding of some registers
  IDo.new_pc = IDi.new_pc;
  IDo.instruction = IDi.instruction;

  //TODO set vala, valb, memRead, memWrite, branch, memToReg, dest, regWrite
}

void branch(int addr) {
  branch_taken = true;
  pc = addr;
}

void branch_link(int addr) {
  registers[RA] = pc;
  branch(addr);
}

void EX() {
  //simple forwarding
  EXo.new_pc = EXi.new_pc;
  EXo.valb = EXi.valb;
  EXo.dest = EXi.dest;
  EXo.RegWrite = EXi.RegWrite;
  EXo.memRead = EXi.memRead;
  EXo.memWrite = EXi.memWrite;
  EXo.memToReg = EXi.memToReg;
  EXo.instruction = EXi.instruction;

  EXo.ALU_result = ALU(EXi.vala, EXi.valb, EXi.instruction);

  //branch & jump instructions
  //0x04, 0x01, 0x07, 0x06, 0x05, 0x02, 0x03, 0x00
  //TODO: Check that jump instructions are proper
  int opcode = (EXi.instruction & 0xFC000000)>>26;
  if (opcode < 0x08) {
    int vala = EXi.vala;
    int valb = EXi.valb;
    switch (opcode) {
      case 0x04:
        //branch on equal
        if (vala == valb) branch(EXo.ALU_result);
        break;
      case 0x05:
        if (vala != valb) branch(EXo.ALU_result);
        break;
      case 0x01: {
        //branch >= 0 and branch > 0
        if ((valb == 1 && vala >= 0) || (valb == 0 && vala < 0)) {
          branch(EXo.ALU_result);
        }
        //branch >= 0 and link
        else if ((valb == 0x11 && vala >= 0) || (valb == 0x10 && vala < 0)) {
          branch_link(EXo.ALU_result);
        }
        break;
      }
      case 0x07:
        if (vala > 0) branch(EXo.ALU_result);
        break;
      case 0x06:
        if (vala <= 0) branch(EXo.ALU_result);
        break;
      case 0x02:
        branch(EXo.ALU_result);
        break;
      case 0x03:
        branch(EXo.ALU_result);
        break;
      case 0x00: {
        int funct = EXi.instruction & 0x3F;
        if (funct == 0x09) branch_link(vala);
        else if (funct == 0x08) branch(vala);
        break;
      }
    }
  }
}

void MEM() {
  //simple forwarding
  MEMo.ALU_result = MEMi.ALU_result;
  MEMo.dest = MEMi.dest;
  MEMo.RegWrite = MEMi.RegWrite;
  MEMo.memToReg = MEMi.memToReg;

  //TODO: byte align? Do we have to divide this by 4?
  //if we need to read from memory, do so.
  int opcode = (MEMi.instruction & 0xFC000000)>>26;

  //load instructions
  if (MEMi.memRead) {
    int full_word = memory[(MEMi.ALU_result - mem_start)/4];

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
        if (opcode == 0x20) data = data&0x8000 ? data|0xFFFF0000 : data;
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
  //if we need to write, write!
  if (WBi.RegWrite) registers[WBi.dest] = WBi.memToReg ? WBi.data : WBi.ALU_result;
}

int ALU(int vala, int valb, int instruction) {
  return 0;
}
