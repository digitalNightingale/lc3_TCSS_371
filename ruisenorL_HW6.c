#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

#define OPCODE(instr)     (instr >> 12 & 0x000F)    // opcode
#define DSTREG(instr)     (instr >> 9 & 0x0007)     // DR
#define SRCREG(instr)     (instr >> 6 & 0x0007)     // SR1
#define SR2REG(instr)     (instr & 0x0007)          // SR2
#define BASEREG(instr)    (instr >> 6 & 0x0007)     // BASEREG
#define IMMBIT(instr)     ((instr & 0x0020) >> 5)   // bit[5] 1 or 0
#define IMMVAL(instr)     ((instr << 27 ) >> 27)    // imm5
#define OFFSET6(instr)    ((instr << 26 ) >> 26)    // offset6
#define TRAPVECT8(instr)  ((instr << 24 ) >> 24)    // trapvect8
#define PCOFFSET9(instr)  ((instr << 23 ) >> 23)    // pcoffset9
#define PCOFFSET11(instr) ((instr << 21 ) >> 21)    // pcoffset11
#define NBIT(instr)       ((instr & 0x0800) >> 11)  //nbit
#define ZBIT(instr)       ((instr & 0x0400) >> 10)  //zbit
#define PBIT(instr)       ((instr & 0x0200) >> 9)   //pbit

int16_t  memory[65536];
int16_t regs[8];
int16_t pc, ir;
int16_t tempR7;

struct {
    int junk:13;
    unsigned int p:1;
    unsigned int n:1;
    unsigned int z:1;
} psr;   // process status register

int readStuff(char *fileName) {
    struct stat st;
    stat(fileName, &st);
    int size_in_bytes = st.st_size;
    FILE *infile = fopen(fileName,"r");
    if (infile == NULL) {
        perror("Can not open file");
        return 0;
    }
    int16_t load_start_addr;
    int words_read = fread(&load_start_addr,sizeof(int16_t), 1, infile);
    char *cptr = (char *)&load_start_addr;

    char temp;
    temp = *cptr;
    *cptr = *(cptr+1);
    *(cptr+1) = temp;

    pc = load_start_addr;

    int instrs_to_load = (size_in_bytes-2)/2;
    words_read = fread(&memory[load_start_addr], sizeof(int16_t),instrs_to_load, infile);

    int i;
    cptr = (char *)&memory[load_start_addr];
    for (i = 0; i < instrs_to_load; i++) {
        temp = *cptr;
        *cptr = *(cptr+1);
        *(cptr+1) = temp;
        cptr += 2;
    }
    return 0;
}

void setCC(int16_t dest_reg) {
    if (regs[dest_reg] == 0) {
        psr.z = 1;
        psr.n = 0;
        psr.p = 0;
    } else if (regs[dest_reg] < 0) {
        psr.z = 0;
        psr.n = 1;
        psr.p = 0;
    } else {   // else if > 0
        psr.z = 0;
        psr.n = 0;
        psr.p = 1;
    }
}

int main(int argc, char **argv) {
    //./MyHW6 trapvectortable.obj out.obj puts.obj halt.obj trapcalls.obj
    char *objFileName;
    int i;
    for (i = 0; i < (argc - 1); i++) {
        objFileName = argv[i + 1];
        readStuff(objFileName);
    }

    // main loop for fetching and executing instructions
    // for now, we do this until we run into the instruction with opcode 13
    psr.z = 1;    // need a initiate psr
    memory[0xFFFE] = 0x8000;  //MCR halt
    memory[0xFE04] = 0x8000;  //DSR
    memory[0xFE06] = 0;       //DDR
    while (memory[0xFFFE] & 0x8000) {   // one instruction executed on each rep.

        ir = memory[pc]; //fetched the instruction
        pc++;

        int16_t opcode = OPCODE(ir);
        //printf("opcode = %02x\n", opcode);

        int16_t dest_reg, base_reg, src_reg, sr2_reg,
                imm_bit, imm_val, offset6, pcoffset9,
                pcoffset11, trapvect8, n_bit, z_bit, p_bit;

        switch(opcode) {

            case 0: // BR n[11] z[10] p[9]
            //printf("\n\nBranch Instruction\n");
            n_bit = NBIT(ir);
            z_bit = ZBIT(ir);
            p_bit = PBIT(ir);
            pcoffset9 = PCOFFSET9(ir);
            if ((psr.n && n_bit) || (psr.z && z_bit) || (psr.p && p_bit)){
                pc = pc + pcoffset9;
            }
            break;

            case 1:  // ADD
            // dest_reg = DSTREG(ir);
            // src_reg = SRCREG(ir);
            // imm_bit = IMMBIT(ir);
            // imm_val = IMMVAL(ir);
            // if (imm_bit == 0) {
            //     sr2_reg = SR2REG(ir);
            //     regs[dest_reg] = regs[src_reg] + regs[sr2_reg];
            // } else {
            //     regs[dest_reg] = regs[src_reg] + imm_val;
            // }
            // setCC(dest_reg);
            // break;
            case 5: // AND
            //printf("\n\nAND Instruction\n");
            dest_reg = DSTREG(ir);
            //printf("dest reg = %hu\n", dest_reg);
            src_reg = SRCREG(ir);
            //printf("src reg = %hu\n", src_reg);
            imm_bit = IMMBIT(ir);
            //printf("imm bit = %hu\n", imm_bit);
            if (imm_bit) {
                imm_val = IMMVAL(ir);
                //printf("imm val = %hd\n", imm_val);
                if (opcode == 1){
                    regs[dest_reg] = regs[src_reg] + imm_val;
                } else {
                    regs[dest_reg] = regs[src_reg] & imm_val;
                }
            } else {
                sr2_reg = SR2REG(ir);
                //printf("sr2 reg = %hd\n", sr2_reg);
                if (opcode == 1) {
                    regs[dest_reg] = regs[src_reg] + regs[sr2_reg];
                } else {
                    regs[dest_reg] = regs[src_reg] & regs[sr2_reg];
                }
            }
            setCC(dest_reg);
            break;

            case 9: // NOT
            //printf("\n\nNOT Instruction\n");
            dest_reg = DSTREG(ir);
            //printf("dest reg = %hu\n", dest_reg);
            src_reg = SRCREG(ir);
            //printf("src reg = %hu\n", src_reg);
            regs[dest_reg] = ~(regs[src_reg]);
            setCC(dest_reg);
            break;

            case 2: // LD
            // //printf("\n\nLoad Instruction\n");
            // dest_reg = DSTREG(ir);
            // //printf("dest reg = %hu\n", dest_reg);
            // pcoffset9 = PCOFFSET9(ir);
            // //printf("pcoffset9 = %hd\n", pcoffset9);
            // regs[dest_reg] = memory[pc + pcoffset9];
            // setCC(dest_reg);
            // break;
            case 10: // LDI
            //printf("\n\nLDI Instruction\n");
            dest_reg = DSTREG(ir);
            //printf("dest reg = %hu\n", dest_reg);
            pcoffset9 = PCOFFSET9(ir);
            //printf("pcoffset9 = %hd\n", pcoffset9);
            if (opcode == 2) {
                regs[dest_reg] = memory[pc + pcoffset9]; //memory[(unsigned short)pc + pcoffset9]
            } else {
                regs[dest_reg] = memory[(unsigned short)memory[pc + pcoffset9]];
            }
            setCC(dest_reg);
            break;

            case 14: // LEA
            //printf("\n\nLEA Instruction\n");
            dest_reg = DSTREG(ir);
            //printf("dest reg = %hu\n", dest_reg);
            pcoffset9 = PCOFFSET9(ir);
            //printf("pcoffset9 = %hd\n", pcoffset9);
            regs[dest_reg] = pc + pcoffset9;
            setCC(dest_reg);
            break;

            case 6: // LDR
            //printf("\n\nLDR Instruction\n");
            dest_reg = DSTREG(ir);
            //printf("dest reg = %hu\n", dest_reg);
            base_reg = BASEREG(ir);
            //printf("src reg = %hu\n", src_reg);
            offset6 = OFFSET6(ir);
            //printf("offset6 = %hd\n", offset6);
            regs[dest_reg] = memory[regs[base_reg]  + offset6];
            setCC(dest_reg);
            break;

            case 3: // ST
            // //printf("\n\nStore Instruction\n");
            // dest_reg = DSTREG(ir);
            // //printf("SR reg = %hu\n", dest_reg);
            // pcoffset9 = PCOFFSET9(ir);
            // //printf("pcoffset9 = %hd\n", pcoffset9);
            // memory[pc + pcoffset9] = regs[dest_reg];
            // break;
            case 11: // STI
            //printf("\n\nSTI Instruction\n");
            src_reg = DSTREG(ir);
            //printf("SR reg = %hu\n", dest_reg);
            pcoffset9 = PCOFFSET9(ir);
            //printf("pcoffset9 = %hd\n", pcoffset9);
            if (opcode == 3) {
                memory[pc + pcoffset9] = regs[src_reg];
                if (pc + pcoffset9 == 0xFE06) {
                    char c = memory[0xFE06];
                    printf("%c", c);
                }
            } else {
                memory[(unsigned short)memory[pc + pcoffset9]] = regs[src_reg];
                if ((unsigned short)memory[pc + pcoffset9] == 0xFE06) {
                    char c = memory[0xFE06];
                    printf("%c", c);
                }
            }
            break;

            case 7: // STR
            //printf("\n\nSTR Instruction\n");
            src_reg = DSTREG(ir);
            //printf("SR reg = %hu\n", dest_reg);
            base_reg = BASEREG(ir);
            //printf("Base reg = %hu\n", src_reg);
            offset6 = OFFSET6(ir);
            //printf("offset6 = %hd\n", offset6);
            memory[regs[base_reg]  + offset6] = regs[src_reg];
            if (regs[base_reg] + offset6 == 0xFE06) {
                char c = memory[0xFE06];
                printf("%c", c);
            }
            break;

            case 4: // JSR
            //printf("\n\nJSR Instruction\n");
            pcoffset11 = PCOFFSET11(ir); //SEXT
            //printf("pcoffset11 = %hd\n", pcoffset11);
            tempR7 = pc;
            pc = pc + pcoffset11;
            regs[7] = tempR7; //R7 = TEMP;
            break;

            case 12: // RET
            //printf("\n\nRET Instruction\n");
            pc = regs[7];
            break;

            case 15: // TRAP
            //printf("\n\nTRAP Instruction\n");
            trapvect8 = TRAPVECT8(ir); // ZEXT
            regs[7] = pc; //R7 = pc;
            pc = memory[trapvect8];
            break;
        } // switch ends
    }
}
