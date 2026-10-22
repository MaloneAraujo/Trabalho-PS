#include "interno.h"
#include <string.h>

/* MOT: subconjunto inteiro do simulador. Sem ponto flutuante, E/S ou privilegios. */
static const Operacao mot[] = {
    {"ADD",0x18,3,OPER_MEM}, {"ADDR",0x90,2,OPER_REG_REG},
    {"AND",0x40,3,OPER_MEM}, {"CLEAR",0xB4,2,OPER_REG},
    {"COMP",0x28,3,OPER_MEM}, {"COMPR",0xA0,2,OPER_REG_REG},
    {"DIV",0x24,3,OPER_MEM}, {"DIVR",0x9C,2,OPER_REG_REG},
    {"J",0x3C,3,OPER_MEM}, {"JEQ",0x30,3,OPER_MEM},
    {"JGT",0x34,3,OPER_MEM}, {"JLT",0x38,3,OPER_MEM},
    {"JSUB",0x48,3,OPER_MEM}, {"LDA",0x00,3,OPER_MEM},
    {"LDB",0x68,3,OPER_MEM}, {"LDCH",0x50,3,OPER_MEM},
    {"LDL",0x08,3,OPER_MEM}, {"LDS",0x6C,3,OPER_MEM},
    {"LDT",0x74,3,OPER_MEM}, {"LDX",0x04,3,OPER_MEM},
    {"MUL",0x20,3,OPER_MEM}, {"MULR",0x98,2,OPER_REG_REG},
    {"OR",0x44,3,OPER_MEM}, {"RMO",0xAC,2,OPER_REG_REG},
    {"RSUB",0x4C,3,OPER_ZERO}, {"SHIFTL",0xA4,2,OPER_SHIFT},
    {"SHIFTR",0xA8,2,OPER_SHIFT}, {"STA",0x0C,3,OPER_MEM},
    {"STB",0x78,3,OPER_MEM}, {"STCH",0x54,3,OPER_MEM},
    {"STL",0x14,3,OPER_MEM}, {"STS",0x7C,3,OPER_MEM},
    {"STT",0x84,3,OPER_MEM},
    {"STX",0x10,3,OPER_MEM}, {"SUB",0x1C,3,OPER_MEM},
    {"SUBR",0x94,2,OPER_REG_REG}, {"TIX",0x2C,3,OPER_MEM},
    {"TIXR",0xB8,2,OPER_REG}
};

const Operacao *buscar_operacao(const char *nome) {
    size_t i;
    if (*nome == '+') ++nome;
    for (i = 0; i < sizeof mot / sizeof mot[0]; ++i)
        if (!strcmp(nome, mot[i].nome)) return &mot[i];
    return NULL;
}

Diretiva buscar_diretiva(const char *nome) {
    static const char *pot[] = {"", "START", "END", "BYTE", "WORD", "RESB",
                               "RESW", "BASE", "NOBASE", "EQU", "ORG"};
    size_t i;
    for (i = 1; i < sizeof pot / sizeof pot[0]; ++i)
        if (!strcmp(nome, pot[i])) return (Diretiva)i;
    return DIR_NENHUMA;
}

int numero_registrador(const char *nome) {
    static const char *nomes[] = {"A","X","L","B","S","T","","","PC","SW"};
    int i;
    if (!*nome) return -1;
    for (i = 0; i < 10; ++i) if (!strcmp(nome, nomes[i])) return i;
    return -1;
}
