#include <stdio.h>
#include "decodificador.h"


/* Identifica o formato da instrucao */

int identificarFormato(uint8_t primeiroByte)
{
    uint8_t opcode;

    /* Formato 1: instruções de 1 byte (FIX, FLOAT, HIO, NORM, SIO, TIO) */
    switch (primeiroByte)
    {
        case 0xC4: /* FIX */
        case 0xC0: /* FLOAT */
        case 0xF4: /* HIO */
        case 0xC8: /* NORM */
        case 0xF0: /* SIO */
        case 0xF8: /* TIO */
            return 1;
    }

    /* Formato 2: instruções de 2 bytes (ADDR, CLEAR, COMPR, DIVR, MULR, RMO, SHIFTL, SHIFTR, SUBR, SVC, TIXR) */
    switch (primeiroByte)
    {
        case 0x90: /* ADDR */
        case 0xB4: /* CLEAR */
        case 0xA0: /* COMPR */
        case 0x9C: /* DIVR */
        case 0x98: /* MULR */
        case 0xAC: /* RMO */
        case 0xA4: /* SHIFTL */
        case 0xA8: /* SHIFTR */
        case 0x94: /* SUBR */
        case 0xB0: /* SVC */
        case 0xB8: /* TIXR */
            return 2;
    }

    /* Formatos 3 e 4: Opcode usa os 6 bits mais significativos */
    opcode = primeiroByte & 0xFC;

    /* Testa opcodes conhecidos do SIC/XE */
    switch (opcode)
    {
        case 0x18: /* ADD */   case 0x58: /* ADDF */  case 0x40: /* AND */
        case 0x28: /* COMP */  case 0x88: /* COMPF */ case 0x24: /* DIV */
        case 0x64: /* DIVF */  case 0x3C: /* J */     case 0x30: /* JEQ */
        case 0x34: /* JGT */   case 0x38: /* JLT */   case 0x48: /* JSUB */
        case 0x00: /* LDA */   case 0x68: /* LDB */   case 0x50: /* LDCH */
        case 0x70: /* LDF */   case 0x08: /* LDL */   case 0x6C: /* LDS */
        case 0x74: /* LDT */   case 0x04: /* LDX */   case 0xD0: /* LPS */
        case 0x20: /* MUL */   case 0x60: /* MULF */  case 0x44: /* OR */
        case 0xD8: /* RD */    case 0x4C: /* RSUB */  case 0xEC: /* SSK */
        case 0x0C: /* STA */   case 0x78: /* STB */   case 0x54: /* STCH */
        case 0x80: /* STF */   case 0xD4: /* STI */   case 0x14: /* STL */
        case 0x7C: /* STS */   case 0xE8: /* STSW */  case 0x84: /* STT */
        case 0x10: /* STX */   case 0x1C: /* SUB */   case 0x5C: /* SUBF */
        case 0xE0: /* TD */    case 0x2C: /* TIX */   case 0xDC: /* WD */
            return 3; /* Pode ser formato 3 ou 4 (definido pela flag 'e') */
    }

    return 0; /* Opcode invalido/não encontrado */
}

/* Le uma palavra de 24 bits */

int lerPalavra(
    const uint8_t *memoria,
    size_t tamanhoMemoria,
    uint32_t endereco,
    uint32_t *valor
) {

    if (endereco > tamanhoMemoria ||
        tamanhoMemoria - endereco < 3) {
        return 0;
    }

    *valor = ((uint32_t)memoria[endereco] << 16)
           | ((uint32_t)memoria[endereco + 1] << 8)
           | memoria[endereco + 2];

    return 1;
}


/* Converte deslocamento de 12 bits com sinal */

int converterDeslocamento(int valor) {

    if (valor & 0x800) {
        return valor - 4096;
    }

    return valor;
}


/* Calcula o endereco efetivo */

int calcularEndereco(
    const uint8_t *memoria,
    size_t tamanhoMemoria,
    uint32_t PC,
    uint32_t X,
    uint32_t B,
    Instrucao *inst
) {

    uint32_t endereco;
    uint32_t valor;

    int deslocamento;


    /* Instrucao SIC */

    if (inst->n == 0 && inst->i == 0) {

        endereco = inst->endereco;

        if (inst->x) {
            endereco += X;
        }

        inst->enderecoEfetivo = endereco;

        return 1;
    }


    /* Formato 4 */

    if (inst->e == 1) {

        endereco = inst->endereco;
    }

    /* Formato 3 */

    else {

        deslocamento = inst->deslocamento;

        /* Relativo ao PC */

        if (inst->p == 1) {

            deslocamento =
                converterDeslocamento(deslocamento);

            endereco = inst->proximoPC + deslocamento;
        }

        /* Relativo a B */

        else if (inst->b == 1) {

            endereco = B + deslocamento;
        }

        /* Direto */

        else {

            endereco = deslocamento;
        }
    }


    /* Indexacao */

    if (inst->x == 1) {
        endereco += X;
    }


    /* Indireto */

    if (inst->n == 1 && inst->i == 0) {

        if (!lerPalavra(
            memoria,
            tamanhoMemoria,
            endereco,
            &valor
        )) {
            return 0;
        }

        endereco = valor;
        inst->indireto = 1;
    }


    inst->enderecoEfetivo = endereco;


    /* Imediato */

    if (inst->n == 0 && inst->i == 1) {

        inst->imediato = 1;
        inst->operando = endereco;
    }

    return 1;
}


/* Decodifica a instrucao */

int decodificarInstrucao(
    const uint8_t *memoria,
    size_t tamanhoMemoria,
    uint32_t PC,
    uint32_t X,
    uint32_t B,
    Instrucao *inst
) {

    uint8_t byte1, byte2, byte3, byte4;

    int formato;


    if (memoria == NULL || inst == NULL) {
        return 0;
    }

    if (PC >= tamanhoMemoria) {
        return 0;
    }


    /* Primeiro byte */

    byte1 = memoria[PC];

    formato = identificarFormato(byte1);

    if (formato == 0) {
        return 0;
    }


    /* Inicializa a estrutura */

    *inst = (Instrucao){0};

    inst->formato = formato;
    inst->tamanho = formato;


    /* Formato 1 */

    if (formato == 1) {

        inst->opcode = byte1;
        inst->proximoPC = PC + 1;

        return 1;
    }


    if (PC + 1 >= tamanhoMemoria) {
        return 0;
    }

    byte2 = memoria[PC + 1];


    /* Formato 2 */

    if (formato == 2) {

        inst->opcode = byte1;

        inst->r1 = (byte2 >> 4) & 0x0F;
        inst->r2 = byte2 & 0x0F;

        inst->proximoPC = PC + 2;

        return 1;
    }


    /* Formato 3 ou 4 */

    inst->opcode = byte1 & 0xFC;

    inst->n = (byte1 >> 1) & 1;
    inst->i = byte1 & 1;

    inst->x = (byte2 >> 7) & 1;
    inst->b = (byte2 >> 6) & 1;
    inst->p = (byte2 >> 5) & 1;
    inst->e = (byte2 >> 4) & 1;


    if (inst->b && inst->p) {
        return 0;
    }


    /* Formato 3 */

    if (inst->e == 0) {

        if (PC > tamanhoMemoria ||
            tamanhoMemoria - PC < 3) {
            return 0;
        }

        byte3 = memoria[PC + 2];

        inst->tamanho = 3;

        /* SIC */

        if (inst->n == 0 && inst->i == 0) {

            inst->endereco =
                ((byte2 & 0x7F) << 8) | byte3;
        }

        /* SIC/XE */

        else {

            inst->deslocamento =
                ((byte2 & 0x0F) << 8) | byte3;
        }

        inst->proximoPC = PC + 3;
    }


    /* Formato 4 */

    else {

        if (PC > tamanhoMemoria ||
            tamanhoMemoria - PC < 4) {
            return 0;
        }

        byte3 = memoria[PC + 2];
        byte4 = memoria[PC + 3];

        inst->tamanho = 4;

        inst->endereco =
            ((uint32_t)(byte2 & 0x0F) << 16)
            | ((uint32_t)byte3 << 8)
            | byte4;

        inst->proximoPC = PC + 4;
    }


    /* Calcula o endereco */

    return calcularEndereco(
        memoria,
        tamanhoMemoria,
        PC,
        X,
        B,
        inst
    );
}