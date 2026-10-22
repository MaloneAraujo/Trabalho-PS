#ifndef MONTADOR_INTERNO_H
#define MONTADOR_INTERNO_H
#include "montador.h"

typedef enum { OPER_MEM, OPER_ZERO, OPER_REG, OPER_REG_REG, OPER_SHIFT } TipoOperando;
typedef struct { const char *nome; unsigned opcode, formato; TipoOperando tipo; } Operacao;
typedef enum { DIR_NENHUMA, DIR_START, DIR_END, DIR_BYTE, DIR_WORD,
    DIR_RESB, DIR_RESW, DIR_BASE, DIR_NOBASE, DIR_EQU, DIR_ORG } Diretiva;

const Operacao *buscar_operacao(const char *nome);
Diretiva buscar_diretiva(const char *nome);
int numero_registrador(const char *nome);
int erro_montagem(Montagem *m, const char *formato, ...);
int analisar_linha(Montagem *m, Linha *l);
int definir_simbolo(Montagem *m, const char *nome, int32_t valor, int relativo);
int avaliar(Montagem *m, const char *texto, unsigned lc, int base_numerica,
            int32_t *valor, int *relativo);
int bytes_constante(Montagem *m, const char *texto, uint8_t *bytes, unsigned *tamanho);
int primeiro_passo(FILE *fonte, Montagem *m);
int segundo_passo(Montagem *m);
#endif
