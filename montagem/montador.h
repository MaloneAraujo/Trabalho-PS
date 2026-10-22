#ifndef MONTADOR_H
#define MONTADOR_H

#include <stdio.h>
#include <stdint.h>
#include "sicxe_config.h"

/* Limites explicitos: erros sao reportados, nunca truncados silenciosamente. */
#define ASM_LINHAS 2048
#define ASM_SIMBOLOS 1024
#define ASM_TEXTO 512
#define ASM_NOME 32
#define ASM_OPERANDO 256

typedef struct {
    char nome[ASM_NOME];
    int32_t valor;
    int relativo; /* 1: endereco; 0: constante absoluta (inclui diferencas). */
} Simbolo;

typedef struct {
    unsigned numero, endereco, tamanho;
    char fonte[ASM_TEXTO], rotulo[ASM_NOME], operacao[16];
    char operando[ASM_OPERANDO];
    int instrucao;
} Linha;

typedef struct {
    Linha linhas[ASM_LINHAS];
    Simbolo simbolos[ASM_SIMBOLOS];
    unsigned nlinhas, nsimbolos, inicio, fim, entrada, bytes;
    char nome[7], erro[256];
    uint8_t imagem[MEM_TAMANHO], ocupado[MEM_TAMANHO], emitido[MEM_TAMANHO];
    unsigned linha_atual;
} Montagem;

/* Alocar Montagem no heap (calloc): a listagem intermediaria e grande.
 * As funcoes retornam 1 em sucesso e 0 em erro; erro contem linha e motivo.
 * Nenhum arquivo de saida e criado durante a montagem. */
int montar(FILE *fonte, Montagem *m);
int escrever_objeto(FILE *saida, const Montagem *m);
int escrever_listagem(FILE *saida, const Montagem *m);

#endif
