#ifndef MACROS_INTERNO_H
#define MACROS_INTERNO_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#define MC_TEXTO 512
#define MC_NOME 32
#define MC_PARAMS 16
#define MC_NIVEIS 32
#define MC_DEFINICOES 512
#define MC_LINHAS 100000u

/*
 * Estrutura de uma macro.
 * Armazena nome, parâmetros formais e corpo textual em Assembly.
 * Suporta redefinições encadeadas via 'anterior' (as versões antigas
 * continuam válidas na pilha de expansão enquanto houver referências ativas).
 */
typedef struct McMacro {
    char nome[MC_NOME];
    char parametros[MC_PARAMS][MC_NOME];
    unsigned nparametros;
    char **corpo;
    unsigned nlinhas;
    unsigned capacidade_linhas;
    struct McMacro *anterior; /* Versões antigas permanecem válidas na pilha */
} McMacro;

typedef struct {
    const McMacro *macro;
    char argumentos[MC_PARAMS][MC_TEXTO];
    unsigned proxima, serial, definicoes;
} McQuadro;

typedef struct {
    McMacro *tabela;
    unsigned ndefinicoes, armazenadas, linha, passos, emitidas, serial, topo;
    McQuadro pilha[MC_NIVEIS];
    char *erro;
    size_t tamanho_erro;
} McContexto;

typedef struct {
    char rotulo[MC_NOME], operacao[MC_NOME], operandos[MC_TEXTO];
    int vazia;
} McLinha;

/*
 * ============================================================================
 * Módulo: Estruturas de Dados e Tabela de Definições (Malone - Integrante 1)
 * ============================================================================
 */

/* Comparação de strings insensível a maiúsculas/minúsculas */
int mc_igual(const char *a, const char *b);

/* Inicialização e criação de contexto/tabela */
void mc_contexto_inicializar(McContexto *c, char *erro, size_t tamanho_erro);
McContexto *mc_contexto_criar(char *erro, size_t tamanho_erro);

/* Busca de macro pelo nome na tabela de definições */
McMacro *mc_buscar(McContexto *c, const char *nome);

/* Cadastro de nova macro ou redefinição encadeada */
McMacro *mc_cadastrar(McContexto *c, const char *nome,
                      const char parametros[MC_PARAMS][MC_NOME], unsigned nparametros);

/* Adiciona uma linha de código ao corpo da macro */
int mc_adicionar_linha(McContexto *c, McMacro *m, const char *linha);

/* Adiciona um parâmetro formal à macro */
int mc_adicionar_parametro(McMacro *m, const char *parametro);

/* Desalocação e liberação de memória */
void mc_liberar_macro(McMacro *m);
void mc_liberar(McContexto *c);

/*
 * ============================================================================
 * Compatibilidade com a especificação conceitual do projeto (PDF pág 5 e 18)
 * ============================================================================
 */
typedef McMacro DefinicaoMacro;
typedef McContexto TabelaMacros;

#define MACRO_NOME_TAM MC_NOME
#define MACRO_LINHA_TAM MC_TEXTO
#define MACRO_MAX_PARAMS MC_PARAMS

DefinicaoMacro *macro_definicao_criar(const char *nome, int nivel_definicao);
void macro_definicao_destruir(DefinicaoMacro *macro);
int macro_adicionar_parametro(DefinicaoMacro *macro, const char *parametro);
int macro_buscar_parametro(const DefinicaoMacro *macro, const char *nome);
const char *macro_obter_parametro(const DefinicaoMacro *macro, int indice);
int macro_adicionar_linha(DefinicaoMacro *macro, const char *linha);
const char *macro_obter_linha(const DefinicaoMacro *macro, int indice);

TabelaMacros *macro_tabela_criar(void);
void macro_tabela_destruir(TabelaMacros *tabela);
void macro_tabela_liberar(TabelaMacros *tabela);
int macro_tabela_inserir(TabelaMacros *tabela, DefinicaoMacro *macro);
DefinicaoMacro *macro_tabela_buscar(const TabelaMacros *tabela, const char *nome);

/*
 * ============================================================================
 * Funções compartilhadas com os outros módulos do grupo
 * ============================================================================
 */
int mc_erro(McContexto *c, const char *formato, ...);
int mc_identificador(const char *s);
int mc_operacao(const char *s);
int mc_parse(McContexto *c, const char *texto, McLinha *l);
int mc_argumentos(McContexto *c, const char *texto,
                  char argumentos[MC_PARAMS][MC_TEXTO], unsigned *n);

/* Módulo de Gabriel Britto (definições aninhadas e parâmetros formais) */
#ifndef MAX_NOME
#define MAX_NOME    MC_NOME
#endif
#ifndef MAX_PARAMS
#define MAX_PARAMS  MC_PARAMS
#endif
#ifndef MAX_LINHAS
#define MAX_LINHAS  256
#endif
#ifndef TAM_LINHA
#define TAM_LINHA   MC_TEXTO
#endif
#ifndef MAX_NIVEL
#define MAX_NIVEL   (MC_NIVEIS + 1)
#endif

#ifndef MODO_NORMAL
#define MODO_NORMAL 0
#define MODO_DEF    1
#define MODO_EXP    2
#endif

#ifndef CONTEXTO_DEF_DEFINIDO
#define CONTEXTO_DEF_DEFINIDO
typedef struct {
    char nome[MAX_NOME];
    char params[MAX_PARAMS][MAX_NOME];
    int  qtd_params;
} NivelDef;

typedef struct {
    int            d;
    NivelDef       pilha[MAX_NIVEL];
    DefinicaoMacro atual;
} ContextoDef;
#endif

void iniciar_contexto(ContextoDef *ctx);
int empilhar_nivel(ContextoDef *ctx, const char *nome, const char *lista_params, char *erro, size_t tam_erro);
void desempilhar_nivel(ContextoDef *ctx);
void trocar_params(const ContextoDef *ctx, const char *linha, char *saida);
int iniciar_def(ContextoDef *ctx, const char *nome, const char *params, char *erro, size_t tam_erro);
int processar_linha_def(ContextoDef *ctx, const char *linha, char *erro, size_t tam_erro);
int definir_macro(FILE *arq, const char *nome, const char *params, char *erro, size_t tam_erro);
int definir_macro_interna(const char **linhas, int total, int *lidas, char *erro, size_t tam_erro);

/* Módulo de João Victor (expansão) */
int mc_empilhar(McContexto *c, const McMacro *m,
                const char argumentos[MC_PARAMS][MC_TEXTO], unsigned n);
int mc_proxima_expansao(McContexto *c, char linha[MC_TEXTO]);

#endif /* MACROS_INTERNO_H */
