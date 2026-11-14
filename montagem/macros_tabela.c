#include "macros_interno.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/*
 * ============================================================================
 * TRABALHO DE SISTEMAS / SIC/XE - PROCESSADOR DE MACROS
 * Módulo: Estruturas de Dados e Tabela de Definições
 * Responsável: Malone Mendes de Araujo (Integrante 1)
 *
 * Implementa a tabela de definições de macros, armazenamento de parâmetros
 * formais, corpo em Assembly textual, busca insensível a maiúsculas/minúsculas,
 * suporte a redefinições encadeadas e desalocação completa de memória.
 * ============================================================================
 */

/*
 * Compara duas strings ignorando maiúsculas e minúsculas (padrão SIC/XE).
 */
int mc_igual(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return (!*a && !*b);
}

/*
 * Inicializa a estrutura de contexto e sua tabela de macros.
 */
void mc_contexto_inicializar(McContexto *c, char *erro, size_t tamanho_erro) {
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->erro = erro;
    c->tamanho_erro = tamanho_erro;
    if (erro && tamanho_erro) {
        erro[0] = '\0';
    }
}

/*
 * Cria e aloca dinamicamente um novo contexto.
 */
McContexto *mc_contexto_criar(char *erro, size_t tamanho_erro) {
    McContexto *c = (McContexto *)calloc(1, sizeof(McContexto));
    if (c) {
        mc_contexto_inicializar(c, erro, tamanho_erro);
    }
    return c;
}

/*
 * Busca uma macro na tabela pelo nome.
 * Retorna a versão ativa mais recente (caso tenha sofrido redefinição).
 */
McMacro *mc_buscar(McContexto *c, const char *nome) {
    if (!c || !c->tabela || !nome) return NULL;

    for (unsigned i = 0; i < c->ndefinicoes; i++) {
        if (mc_igual(c->tabela[i].nome, nome)) {
            return &c->tabela[i];
        }
    }
    return NULL;
}

/*
 * Adiciona um parâmetro formal à macro.
 */
int mc_adicionar_parametro(McMacro *m, const char *parametro) {
    if (!m || !parametro || m->nparametros >= MC_PARAMS) return 0;

    strncpy(m->parametros[m->nparametros], parametro, MC_NOME - 1);
    m->parametros[m->nparametros][MC_NOME - 1] = '\0';
    m->nparametros++;
    return 1;
}

/*
 * Cadastra uma nova macro ou registra uma redefinição na tabela.
 * Suporte a redefinições encadeadas: se já existir uma macro com o mesmo nome,
 * ela é preservada em memória e ligada via 'anterior', permitindo que a nova
 * versão tenha parâmetros ou corpo de tamanho diferente sem invalidar referências
 * que ainda estejam sendo processadas na pilha de expansão.
 */
McMacro *mc_cadastrar(McContexto *c, const char *nome,
                      const char parametros[MC_PARAMS][MC_NOME], unsigned nparametros) {
    if (!c || !nome || nparametros > MC_PARAMS) return NULL;

    McMacro *existente = mc_buscar(c, nome);
    if (existente) {
        /* Cria cópia da versão anterior para manter na cadeia */
        McMacro *antiga = (McMacro *)malloc(sizeof(McMacro));
        if (!antiga) return NULL;
        memcpy(antiga, existente, sizeof(McMacro));

        /* Atualiza a entrada ativa com os novos dados e aponta para a anterior */
        memset(existente, 0, sizeof(McMacro));
        strncpy(existente->nome, nome, MC_NOME - 1);
        existente->nome[MC_NOME - 1] = '\0';
        existente->nparametros = nparametros;
        for (unsigned i = 0; i < nparametros; i++) {
            strncpy(existente->parametros[i], parametros[i], MC_NOME - 1);
            existente->parametros[i][MC_NOME - 1] = '\0';
        }
        existente->anterior = antiga;
        return existente;
    }

    /* Nova macro: redimensiona a tabela se necessário */
    if (c->ndefinicoes >= c->armazenadas) {
        unsigned nova_cap = (c->armazenadas == 0) ? 16 : c->armazenadas * 2;
        if (nova_cap > MC_DEFINICOES) nova_cap = MC_DEFINICOES;
        if (c->ndefinicoes >= nova_cap) return NULL;

        McMacro *nova_tabela = (McMacro *)realloc(c->tabela, nova_cap * sizeof(McMacro));
        if (!nova_tabela) return NULL;
        c->tabela = nova_tabela;
        c->armazenadas = nova_cap;
    }

    McMacro *m = &c->tabela[c->ndefinicoes++];
    memset(m, 0, sizeof(McMacro));
    strncpy(m->nome, nome, MC_NOME - 1);
    m->nome[MC_NOME - 1] = '\0';
    m->nparametros = nparametros;
    for (unsigned i = 0; i < nparametros; i++) {
        strncpy(m->parametros[i], parametros[i], MC_NOME - 1);
        m->parametros[i][MC_NOME - 1] = '\0';
    }
    m->anterior = NULL;
    return m;
}

/*
 * Adiciona uma linha de instrução/texto ao corpo da macro.
 */
int mc_adicionar_linha(McContexto *c, McMacro *m, const char *linha) {
    (void)c;
    if (!m || !linha) return 0;

    if (m->nlinhas >= m->capacidade_linhas) {
        unsigned nova_cap = (m->capacidade_linhas == 0) ? 16 : m->capacidade_linhas * 2;
        char **novo_corpo = (char **)realloc(m->corpo, nova_cap * sizeof(char *));
        if (!novo_corpo) return 0;
        m->corpo = novo_corpo;
        m->capacidade_linhas = nova_cap;
    }

    size_t len = strlen(linha);
    char *copia = (char *)malloc(len + 1);
    if (!copia) return 0;
    memcpy(copia, linha, len + 1);

    m->corpo[m->nlinhas++] = copia;
    return 1;
}

/*
 * Libera os recursos de uma macro e de toda a sua cadeia de redefinições anteriores.
 */
void mc_liberar_macro(McMacro *m) {
    if (!m) return;

    if (m->corpo) {
        for (unsigned i = 0; i < m->nlinhas; i++) {
            free(m->corpo[i]);
        }
        free(m->corpo);
        m->corpo = NULL;
    }
    m->nlinhas = 0;
    m->capacidade_linhas = 0;

    if (m->anterior) {
        mc_liberar_macro(m->anterior);
        free(m->anterior);
        m->anterior = NULL;
    }
}

/*
 * Libera todas as macros da tabela e a própria tabela do contexto.
 */
void mc_liberar(McContexto *c) {
    if (!c) return;

    if (c->tabela) {
        for (unsigned i = 0; i < c->ndefinicoes; i++) {
            mc_liberar_macro(&c->tabela[i]);
        }
        free(c->tabela);
        c->tabela = NULL;
    }
    c->ndefinicoes = 0;
    c->armazenadas = 0;
    c->topo = 0;
    c->passos = 0;
    c->emitidas = 0;
}

/*
 * ============================================================================
 * Funções de compatibilidade conceitual do PDF
 * ============================================================================
 */

DefinicaoMacro *macro_definicao_criar(const char *nome, int nivel_definicao) {
    (void)nivel_definicao;
    if (!nome) return NULL;
    DefinicaoMacro *m = (DefinicaoMacro *)calloc(1, sizeof(DefinicaoMacro));
    if (m) {
        strncpy(m->nome, nome, MC_NOME - 1);
        m->nome[MC_NOME - 1] = '\0';
    }
    return m;
}

void macro_definicao_destruir(DefinicaoMacro *macro) {
    if (!macro) return;
    mc_liberar_macro(macro);
    free(macro);
}

int macro_adicionar_parametro(DefinicaoMacro *macro, const char *parametro) {
    return mc_adicionar_parametro(macro, parametro);
}

int macro_buscar_parametro(const DefinicaoMacro *macro, const char *nome) {
    if (!macro || !nome) return -1;
    for (unsigned i = 0; i < macro->nparametros; i++) {
        if (mc_igual(macro->parametros[i], nome)) {
            return (int)i;
        }
    }
    return -1;
}

const char *macro_obter_parametro(const DefinicaoMacro *macro, int indice) {
    if (!macro || indice < 0 || (unsigned)indice >= macro->nparametros) return NULL;
    return macro->parametros[indice];
}

int macro_adicionar_linha(DefinicaoMacro *macro, const char *linha) {
    return mc_adicionar_linha(NULL, macro, linha);
}

const char *macro_obter_linha(const DefinicaoMacro *macro, int indice) {
    if (!macro || indice < 0 || (unsigned)indice >= macro->nlinhas) return NULL;
    return macro->corpo[indice];
}

TabelaMacros *macro_tabela_criar(void) {
    return mc_contexto_criar(NULL, 0);
}

void macro_tabela_destruir(TabelaMacros *tabela) {
    mc_liberar(tabela);
}

void macro_tabela_liberar(TabelaMacros *tabela) {
    if (!tabela) return;
    mc_liberar(tabela);
    free(tabela);
}

int macro_tabela_inserir(TabelaMacros *tabela, DefinicaoMacro *macro) {
    if (!tabela || !macro) return 0;
    McMacro *cadastrada = mc_cadastrar(tabela, macro->nome, (const char (*)[MC_NOME])macro->parametros, macro->nparametros);
    if (!cadastrada) return 0;
    for (unsigned i = 0; i < macro->nlinhas; i++) {
        mc_adicionar_linha(tabela, cadastrada, macro->corpo[i]);
    }
    return 1;
}

DefinicaoMacro *macro_tabela_buscar(const TabelaMacros *tabela, const char *nome) {
    return mc_buscar((McContexto *)tabela, nome);
}
