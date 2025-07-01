#include "asm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

#define MAX_LINE 256
#define MAX_CODE 4096

enum WordFormat
{
    NONE,               //None
    ADDRESS,            //AAAA AAAA
    DATA,               //DDDD DDDD
    OPCODE,             //0000 0000
    OPCODE_COND,        //0000 CCCC
    OPCODE_ADDR,        //0000 AAAA
    OPCODE_DATA,        //0000 DDDD
    OPCODE_REGPAIR_0,   //0000 RRR0
    OPCODE_REGPAIR_1,   //0000 RRR1
    OPCODE_REGISTER,    //0000 RRRR
};

typedef struct
{
    char* mnemonic;
    unsigned char opcode;
    int num_modifiers;
    int word0_format;
    int word1_format;
} Instruction;

Instruction instructions[] = 
{
    {"NOP", 0x00, 0, NONE,             NONE },
    {"JCN", 0x01, 2, OPCODE_COND,      ADDRESS },
    {"FIM", 0x02, 2, OPCODE_REGPAIR_0, DATA },
    {"SRC", 0x02, 1, OPCODE_REGPAIR_1, NONE },
    {"FIN", 0x03, 1, OPCODE_REGPAIR_0, NONE },
    {"JIN", 0x03, 1, OPCODE_REGPAIR_1, NONE },
    {"JUN", 0x04, 1, OPCODE_ADDR,      ADDRESS },
    {"JMS", 0x05, 1, OPCODE_ADDR,      ADDRESS },
    {"INC", 0x06, 1, OPCODE_REGISTER,  NONE },
    {"ISZ", 0x07, 2, OPCODE_REGISTER,  ADDRESS },
    {"ADD", 0x08, 1, OPCODE_REGISTER,  NONE },
    {"SUB", 0x09, 1, OPCODE_REGISTER,  NONE },
    {"LD",  0x0A, 1, OPCODE_REGISTER,  NONE },
    {"XCH", 0x0B, 1, OPCODE_REGISTER,  NONE },
    {"BBL", 0x0C, 1, OPCODE_DATA,      NONE },
    {"LDM", 0x0D, 1, OPCODE_DATA,      NONE },
    {"CLB", 0xF0, 0, OPCODE,           NONE },
    {"CLC", 0xF1, 0, OPCODE,           NONE }, 
    {"IAC", 0xF2, 0, OPCODE,           NONE }, 
    {"CMC", 0xF3, 0, OPCODE,           NONE }, 
    {"CMA", 0xF4, 0, OPCODE,           NONE },
    {"RAL", 0xF5, 0, OPCODE,           NONE }, 
    {"RAR", 0xF6, 0, OPCODE,           NONE }, 
    {"TCC", 0xF7, 0, OPCODE,           NONE }, 
    {"DAC", 0xF8, 0, OPCODE,           NONE },
    {"TCS", 0xF9, 0, OPCODE,           NONE }, 
    {"STC", 0xFA, 0, OPCODE,           NONE }, 
    {"DAA", 0xFB, 0, OPCODE,           NONE }, 
    {"KBP", 0xFC, 0, OPCODE,           NONE },
    {"DCL", 0xFD, 0, OPCODE,           NONE }, 
    {"WRM", 0xE0, 0, OPCODE,           NONE }, 
    {"WMP", 0xE1, 0, OPCODE,           NONE },
    {"WRR", 0xE2, 0, OPCODE,           NONE }, 
    {"WPM", 0xE3, 0, OPCODE,           NONE }, 
    {"WR0", 0xE4, 0, OPCODE,           NONE }, 
    {"WR1", 0xE5, 0, OPCODE,           NONE },
    {"WR2", 0xE6, 0, OPCODE,           NONE }, 
    {"WR3", 0xE7, 0, OPCODE,           NONE }, 
    {"SBM", 0xE8, 0, OPCODE,           NONE }, 
    {"RDM", 0xE9, 0, OPCODE,           NONE },
    {"RDR", 0xEA, 0, OPCODE,           NONE }, 
    {"ADM", 0xEB, 0, OPCODE,           NONE }, 
    {"RD0", 0xEC, 0, OPCODE,           NONE }, 
    {"RD1", 0xED, 0, OPCODE,           NONE },
    {"RD2", 0xEE, 0, OPCODE,           NONE }, 
    {"RD3", 0xEF, 0, OPCODE,           NONE }, 
    {NULL,  0,    0, NONE,             NONE }
};

unsigned char code[MAX_CODE];
int code_pos = 0;

static void error(const char* msg)
{
    printf("Error: %s\n", msg);
    exit(1);
}
static void print_version()
{
    printf("Intel 4004 Assembler [Version %d.%d.%d]\n(c) Adam Lacko. Available under MIT license.\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}
static void print_help()
{
    printf("Usage: asm [OPTIONS] <INPUT_FILE> <OUTPUT_FILE>\n\nOptions:\n--help        Show this help message and exit.\n--version     Show version and exit.\n");
}
static int find_instruction(const char* mnemonic)
{
    for (int i = 0; instructions[i].mnemonic; i++)
    {
        if (strcmp(mnemonic, instructions[i].mnemonic) == 0)
        {
            return i;
        }
    }
    return -1;
}
static void emit_byte(unsigned char byte)
{
    if (code_pos >= MAX_CODE)
    {
        error("Code memory overflow");
    }
    code[code_pos++] = byte;
}
static void save_output(const char* filename)
{
    FILE* fp = fopen(filename, "wb");
    if (!fp)
    {
        error("Cannot open output file");
    }
    fwrite(code, 1, code_pos, fp);
    fclose(fp);
}
static void assemble_file(const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
    {
        error("Cannot open input file");
    }

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, fp))
    {
        line[strcspn(line, "\n;")] = 0;
        if (strlen(line) > 0)
        {
            assemble_line(line);
        }
    }
    fclose(fp);
}

void assemble_line(char* line)
{
    char mnemonic[8];
    int mod1, mod2;
    int inst_idx;

    int num_mods = sscanf_s(line, "%s %x %x", mnemonic, 8, &mod1, &mod2);

    inst_idx = find_instruction(mnemonic);
    if (inst_idx == -1)
    {
        error("Invalid mnemonic");
    }

    Instruction* inst = &instructions[inst_idx];
    if ((num_mods - 1) > inst->num_modifiers)
    {
        error("Too many modifiers");
    }
    if ((num_mods - 1) < inst->num_modifiers)
    {
        error("Too few modifiers");
    }

    switch (inst->word0_format)
    {
    case NONE:
        break;
    case OPCODE:
        emit_byte(inst->opcode);
        break;
    case OPCODE_COND:
    case OPCODE_ADDR:
    case OPCODE_DATA:
    case OPCODE_REGISTER:
        emit_byte((inst->opcode & 0x0F) << 4 | (mod1 & 0x0F));
        break;
    case OPCODE_REGPAIR_0:
        emit_byte((inst->opcode & 0x0F) << 4 | ((mod1 & 0x07) << 1));
        break;
    case OPCODE_REGPAIR_1:
        emit_byte((inst->opcode & 0x0F) << 4 | ((mod1 & 0x07) << 1) | 0x01);
        break;
    default:
        break;
    }

    switch (inst->word1_format)
    {
    case NONE:
        break;
    case ADDRESS:
    case DATA:
        emit_byte(mod2);
        break;
    default:
        break;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        if (argc == 2)
        {
            if (strcmp(argv[1], "--version") == 0)
            {
                print_version();
                return 0;
            }
            if (strcmp(argv[1], "--help") == 0)
            {
                print_help();
                return 0;
            }
        }
        print_help();
        return -1;
    }
    assemble_file(argv[1]);
    save_output(argv[2]);
    return 0;
}