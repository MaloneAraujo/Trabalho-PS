/* Controlador de uma passagem: conecta parser, tabela e expansao do grupo. */
#include "macros.h"
#include "macros_interno.h"
#include "macros_parser.h"
#include "interno.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ContextoDef ctx_def;
    MacroPrototipo niveis[MC_NIVEIS];
    unsigned profundidade;
    int espera_prototipo;
    McMacro *macro;
} Definicao;

int mc_erro(McContexto *c, const char *formato, ...) {
    va_list args;
    if (c && c->erro && c->tamanho_erro) {
        char mensagem[256];
        va_start(args, formato);
        vsnprintf(mensagem, sizeof mensagem, formato, args);
        va_end(args);
        snprintf(c->erro, c->tamanho_erro, "linha %u: %s", c->linha, mensagem);
    }
    return 0;
}

int mc_identificador(const char *s) { return macro_identificador_valido(s); }

int mc_operacao(const char *s) {
    if (!s || !*s) return 0;
    if (mc_igual(s, "MCDEFN") || mc_igual(s, "MCEND")) return 1;
    return buscar_operacao(*s == '+' ? s + 1 : s) != NULL ||
           buscar_diretiva(s) != DIR_NENHUMA;
}

static int reconhecer(const char *s, void *contexto) {
    return mc_operacao(s) || mc_buscar((McContexto *)contexto, s) != NULL;
}

/* A expansao consulta apenas os delimitadores para adiar .SER.
 * Nao validar rotulos aqui: dentro de uma definicao ainda podem ter .SER. */
int mc_parse(McContexto *c, const char *texto, McLinha *l) {
    MacroLinhaInfo info;
    char erro[256];
    if (!macro_analisar_linha(texto, &info, erro, sizeof erro)) return mc_erro(c, "%s", erro);
    memset(l, 0, sizeof *l);
    l->vazia = info.tipo == MACRO_LINHA_VAZIA || info.tipo == MACRO_LINHA_COMENTARIO;
    if (info.tipo == MACRO_LINHA_DEFINICAO_V1 || info.tipo == MACRO_LINHA_DELIMITADOR_V2)
        strcpy(l->operacao, "MCDEFN");
    if (info.tipo == MACRO_LINHA_FIM_DEFINICAO) strcpy(l->operacao, "MCEND");
    return 1;
}

/* Lista posicional auxiliar; o controlador usa macro_analisar_chamada
 * para tambem validar palavras-chave, defaults e parametros omitidos. */
int mc_argumentos(McContexto *c, const char *texto,
                  char argumentos[MC_PARAMS][MC_TEXTO], unsigned *n) {
    size_t pos = 0;
    int aspas = 0;
    const char *p = texto;
    *n = 0;
    while (isspace((unsigned char)*p)) ++p;
    if (!*p) return 1;
    for (;;) {
        char ch = *p++;
        if (ch == '\'') aspas = !aspas;
        if ((!aspas && ch == ',') || !ch) {
            size_t inicio = 0;
            if (*n >= MC_PARAMS) return mc_erro(c, "argumentos demais");
            argumentos[*n][pos] = 0;
            while (isspace((unsigned char)argumentos[*n][inicio])) ++inicio;
            while (pos > inicio && isspace((unsigned char)argumentos[*n][pos-1])) --pos;
            memmove(argumentos[*n], argumentos[*n] + inicio, pos-inicio);
            argumentos[(*n)++][pos-inicio] = 0;
            pos = 0;
            if (!ch) return aspas ? mc_erro(c, "aspas nao fechadas") : 1;
        } else {
            if (*n >= MC_PARAMS || pos + 1 >= MC_TEXTO) return mc_erro(c, "argumento muito longo");
            argumentos[*n][pos++] = ch;
        }
    }
}

static int anexar(McContexto *c, char *saida, size_t *n, const char *texto, size_t tam) {
    if (*n + tam >= MC_TEXTO) return mc_erro(c, "linha de definicao muito longa");
    memcpy(saida + *n, texto, tam);
    *n += tam;
    saida[*n] = 0;
    return 1;
}

/* Converte nomes formais em #(nivel,indice), respeitando escopo, strings,
 * comentarios e marcadores ja codificados pelos exemplos do grupo. */
static int codificar(McContexto *c, const Definicao *d, const char *p, char *saida) {
    size_t n = 0;
    int aspas = 0;
    const char *inicio = p, *orig = p;
    saida[0] = 0;
    while (isspace((unsigned char)*inicio)) ++inicio;
    if (*inicio == '.' || *inicio == ';') return anexar(c, saida, &n, p, strlen(p));
    while (*p) {
        if (!aspas && *p == ';') {
            if (!anexar(c, saida, &n, p, strlen(p))) return 0;
            break;
        }
        if (*p == '\'') aspas = !aspas;
        if (!aspas && p[0] == '#' && p[1] == '(') {
            const char *fim = strchr(p, ')');
            if (!fim) return mc_erro(c, "marcador de parametro incompleto");
            if (!anexar(c, saida, &n, p, (size_t)(fim-p+1))) return 0;
            p = fim+1;
        } else if (!aspas && (isalpha((unsigned char)*p) || *p == '_')) {
            const char *fim = p;
            char palavra[MC_TEXTO], marcador[40];
            unsigned nivel, i;
            int achou = 0;
            while (isalnum((unsigned char)*fim) || *fim == '_') ++fim;
            memcpy(palavra, p, (size_t)(fim-p)); palavra[fim-p] = 0;
            if (*fim != '=') {
                for (nivel = d->profundidade; nivel && !achou; --nivel)
                    for (i = 0; i < d->niveis[nivel-1].quantidadeParametros; ++i)
                        if (mc_igual(palavra, d->niveis[nivel-1].parametros[i].nome)) {
                            snprintf(marcador, sizeof marcador, "#(%u,%u)", nivel, i);
                            if (!anexar(c, saida, &n, marcador, strlen(marcador))) return 0;
                            achou = 1; break;
                        }
            }
            if (!achou && !anexar(c, saida, &n, p, (size_t)(fim-p))) return 0;
            p = fim;
        } else if (!anexar(c, saida, &n, p++, 1)) return 0;
    }
    trocar_params(&d->ctx_def, orig, saida);
    return 1;
}

static int prototipo(McContexto *c, const MacroPrototipo *p) {
    if (p->quantidadeParametros > MC_PARAMS) return mc_erro(c, "limite de %u parametros", MC_PARAMS);
    if (mc_operacao(p->nome)) return mc_erro(c, "nome de macro reservado: %s", p->nome);
    return 1;
}

static int registrar(McContexto *c, Definicao *d, const MacroPrototipo *p,
                     MacroPrototipo **prototipos) {
    char params[MC_PARAMS][MC_NOME] = {{0}};
    char lista_params[MC_TEXTO] = "";
    char erro_def[256] = "";
    MacroPrototipo *copia;
    unsigned i, indice;
    if (!prototipo(c, p)) return 0;
    if (d->profundidade == MC_NIVEIS) return mc_erro(c, "limite de definicoes aninhadas");
    for (i = 0; i < p->quantidadeParametros; ++i) {
        if (i > 0) strncat(lista_params, ",", sizeof(lista_params) - strlen(lista_params) - 1);
        strncat(lista_params, p->parametros[i].nome, sizeof(lista_params) - strlen(lista_params) - 1);
    }
    if (!empilhar_nivel(&d->ctx_def, p->nome, lista_params, erro_def, sizeof erro_def))
        return mc_erro(c, "%s", erro_def);
    if (!d->profundidade) {
        copia = malloc(sizeof *copia);
        if (!copia) return mc_erro(c, "memoria insuficiente para prototipo");
        *copia = *p;
        for (i = 0; i < p->quantidadeParametros; ++i) strcpy(params[i], p->parametros[i].nome);
        d->macro = mc_cadastrar(c, p->nome, (const char (*)[MC_NOME])params, (unsigned)p->quantidadeParametros);
        if (!d->macro) { free(copia); return mc_erro(c, "falha ao cadastrar macro"); }
        indice = (unsigned)(d->macro - c->tabela);
        free(prototipos[indice]); prototipos[indice] = copia;
    }
    d->niveis[d->profundidade++] = *p;
    return 1;
}

static int guardar(McContexto *c, Definicao *d, const char *linha) {
    if (d->macro->nlinhas >= MC_LINHAS) return mc_erro(c, "limite de linhas na definicao");
    if (!mc_adicionar_linha(c, d->macro, linha)) return mc_erro(c, "memoria insuficiente para corpo da macro");
    return 1;
}

static int emitir(McContexto *c, FILE *saida, const char *linha) {
    if (++c->emitidas > MC_LINHAS) return mc_erro(c, "limite de linhas emitidas");
    if (fprintf(saida, "%s\n", linha) < 0) return mc_erro(c, "falha ao escrever Assembly expandido");
    return 1;
}

/* Prioriza a pilha: cada linha expandida passa novamente pelo controlador. */
static int proxima(McContexto *c, FILE *entrada, char *linha) {
    int r = mc_proxima_expansao(c, linha);
    size_t n;
    if (r) return r;
    if (!fgets(linha, MC_TEXTO, entrada)) {
        if (ferror(entrada)) { mc_erro(c, "falha ao ler entrada"); return -1; }
        return 0;
    }
    if (++c->linha > MC_LINHAS) { mc_erro(c, "limite de linhas de entrada"); return -1; }
    n = strlen(linha);
    if (n == MC_TEXTO-1 && linha[n-1] != '\n' && !feof(entrada)) {
        mc_erro(c, "linha de entrada muito longa"); return -1;
    }
    while (n && (linha[n-1] == '\n' || linha[n-1] == '\r')) linha[--n] = 0;
    if (c->linha == 1 && n >= 3 && (unsigned char)linha[0] == 0xef &&
        (unsigned char)linha[1] == 0xbb && (unsigned char)linha[2] == 0xbf)
        memmove(linha, linha+3, n-2);
    return 1;
}

static int chamada(McContexto *c, FILE *saida, const char *texto,
                    const MacroLinhaInfo *info, MacroPrototipo **prototipos) {
    MacroCamposLinha campos;
    MacroChamada args;
    MacroPrototipo *p;
    McMacro *m;
    char erro[256], reais[MC_PARAMS][MC_TEXTO] = {{0}}, invocacao[MC_TEXTO];
    unsigned i, j;
    int r;
    if (!macro_separar_campos(info, reconhecer, c, &campos, erro, sizeof erro)) return mc_erro(c, "%s", erro);
    m = mc_buscar(c, campos.operacao);
    if (!m) {
        if (!mc_operacao(campos.operacao)) return mc_erro(c, "operacao ou macro nao definida: %s", campos.operacao);
        return emitir(c, saida, texto);
    }
    p = prototipos[m - c->tabela];
    if (!p) return mc_erro(c, "prototipo de macro indisponivel");
    /* O parser V1 nao aceita rotulo na chamada; passa somente operacao/operandos.
     * O rotulo do chamador e preservado por EQU * no endereco da expansao. */
    if (campos.temRotulo && !p->possuiParametroRotulo) {
        if (snprintf(invocacao, sizeof invocacao, "%s %s", campos.operacao, campos.operando) >= (int)sizeof invocacao)
            return mc_erro(c, "chamada muito longa");
    } else snprintf(invocacao, sizeof invocacao, "%s", texto);
    r = macro_analisar_chamada(invocacao, p, &args, erro, sizeof erro);
    if (r != 1) return mc_erro(c, "%s", r < 0 ? erro : "chamada de macro invalida");
    for (i = 0; i < m->nparametros; ++i) {
        if (p->parametros[i].palavraChave) {
            const char *valor = p->parametros[i].valorPadrao;
            for (j = 0; j < args.quantidadeArgumentos; ++j)
                if (mc_igual(args.argumentos[j].nome, p->parametros[i].nome)) { valor = args.argumentos[j].valor; break; }
            snprintf(reais[i], MC_TEXTO, "%s", valor);
        } else snprintf(reais[i], MC_TEXTO, "%s", args.argumentos[i].valor);
    }
    if (!mc_empilhar(c, m, (const char (*)[MC_TEXTO])reais, m->nparametros)) return 0;
    if (campos.temRotulo && !p->possuiParametroRotulo) {
        snprintf(invocacao, sizeof invocacao, "%s EQU *", campos.rotulo);
        if (!emitir(c, saida, invocacao)) return 0;
    }
    return 1;
}

int processar_macros(FILE *entrada, FILE *saida, char *erro, size_t tamanhoErro) {
    McContexto *c = mc_contexto_criar(erro, tamanhoErro);
    Definicao *d = calloc(1, sizeof *d);
    MacroPrototipo *prototipos[MC_DEFINICOES] = {0};
    MacroLinhaInfo *info = malloc(sizeof *info);
    MacroPrototipo *p = malloc(sizeof *p);
    char linha[MC_TEXTO], convertida[MC_TEXTO], detalhe[256];
    unsigned i;
    int r = 0, ok = 0;
    if (!c || !d || !info || !p) {
        if (erro && tamanhoErro) snprintf(erro, tamanhoErro, "memoria insuficiente");
        goto fim;
    }
    iniciar_contexto(&d->ctx_def);
    if (!entrada || !saida || entrada == saida) { mc_erro(c, "arquivos de entrada/saida invalidos"); goto fim; }
    while ((r = proxima(c, entrada, linha)) > 0) {
        if (!macro_analisar_linha(linha, info, detalhe, sizeof detalhe)) { mc_erro(c, "%s", detalhe); goto fim; }
        if (d->espera_prototipo) {
            if (info->tipo == MACRO_LINHA_VAZIA || info->tipo == MACRO_LINHA_COMENTARIO) continue;
            if (info->tipo != MACRO_LINHA_NORMAL || !macro_analisar_prototipo_v2(linha, p, detalhe, sizeof detalhe)) {
                mc_erro(c, "prototipo V2 invalido"); goto fim;
            }
            if (d->profundidade && !guardar(c, d, linha)) goto fim;
            if (!registrar(c, d, p, prototipos)) goto fim;
            d->espera_prototipo = 0;
        } else if (info->tipo == MACRO_LINHA_DELIMITADOR_V2) {
            if (d->profundidade && !guardar(c, d, linha)) goto fim;
            d->espera_prototipo = 1;
        } else if (info->tipo == MACRO_LINHA_DEFINICAO_V1) {
            if (d->profundidade && !guardar(c, d, linha)) goto fim;
            if (!registrar(c, d, &info->prototipo, prototipos)) goto fim;
        } else if (info->tipo == MACRO_LINHA_FIM_DEFINICAO) {
            if (!d->profundidade) { mc_erro(c, "MCEND sem MCDEFN"); goto fim; }
            if (d->profundidade > 1 && !guardar(c, d, linha)) goto fim;
            desempilhar_nivel(&d->ctx_def);
            --d->profundidade;
        } else if (d->profundidade) {
            if (!codificar(c, d, linha, convertida) || !guardar(c, d, convertida)) goto fim;
        } else if (info->tipo == MACRO_LINHA_VAZIA || info->tipo == MACRO_LINHA_COMENTARIO) {
            if (!emitir(c, saida, linha)) goto fim;
        } else if (!chamada(c, saida, linha, info, prototipos)) goto fim;
    }
    if (r < 0) goto fim;
    if (d->profundidade || d->espera_prototipo) { mc_erro(c, "definicao incompleta: falta prototipo ou MCEND"); goto fim; }
    if (fflush(saida) || ferror(saida)) { mc_erro(c, "falha ao finalizar Assembly expandido"); goto fim; }
    ok = 1;
fim:
    for (i = 0; i < MC_DEFINICOES; ++i) free(prototipos[i]);
    if (c) { mc_liberar(c); free(c); }
    free(d); free(info); free(p);
    return ok;
}
