/* Parte 4 - expansao e chamadas aninhadas (Joao Victor).
 * O controlador reconhece chamadas; este modulo mantem os contextos ativos.
 * Nao realiza montagem, leitura do fonte nem cadastro de definicoes. */
#include "macros_interno.h"
#include <ctype.h>
#include <string.h>

int mc_empilhar(McContexto *c, const McMacro *m,
                const char argumentos[MC_PARAMS][MC_TEXTO], unsigned n) {
    McQuadro *q;
    unsigned i;
    if (!m || n > MC_PARAMS || n != m->nparametros || (n && !argumentos))
        return mc_erro(c, "quantidade incorreta de argumentos");
    if (c->topo == MC_NIVEIS) return mc_erro(c, "limite de profundidade de expansao");
    if (c->serial == MC_LINHAS) return mc_erro(c, "limite de expansoes");
    for (i = 0; i < c->topo; ++i)
        if (mc_igual(c->pilha[i].macro->nome, m->nome))
            return mc_erro(c, "recursao de macro: %s", m->nome);
    q = &c->pilha[c->topo++];
    memset(q, 0, sizeof *q);
    q->macro = m;
    q->serial = ++c->serial;
    for (i = 0; i < n; ++i) strcpy(q->argumentos[i], argumentos[i]);
    return 1;
}

static int anexar(McContexto *c, char *saida, size_t *n, const char *s, size_t tam) {
    if (*n + tam >= MC_TEXTO) return mc_erro(c, "linha expandida muito longa");
    memcpy(saida + *n, s, tam); *n += tam; saida[*n] = 0;
    return 1;
}

/* #(1,i) pertence a esta chamada. #(d,i), d>1, pertence a uma definicao
 * interna: diminui um nivel sem substitui-lo. Isso preserva o sombreamento
 * dos parametros formais e captura apenas os valores do contexto externo. */
static int substituir(McContexto *c, McQuadro *q, const char *p, char *saida) {
    size_t n = 0;
    int aspas = 0;
    const char *inicio = p;
    while (isspace((unsigned char)*inicio)) ++inicio;
    saida[0] = 0;
    if (*inicio == '.' || *inicio == ';') return anexar(c, saida, &n, p, strlen(p));
    while (*p) {
        if (!aspas && *p == ';') return anexar(c, saida, &n, p, strlen(p));
        if (*p == '\'') aspas = !aspas;
        if (!aspas && p[0] == '#' && p[1] == '(') {
            unsigned nivel, indice; int usados = 0;
            char marcador[40];
            if (sscanf(p, "#(%u,%u)%n", &nivel, &indice, &usados) != 2 ||
                !usados || nivel < 1 || nivel > MC_NIVEIS || indice >= MC_PARAMS)
                return mc_erro(c, "referencia interna de parametro invalida");
            if (nivel == 1) {
                if (indice >= q->macro->nparametros) return mc_erro(c, "indice de parametro invalido");
                /* Nao reprocessar o argumento inserido: substituicao simultanea. */
                if (!anexar(c, saida, &n, q->argumentos[indice], strlen(q->argumentos[indice]))) return 0;
            } else {
                snprintf(marcador, sizeof marcador, "#(%u,%u)", nivel-1, indice);
                if (!anexar(c, saida, &n, marcador, strlen(marcador))) return 0;
            }
            p += usados;
        } else if (!aspas && !q->definicoes && strncmp(p, ".SER", 4) == 0 &&
                   !isalnum((unsigned char)p[4]) && p[4] != '_') {
            char serial[24];
            snprintf(serial, sizeof serial, "%u", q->serial);
            if (!anexar(c, saida, &n, serial, strlen(serial))) return 0;
            p += 4;
        } else {
            if (!anexar(c, saida, &n, p++, 1)) return 0;
        }
    }
    return 1;
}

int mc_proxima_expansao(McContexto *c, char linha[MC_TEXTO]) {
    McQuadro *q;
    McLinha parsed;
    while (c->topo) {
        q = &c->pilha[c->topo-1];
        if (q->proxima == q->macro->nlinhas) { --c->topo; continue; }
        if (++c->passos > MC_LINHAS) { mc_erro(c, "limite de trabalho de expansao"); return -1; }
        if (!substituir(c, q, q->macro->corpo[q->proxima++], linha)) return -1;
        if (!mc_parse(c, linha, &parsed)) return -1;
        if (!parsed.vazia && mc_igual(parsed.operacao, "MCDEFN")) ++q->definicoes;
        if (!parsed.vazia && mc_igual(parsed.operacao, "MCEND")) {
            if (!q->definicoes) { mc_erro(c, "MCEND inesperado no corpo"); return -1; }
            --q->definicoes;
        }
        return 1;
    }
    return 0;
}
