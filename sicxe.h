/*
 * Contrato publico do nucleo. Usado pelo terminal e pelos testes nesta etapa.
 * sim_real.c integra os modulos de memoria, decodificacao e instrucoes.
 */
#ifndef SICXE_H
#define SICXE_H

#include <stddef.h>
#include <stdint.h>
#include "sicxe_config.h"

/* Resultado de sim_passo */
enum { SIM_OK = 0, SIM_FIM = 1, SIM_ERRO = -1 };

/* Estado da maquina. Os registradores usam so os 24 bits (F: 48 bits) de baixo. */
typedef struct {
    uint32_t A, X, L, B, S, T, PC, SW;
    uint64_t F;
    uint8_t *mem;      /* memoria, em bytes */
    size_t   mem_tam;  /* tamanho da memoria em bytes */
} Maquina;

/* Cria uma visao do estado global e o zera. Uma maquina por vez nesta etapa. */
Maquina *sim_criar(void);

/* Libera a maquina. */
void sim_destruir(Maquina *m);

/* Zera registradores e volta PC para a entrada carregada. Mantem a memoria. */
void sim_reset(Maquina *m);

/* Carrega um programa do arquivo para a memoria e zera registradores.
 * Retorna 0 em sucesso; em falha preserva o estado anterior e escreve erro. */
int sim_carregar(Maquina *m, const char *caminho, char *erro, size_t erro_tam);

/* Carrega imagem absoluta desde o endereco 0, sem arquivos intermediarios.
 * Valida antes de alterar o estado; zera o restante da memoria e registradores.
 * Permite usar m->mem como origem. Retorno: 0 sucesso, -1 erro. */
int sim_carregar_imagem(Maquina *m, const uint8_t *imagem, size_t tamanho,
                       uint32_t inicio, char *erro, size_t erro_tam);

/* Executa UMA instrucao.
 * desc recebe um texto curto da instrucao executada (ex.: "000000: LDA #5"),
 * ou a mensagem de erro se o retorno for SIM_ERRO.
 * Retorna SIM_OK (pode continuar), SIM_FIM (programa terminou) ou SIM_ERRO. */
int sim_passo(Maquina *m, char *desc, size_t desc_tam);

#endif
