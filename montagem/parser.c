#include "interno.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

int erro_montagem(Montagem *m, const char *formato, ...) {
    va_list args;
    int n = snprintf(m->erro, sizeof m->erro, "Linha %u: ", m->linha_atual);
    va_start(args, formato);
    vsnprintf(m->erro + n, sizeof m->erro - (size_t)n, formato, args);
    va_end(args);
    return 0;
}

static int identificador(const char *s) {
    if (!isalpha((unsigned char)*s) && *s != '_') return 0;
    for (++s; *s; ++s) if (!isalnum((unsigned char)*s) && *s != '_') return 0;
    return 1;
}

static char *token(char **cursor) {
    char *inicio;
    while (isspace((unsigned char)**cursor)) ++*cursor;
    inicio = *cursor;
    while (**cursor && !isspace((unsigned char)**cursor)) ++*cursor;
    if (**cursor) *(*cursor)++ = '\0';
    return inicio;
}

int analisar_linha(Montagem *m, Linha *l) {
    char copia[ASM_TEXTO], *p, *primeiro, *op, *fim;
    int aspas = 0;
    strcpy(copia, l->fonte);
    p = copia;
    while (isspace((unsigned char)*p)) ++p;
    if (!*p || *p == '.' || *p == ';') return 1;
    for (p = copia; *p; ++p) {
        if (*p == '\'') aspas = !aspas;
        if (!aspas && *p == ';') { *p = '\0'; break; }
        if (!aspas) *p = (char)toupper((unsigned char)*p);
    }
    if (aspas) return erro_montagem(m, "aspas nao fechadas");
    p = copia;
    while (isspace((unsigned char)*p)) ++p;
    if (!*p || *p == '.') return 1;
    primeiro = token(&p);
    if (buscar_operacao(primeiro) || buscar_diretiva(primeiro)) op = primeiro;
    else {
        size_t n = strlen(primeiro);
        if (n && primeiro[n-1] == ':') primeiro[--n] = '\0';
        if (n >= sizeof l->rotulo || !identificador(primeiro))
            return erro_montagem(m, "rotulo invalido ou longo demais: %s", primeiro);
        strcpy(l->rotulo, primeiro);
        op = token(&p);
    }
    if (!*op || strlen(op) >= sizeof l->operacao ||
        (!buscar_operacao(op) && !buscar_diretiva(op)))
        return erro_montagem(m, "operacao desconhecida: %s", *op ? op : primeiro);
    strcpy(l->operacao, op);
    while (isspace((unsigned char)*p)) ++p;
    fim = p + strlen(p);
    while (fim > p && isspace((unsigned char)fim[-1])) *--fim = '\0';
    if (strlen(p) >= sizeof l->operando) return erro_montagem(m, "operando longo demais");
    strcpy(l->operando, p);
    return 1;
}

int definir_simbolo(Montagem *m, const char *nome, int32_t valor, int relativo) {
    unsigned i;
    if (!*nome) return 1;
    for (i = 0; i < m->nsimbolos; ++i)
        if (!strcmp(m->simbolos[i].nome, nome)) return erro_montagem(m, "simbolo duplicado: %s", nome);
    if (m->nsimbolos == ASM_SIMBOLOS) return erro_montagem(m, "limite de simbolos excedido");
    strcpy(m->simbolos[m->nsimbolos].nome, nome);
    m->simbolos[m->nsimbolos].valor = valor;
    m->simbolos[m->nsimbolos++].relativo = relativo;
    return 1;
}

/* Expressoes de soma/subtracao, com '*' representando o LC atual.
 * EQU/ORG/reservas so podem usar simbolos ja definidos no passo 1. */
int avaliar(Montagem *m, const char *texto, unsigned lc, int base_numerica,
            int32_t *valor, int *relativo) {
    const char *p = texto;
    int sinal = 1, rel = 0, termos = 0;
    int64_t total = 0;
    while (isspace((unsigned char)*p)) ++p;
    if (*p == '+' || *p == '-') sinal = *p++ == '-' ? -1 : 1;
    for (;;) {
        char termo[ASM_OPERANDO], *fim;
        size_t n = 0;
        int tr = 0, base = base_numerica;
        int64_t v;
        unsigned i;
        while (isspace((unsigned char)*p)) ++p;
        while (*p && *p != '+' && *p != '-' && !isspace((unsigned char)*p)) {
            if (n + 1 == sizeof termo) return erro_montagem(m, "expressao longa demais");
            termo[n++] = *p++;
        }
        termo[n] = '\0';
        if (!n) return erro_montagem(m, "expressao invalida: %s", texto);
        if (!strcmp(termo, "*")) { v = lc; tr = 1; }
        else if (isdigit((unsigned char)termo[0]) || termo[0] == '$' ||
                 (base_numerica == 16 && strspn(termo, "0123456789ABCDEF") == n)) {
            char *inicio = termo;
            if (*inicio == '$') { base = 16; ++inicio; }
            else if (inicio[0] == '0' && inicio[1] == 'X') { base = 16; inicio += 2; }
            errno = 0;
            v = strtoll(inicio, &fim, base);
            if (errno || !*inicio || *fim || v > INT32_MAX)
                return erro_montagem(m, "numero invalido ou fora do limite: %s", termo);
        } else {
            for (i = 0; i < m->nsimbolos; ++i)
                if (!strcmp(m->simbolos[i].nome, termo)) break;
            if (i == m->nsimbolos) return erro_montagem(m, "simbolo indefinido: %s", termo);
            v = m->simbolos[i].valor; tr = m->simbolos[i].relativo;
        }
        total += sinal * v; rel += sinal * tr; ++termos;
        if (total < INT32_MIN || total > INT32_MAX) return erro_montagem(m, "overflow na expressao");
        while (isspace((unsigned char)*p)) ++p;
        if (!*p) break;
        if (*p != '+' && *p != '-') return erro_montagem(m, "expressao invalida: %s", texto);
        sinal = *p++ == '-' ? -1 : 1;
    }
    if (!termos || (rel != 0 && rel != 1)) return erro_montagem(m, "expressao relativa invalida");
    *valor = (int32_t)total; *relativo = rel;
    return 1;
}

static int hex_valor(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)toupper((unsigned char)c);
    return c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

int bytes_constante(Montagem *m, const char *s, uint8_t *bytes, unsigned *tamanho) {
    size_t n = strlen(s), i;
    if (n < 4 || s[1] != '\'' || s[n-1] != '\'' || (s[0] != 'C' && s[0] != 'X'))
        return erro_montagem(m, "BYTE espera C'texto' ou X'hexadecimal' nao vazio");
    n -= 3;
    if (s[0] == 'C') {
        *tamanho = (unsigned)n;
        for (i = 0; i < n; ++i) {
            if (s[i+2] == '\'') return erro_montagem(m, "aspas internas em BYTE nao suportadas");
            if (bytes) bytes[i] = (uint8_t)s[i+2];
        }
    } else {
        if (n % 2) return erro_montagem(m, "BYTE hexadecimal precisa de pares de digitos");
        *tamanho = (unsigned)(n / 2);
        for (i = 0; i < n; i += 2) {
            int a = hex_valor(s[i+2]), b = hex_valor(s[i+3]);
            if (a < 0 || b < 0) return erro_montagem(m, "digito hexadecimal invalido em BYTE");
            if (bytes) bytes[i/2] = (uint8_t)(a * 16 + b);
        }
    }
    return 1;
}
