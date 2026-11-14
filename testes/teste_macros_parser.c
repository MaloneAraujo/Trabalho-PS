#include "../montagem/macros_parser.h"

#include <stdio.h>
#include <string.h>

static unsigned passaram = 0;
static unsigned falharam = 0;

static void conferir(int condicao, const char *nome) {
    if (condicao) {
        passaram++;
        printf("OK    | %s\n", nome);
    } else {
        falharam++;
        printf("FALHA | %s\n", nome);
    }
}

/* Simula a consulta a MOT/POT que o controlador vai fornecer. */
static int operacao_conhecida(const char *nome, void *contexto) {
    static const char *operacoes[] = {
        "LDA", "ADD", "STA", "START", "END", "RESW", "RESB", "J", "INCR"
    };
    size_t i;
    (void)contexto;

    for (i = 0; i < sizeof operacoes / sizeof operacoes[0]; i++) {
        if (strcmp(nome, operacoes[i]) == 0) return 1;
    }
    return 0;
}

static void testar_linhas(void) {
    MacroLinhaInfo linha;
    char erro[256] = "";
    int ok;

    ok = macro_analisar_linha("INCR MCDEFN L, A", &linha, erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_DEFINICAO_V1 &&
             strcmp(linha.prototipo.nome, "INCR") == 0 &&
             linha.prototipo.quantidadeParametros == 2 &&
             strcmp(linha.prototipo.parametros[0].nome, "L") == 0 &&
             strcmp(linha.prototipo.parametros[1].nome, "A") == 0,
             "reconhece definicao V1 e separa parametros formais");

    ok = macro_analisar_linha(" incr: mcdefn L, A ; comentario", &linha,
                              erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_DEFINICAO_V1 &&
             strcmp(linha.prototipo.nome, "INCR") == 0 &&
             strcmp(linha.comentario, "; comentario") == 0,
             "aceita nome em minusculas, dois-pontos e comentario");

    ok = macro_analisar_linha(" MCDEFN ; delimitador V2", &linha,
                              erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_DELIMITADOR_V2,
             "reconhece MCDEFN isolado da versao 2");

    ok = macro_analisar_linha("MCEND", &linha, erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_FIM_DEFINICAO,
             "reconhece MCEND");

    ok = macro_analisar_linha("MCEND EXTRA", &linha, erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "rejeita MCEND com campos excedentes");

    ok = macro_analisar_linha("MCDEFN EXTRA", &linha, erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "rejeita MCDEFN V2 com campos excedentes");

    ok = macro_analisar_linha("   ", &linha, erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_VAZIA, "reconhece linha vazia");

    ok = macro_analisar_linha(". comentario inteiro", &linha, erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_COMENTARIO,
             "reconhece comentario iniciado por ponto");

    ok = macro_analisar_linha("; comentario inteiro", &linha, erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_COMENTARIO,
             "reconhece comentario iniciado por ponto-e-virgula");

    ok = macro_analisar_linha("LOOP LDA C'abc;DEF' ; comentario final",
                              &linha, erro, sizeof erro);
    conferir(ok && linha.tipo == MACRO_LINHA_NORMAL &&
             strcmp(linha.codigo, "LOOP LDA C'abc;DEF'") == 0 &&
             strcmp(linha.comentario, "; comentario final") == 0,
             "preserva instrucao completa, constante e comentario final");

    ok = macro_analisar_linha("MACRO MCDEFN A, A", &linha, erro, sizeof erro);
    conferir(!ok && strstr(erro, "repetido") != NULL,
             "rejeita parametros formais repetidos");

    ok = macro_analisar_linha("MACRO MCDEFN A, 'SEM FIM", &linha,
                              erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "rejeita constante sem aspas de fechamento");

    ok = macro_analisar_linha("1MACRO MCDEFN A", &linha, erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "rejeita nome de macro iniciado por numero");
}

static void testar_prototipo_v2(void) {
    MacroPrototipo prototipo;
    char erro[256] = "";
    int ok;

    ok = macro_analisar_prototipo_v2("L INCR A", &prototipo, erro, sizeof erro);
    conferir(ok && prototipo.sintaxe == MACRO_SINTAXE_V2 &&
             prototipo.possuiParametroRotulo &&
             strcmp(prototipo.nome, "INCR") == 0 &&
             prototipo.quantidadeParametros == 2 &&
             strcmp(prototipo.parametros[0].nome, "L") == 0 &&
             strcmp(prototipo.parametros[1].nome, "A") == 0,
             "separa prototipo V2 com parametro de rotulo");

    ok = macro_analisar_prototipo_v2("INCR L=,A=ACCUM,B=1,C=ACCUM",
                                     &prototipo, erro, sizeof erro);
    conferir(ok && !prototipo.possuiParametroRotulo &&
             prototipo.quantidadeParametros == 4 &&
             prototipo.parametros[0].palavraChave &&
             !prototipo.parametros[0].temPadrao &&
             prototipo.parametros[0].valorPadrao[0] == '\0' &&
             prototipo.parametros[1].temPadrao &&
             strcmp(prototipo.parametros[1].valorPadrao, "ACCUM") == 0 &&
             strcmp(prototipo.parametros[2].valorPadrao, "1") == 0,
             "le parametros por palavra-chave e defaults inclusive vazio");

    ok = macro_analisar_prototipo_v2("INCR L = , A = 5", &prototipo,
                                     erro, sizeof erro);
    conferir(ok && prototipo.quantidadeParametros == 2 &&
             strcmp(prototipo.parametros[0].nome, "L") == 0 &&
             strcmp(prototipo.parametros[1].valorPadrao, "5") == 0,
             "aceita espacos ao redor de igual");

    ok = macro_analisar_prototipo_v2("INCR", &prototipo, erro, sizeof erro);
    conferir(ok && prototipo.quantidadeParametros == 0 &&
             !prototipo.possuiParametroRotulo,
             "aceita prototipo sem parametros");

    ok = macro_analisar_prototipo_v2("INCR L=,L=2", &prototipo,
                                     erro, sizeof erro);
    conferir(!ok && strstr(erro, "repetido") != NULL,
             "rejeita nome formal repetido em prototipo V2");

    ok = macro_analisar_prototipo_v2("INCR A, B=2", &prototipo,
                                     erro, sizeof erro);
    conferir(!ok && strstr(erro, "misture") != NULL,
             "rejeita mistura de parametro posicional e palavra-chave");

    ok = macro_analisar_prototipo_v2("", &prototipo, erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "rejeita prototipo vazio");
}

static void testar_chamadas_v1(void) {
    MacroLinhaInfo linha;
    MacroChamada chamada;
    char erro[256] = "";
    int r;

    macro_analisar_linha("INCR MCDEFN L, A", &linha, erro, sizeof erro);

    r = macro_analisar_chamada("INCR MARK, COUNT", &linha.prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 1 && chamada.quantidadeArgumentos == 2 &&
             strcmp(chamada.argumentos[0].valor, "MARK") == 0 &&
             strcmp(chamada.argumentos[1].valor, "COUNT") == 0,
             "reconhece chamada V1 com argumentos posicionais");

    r = macro_analisar_chamada("INCR , COUNT", &linha.prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 1 && chamada.quantidadeArgumentos == 2 &&
             chamada.argumentos[0].valor[0] == '\0' &&
             strcmp(chamada.argumentos[1].valor, "COUNT") == 0,
             "preserva parametro posicional omitido");

    r = macro_analisar_chamada("INCR MARK", &linha.prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == -1 && strstr(erro, "quantidade") != NULL,
             "rejeita quantidade incorreta de argumentos posicionais");

    r = macro_analisar_chamada("OUTRA A, B", &linha.prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 0 && erro[0] == '\0',
             "retorna zero quando a linha nao e chamada desta macro");

    r = macro_analisar_chamada("INCR MARK, C'UM, DOIS' ; comentario",
                               &linha.prototipo, &chamada, erro, sizeof erro);
    conferir(r == 1 && strcmp(chamada.argumentos[1].valor, "C'UM, DOIS'") == 0,
             "nao separa virgula dentro de constante entre aspas");
}

static void testar_chamadas_v2(void) {
    MacroPrototipo prototipo;
    MacroChamada chamada;
    char erro[256] = "";
    int r;

    macro_analisar_prototipo_v2("L INCR A", &prototipo, erro, sizeof erro);

    r = macro_analisar_chamada("MARK INCR COUNT", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 1 && chamada.quantidadeArgumentos == 2 &&
             strcmp(chamada.argumentos[0].valor, "MARK") == 0 &&
             strcmp(chamada.argumentos[1].valor, "COUNT") == 0,
             "chamada V2 com rotulo");

    r = macro_analisar_chamada("INCR COUNT", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 1 && chamada.quantidadeArgumentos == 2 &&
             chamada.argumentos[0].valor[0] == '\0' &&
             strcmp(chamada.argumentos[1].valor, "COUNT") == 0,
             "chamada V2 sem rotulo");

    macro_analisar_prototipo_v2("INCR L=,A=ACCUM,B=1", &prototipo,
                                erro, sizeof erro);

    r = macro_analisar_chamada("INCR L=MARK,A=COUNT,B=1", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 1 && chamada.quantidadeArgumentos == 3 &&
             chamada.argumentos[0].palavraChave &&
             strcmp(chamada.argumentos[0].nome, "L") == 0 &&
             strcmp(chamada.argumentos[0].valor, "MARK") == 0,
             "chamada V2 por palavra-chave");

    r = macro_analisar_chamada("INCR A=COUNT,B=2", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == -1 && strstr(erro, "obrigatorio ausente") != NULL,
             "rejeita parametro sem default que foi omitido");

    r = macro_analisar_chamada("INCR L=MARK", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == 1 && chamada.quantidadeArgumentos == 1,
             "aceita omissao dos parametros que possuem default");

    r = macro_analisar_chamada("INCR L=MARK,L=OUTRO", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == -1 && strstr(erro, "repetido") != NULL,
             "rejeita argumento por palavra-chave repetido");

    r = macro_analisar_chamada("INCR L=MARK,Z=OUTRO", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == -1 && strstr(erro, "desconhecido") != NULL,
             "rejeita parametro nomeado que nao existe no prototipo");

    r = macro_analisar_chamada("INCR MARK, COUNT", &prototipo,
                               &chamada, erro, sizeof erro);
    conferir(r == -1 && strstr(erro, "palavra-chave") != NULL,
             "rejeita argumentos posicionais em macro por palavra-chave");
}

static void testar_separacao_campos(void) {
    MacroLinhaInfo linha;
    MacroCamposLinha campos;
    char erro[256] = "";
    int ok;

    ok = macro_analisar_linha("LOOP: LDA C'abc;Def' ; comentario",
                              &linha, erro, sizeof erro);
    ok = ok && macro_separar_campos(&linha, operacao_conhecida, NULL,
                                    &campos, erro, sizeof erro);
    conferir(ok && campos.temRotulo &&
             strcmp(campos.rotulo, "LOOP") == 0 &&
             strcmp(campos.operacao, "LDA") == 0 &&
             strcmp(campos.operando, "C'abc;Def'") == 0 &&
             strcmp(campos.comentario, "; comentario") == 0,
             "separa rotulo, operacao, operando e comentario");

    ok = macro_analisar_linha("  LDA #10", &linha, erro, sizeof erro);
    ok = ok && macro_separar_campos(&linha, operacao_conhecida, NULL,
                                    &campos, erro, sizeof erro);
    conferir(ok && !campos.temRotulo &&
             strcmp(campos.operacao, "LDA") == 0 &&
             strcmp(campos.operando, "#10") == 0,
             "separa instrucao sem rotulo com indentacao");

    ok = macro_analisar_linha("START 0", &linha, erro, sizeof erro);
    ok = ok && macro_separar_campos(&linha, operacao_conhecida, NULL,
                                    &campos, erro, sizeof erro);
    conferir(ok && !campos.temRotulo &&
             strcmp(campos.operacao, "START") == 0 &&
             strcmp(campos.operando, "0") == 0,
             "usa callback para reconhecer operacao sem indentacao");

    ok = macro_analisar_linha("  ADD A, B", &linha, erro, sizeof erro);
    ok = ok && macro_separar_campos(&linha, NULL, NULL,
                                    &campos, erro, sizeof erro);
    conferir(ok && !campos.temRotulo &&
             strcmp(campos.operacao, "ADD") == 0 &&
             strcmp(campos.operando, "A, B") == 0,
             "usa indentacao quando callback nao foi informado");
}

static void testar_erros_e_limites(void) {
    MacroLinhaInfo linha;
    MacroChamada chamada;
    char erro[256] = "";
    char grande[MACRO_PARSER_TAM_LINHA + 1];
    char lista[MACRO_PARSER_TAM_LINHA];
    char definicao[MACRO_PARSER_TAM_LINHA];
    size_t i, usados = 0;
    int ok;

    ok = macro_analisar_linha(NULL, &linha, erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "trata ponteiro de linha nulo");

    ok = macro_analisar_linha("MCDEFN", NULL, erro, sizeof erro);
    conferir(!ok && erro[0] != '\0', "trata ponteiro de resultado nulo");

    ok = macro_analisar_chamada("INCR A", NULL, &chamada, erro, sizeof erro);
    conferir(ok == -1 && erro[0] != '\0', "trata prototipo nulo na chamada");

    memset(grande, 'A', sizeof grande - 1);
    grande[sizeof grande - 1] = '\0';
    ok = macro_analisar_linha(grande, &linha, erro, sizeof erro);
    conferir(!ok && strstr(erro, "excede") != NULL,
             "rejeita linha maior que o limite");

    for (i = 0; i < MACRO_PARSER_MAX_PARAMETROS + 1; i++) {
        int n = snprintf(lista + usados, sizeof lista - usados,
                         "%sP%zu", i == 0 ? "" : ",", i);
        if (n < 0 || (size_t)n >= sizeof lista - usados) break;
        usados += (size_t)n;
    }
    strcpy(definicao, "MACRO MCDEFN ");
    strcat(definicao, lista);
    ok = macro_analisar_linha(definicao, &linha, erro, sizeof erro);
    conferir(!ok && strstr(erro, "maxima") != NULL,
             "rejeita mais de 32 parametros formais");

    ok = macro_analisar_chamada("INCR A, 'SEM FIM", NULL,
                                &chamada, erro, sizeof erro);
    conferir(ok == -1, "retorna erro para chamada com aspas sem fechamento");

    conferir(macro_identificador_valido("NOME") &&
             macro_identificador_valido("_ROTULO9") &&
             macro_identificador_valido("MACRO_2"),
             "aceita identificadores validos");
    conferir(!macro_identificador_valido("") &&
             !macro_identificador_valido("9NOME") &&
             !macro_identificador_valido("NOME-ERRADO") &&
             !macro_identificador_valido(NULL),
             "rejeita identificadores invalidos");
}

int main(void) {
    testar_linhas();
    testar_prototipo_v2();
    testar_chamadas_v1();
    testar_chamadas_v2();
    testar_separacao_campos();
    testar_erros_e_limites();

    printf("\nTestes do parser: %u aprovados; %u falharam.\n",
           passaram, falharam);
    return falharam == 0 ? 0 : 1;
}
