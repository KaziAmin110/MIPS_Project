/*
    MIPS_Project
    By: Kazi Amin, Hashhim Alkhateeb, Sadman Nahin
*/

#include "spimcore.h"


/* ALU */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{ /* code */

    if(ALUControl == 0){ //ALU 000: Z = A+B
        *ALUresult = A + B;
    }

    else if (ALUControl == 1){ //ALU 001: Z = A-B
        *ALUresult = A - B;
    }

    else if (ALUControl == 2){ //ALU 010: Set less than (signed)
        if((int) A < (int) B) //cast A and B to signed ints and compare
                *ALUresult = 1;
            else
                *ALUresult = 0;
    }

    else if (ALUControl == 3){ //ALU 011: Set less than (unsigned)
        *ALUresult = A < B;
    }

    else if (ALUControl == 4){ // ALU 100: Z = A AND B
        *ALUresult = A & B; 
    }

    else if (ALUControl == 5){ // ALU 101: Z = A OR B
        *ALUresult = A | B;
    }

    else if (ALUControl == 6){ //ALU 110: Z = shift B by 16 units
        *ALUresult = B << 16;
    }

    else if (ALUControl == 7){ //ALU 111: Z = NOT A
        *ALUresult = ~A;
    }


    if(*ALUresult == 0) //set Zero value
        *Zero = 1;
    else
        *Zero = 0;
    
}

/* instruction fetch */
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{

    if(PC % 4 != 0) //check for word alignment. (address of all instructions are a multiple of 4)
        return 1; //not alligned

    unsigned index = PC >> 2; // to get actual location. Mem is array of words
    
    *instruction = Mem[index]; //store instruction

    return 0;

}


/* instruction partition */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{

    *op = (instruction >> 26) & 63; //from Bits 31-26
    *r1 = (instruction >> 21) & 31; //from Bits 25-21
    *r2 = (instruction >> 16) & 31; //from Bits 20-16
    *r3 = (instruction >> 11) & 31; //from Bits 15-11
    *funct = instruction & 63; //from Bits 5-0
    *offset = instruction & 65535; //from Bits 15-0
    *jsec = instruction & 67108863; //from Bits 25-0

}



/* instruction decode */
int instruction_decode(unsigned op,struct_controls *controls)
{

    controls->RegDst = 0;
    controls->Jump = 0;
    controls->Branch = 0;
    controls->MemRead = 0;
    controls->MemtoReg = 0;        //Set Defualt Values
    controls->ALUOp = 0;
    controls->MemWrite = 0;
    controls->ALUSrc = 0;
    controls->RegWrite = 0;


    switch (op){

        case 0: //r-type
            controls->RegDst = 1;
            controls->RegWrite = 1;
            controls->ALUOp = 7;
            break;
        case 2: //jump 
            controls->RegDst = 2;
            controls->Jump = 1;
            break;
        case 4: //beq 
            controls->RegDst = 2;
            controls->Branch = 1;
            controls->MemtoReg = 2;
            controls->ALUSrc = 1;
            break;
        case 8: //addi
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            break;
        case 10: //slti
            controls->ALUOp = 2;
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            //controls->RegDst = 1;
            break;
        case 11: //sltiu
            controls->ALUOp = 3;
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            //controls->RegDst = 1;
            break;
        case 15: //load immediate unsigned
            controls->ALUOp = 6;
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            //controls->RegDst = 1;
            break;
        case 35: //lw
            controls->MemRead = 1;
            controls->MemtoReg = 1;
            controls->ALUSrc = 1;
            controls->RegWrite = 1;
            break;
        case 43: //sw
            controls->RegDst = 2;
            controls->MemtoReg = 2;
            controls->MemWrite = 1;
            controls->ALUSrc = 1;
            break;
        default: //halt condition
            return 1; 

    }
    return 0;

}

/* Read Register */
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
    *data1 = Reg[r1]; //reads and stores value from Reg[index] into variable
    *data2 = Reg[r2];
}


/* Sign Extend */
void sign_extend(unsigned offset,unsigned *extended_value)
{
    if(offset & 0x00008000){ //checks if the most significant bit (16th) is set to 1 (meaning it is negative)
        *extended_value = offset | 0xffff0000; //sets the upper 16 bits to 1
    }
    else{
        *extended_value = offset; 
    }
}

/* ALU operations */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
    if(ALUSrc == 1) //If ALUSrc is 1, use an immediate value
        data2 = extended_value; //set data2 to the immediate value


    if(ALUOp == 7){ //R-type
        switch(funct){ //look at funct

            case 4: //and
                ALUOp = 6;
                break;

            case 32: //add
                ALUOp = 0;
                break;

            case 34: //sub
                ALUOp = 1;
                break;

            case 36: //and
                ALUOp = 4;
                break;

            case 37: //or
                ALUOp = 5;
                break;

            case 39: //nor
                ALUOp = 7;
                break;

            case 42: //slt
                ALUOp = 2;
                break;

            case 43: //uslt
                ALUOp = 3;
                break;

            default: //halt
                return 1;
        }

        ALU(data1, data2, ALUOp, ALUresult, Zero); //call ALU function
    }
    else
        ALU(data1, data2, ALUOp, ALUresult, Zero); //call ALU function

    return 0;
}

/* Read / Write Memory */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{

    if(MemRead == 1){ //memRead is set
        if((ALUresult % 4) == 0)
            *memdata = Mem[ALUresult / 4]; //read data from memory
        else //halt
            return 1;
    }

    if(MemWrite == 1){ //MemWrite is set
        if((ALUresult % 4) == 0)
            Mem[ALUresult / 4] = data2; //copy data to memory
        else //halt
            return 1;
    }

    return 0;

}


/* Write Register */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
    if(RegWrite == 1 && MemtoReg == 1){ //if RegWrite and MemtoReg are 1, then data comes from memory
        if(RegDst == 0)
            Reg[r2] = memdata;
        else if(RegDst == 1)
            Reg[r3] = memdata;
    }
    else if(RegWrite == 1 && MemtoReg == 0){ //RegWrite is 1 and MemtoReg is 0, then data comes from ALUresult
        if(RegDst == 0)
            Reg[r2] = ALUresult;
        else if(RegDst == 1)
            Reg[r3] = ALUresult;
        
    }
}

/* PC update */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
    *PC += 4;
    
    if(Jump == 1){
        *PC = (jsec * 4) | (*PC & 0xf0000000); //left shift jsec by 2 bits. combines upper 4 bits of PC with lower 28 bits of jsec
    }

    if(Branch == 1 && Zero == 1){ //zero and branch are set
        *PC += extended_value * 4; //PC += equals extended value shifted left 2 bits
    }
}