#ifndef MACROS_H
#define MACROS_H

#include <stdio.h>
#include <stddef.h>
#include "montador.h"

/* Contrato compartilhado: implementado pelo grupo, nao pelo integrador.
 * Retorna 1 no sucesso ou 0 no erro. Nao fecha os FILE recebidos.
 * A saida deve conter somente Assembly expandido; erros vao para erro.
 * A funcao deve verificar erros de leitura/escrita dos seus streams. */
int processar_macros(FILE *entrada, FILE *saida, char *erro, size_t tamanhoErro);

/* Executar no diretorio em que MASMAPRG.ASM deve ser gerado.
 * Montagem deve ser alocada no heap, como no montador original.
 * Em falha, preenche erro (se fornecido) e montagem->erro.
 * Nao gera .obj/.lst: essa responsabilidade continua com o chamador.
 * Chamadas concorrentes no mesmo diretorio nao sao suportadas. */
int montar_com_macros(const char *arquivoEntrada, Montagem *montagem,
                      char *erro, size_t tamanhoErro);

#endif
