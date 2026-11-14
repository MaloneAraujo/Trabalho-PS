#include "macros_interno.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int total, falhas;
#define CHECK(expr, nome) do { total++; if (expr) printf("OK | %s\n", nome); else { printf("FALHA | %s\n", nome); falhas++; } } while (0)
int main(void) {
    char erro[128];
    McContexto *c = mc_contexto_criar(erro, sizeof erro);
    char params[MC_PARAMS][MC_NOME] = {{0}};
    McMacro *m, *antiga;
    CHECK(c != NULL, "cria contexto");
    if (!c) return 1;
    CHECK(mc_igual("soma", "SOMA") && !mc_igual("SOMA", "SOMAR"), "comparacao sem diferenciar caixa");
    m = mc_cadastrar(c, "SOMA", (const char (*)[MC_NOME])params, 0);
    CHECK(m != NULL && mc_buscar(c, "soma") == m, "cadastra e busca macro sem diferenciar caixa");
    CHECK(mc_adicionar_linha(c, m, " ADD A,B") && mc_adicionar_linha(c, m, " RSUB"), "adiciona linhas ao corpo");
    CHECK(m->nlinhas == 2 && strcmp(m->corpo[0], " ADD A,B") == 0, "preserva ordem e conteudo do corpo");
    CHECK(mc_adicionar_parametro(m, "ARG") && m->nparametros == 1, "adiciona parametro formal");
    CHECK(!mc_adicionar_parametro(NULL, "X") && !mc_adicionar_linha(c, NULL, "X"), "rejeita macro nula nas operacoes");
    CHECK(mc_buscar(c, "INEXISTENTE") == NULL, "busca macro inexistente");
    antiga = m;
    memset(params, 0, sizeof params); strcpy(params[0], "NOVO");
    m = mc_cadastrar(c, "SOMA", (const char (*)[MC_NOME])params, 1);
    CHECK(m == antiga && m->anterior != NULL, "redefine macro mantendo versao anterior");
    CHECK(strcmp(m->parametros[0], "NOVO") == 0 && m->anterior->nlinhas == 2, "nova versao atualiza parametros e preserva corpo antigo");
    CHECK(mc_cadastrar(c, "INVALIDA", (const char (*)[MC_NOME])params, MC_PARAMS + 1) == NULL, "rejeita quantidade excessiva de parametros");
    CHECK(mc_adicionar_parametro(m, "X") && m->nparametros == 2, "parametro adicional aceito dentro do limite");
    mc_liberar(c); free(c);
    CHECK(1, "liberacao do contexto executada");
    printf("\nTabela: %d verificacoes; %d falhas.\n", total, falhas);
    return falhas ? 1 : 0;
}
