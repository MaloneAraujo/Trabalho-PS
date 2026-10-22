#ifndef PROGRAMA_H
#define PROGRAMA_H
#include "sicxe.h"

/* Ponte independente da Raylib: .asm monta em memoria; demais arquivos usam
 * o carregador H/T/E ou binario. Em erro, preserva a maquina anterior.
 * Retorno 0=sucesso, -1=erro. */
int programa_carregar(Maquina *m, const char *caminho, char *erro, size_t tamanho);
#endif
