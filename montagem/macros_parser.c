#include "macros_parser.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Escreve a mensagem de erro. Devolve 0 para eu poder dar return direto. */
static int erro(char *msg, size_t tam, const char *formato, ...) {
    va_list args;

    if (msg != NULL && tam > 0) {
        va_start(args, formato);
        vsnprintf(msg, tam, formato, args);
        va_end(args);
    }
    return 0;
}

static void limparErro(char *msg, size_t tam) {
    if (msg != NULL && tam > 0) msg[0] = '\0';
}

/* So copia se couber inteiro. Se nao couber, deixo o destino quieto. */
static int copiar(char *dest, size_t tam, const char *origem) {
    size_t i, n;

    if (tam == 0) return 0;
    if (origem == NULL) origem = "";
    n = strlen(origem);
    if (n >= tam) return 0;
    for (i = 0; i <= n; i++) dest[i] = origem[i];
    return 1;
}

/* Tira espaco do comeco e do fim da string. */
static void trim(char *s) {
    size_t inicio = 0, fim = strlen(s), i = 0;

    while (s[inicio] != '\0' && isspace((unsigned char)s[inicio])) inicio++;
    while (fim > inicio && isspace((unsigned char)s[fim - 1])) fim--;
    while (inicio < fim) {
        s[i] = s[inicio];
        i++;
        inicio++;
    }
    s[i] = '\0';
}

static char *pularEspacos(char *s) {
    while (*s != '\0' && isspace((unsigned char)*s)) s++;
    return s;
}

static void tirarQuebra(char *s) {
    size_t n = strlen(s);

    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        n--;
        s[n] = '\0';
    }
}

static void tirarDoisPontos(char *s) {
    size_t n = strlen(s);

    if (n > 0 && s[n - 1] == ':') s[n - 1] = '\0';
}

/* Maiuscula fora das aspas. Dentro de C'...' eu nao mexo. */
static void maiusculas(char *s) {
    int aspas = 0;
    size_t i;

    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\'') aspas = 1 - aspas;
        else if (aspas == 0) s[i] = (char)toupper((unsigned char)s[i]);
    }
}

/* Separa o comentario. ';' dentro de aspas nao vale. */
static int separarComentario(char *codigo, char *comentario, size_t tamComentario, char *msg, size_t tamMsg) {
    int aspas = 0;
    size_t i;

    if (comentario != NULL && tamComentario > 0) comentario[0] = '\0';

    for (i = 0; codigo[i] != '\0'; i++) {
        if (codigo[i] == '\'') aspas = 1 - aspas;
        else if (codigo[i] == ';' && aspas == 0) {
            if (comentario != NULL && copiar(comentario, tamComentario, codigo + i) == 0)
                return erro(msg, tamMsg, "comentario muito longo");
            codigo[i] = '\0';
            break;
        }
    }
    if (aspas != 0) return erro(msg, tamMsg, "constante entre aspas nao foi fechada");

    i = strlen(codigo);
    while (i > 0 && isspace((unsigned char)codigo[i - 1])) {
        i--;
        codigo[i] = '\0';
    }
    return 1;
}

/* Corta a proxima palavra. O \0 fica no lugar do espaco. */
static char *proximoToken(char **p) {
    char *inicio;

    *p = pularEspacos(*p);
    inicio = *p;
    while (**p != '\0' && isspace((unsigned char)**p) == 0) (*p)++;
    if (**p != '\0') {
        **p = '\0';
        (*p)++;
    }
    return inicio;
}

int macro_identificador_valido(const char *s) {
    size_t i;

    if (s == NULL || s[0] == '\0') return 0;
    if (isalpha((unsigned char)s[0]) == 0 && s[0] != '_') return 0;
    for (i = 1; s[i] != '\0'; i++) {
        if (isalnum((unsigned char)s[i]) == 0 && s[i] != '_') return 0;
    }
    return 1;
}

static int copiarNome(char *dest, const char *origem, char *msg, size_t tamMsg) {
    const char *texto;

    if (origem == NULL) texto = "";
    else texto = origem;
    if (origem == NULL || origem[0] == '\0' || strlen(origem) >= MACRO_PARSER_TAM_NOME ||
        macro_identificador_valido(origem) == 0)
        return erro(msg, tamMsg, "identificador invalido ou muito longo: '%s'", texto);
    copiar(dest, MACRO_PARSER_TAM_NOME, origem);
    return 1;
}

/* Acha o '=' que nao esta dentro de aspas. */
static char *procurarIgual(char *s) {
    int aspas = 0;
    size_t i;

    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\'') aspas = 1 - aspas;
        else if (s[i] == '=' && aspas == 0) return s + i;
    }
    return NULL;
}

/* Quebra a lista na virgula. Virgula entre aspas continua no mesmo item. */
static int separarLista(const char *s, char itens[][MACRO_PARSER_TAM_OPERANDO], size_t *qtd, char *msg, size_t tamMsg) {
    char item[MACRO_PARSER_TAM_OPERANDO];
    size_t i, n = 0;
    int aspas = 0;

    *qtd = 0;
    if (s == NULL) return 1;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 1;

    for (i = 0; ; i++) {
        char c = s[i];
        if (c == '\'') aspas = 1 - aspas;
        if ((c == ',' && aspas == 0) || c == '\0') {
            item[n] = '\0';
            trim(item);
            if (*qtd >= MACRO_PARSER_MAX_PARAMETROS)
                return erro(msg, tamMsg, "quantidade maxima de parametros excedida (%d)", MACRO_PARSER_MAX_PARAMETROS);
            if (copiar(itens[*qtd], MACRO_PARSER_TAM_OPERANDO, item) == 0)
                return erro(msg, tamMsg, "parametro muito longo");
            (*qtd)++;
            n = 0;
            if (c == '\0') break;
        } else {
            if (n + 1 >= sizeof item)
                return erro(msg, tamMsg, "parametro excede %d caracteres", MACRO_PARSER_TAM_OPERANDO - 1);
            item[n] = c;
            n++;
        }
    }
    if (aspas != 0) return erro(msg, tamMsg, "constante entre aspas nao foi fechada");
    return 1;
}

static int analisarFormais(const char *s, MacroPrototipo *p, char *msg, size_t tamMsg) {
    char itens[MACRO_PARSER_MAX_PARAMETROS][MACRO_PARSER_TAM_OPERANDO];
    size_t qtd, i, j;
    int tipo = -1; /* -1 ainda nao vi, 0 posicional, 1 palavra-chave */

    if (separarLista(s, itens, &qtd, msg, tamMsg) == 0) return 0;

    for (i = 0; i < qtd; i++) {
        char *igual = procurarIgual(itens[i]);
        MacroParametroFormal *f = &p->parametros[i];
        memset(f, 0, sizeof *f);
        if (igual != NULL) {
            *igual = '\0';
            trim(itens[i]);
            trim(igual + 1);
            if (copiarNome(f->nome, itens[i], msg, tamMsg) == 0) return 0;
            f->palavraChave = 1;
            if (igual[1] != '\0') f->temPadrao = 1;
            if (copiar(f->valorPadrao, sizeof f->valorPadrao, igual + 1) == 0)
                return erro(msg, tamMsg, "valor default muito longo");
            if (tipo == 0)
                return erro(msg, tamMsg, "nao misture parametros posicionais e por palavra-chave");
            tipo = 1;
        } else {
            trim(itens[i]);
            if (copiarNome(f->nome, itens[i], msg, tamMsg) == 0) return 0;
            if (tipo == 1)
                return erro(msg, tamMsg, "nao misture parametros posicionais e por palavra-chave");
            tipo = 0;
        }
        for (j = 0; j < i; j++) {
            if (strcmp(p->parametros[j].nome, f->nome) == 0)
                return erro(msg, tamMsg, "parametro formal repetido: '%s'", f->nome);
        }
    }
    p->quantidadeParametros = qtd;
    return 1;
}

static int analisarArgumentos(const char *s, MacroChamada *c, char *msg, size_t tamMsg) {
    char itens[MACRO_PARSER_MAX_PARAMETROS][MACRO_PARSER_TAM_OPERANDO];
    size_t qtd, i, j;

    if (separarLista(s, itens, &qtd, msg, tamMsg) == 0) return 0;

    for (i = 0; i < qtd; i++) {
        char *igual = procurarIgual(itens[i]);
        MacroArgumento *a = &c->argumentos[i];
        memset(a, 0, sizeof *a);
        if (igual != NULL) {
            *igual = '\0';
            trim(itens[i]);
            trim(igual + 1);
            if (copiarNome(a->nome, itens[i], msg, tamMsg) == 0) return 0;
            a->palavraChave = 1;
            if (copiar(a->valor, sizeof a->valor, igual + 1) == 0)
                return erro(msg, tamMsg, "argumento muito longo");
            for (j = 0; j < i; j++) {
                if (c->argumentos[j].palavraChave != 0 &&
                    strcmp(c->argumentos[j].nome, a->nome) == 0)
                    return erro(msg, tamMsg, "argumento repetido: '%s'", a->nome);
            }
        } else if (copiar(a->valor, sizeof a->valor, itens[i]) == 0) {
            return erro(msg, tamMsg, "argumento muito longo");
        }
    }
    c->quantidadeArgumentos = qtd;
    return 1;
}

static int validarArgumentos(const MacroPrototipo *p, const MacroChamada *c, char *msg, size_t tamMsg) {
    size_t i, j;
    int porNome = 0;
    int achou;

    if (p->quantidadeParametros > 0 && p->parametros[0].palavraChave != 0) porNome = 1;
    if (p->quantidadeParametros > MACRO_PARSER_MAX_PARAMETROS)
        return erro(msg, tamMsg, "prototipo possui parametros demais");

    if (porNome == 0) {
        if (c->quantidadeArgumentos != p->quantidadeParametros)
            return erro(msg, tamMsg, "quantidade de argumentos incorreta: esperados %u, recebidos %u", (unsigned)p->quantidadeParametros, (unsigned)c->quantidadeArgumentos);
        for (i = 0; i < c->quantidadeArgumentos; i++) {
            if (c->argumentos[i].palavraChave != 0)
                return erro(msg, tamMsg, "macro espera argumentos posicionais");
        }
    } else {
        for (i = 0; i < c->quantidadeArgumentos; i++) {
            if (c->argumentos[i].palavraChave == 0)
                return erro(msg, tamMsg, "macro espera argumentos por palavra-chave");
            achou = 0;
            for (j = 0; j < p->quantidadeParametros; j++) {
                if (strcmp(c->argumentos[i].nome, p->parametros[j].nome) == 0) {
                    achou = 1;
                    break;
                }
            }
            if (achou == 0)
                return erro(msg, tamMsg, "parametro desconhecido na chamada: '%s'", c->argumentos[i].nome);
        }
        for (i = 0; i < p->quantidadeParametros; i++) {
            achou = 0;
            for (j = 0; j < c->quantidadeArgumentos; j++) {
                if (strcmp(p->parametros[i].nome, c->argumentos[j].nome) == 0) {
                    achou = 1;
                    break;
                }
            }
            if (achou == 0 && p->parametros[i].temPadrao == 0)
                return erro(msg, tamMsg, "parametro obrigatorio ausente: '%s'", p->parametros[i].nome);
        }
    }
    return 1;
}

/* Classifica a linha: vazia, comentario, MCEND, MCDEFN ou definicao V1. */
int macro_analisar_linha(const char *linha, MacroLinhaInfo *r, char *msg, size_t tamMsg) {
    char trabalho[MACRO_PARSER_TAM_LINHA];
    char *p, *primeiro, *segundo;

    limparErro(msg, tamMsg);
    if (linha == NULL || r == NULL) return erro(msg, tamMsg, "linha ou resultado nulo");
    if (strlen(linha) >= MACRO_PARSER_TAM_LINHA)
        return erro(msg, tamMsg, "linha excede o limite de %d caracteres", MACRO_PARSER_TAM_LINHA - 1);

    memset(r, 0, sizeof *r);
    copiar(r->original, sizeof r->original, linha);
    tirarQuebra(r->original);
    copiar(r->codigo, sizeof r->codigo, r->original);
    p = pularEspacos(r->codigo);
    if (*p == '\0') {
        r->tipo = MACRO_LINHA_VAZIA;
        return 1;
    }
    if (*p == '.' || *p == ';') {
        r->tipo = MACRO_LINHA_COMENTARIO;
        copiar(r->comentario, sizeof r->comentario, p);
        r->codigo[0] = '\0';
        return 1;
    }
    if (separarComentario(r->codigo, r->comentario, sizeof r->comentario, msg, tamMsg) == 0) return 0;
    maiusculas(r->codigo);
    p = pularEspacos(r->codigo);
    if (*p == '\0') {
        r->tipo = MACRO_LINHA_VAZIA;
        return 1;
    }
    if (copiar(trabalho, sizeof trabalho, p) == 0)
        return erro(msg, tamMsg, "linha de trabalho muito longa");

    p = trabalho;
    primeiro = proximoToken(&p);
    tirarDoisPontos(primeiro);
    if (strcmp(primeiro, "MCEND") == 0) {
        segundo = proximoToken(&p);
        if (segundo[0] != '\0') return erro(msg, tamMsg, "MCEND nao deve possuir outros campos");
        r->tipo = MACRO_LINHA_FIM_DEFINICAO;
        return 1;
    }
    if (strcmp(primeiro, "MCDEFN") == 0) {
        segundo = proximoToken(&p);
        if (segundo[0] != '\0') return erro(msg, tamMsg, "na versao 2, MCDEFN deve aparecer sozinho");
        r->tipo = MACRO_LINHA_DELIMITADOR_V2;
        return 1;
    }
    segundo = proximoToken(&p);
    if (strcmp(segundo, "MCDEFN") == 0) {
        if (copiarNome(r->prototipo.nome, primeiro, msg, tamMsg) == 0) return 0;
        r->prototipo.sintaxe = MACRO_SINTAXE_V1;
        r->prototipo.possuiParametroRotulo = 0;
        if (analisarFormais(p, &r->prototipo, msg, tamMsg) == 0) return 0;
        r->tipo = MACRO_LINHA_DEFINICAO_V1;
    } else {
        r->tipo = MACRO_LINHA_NORMAL;
    }
    return 1;
}

/* Prototipo da versao 2, a linha que vem depois do MCDEFN sozinho. */
int macro_analisar_prototipo_v2(const char *linha, MacroPrototipo *r, char *msg, size_t tamMsg) {
    char codigo[MACRO_PARSER_TAM_LINHA], lista[MACRO_PARSER_TAM_LINHA];
    char formais[MACRO_PARSER_TAM_LINHA];
    char *p, *primeiro, *segundo, *resto, *igual;
    MacroPrototipo outros;
    int temRotulo = 0;
    size_t i, j;

    formais[0] = '\0';
    limparErro(msg, tamMsg);
    if (linha == NULL || r == NULL) return erro(msg, tamMsg, "linha ou prototipo nulo");
    if (strlen(linha) >= sizeof codigo)
        return erro(msg, tamMsg, "linha de prototipo muito longa");
    copiar(codigo, sizeof codigo, linha);
    tirarQuebra(codigo);
    if (separarComentario(codigo, NULL, 0, msg, tamMsg) == 0) return 0;
    maiusculas(codigo);

    p = codigo;
    primeiro = proximoToken(&p);
    segundo = proximoToken(&p);
    resto = pularEspacos(p);
    if (primeiro[0] == '\0') return erro(msg, tamMsg, "prototipo vazio");
    memset(r, 0, sizeof *r);
    r->sintaxe = MACRO_SINTAXE_V2;

    if (segundo[0] == '\0') {
        if (copiarNome(r->nome, primeiro, msg, tamMsg) == 0) return 0;
    } else {
        if (strlen(segundo) + strlen(resto) + 2 >= sizeof lista)
            return erro(msg, tamMsg, "lista de parametros muito longa");
        strcpy(lista, segundo);
        if (resto[0] != '\0') {
            strcat(lista, " ");
            strcat(lista, resto);
        }
        igual = procurarIgual(lista);
        /* Se tem '=', o primeiro nome e a macro. Se nao tem, e rotulo + macro. */
        if (igual != NULL) {
            if (copiarNome(r->nome, primeiro, msg, tamMsg) == 0) return 0;
            copiar(formais, sizeof formais, lista);
        } else {
            if (macro_identificador_valido(primeiro) == 0)
                return erro(msg, tamMsg, "parametro formal de rotulo invalido: '%s'", primeiro);
            if (copiarNome(r->nome, segundo, msg, tamMsg) == 0) return 0;
            r->possuiParametroRotulo = 1;
            temRotulo = 1;
            copiar(formais, sizeof formais, resto);
        }
    }

    memset(&outros, 0, sizeof outros);
    if (analisarFormais(formais, &outros, msg, tamMsg) == 0) return 0;
    if (temRotulo == 1) {
        if (copiarNome(r->parametros[0].nome, primeiro, msg, tamMsg) == 0) return 0;
        r->quantidadeParametros = 1;
    }
    if (r->quantidadeParametros + outros.quantidadeParametros > MACRO_PARSER_MAX_PARAMETROS)
        return erro(msg, tamMsg, "quantidade maxima de parametros excedida (%d)", MACRO_PARSER_MAX_PARAMETROS);
    for (i = 0; i < outros.quantidadeParametros; i++)
        r->parametros[r->quantidadeParametros + i] = outros.parametros[i];
    r->quantidadeParametros = r->quantidadeParametros + outros.quantidadeParametros;

    for (i = 0; i < r->quantidadeParametros; i++) {
        for (j = 0; j < i; j++) {
            if (strcmp(r->parametros[i].nome, r->parametros[j].nome) == 0)
                return erro(msg, tamMsg, "parametro formal repetido: '%s'", r->parametros[i].nome);
        }
    }
    return 1;
}

/* Separa rotulo, operacao e operando. O callback diz se a palavra e instrucao. */
int macro_separar_campos(const MacroLinhaInfo *linha, MacroReconhecerOperacao reconhecer, void *contexto, MacroCamposLinha *campos, char *msg, size_t tamMsg) {
    char copia[MACRO_PARSER_TAM_LINHA];
    char *p, *primeiro, *resto, *operacao;
    int indentada = 0, primeiraEhOperacao = 0, semRotulo = 0;

    limparErro(msg, tamMsg);
    if (linha == NULL || campos == NULL) return erro(msg, tamMsg, "linha ou campos nulo");
    memset(campos, 0, sizeof *campos);
    copiar(campos->comentario, sizeof campos->comentario, linha->comentario);
    if (linha->tipo == MACRO_LINHA_VAZIA || linha->tipo == MACRO_LINHA_COMENTARIO) return 1;
    if (copiar(copia, sizeof copia, linha->codigo) == 0)
        return erro(msg, tamMsg, "linha muito longa");
    p = pularEspacos(copia);
    if (*p == '\0') return 1;
    if (linha->original[0] == ' ' || linha->original[0] == '\t') indentada = 1;
    primeiro = proximoToken(&p);
    resto = pularEspacos(p);
    if (reconhecer != NULL) primeiraEhOperacao = reconhecer(primeiro, contexto);

    /* Sem rotulo: a primeira palavra ja e operacao, ou a linha comeca
       com espaco e nao tem callback, ou so existe uma palavra. */
    if (primeiraEhOperacao != 0) semRotulo = 1;
    else if (reconhecer == NULL && indentada == 1) semRotulo = 1;
    else if (resto[0] == '\0') semRotulo = 1;

    if (semRotulo == 1) {
        operacao = primeiro;
        if (copiar(campos->operacao, sizeof campos->operacao, operacao) == 0 ||
            copiar(campos->operando, sizeof campos->operando, resto) == 0)
            return erro(msg, tamMsg, "operacao ou operando muito longo");
    } else {
        tirarDoisPontos(primeiro);
        if (copiarNome(campos->rotulo, primeiro, msg, tamMsg) == 0) return 0;
        campos->temRotulo = 1;
        p = resto;
        operacao = proximoToken(&p);
        if (copiar(campos->operacao, sizeof campos->operacao, operacao) == 0 ||
            copiar(campos->operando, sizeof campos->operando, pularEspacos(p)) == 0)
            return erro(msg, tamMsg, "operacao ou operando muito longo");
    }
    return 1;
}

/* 1 se for chamada desta macro, 0 se for outra linha, -1 se a chamada estiver errada. */
int macro_analisar_chamada(const char *linha, const MacroPrototipo *p, MacroChamada *r, char *msg, size_t tamMsg) {
    char codigo[MACRO_PARSER_TAM_LINHA];
    char *cursor, *primeiro, *resto, *q, *operacao;
    MacroChamada outros;
    int reconhecida = 0;
    size_t i;

    limparErro(msg, tamMsg);
    if (linha == NULL || p == NULL || r == NULL || p->nome[0] == '\0') {
        erro(msg, tamMsg, "linha, prototipo ou resultado invalido");
        return -1;
    }
    if (strlen(linha) >= sizeof codigo) {
        erro(msg, tamMsg, "linha de chamada muito longa");
        return -1;
    }
    copiar(codigo, sizeof codigo, linha);
    tirarQuebra(codigo);
    if (separarComentario(codigo, NULL, 0, msg, tamMsg) == 0) return -1;
    maiusculas(codigo);
    cursor = pularEspacos(codigo);
    if (cursor[0] == '\0' || cursor[0] == '.') return 0;
    primeiro = proximoToken(&cursor);
    resto = pularEspacos(cursor);
    memset(r, 0, sizeof *r);
    copiar(r->nome, sizeof r->nome, p->nome);
    r->sintaxe = p->sintaxe;

    if (p->sintaxe == MACRO_SINTAXE_V1) {
        tirarDoisPontos(primeiro);
        if (strcmp(primeiro, p->nome) != 0) return 0;
        reconhecida = 1;
        if (analisarArgumentos(resto, r, msg, tamMsg) == 0) return -1;
    } else if (p->sintaxe == MACRO_SINTAXE_V2) {
        if (p->possuiParametroRotulo != 0) {
            if (strcmp(primeiro, p->nome) == 0) {
                /* Chamou sem rotulo: o primeiro argumento fica vazio. */
                r->argumentos[0].valor[0] = '\0';
                r->quantidadeArgumentos = 1;
            } else {
                q = resto;
                operacao = proximoToken(&q);
                tirarDoisPontos(primeiro);
                tirarDoisPontos(operacao);
                if (strcmp(operacao, p->nome) != 0) return 0;
                if (copiarNome(r->argumentos[0].valor, primeiro, msg, tamMsg) == 0) return -1;
                r->quantidadeArgumentos = 1;
                resto = pularEspacos(q);
            }
            reconhecida = 1;
            memset(&outros, 0, sizeof outros);
            if (analisarArgumentos(resto, &outros, msg, tamMsg) == 0) return -1;
            if (r->quantidadeArgumentos + outros.quantidadeArgumentos > MACRO_PARSER_MAX_PARAMETROS) {
                erro(msg, tamMsg, "argumentos demais");
                return -1;
            }
            for (i = 0; i < outros.quantidadeArgumentos; i++)
                r->argumentos[r->quantidadeArgumentos + i] = outros.argumentos[i];
            r->quantidadeArgumentos = r->quantidadeArgumentos + outros.quantidadeArgumentos;
        } else {
            tirarDoisPontos(primeiro);
            if (strcmp(primeiro, p->nome) != 0) return 0;
            reconhecida = 1;
            if (analisarArgumentos(resto, r, msg, tamMsg) == 0) return -1;
        }
    } else {
        erro(msg, tamMsg, "versao de sintaxe desconhecida");
        return -1;
    }
    if (reconhecida != 0 && validarArgumentos(p, r, msg, tamMsg) == 0) return -1;
    if (reconhecida != 0) return 1;
    return 0;
}
