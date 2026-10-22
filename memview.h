/*
 * memview.h - ponto de encaixe entre a janela principal (parte 5) e a
 * apresentacao grafica da memoria (parte 6).
 *
 * Quem faz a parte 6 reescreve memview.c mantendo estas funcoes, e a janela
 * principal continua funcionando sem mudar nada.
 */
#ifndef MEMVIEW_H
#define MEMVIEW_H

#include "raylib.h"
#include "sicxe.h"

#define MEMVIEW_COLUNAS 16 /* bytes por linha */

/* Chamar ANTES de cada sim_passo: guarda uma copia da memoria para destacar o que mudou. */
void memview_antes_do_passo(const Maquina *m);

/* Apaga os destaques de "o que mudou" (ao carregar programa ou resetar). */
void memview_esquecer_mudancas(void);
void memview_liberar(void);

/* Quantas linhas de MEMVIEW_COLUNAS bytes cabem na area. */
int memview_linhas_visiveis(Rectangle area);

/* Desenha a memoria dentro de area, a partir do endereco inicio. */
void memview_desenhar(const Maquina *m, Rectangle area, uint32_t inicio);

#endif
