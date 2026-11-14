/* Teste integrado dos modulos de parser, tabela e expansao de macros.
 * Nao substitui o teste ponta a ponta do controlador processar_macros,
 * que ainda nao esta presente neste pacote. */
#include "../montagem/macros_interno.h"
#include "../montagem/macros_parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static int total, falhas;
#define CHECK(cond, nome) do { \
    ++total; \
    if (cond) printf("OK | %s\n", nome); \
    else { printf("FALHA | %s (linha %d)\n", nome, __LINE__); ++falhas; } \
} while (0)

/* Adaptador minimo para o contrato usado pelo modulo de expansao. */
int mc_erro(McContexto *c, const char *formato, ...) {
    va_list args;
    if (c && c->erro && c->tamanho_erro) {
        va_start(args, formato);
        vsnprintf(c->erro, c->tamanho_erro, formato, args);
        va_end(args);
    }
    return 0;
}

int mc_parse(McContexto *c, const char *texto, McLinha *linha) {
    MacroLinhaInfo info;
    char erro[128] = "";
    (void)c;
    if (!macro_analisar_linha(texto, &info, erro, sizeof erro)) return 0;
    memset(linha, 0, sizeof *linha);
    if (info.tipo == MACRO_LINHA_DELIMITADOR_V2) strcpy(linha->operacao, "MCDEFN");
    if (info.tipo == MACRO_LINHA_FIM_DEFINICAO) strcpy(linha->operacao, "MCEND");
    return 1;
}

static int criar_macro(McContexto *c, const char *prototipo,
                       const char *const corpo[], size_t ncorpo,
                       MacroPrototipo *proto_saida) {
    MacroPrototipo p;
    char erro[256] = "";
    char params[MC_PARAMS][MC_NOME] = {{0}};
    McMacro *m;
    size_t i;
    if (!macro_analisar_prototipo_v2(prototipo, &p, erro, sizeof erro)) return 0;
    if (p.quantidadeParametros > MC_PARAMS) return 0;
    for (i = 0; i < p.quantidadeParametros; ++i)
        snprintf(params[i], MC_NOME, "%s", p.parametros[i].nome);
    m = mc_cadastrar(c, p.nome, (const char (*)[MC_NOME])params,
                     (unsigned)p.quantidadeParametros);
    if (!m) return 0;
    for (i = 0; i < ncorpo; ++i)
        if (!mc_adicionar_linha(c, m, corpo[i])) return 0;
    if (proto_saida) *proto_saida = p;
    return 1;
}

int main(void) {
    char erro[256] = "", args[MC_PARAMS][MC_TEXTO] = {{0}};
    char linha[MC_TEXTO];
    McContexto *c = mc_contexto_criar(erro, sizeof erro);
    MacroPrototipo p_soma, p_dupla;
    MacroLinhaInfo info;
    MacroChamada chamada;
    McMacro *soma, *dupla;
    int r;
    const char *corpo_soma[] = {"LDA #(1,0)", "ADD #(1,1)", "STA RESULT"};
    const char *corpo_dupla[] = {"SOMA A=#(1,0),B=#(1,0)"};

    CHECK(c != NULL, "cria contexto da tabela");
    if (!c) return 1;

    CHECK(macro_analisar_linha("MCDEFN", &info, erro, sizeof erro) &&
          info.tipo == MACRO_LINHA_DELIMITADOR_V2, "parser reconhece inicio de definicao");
    CHECK(criar_macro(c, "SOMA A=,B=", corpo_soma, 3, &p_soma),
          "parser e tabela cadastram macro SOMA");
    CHECK(criar_macro(c, "DUPLA X=", corpo_dupla, 1, &p_dupla),
          "parser e tabela cadastram macro aninhada DUPLA");

    soma = mc_buscar(c, "soma");
    dupla = mc_buscar(c, "DUPLA");
    CHECK(soma && dupla && soma->nlinhas == 3 && dupla->nlinhas == 1,
          "busca macros cadastradas e confere corpos");

    strcpy(args[0], "TOTAL");
    CHECK(mc_empilhar(c, dupla, (const char (*)[MC_TEXTO])args, 1),
          "inicia expansao da macro externa");
    r = mc_proxima_expansao(c, linha);
    CHECK(r == 1 && strcmp(linha, "SOMA A=TOTAL,B=TOTAL") == 0,
          "expande parametros da chamada aninhada");

    r = macro_analisar_chamada(linha, &p_soma, &chamada, erro, sizeof erro);
    CHECK(r == 1 && chamada.quantidadeArgumentos == 2,
          "parser reconhece chamada interna apos expansao externa");
    if (r == 1) {
        memset(args, 0, sizeof args);
        snprintf(args[0], MC_TEXTO, "%s", chamada.argumentos[0].valor);
        snprintf(args[1], MC_TEXTO, "%s", chamada.argumentos[1].valor);
        CHECK(mc_empilhar(c, soma, (const char (*)[MC_TEXTO])args, 2),
              "empilha macro interna durante expansao externa");
    } else {
        CHECK(0, "empilha macro interna durante expansao externa");
    }

    CHECK(mc_proxima_expansao(c, linha) == 1 && strcmp(linha, "LDA TOTAL") == 0,
          "expande primeira instrucao da macro interna");
    CHECK(mc_proxima_expansao(c, linha) == 1 && strcmp(linha, "ADD TOTAL") == 0,
          "expande segunda instrucao da macro interna");
    CHECK(mc_proxima_expansao(c, linha) == 1 && strcmp(linha, "STA RESULT") == 0,
          "expande ultima instrucao da macro interna");
    CHECK(mc_proxima_expansao(c, linha) == 0 && c->topo == 0,
          "retorna ao contexto externo e encerra pilha corretamente");

    CHECK(macro_analisar_linha("MCEND", &info, erro, sizeof erro) &&
          info.tipo == MACRO_LINHA_FIM_DEFINICAO, "parser reconhece fim de definicao");

    {
        ContextoDef ctx_def;
        CHECK(iniciar_def(&ctx_def, "EXTERNA", "DEST,VAL", erro, sizeof erro) && ctx_def.d == 1,
              "inicia definicao externa no nivel d=1");
        CHECK(processar_linha_def(&ctx_def, "INTERNA MCDEFN D, V", erro, sizeof erro) == MODO_DEF &&
              ctx_def.d == 2,
              "reconhece definicao aninhada e sobe para nivel d=2");
        CHECK(processar_linha_def(&ctx_def, "LDA D", erro, sizeof erro) == MODO_DEF &&
              strcmp(ctx_def.atual.corpo[1], "LDA #(2,0)") == 0,
              "codifica parametro formal do nivel interno como #(2,0)");
        CHECK(processar_linha_def(&ctx_def, "MCEND", erro, sizeof erro) == MODO_DEF &&
              ctx_def.d == 1,
              "MCEND interno reduz para d=1 sem encerrar macro externa");
        CHECK(processar_linha_def(&ctx_def, "INTERNA DEST, VAL", erro, sizeof erro) == MODO_DEF &&
              strcmp(ctx_def.atual.corpo[3], "INTERNA #(1,0), #(1,1)") == 0,
              "codifica parametros formais do nivel externo como #(1,i)");
        CHECK(processar_linha_def(&ctx_def, "MCEND", erro, sizeof erro) == MODO_NORMAL &&
              ctx_def.d == 0,
              "MCEND externo encerra definicao e retorna ao modo normal");
        mc_liberar_macro(&ctx_def.atual);
    }

    mc_liberar(c);
    free(c);
    printf("\nTeste integrado dos modulos disponiveis: %d verificacoes; %d falhas.\n",
           total, falhas);
    return falhas ? 1 : 0;
}
