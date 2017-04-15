#include "processor.h"

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

void init(char* filename) {
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

  //set SP, FP, pc
  registers[SP] = memory[0];
  registers[FP] = memory[1];
  pc = memory[5];

  //Build empty structures and zero out everything
  empty_inst.opcode = 0;
  empty_inst.rs = 0;
  empty_inst.rt = 0;
  empty_inst.rd = 0;
  empty_inst.shamt = 0;
  empty_inst.funct = 0;
  empty_inst.immediate = 0;
  empty_inst.dest = 0;

  empty_data.mem = 0;
  empty_data.rs = 0;
  empty_data.rt = 0;
  empty_data.rd = 0;
  empty_data.immediate = 0;
  empty_data.ALU_result = 0;

  empty_IFID.pc = 0;
  empty_IFID.new_pc = 0;
  empty_IFID.instruction = 0;

  empty_IDEX.pc = 0;
  empty_IDEX.new_pc = 0;
  empty_IDEX.instruction = empty_inst;
  empty_IDEX.data = empty_data;
  empty_IDEX.mem_read = false;
  empty_IDEX.mem_write = false;
  empty_IDEX.branch = false;
  empty_IDEX.reg_write = false;
  empty_IDEX.mem_to_reg = false;

  empty_EXMEM.pc = 0;
  empty_EXMEM.new_pc = 0;
  empty_EXMEM.data = empty_data;
  empty_EXMEM.instruction = empty_inst;
  empty_EXMEM.mem_read = false;
  empty_EXMEM.mem_write = false;
  empty_EXMEM.branch = false;
  empty_EXMEM.reg_write = false;
  empty_EXMEM.mem_to_reg = false;

  empty_MEMWB.pc = 0;
  empty_MEMWB.data = empty_data;
  empty_MEMWB.instruction = empty_inst;
  empty_MEMWB.reg_write = false;
  empty_MEMWB.mem_to_reg = false;

  //set everything up as empty
  IFID = empty_IFID;
  IDEX = empty_IDEX;
  EXMEM = empty_EXMEM;
  MEMWB = empty_MEMWB;
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

void hazard_detection() {
  //TODO: implement this
}

void IF() {
  if (stall_IF) return;

  //TODO: check this, should this be before the stall? If so, fix
  //      all of these statements
  IFID = empty_IFID;

  IFID.pc = pc;
  IFID.instruction = memory[pc];

  //increment PC
  pc++;
  IFID.new_pc = pc;
}

void ID() {
  if (stall_ID) return;

  IDEX = empty_IDEX;

  //simple forwarding
  IDEX.pc = IFID.pc;
  IDEX.new_pc = IFID.new_pc;

  //Instruction Formats:
  //  R: opcode (6) | rs (5) | rt (5) | rd (5) | shamt (5) | funct (6)
  //  I: opcode (6) | rs (5) | rt (5) | IMM (16)
  //  J: opcode (6) | Pseudo-Address (26)

  int instruction = IFID.instruction;
  int opcode = (instruction&0xFC000000)>>26;
  
  IDEX.instruction.opcode = opcode;

  if (opcode == 0x00 || opcode == 0x1F) {
    //R type instructions. All instructions of the same type should have
    //the same "instruction" registers
    int funct = instruction & 0x3F;

    IDEX.instruction.funct = funct;

    IDEX.instruction.rs = (instruction&0x03E00000)>>21;
    IDEX.instruction.rt = (instruction&0x001F0000)>>16;
    IDEX.instruction.rd = (instruction&0x0000F800)>>11;

    IDEX.instruction.shamt = (instruction&0x7C0)>>6;

    IDEX.instruction.dest = IDEX.instruction.rd;

    //fill data
    IDEX.data.rs = registers[IDEX.instruction.rs];
    IDEX.data.rt = registers[IDEX.instruction.rt];
    IDEX.data.rd = registers[IDEX.instruction.rd];

    //set the control booleans
    if (funct == 0x08) {
      //jr instruction
      IDEX.branch = true;
    }
    else {
      //all of the arithmetic ops
      IDEX.reg_write = true;
    }
  }
  else if (opcode == 0x02 || opcode == 0x03) {
    //J instructions
    IDEX.instruction.immediate = instruction & 0x3FFFFFFF;

    IDEX.data.immediate = IDEX.instruction.immediate;

    //set the control booleans
    IDEX.branch = true;
  }
  else {
    //I instructions
    IDEX.instruction.rs = (instruction&0x03E00000)>>21;
    IDEX.instruction.rt = (instruction&0x001F0000)>>16;
    IDEX.instruction.immediate = (int) ((short int) (instruction&0x0000FFFF));

    //fill data
    IDEX.data.immediate = IDEX.instruction.immediate;
    IDEX.data.rs = registers[IDEX.instruction.rs];
    IDEX.data.rt = registers[IDEX.instruction.rt];

    //set the control booleans
    if (opcode >= 0x08 && opcode <= 0x0F) {
      //Arithmetic operations
      IDEX.reg_write = true;

      //set dest
      IDEX.instruction.dest = IDEX.instruction.rt;
    }
    else if ((opcode >= 0x04 && opcode <= 0x07) || opcode == 0x01) {
      //Branch operations
      IDEX.branch = true;
    }
    else if ((opcode >= 0x23 && opcode <= 0x25) || opcode == 0x20) {
      //Load operations
      IDEX.mem_read = true;
      IDEX.reg_write = true;
      IDEX.mem_to_reg = true;

      //set dest
      IDEX.instruction.dest = IDEX.instruction.rt;
    }
    else if ((opcode >= 0x28 && opcode <= 0x29) || opcode == 0x2B) {
      //Store operations
      IDEX.mem_write = true;
    }
  }

  //TODO: implement branches
}

void EX() {
  if (stall_EX) return;

  EXMEM = empty_EXMEM;

  //simple copies
  EXMEM.pc = IDEX.pc;
  EXMEM.new_pc = IDEX.new_pc;
  EXMEM.data = IDEX.data;
  EXMEM.instruction = IDEX.instruction;
  EXMEM.mem_read = IDEX.mem_read;
  EXMEM.mem_write = IDEX.mem_write;
  EXMEM.branch = IDEX.branch;
  EXMEM.reg_write = IDEX.reg_write;
  EXMEM.mem_to_reg = IDEX.mem_to_reg;

  EXMEM.data.ALU_result = ALU(IDEX.data, IDEX.instruction);
}

void MEM() {
  if (stall_MEM) return;

  MEMWB = empty_MEMWB;

  //simple copies
  //TODO: check all of these simple copies
  MEMWB.pc = EXMEM.pc;
  MEMWB.data = EXMEM.data;
  MEMWB.instruction = EXMEM.instruction;
  MEMWB.reg_write = EXMEM.reg_write;
  MEMWB.mem_to_reg = EXMEM.mem_to_reg;

  int opcode = EXMEM.instruction.opcode;

  //TODO: double check endianness

  //load instructions
  if (EXMEM.mem_read) {
    int addr = EXMEM.data.ALU_result;

    int full_word = memory[(addr - mem_start)/4];

    //temp variable for signed halfwords
    int data = 0;

    switch (opcode) {
      case 0x20:
        //lb
      case 0x24:
        //lbu
        switch (addr%4) {
          case 0:
            data = (full_word & 0xFF000000)>>24;
            break;
          case 1:
            data = (full_word & 0x00FF0000)>>16;
            break;
          case 2:
            data = (full_word & 0x0000FF00)>>8;
            break;
          case 3:
            data = full_word & 0x000000FF;
            break;
        }

        //sign extend
        //TODO: check this
        if (opcode == 0x20) {
          data = (int) ((char) data);
        }

        MEMWB.data.mem = data;
        break;
      case 0x25:
        //lhu
        switch (addr%2) {
          case 0:
            MEMWB.data.mem = (full_word & 0xFFFF0000)>>16;
            break;
          case 1:
            MEMWB.data.mem = full_word & 0x0000FFFF;
            break;
        }
        break;
      case 0x23:
        //lw
        MEMWB.data.mem = full_word;
        break;
    }
  }
  else MEMWB.data.mem = 0;

  //similar to all of the read stuff, just with write
  if (EXMEM.mem_write) {
    int addr = EXMEM.data.ALU_result;
    int word_addr = (addr-mem_start)/4;

    switch (opcode) {
      case 0x28:
        //sb
        switch (addr%4) {
          case 0:
            memory[word_addr] &= 0x00FFFFFFF;
            memory[word_addr] |= EXMEM.data.rt << 24;
            break;
          case 1:
            memory[word_addr] &= 0xFF00FFFF;
            memory[word_addr] |= (EXMEM.data.rt & 0xFF0000) << 16;
            break;
          case 2:
            memory[word_addr] &= 0xFFFF00FF;
            memory[word_addr] |= (EXMEM.data.rt & 0xFF00) << 8;
            break;
          case 3:
            memory[word_addr] &= 0xFFFFFF00;
            memory[word_addr] |= EXMEM.data.rt & 0xFF;
            break;
        }
        break;
      case 0x29:
        //sh
        switch (addr%2) {
          case 0:
            memory[word_addr] &= 0x0000FFFF;
            memory[word_addr] |= EXMEM.data.rt << 16;
            break;
          case 1:
            memory[word_addr] &= 0xFFFF0000;
            memory[word_addr] |= EXMEM.data.rt & 0xFFFF;
            break;
        }
        break;
      case 0x2B:
        //sw
        memory[word_addr] = EXMEM.data.rt;
        break;
    }
  }
}

void WB() {
  if (stall_WB) return;

  //if we need to write, write!
  if (MEMWB.reg_write && MEMWB.instruction.dest != 0) {
    registers[MEMWB.instruction.dest] = MEMWB.mem_to_reg ? MEMWB.data.mem : MEMWB.data.ALU_result;
  }
}

int ALU(read_data data, inst instruction) {
  int op = instruction.opcode;
  int funct = instruction.funct;

  switch (op) {
    //----------
    // branches 
    //----------
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x01:
      return 0;

    //-------
    // jumps
    //-------
    case 0x02:
    case 0x03:
      printf("Jump got to EX stage, make sure they are implemented properly\n");
      return 0;

    //------------
    // immediates
    //------------
    case 0x08:
      //addi
      //same as addiu, we don't care about traps
    case 0x09:
      //addiu
      return data.rs + data.immediate;
    case 0x0C:
      //andi
      return data.rs & data.immediate;
    case 0x0E:
      //xori
      return data.rs ^ data.immediate;
    case 0x0D:
      //ori
      return data.rs | data.immediate;
    case 0x0A:
      //slti
      return data.rs < data.immediate;
    case 0x0B:
      //sltiu
      return (unsigned int) data.rs < (data.immediate & 0x0000FFFF);
    case 0x0F:
      //lui
      return data.immediate << 16;

    //------------
    // load/store
    //------------
    case 0x20:
    case 0x24:
    case 0x25:
    case 0x23:
    case 0x28:
    case 0x29:
    case 0x2B:
      return data.rs + data.immediate;

    //--------
    // R type
    //--------
    case 0x1F:
      //seb
      if (funct == 0x20) return (int) ((char) (data.rt & 0xFF));
      break;
    case 0x00:
      switch (funct) {
        case 0x32:
          //add
        case 0x33:
          //addu
          //Again, the same as add, no traps
          return data.rs + data.rt;
        case 0x24:
          //and
          return data.rs & data.rt;
        case 0x08:
          //jr
          printf("Jump got to EX stage, make sure they are implemented properly\n");
          return 0;
        case 0x27:
          //nor
          return ~(data.rs | data.rt);
        case 0x25:
          //or
          return data.rs | data.rt;
        case 0x0B:
          //movn
          return data.rt ? data.rs : data.rd;
        case 0x0A:
          //movz
          return data.rt ? data.rd : data.rs;
        case 0x2A:
          //slt
          return data.rs < data.rt;
        case 0x2B:
          //sltu
          return (unsigned int) data.rs < (unsigned int) data.rt;
        case 0x00:
          //sll
          return data.rt << instruction.shamt;
        case 0x02:
          //srl
          return (int) (((unsigned int) data.rt) >> instruction.shamt);
        case 0x22:
          //sub
          return data.rs - data.rt;
        case 0x23:
          //subu
          return (unsigned int) data.rs - (unsigned int) data.rt;
        case 0x26:
          //xor
          return data.rs ^ data.rt;
      }
      break;
  }

  printf("Opcode %02x | Function %02x not implemented\n", op, funct);
  return 0;
}
