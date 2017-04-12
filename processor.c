#include "processor.h"
//TODO: opcodes 0x18-0x2B not implemented in ALU

//#define DEBUG 0

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

void init() {
#ifdef DEBUG
  printf("Initializing everything\n");
#endif

  //zero out EVERYTHING
  empty_inst.opcode = 0;
  empty_inst.rs = 0;
  empty_inst.rt = 0;
  empty_inst.rd = 0;
  empty_inst.shamt = 0;
  empty_inst.funct = 0;
  empty_inst.immediate = 0;

  emptyIFID.pc = 0;
  emptyIFID.instruction = 0;
  IFo = emptyIFID;
  IDi = emptyIFID;

  emptyIDEX.pc = 0;
  emptyIDEX.instruction = empty_inst;
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

  emptyEXMEM.pc = 0;
  emptyEXMEM.instruction = empty_inst;
  emptyEXMEM.ALU_result = 0;
  emptyEXMEM.valb = 0;
  emptyEXMEM.dest = 0;
  emptyEXMEM.memRead = false;
  emptyEXMEM.memWrite = false;
  emptyEXMEM.memToReg = false;
  EXo = emptyEXMEM;
  MEMi = emptyEXMEM;

  emptyMEMWB.pc = 0;
  emptyMEMWB.data = 0;
  emptyMEMWB.dest = 0;
  emptyMEMWB.RegWrite = false;
  emptyMEMWB.memToReg = false;
  emptyMEMWB.instruction = empty_inst;
  MEMo = emptyMEMWB;
  WBi = emptyMEMWB;

#ifdef DEBUG
  printf("Initialization complete\n");
#endif
}

void read_file(char* filename) {
#ifdef DEBUG
  printf("Reading file\n");
#endif
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
#ifdef DEBUG
  printf("Reading file complete\n");
#endif
}

bool clock() {
#ifdef DEBUG
  printf("Starting clock cycle\n");
#endif
  //update registers
  if (!stall_ID) {
    IDi = IFo;
  }
  if (!stall_EX) {
    EXi = IDo;
  }
  if (!stall_MEM) {
    MEMi = EXo;
  }
  if (!stall_WB) {
    WBi = MEMo;
  }

  //run through the cycles
  IF();
  ID();
  EX();
  MEM();
  WB();

  hazard_detection();

  clock_cycles++;

  if (pc <= 0) return false;
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
  if (IDo.memWrite && (EXo.instruction.rt == IDo.instruction.rs || EXo.instruction.rt == IDo.instruction.rt)) {
#ifdef DEBUG
  printf("Load/Use hazard detected, stalled\n");
#endif
    printf("Load/Use stall disabled\n");
  /*
    stall_EX = true;
    stall_MEM = true;
    stall_WB = true;
   */
  }

  //EX forwarding
  if (EXo.RegWrite && EXo.instruction.rd != 0) {
#ifdef DEBUG
  printf("EX hazard detected, forwarding\n");
#endif
    if (EXo.instruction.rd == IDo.instruction.rs) {
      IDo.vala = EXo.ALU_result;
      return;
    }
    else if (EXo.instruction.rd == IDo.instruction.rt) {
      IDo.valb = EXo.ALU_result;
      return;
    }
  }

  //MEM forwarding
  if (MEMo.RegWrite && MEMo.instruction.rd != 0) {
#ifdef DEBUG
  printf("MEM hazard detected, forwarding\n");
#endif
    if (MEMo.instruction.rd == IDo.instruction.rs) {
      if (MEMo.memToReg) {
        IDo.vala = MEMo.data;
      }
      else {
        IDo.vala = MEMo.ALU_result;
      }
    }
    else if (MEMo.instruction.rd == IDo.instruction.rt) {
      if (MEMo.memToReg) {
        IDo.valb = MEMo.data;
      }
      else {
        IDo.valb = MEMo.ALU_result;
      }
    }
  }
}

void IF() {
#ifdef DEBUG
  printf("In IF stage, pc %d\n", pc);
#endif

  if (stall_IF) {
#ifdef DEBUG
  printf("IF stall\n");
#endif
    return;
  }

  IFo = emptyIFID;
  IFo.pc = pc;
  IFo.instruction = memory[pc];
  //increment PC
  pc++;
}

void ID() {
#ifdef DEBUG
  printf("In ID stage, pc %d\n", IDi.pc);
#endif

  if (stall_ID) {
#ifdef DEBUG
  printf("ID stall\n");
#endif
    return;
  }

  IDo = emptyIDEX;
  //the simple forwarding of some registers
  IDo.pc = IDi.pc;

  //Instruction Formats:
  //  R: opcode (6) | rs (5) | rt (5) | rd (5) | shamt (5) | funct (6)
  //  I: opcode (6) | rs (5) | rt (5) | IMM (16)
  //  J: opcode (6) | Pseudo-Address (26)

  int instruction = IDi.instruction;

  int opcode = (instruction & 0xFC000000)>>26;
#ifdef DEBUG
  printf("opcode: %08x\n", opcode);
#endif

  IDo.instruction.opcode = opcode;
  if (opcode == 0x00) {
    //R type instructions
    //deconstruct
    int funct = instruction&0x3F;
    IDo.instruction.funct = funct;
    IDo.instruction.rs = (instruction&0x03E00000)>>21;
    IDo.instruction.rt = (instruction&0x001F0000)>>16;
    IDo.instruction.shamt = (instruction & 0x7C0)>>6;
    IDo.vala = registers[IDo.instruction.rs];
    IDo.valb = registers[IDo.instruction.rt];
    IDo.dest = (instruction&0x0000F800)>>11;
    IDo.instruction.rd = IDo.dest;
    IDo.RegWrite = true;


    if (funct == 0x08 || funct == 0x09) {
      IDo.branch = true;
      IDo.RegWrite = false;
    }

#ifdef DEBUG
  printf("R type instruction detected. funct: %08x\n", funct);
#endif
  }
  else if (opcode == 0x02 || opcode == 0x03) {
    //J type instructions
    IDo.instruction.immediate = instruction & 0x3FFFFFF;
    IDo.valb = IDo.instruction.immediate;
    IDo.branch = true;

#ifdef DEBUG
  printf("J type instruction detected. jump immed: %08x\n", IDo.instruction.immediate);
#endif
  }
  else {
#ifdef DEBUG
  printf("I type instruction detected.\n");
#endif

    //I type instructions
    IDo.instruction.rs = (instruction&0x03E00000)>>21;
    IDo.vala = registers[IDo.instruction.rs];
    //sign extend
    IDo.valb = (int) ((short int) (instruction&0x0000FFFF));
    IDo.instruction.immediate = IDo.valb;
    IDo.dest = (instruction&0x001F0000)>>16;

    if (opcode == 0x01 || opcode == 0x04 || opcode == 0x05) {
      //branch instructions
      IDo.branch = true;
    }
    else if (opcode > 0x20) {
      //load/store instructions
      if ((opcode > 0x20 && opcode < 0x28) || opcode == 0x30) {
        //load instructions
        IDo.memRead = true;
        IDo.memToReg = true;
        IDo.RegWrite = true;
      }
      else {
        //store instructions
        IDo.memWrite = true;
      }
    }
    else {
      //arithmetic instructions
      IDo.RegWrite = true;
    }
  }
  
}

void branch(int addr) {
  pc = (addr-mem_start)/4;

#ifdef DEBUG
  printf("Branching to new pc %d.\n", pc);
#endif

  //clear everything
  IFo = emptyIFID;
  IDi = emptyIFID;
  IDo = emptyIDEX;
  EXi = emptyIDEX;
  EXo = emptyEXMEM;
}

void branch_link(int addr) {
  registers[RA] = mem_start + pc*4;
  branch(addr);
}

void EX() {
#ifdef DEBUG
  printf("In EX stage.\n");
#endif

  if (stall_EX) {
#ifdef DEBUG
  printf("Stalling EX.\n");
#endif
    return;
  }

  EXo = emptyEXMEM;
  //simple forwarding
  EXo.dest = EXi.dest;
  EXo.RegWrite = EXi.RegWrite;
  EXo.memRead = EXi.memRead;
  EXo.memWrite = EXi.memWrite;
  EXo.memToReg = EXi.memToReg;
  EXo.instruction = EXi.instruction;
  EXo.pc = EXi.pc;

  int vala = EXi.vala;
  int valb = EXi.valb;
  int valc = 0;
  inst instruction = EXi.instruction;
  int opcode = instruction.opcode;
#ifdef DEBUG
  printf("Opcode: %02x\n", opcode);
#endif

  if (opcode == 0x04 || opcode == 0x05) {
    vala = registers[EXi.dest];
    valb = EXi.vala;
    valc = EXi.valb;
  }
  //shamt!
  if (opcode == 0x00) {
    int funct = instruction.funct;
#ifdef DEBUG
  printf("Funct: %02x\n", funct);
#endif
    if (funct == 0x00 || funct == 0x02 || funct == 0x03) {
      vala = valb;
      valb = instruction.shamt;
    }
    else if (funct == 0x0A || funct == 0x0B) {
      valc = registers[EXi.dest];
    }
  }

  EXo.valb = valb;

  EXo.ALU_result = ALU(vala, valb, valc, instruction);
#ifdef DEBUG
  printf("vala: %04x | valb: %04x | valc: %04x | result: %04x\n", vala, valb, valc, EXo.ALU_result);
#endif
  //TODO: implement high/low register instructions
  //     - Do we need to do this with no mult/div?

  //branch & jump instructions
  if (EXi.branch) {
    switch (opcode) {
      case 0x04:
        //branch on equal
        if (vala == valb) branch(EXo.ALU_result);
        break;
      case 0x05:
        //branch not equal
        if (vala != valb) branch(EXo.ALU_result);
        break;
      case 0x01: {
        //branch >= 0 and branch > 0
        if ((valb == 1 && vala >= 0) || (valb == 0 && vala > 0)) {
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
      case 0x02: {
        branch(EXo.ALU_result);
        break;
      }
      case 0x03:
        branch(EXo.ALU_result);
        break;
      case 0x00: {
        int funct = instruction.funct;
        //TODO: fix this to actually link to the correct register
        if (funct == 0x09) branch_link(vala);
        else if (funct == 0x08) branch(vala);
        break;
      }
    }
  }
}

void MEM() {
#ifdef DEBUG
  printf("In MEM stage.\n");
#endif
  if (stall_MEM) {
#ifdef DEBUG
  printf("Stalling MEM stage.\n");
#endif
    return;
  }

  MEMo = emptyMEMWB;
  //simple forwarding
  MEMo.ALU_result = MEMi.ALU_result;
  MEMo.dest = MEMi.dest;
  MEMo.RegWrite = MEMi.RegWrite;
  MEMo.memToReg = MEMi.memToReg;
  MEMo.pc = MEMi.pc;
  MEMo.instruction = MEMi.instruction;

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
#ifdef DEBUG
  printf("Writing to memory.\n");
#endif
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
#ifdef DEBUG
  printf("In WB stage.\n");
#endif
  if (stall_WB) {
#ifdef DEBUG
  printf("Stalling WB stage.\n");
#endif
    return;
  }
  //if we need to write, write!
  if (WBi.RegWrite && WBi.dest != 0) registers[WBi.dest] = WBi.memToReg ? WBi.data : WBi.ALU_result;
}

long ALU(int input1, int input2, int input3, inst instruction) {
  long result = 0;
  long tmp = 0;
  int funct = instruction.funct;
  int opcode = instruction.opcode;
  int addr = EXi.pc*4 + mem_start;
  switch(opcode) {
    case 0x01:
      result = input2 << 2;
      result += addr;
      return(result);
    case 0x02:
      tmp = addr & 0xF0000000;
      result = tmp | input2<<2;
      return(result);
    case 0x03:
      tmp = addr & 0xF0000000;
      result = tmp | input2<<2;
      return(result);
    case 0x04:
      result = input3 << 2;
      result += addr;
      return(result);
    case 0x05:
      result = input3 << 2;
      result += addr;
      return(result);
    case 0x06:
      result = input2 << 2;
      result += addr;
      return(result);
    case 0x07:
      result = input2 << 2;
      result += addr;
      return(result);
    case 0x08:
      result = input1 + input2;
      return(result);
    case 0x09:
      result = input1 + input2;
      return(result);
    case 0x0A:
      result = input1 < input2;
      return(result);
    case 0x0B:
      result = input1 < input2;
      return(result);
    case 0x0C:
      result = input1 & input2;
      return(result);
    case 0x0D:
      result = input1 | input2;
      return(result);
    case 0x0E:
      result = input1 ^ input2;
      return(result);
    case 0x0F:
      result = input2 << 16;
      return(result);
    case 0x18:
      result = input1 + input2;
      return(result);
    case 0x20:
      result = input1 + input2;
      return(result);
    case 0x21:
      result = input1 + input2;
      return(result);
    case 0x22:
      result = input1 + input2;
      return(result);
    case 0x23:
      result = input1 + input2;
      return(result);
    case 0x24:
      result = input1 + input2;
      return(result);
    case 0x25:
      result = input1 + input2;
      return(result);
    case 0x28:
      result = input1 + input2;
      return(result);
    case 0x29:
      result = input1 + input2;
      return(result);
    case 0x2B:
      result = input1 + input2;
      return(result);
  }
  if (opcode == 0x00) {
    switch(funct) {
      case 0x00:
        result = input1 << input2;
        return(result);
      case 0x02:
        result = (int) ((unsigned int) input1) >> input2;
        return(result);
      case 0x03:
        result = input1 >> input2;
        return(result);
      case 0x04:
        result = input1 << input2;
        return(result);
      case 0x06:
        result = input1 >> input2;
        return(result);
      case 0x07:
        result = input1 >> input2;
        return(result);
      case 0x08:
        return(input2);
      case 0x0A:
        return input2 ? input3 : input1;
      case 0x0B:
        return input2 ? input1 : input3;
      case 0x10:
        return(input1);
      case 0x12:
        return(input1);
      case 0x18:
        result = input1 * input2;
        return(result);
      case 0x19:
        result = (unsigned int) input1 * (unsigned int) input2;
        return(result);
      case 0x1A:
        result = input1 / input2;
        return(result);
      case 0x1B:
        result = input1 / input2;
        return(result);
      case 0x20:
        result = input1 + input2;
        return(result);
      case 0x21:
        result = input1 + input2;
        return(result);
      case 0x22:
        result = input1 - input2;
        return(result);
      case 0x23:
        result = input1 - input2;
        return(result);
      case 0x24:
        result = input1 & input2;
        return(result);
      case 0x25:
        result = input1 | input2;
        return(result);
      case 0x26:
        result = input1 ^ input2;
        return(result);
      case 0x27:
        tmp = input1 | input2;
        result = ~tmp;
        return(result);
      case 0x2A:
        result = input1 < input2;
        return(result);
      case 0x2B:
        result = (unsigned int) input1 < (unsigned int) input2;
        return(result);
    }
  }
  printf("instruction opcode: %d function : %d not found/n", opcode, funct);
  return(0);
}

void print_registers() {
  //this function is meant to be run AFTER clock
  printf("\nIDEX:\n");
  printf("pc: %d\n", IDo.pc);
  printf("vala: %d\n", IDo.vala);
  printf("valb: %d\n", IDo.valb);
  printf("dest: %d\n", IDo.dest);
  printf("memRead: %s\n", IDo.memRead ? "true" : "false");
  printf("memWrite: %s\n", IDo.memWrite ? "true" : "false");
  printf("RegWrite: %s\n", IDo.RegWrite ? "true" : "false");
  printf("branch: %s\n", IDo.branch ? "true" : "false");
  printf("memToReg: %s\n", IDo.memToReg ? "true" : "false");

  printf("\nEXMEM:\n");
  printf("pc: %d\n", EXo.pc);
  printf("ALU_result: %d\n", EXo.ALU_result);
  printf("valb: %d\n", EXo.valb);
  printf("dest: %d\n", EXo.dest);
  printf("memRead: %s\n", EXo.memRead ? "true" : "false");
  printf("memWrite: %s\n", EXo.memWrite ? "true" : "false");
  printf("RegWrite: %s\n", EXo.RegWrite ? "true" : "false");
  printf("memToReg: %s\n", EXo.memToReg ? "true" : "false");

  printf("\nMEMWB:\n");
  printf("pc: %d\n", MEMo.pc);
  printf("ALU_result: %d\n", MEMo.ALU_result);
  printf("data: %d\n", MEMo.data);
  printf("dest: %d\n", MEMo.dest);
  printf("RegWrite: %s\n", MEMo.RegWrite ? "true" : "false");
  printf("memToReg: %s\n", MEMo.memToReg ? "true" : "false");

  int i;
  printf("\nREGISTERS:\n");
  for (i=0; i<32; i++) {
    if (registers[i] != 0)
      printf("%02d: %08x\n", i, registers[i]);
  }
}
