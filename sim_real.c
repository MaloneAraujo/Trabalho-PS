/*
 * sim_real.c - liga as partes 1 a 4 ao contrato de sicxe.h (parte 5: integracao).
 *
 * A interface (main.c) chama sim_criar/sim_carregar/sim_passo/sim_reset. Este arquivo
 * implementa essas funcoes usando o que os colegas escreveram:
 *   parte 1 (memoria.c)       : memoria[] e bancoRegistradores[] sao o ESTADO REAL da maquina
 *   parte 2 (decodificador.c) : decodificarInstrucao() le a instrucao e calcula o endereco efetivo
 *   parte 3 (Aritimetica.c)   : fazer_add, fazer_comp, fazer_addf, ...
 *   parte 4 (instrucoes_movimentacao_desvios.c): LDA, STA, J, JSUB, CLEAR, RMO, ...
 *
 * A struct Maquina de sicxe.h e so uma "vista" para a interface: depois de cada operacao
 * os registradores de bancoRegistradores[] sao copiados para ela, e m->mem aponta direto
 * para memoria[].
 *
 * Decisoes tomadas aqui (confirmar com o grupo):
 *   - Formato do arquivo de programa: objeto absoluto (registros H, T, E).
 *     Se o arquivo nao comecar com 'H', e copiado como bytes crus a partir do endereco 0.
 *   - Codigo condicional no SW: bits baixos 1 = menor, 2 = igual, 3 = maior, 0 = nenhum COMP ainda.
 *   - Registrador F (48 bits): os 48 bits de cima de um double IEEE (sinal, 11 de expoente,
 *     36 de fracao). Aproximacao do formato do livro; Aritimetica.c trabalha com double.
 *   - SHIFTL/SHIFTR r1,n: o campo r2 guarda n-1 (como no livro), logo desloca r2+1 bits.
 *   - Fim do programa: instrucao que salta para ela mesma (ex.: HALT J HALT) vira SIM_FIM.
 *   - Nao suportadas (erro): SVC, LPS, SSK, STI, STSW, RD, WD, TD, SIO, HIO, TIO.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Aritimetica.h"
#include "decodificador.h"
#include "instrucoes_movimentacao_desvios.h"
#include "memoria.h"
#include "sicxe.h"

#define TAM_MEM       ((uint32_t)sizeof memoria)
#define MAX_ARQUIVO   65536
#define SW_CC_MASCARA 0x03u

static uint32_t entrada = 0; /* endereco da primeira instrucao do programa carregado */

/* ---------- registradores (banco da parte 1) ---------- */

static uint32_t reg_ler(int idx) {
    return (uint32_t)(bancoRegistradores[idx].valor & MASK24);
}

static void reg_gravar(int idx, uint32_t v) {
    bancoRegistradores[idx].valor = v & MASK24;
}

/* Valor do registrador como inteiro de 24 bits com sinal (o que Aritimetica.c espera). */
static int32_t reg_com_sinal(int idx) {
    return limitar_24_bits((int32_t)reg_ler(idx));
}

/* Numeros de registrador do formato 2 aceitos: A, X, L, B, S, T, PC, SW. */
static int reg_valido(int idx) {
    return (idx >= 0 && idx <= 5) || idx == REG_PC || idx == REG_SW;
}

static void sincronizar(Maquina *m) {
    m->A = reg_ler(REG_A);
    m->X = reg_ler(REG_X);
    m->L = reg_ler(REG_L);
    m->B = reg_ler(REG_B);
    m->S = reg_ler(REG_S);
    m->T = reg_ler(REG_T);
    m->F = bancoRegistradores[REG_F].valor & MASK48;
    m->PC = reg_ler(REG_PC);
    m->SW = reg_ler(REG_SW);
}

/* ---------- codigo condicional (guardado no SW) ---------- */

static void cc_gravar(int32_t cc) { /* cc: -1 menor, 0 igual, 1 maior (valores de Aritimetica.c) */
    uint64_t sw = bancoRegistradores[REG_SW].valor & ~(uint64_t)SW_CC_MASCARA;
    bancoRegistradores[REG_SW].valor = (sw | (uint64_t)(cc + 2)) & MASK24;
}

static SicxeCodigoCondicional cb_cc(void *ctx) {
    (void)ctx;
    switch (bancoRegistradores[REG_SW].valor & SW_CC_MASCARA) {
        case 1: return SICXE_CC_MENOR;
        case 2: return SICXE_CC_IGUAL;
        case 3: return SICXE_CC_MAIOR;
        default: return (SicxeCodigoCondicional)99; /* nenhum COMP ainda: nao salta */
    }
}

/* ---------- acumuladores de Aritimetica.c <-> banco de registradores ---------- */

static void carregar_a(void) { acumulador_a = reg_com_sinal(REG_A); }
static void guardar_a(void) { reg_gravar(REG_A, (uint32_t)acumulador_a); }

static double f48_para_double(uint64_t f48) {
    uint64_t bits = (f48 & MASK48) << 16;
    double d;
    memcpy(&d, &bits, sizeof d);
    return d;
}

static uint64_t double_para_f48(double d) {
    uint64_t bits;
    memcpy(&bits, &d, sizeof bits);
    return (bits >> 16) & MASK48;
}

static void carregar_f(void) { acumulador_f = f48_para_double(bancoRegistradores[REG_F].valor); }
static void guardar_f(void) { bancoRegistradores[REG_F].valor = double_para_f48(acumulador_f); }

/* ---------- memoria ---------- */

static int operando_valido(uint32_t endereco, uint32_t bytes) {
    return endereco <= TAM_MEM && TAM_MEM - endereco >= bytes;
}

static uint64_t ler_f48_bruto(uint32_t e) {
    uint64_t v = 0;
    for (int i = 0; i < 6; i++) v = (v << 8) | memoria[e + (uint32_t)i];
    return v;
}

static void gravar_f48_bruto(uint32_t e, uint64_t v) {
    for (int i = 5; i >= 0; i--) {
        memoria[e + (uint32_t)i] = (uint8_t)(v & 0xFF);
        v >>= 8;
    }
}

/* Callbacks que a parte 4 exige (SicxeCpu e SicxeMemoria). */
static int indice_banco(SicxeRegistrador r) {
    switch (r) {
        case SICXE_REG_A: return REG_A;
        case SICXE_REG_X: return REG_X;
        case SICXE_REG_L: return REG_L;
        case SICXE_REG_B: return REG_B;
        case SICXE_REG_S: return REG_S;
        case SICXE_REG_T: return REG_T;
        case SICXE_REG_PC: return REG_PC;
        default: return REG_SW;
    }
}

static uint32_t cb_ler_reg(void *ctx, SicxeRegistrador r) {
    (void)ctx;
    return reg_ler(indice_banco(r));
}

static void cb_gravar_reg(void *ctx, SicxeRegistrador r, uint32_t v) {
    (void)ctx;
    reg_gravar(indice_banco(r), v);
}

static uint8_t cb_ler_byte(void *ctx, uint32_t e) {
    (void)ctx;
    return e < TAM_MEM ? memoria[e] : 0;
}

static void cb_gravar_byte(void *ctx, uint32_t e, uint8_t v) {
    (void)ctx;
    if (e < TAM_MEM) memoria[e] = v;
}

static uint32_t cb_ler_palavra(void *ctx, uint32_t e) {
    (void)ctx;
    return ler_word_24bits((int)e);
}

static void cb_gravar_palavra(void *ctx, uint32_t e, uint32_t v) {
    (void)ctx;
    escrever_word_24bits((int)e, v);
}

/* ---------- nomes (para o log da interface) ---------- */

typedef struct {
    uint8_t opcode;
    const char *nome;
} EntradaOp;

static const EntradaOp TABELA[] = {
    {0x18, "ADD"},   {0x58, "ADDF"},  {0x90, "ADDR"},  {0x40, "AND"},   {0xB4, "CLEAR"},
    {0x28, "COMP"},  {0x88, "COMPF"}, {0xA0, "COMPR"}, {0x24, "DIV"},   {0x64, "DIVF"},
    {0x9C, "DIVR"},  {0xC4, "FIX"},   {0xC0, "FLOAT"}, {0xF4, "HIO"},   {0x3C, "J"},
    {0x30, "JEQ"},   {0x34, "JGT"},   {0x38, "JLT"},   {0x48, "JSUB"},  {0x00, "LDA"},
    {0x68, "LDB"},   {0x50, "LDCH"},  {0x70, "LDF"},   {0x08, "LDL"},   {0x6C, "LDS"},
    {0x74, "LDT"},   {0x04, "LDX"},   {0xD0, "LPS"},   {0x20, "MUL"},   {0x60, "MULF"},
    {0x98, "MULR"},  {0xC8, "NORM"},  {0x44, "OR"},    {0xD8, "RD"},    {0xAC, "RMO"},
    {0x4C, "RSUB"},  {0xA4, "SHIFTL"},{0xA8, "SHIFTR"},{0xF0, "SIO"},   {0xEC, "SSK"},
    {0x0C, "STA"},   {0x78, "STB"},   {0x54, "STCH"},  {0x80, "STF"},   {0xD4, "STI"},
    {0x14, "STL"},   {0x7C, "STS"},   {0xE8, "STSW"},  {0x84, "STT"},   {0x10, "STX"},
    {0x1C, "SUB"},   {0x5C, "SUBF"},  {0x94, "SUBR"},  {0xB0, "SVC"},   {0xE0, "TD"},
    {0xF8, "TIO"},   {0x2C, "TIX"},   {0xB8, "TIXR"},  {0xDC, "WD"},
};

static const char *nome_opcode(uint8_t op) {
    for (size_t i = 0; i < sizeof TABELA / sizeof TABELA[0]; i++)
        if (TABELA[i].opcode == op) return TABELA[i].nome;
    return "???";
}

static const char *nome_reg(int idx) {
    static const char *nomes[10] = {"A", "X", "L", "B", "S", "T", "F", "?", "PC", "SW"};
    return (idx >= 0 && idx < 10) ? nomes[idx] : "?";
}

static void descrever(const Instrucao *in, uint32_t pc, char *out, size_t n) {
    const char *nome = nome_opcode(in->opcode);

    if (in->formato == 1) {
        snprintf(out, n, "%06X: %s", (unsigned)pc, nome);
    } else if (in->formato == 2) {
        if (in->opcode == 0xB4 || in->opcode == 0xB8)
            snprintf(out, n, "%06X: %s %s", (unsigned)pc, nome, nome_reg(in->r1));
        else if (in->opcode == 0xA4 || in->opcode == 0xA8)
            snprintf(out, n, "%06X: %s %s,%d", (unsigned)pc, nome, nome_reg(in->r1), in->r2 + 1);
        else
            snprintf(out, n, "%06X: %s %s,%s", (unsigned)pc, nome, nome_reg(in->r1), nome_reg(in->r2));
    } else {
        const char *pre = in->imediato ? "#" : (in->indireto ? "@" : "");
        unsigned v = in->imediato ? in->operando : in->enderecoEfetivo;
        snprintf(out, n, "%06X: %s%s %s%X%s", (unsigned)pc, in->e ? "+" : "", nome, pre, v,
                 in->x ? ",X" : "");
    }
}

/* ---------- execucao de uma instrucao ja decodificada ---------- */

/* Le o operando de 24 bits (imediato ou da memoria), com sinal. Retorna 0 se o endereco for invalido. */
static int ler_operando_word(const Instrucao *in, int32_t *v, char *msg, size_t n) {
    if (in->imediato) {
        *v = limitar_24_bits((int32_t)(in->operando & MASK24));
        return 1;
    }
    if (!operando_valido(in->enderecoEfetivo, 3)) {
        snprintf(msg, n, "endereco %X fora da memoria", (unsigned)in->enderecoEfetivo);
        return 0;
    }
    *v = limitar_24_bits((int32_t)ler_word_24bits((int)in->enderecoEfetivo));
    return 1;
}

/* Le o operando de 48 bits (so da memoria). */
static int ler_operando_float(const Instrucao *in, double *d, char *msg, size_t n) {
    if (in->imediato || !operando_valido(in->enderecoEfetivo, 6)) {
        snprintf(msg, n, "operando de ponto flutuante invalido");
        return 0;
    }
    *d = f48_para_double(ler_f48_bruto(in->enderecoEfetivo));
    return 1;
}

static int mapear_reg(int idx, SicxeRegistrador *saida) {
    switch (idx) {
        case 0: *saida = SICXE_REG_A; return 1;
        case 1: *saida = SICXE_REG_X; return 1;
        case 2: *saida = SICXE_REG_L; return 1;
        case 3: *saida = SICXE_REG_B; return 1;
        case 4: *saida = SICXE_REG_S; return 1;
        case 5: *saida = SICXE_REG_T; return 1;
        case 8: *saida = SICXE_REG_PC; return 1;
        case 9: *saida = SICXE_REG_SW; return 1;
        default: return 0;
    }
}

static int mnemonico_movimentacao(uint8_t op, SicxeMnemonico *mn) {
    switch (op) {
        case 0x00: *mn = SICXE_LDA; return 1;
        case 0x68: *mn = SICXE_LDB; return 1;
        case 0x50: *mn = SICXE_LDCH; return 1;
        case 0x08: *mn = SICXE_LDL; return 1;
        case 0x6C: *mn = SICXE_LDS; return 1;
        case 0x74: *mn = SICXE_LDT; return 1;
        case 0x04: *mn = SICXE_LDX; return 1;
        case 0x0C: *mn = SICXE_STA; return 1;
        case 0x78: *mn = SICXE_STB; return 1;
        case 0x54: *mn = SICXE_STCH; return 1;
        case 0x14: *mn = SICXE_STL; return 1;
        case 0x7C: *mn = SICXE_STS; return 1;
        case 0x84: *mn = SICXE_STT; return 1;
        case 0x10: *mn = SICXE_STX; return 1;
        case 0xB4: *mn = SICXE_CLEAR; return 1;
        case 0xAC: *mn = SICXE_RMO; return 1;
        case 0x3C: *mn = SICXE_J; return 1;
        case 0x30: *mn = SICXE_JEQ; return 1;
        case 0x34: *mn = SICXE_JGT; return 1;
        case 0x38: *mn = SICXE_JLT; return 1;
        case 0x48: *mn = SICXE_JSUB; return 1;
        case 0x4C: *mn = SICXE_RSUB; return 1;
        default: return 0;
    }
}

/* Adaptador: converte a Instrucao do decodificador (parte 2) na SicxeInstrucao da parte 4. */
static int executar_movimentacao(const Instrucao *in, SicxeMnemonico mn, char *msg, size_t n) {
    SicxeCpu cpu = {NULL, cb_ler_reg, cb_gravar_reg, cb_cc};
    SicxeMemoria mem = {NULL, cb_ler_byte, cb_gravar_byte, cb_ler_palavra, cb_gravar_palavra};
    SicxeExecutorMovimentacaoDesvios ex;
    SicxeInstrucao si;

    memset(&si, 0, sizeof si);
    si.mnemonico = mn;

    if (mn == SICXE_CLEAR && in->r1 == REG_F) { /* a parte 4 nao conhece o F */
        bancoRegistradores[REG_F].valor = 0;
        return SIM_OK;
    }

    if (mn == SICXE_CLEAR || mn == SICXE_RMO) {
        si.possui_r1 = true;
        if (!mapear_reg(in->r1, &si.r1)) {
            snprintf(msg, n, "registrador %d nao suportado", in->r1);
            return SIM_ERRO;
        }
        if (mn == SICXE_RMO) {
            si.possui_r2 = true;
            if (!mapear_reg(in->r2, &si.r2)) {
                snprintf(msg, n, "registrador %d nao suportado", in->r2);
                return SIM_ERRO;
            }
        }
    } else if (mn != SICXE_RSUB) {
        si.imediata = in->imediato ? true : false;
        si.valor_ou_endereco = in->imediato ? in->operando : in->enderecoEfetivo;
        if (!si.imediata) {
            uint32_t bytes = (mn == SICXE_LDCH || mn == SICXE_STCH) ? 1 : 3;
            int salto = (mn >= SICXE_J && mn <= SICXE_JSUB);
            if (!salto && !operando_valido(in->enderecoEfetivo, bytes)) {
                snprintf(msg, n, "endereco %X fora da memoria", (unsigned)in->enderecoEfetivo);
                return SIM_ERRO;
            }
        }
    }

    if (sicxe_inicializar_executor_movimentacao_desvios(&ex, &cpu, &mem) != SICXE_EXECUCAO_OK ||
        sicxe_executar_movimentacao_desvio(&ex, &si) != SICXE_EXECUCAO_OK) {
        snprintf(msg, n, "falha na execucao (operando invalido)");
        return SIM_ERRO;
    }
    return SIM_OK;
}

/* Operacao de dois registradores (formato 2). */
static int executar_formato2_aritmetico(const Instrucao *in, char *msg, size_t n) {
    int deslocamento = in->opcode == 0xA4 || in->opcode == 0xA8;
    if (!reg_valido(in->r1) || (!deslocamento && !reg_valido(in->r2))) {
        snprintf(msg, n, "registrador invalido (%d,%d)", in->r1, in->r2);
        return SIM_ERRO;
    }
    int32_t r1 = reg_com_sinal(in->r1);
    int32_t r2 = deslocamento ? 0 : reg_com_sinal(in->r2);

    switch (in->opcode) {
        case 0x90: reg_gravar(in->r2, (uint32_t)fazer_addr(r1, r2)); break;
        case 0x94: reg_gravar(in->r2, (uint32_t)fazer_subr(r1, r2)); break;
        case 0x98: reg_gravar(in->r2, (uint32_t)fazer_mulr(r1, r2)); break;
        case 0x9C:
            if (r1 == 0) {
                snprintf(msg, n, "divisao por zero");
                return SIM_ERRO;
            }
            reg_gravar(in->r2, (uint32_t)fazer_divr(r1, r2));
            break;
        case 0xA0:
            fazer_compr(r1, r2);
            cc_gravar(palavra_status_sw);
            break;
        case 0xA4: reg_gravar(in->r1, (uint32_t)fazer_shiftl(r1, in->r2 + 1)); break;
        case 0xA8: reg_gravar(in->r1, (uint32_t)fazer_shiftr(r1, in->r2 + 1)); break;
        default: break;
    }
    return SIM_OK;
}

static int executar(const Instrucao *in, char *msg, size_t n) {
    SicxeMnemonico mn;
    int32_t v;
    double d;

    switch (in->opcode) {
        /* ----- parte 3: aritmeticas com memoria ----- */
        case 0x18: case 0x1C: case 0x20: case 0x24: case 0x40: case 0x44: case 0x28:
            if (!ler_operando_word(in, &v, msg, n)) return SIM_ERRO;
            if (in->opcode == 0x24 && v == 0) {
                snprintf(msg, n, "divisao por zero");
                return SIM_ERRO;
            }
            carregar_a();
            switch (in->opcode) {
                case 0x18: fazer_add(v); break;
                case 0x1C: fazer_sub(v); break;
                case 0x20: fazer_mul(v); break;
                case 0x24: fazer_div(v); break;
                case 0x40: fazer_and(v); break;
                case 0x44: fazer_or(v); break;
                default: fazer_comp(v); cc_gravar(palavra_status_sw); break;
            }
            guardar_a();
            return SIM_OK;

        /* ----- parte 3: ponto flutuante com memoria ----- */
        case 0x58: case 0x5C: case 0x60: case 0x64: case 0x88:
            if (!ler_operando_float(in, &d, msg, n)) return SIM_ERRO;
            if (in->opcode == 0x64 && d == 0.0) {
                snprintf(msg, n, "divisao por zero (ponto flutuante)");
                return SIM_ERRO;
            }
            carregar_f();
            switch (in->opcode) {
                case 0x58: fazer_addf(d); break;
                case 0x5C: fazer_subf(d); break;
                case 0x60: fazer_mulf(d); break;
                case 0x64: fazer_divf(d); break;
                default: fazer_compf(d); cc_gravar(palavra_status_sw); break;
            }
            guardar_f();
            return SIM_OK;

        case 0x70: /* LDF */
            if (in->imediato || !operando_valido(in->enderecoEfetivo, 6)) break;
            bancoRegistradores[REG_F].valor = ler_f48_bruto(in->enderecoEfetivo);
            return SIM_OK;
        case 0x80: /* STF */
            if (in->imediato || !operando_valido(in->enderecoEfetivo, 6)) break;
            gravar_f48_bruto(in->enderecoEfetivo, bancoRegistradores[REG_F].valor);
            return SIM_OK;

        /* ----- parte 3: formato 1 e formato 2 ----- */
        case 0xC4: carregar_f(); fazer_fix(); guardar_a(); return SIM_OK;
        case 0xC0: carregar_a(); fazer_float(); guardar_f(); return SIM_OK;
        case 0xC8: carregar_f(); fazer_norm(); guardar_f(); return SIM_OK;

        case 0x90: case 0x94: case 0x98: case 0x9C: case 0xA0: case 0xA4: case 0xA8:
            return executar_formato2_aritmetico(in, msg, n);

        /* ----- TIX e TIXR (nenhuma parte cobria) ----- */
        case 0x2C:
            if (!ler_operando_word(in, &v, msg, n)) return SIM_ERRO;
            reg_gravar(REG_X, reg_ler(REG_X) + 1);
            atualizar_codigo_condicional(reg_com_sinal(REG_X), v);
            cc_gravar(palavra_status_sw);
            return SIM_OK;
        case 0xB8:
            if (!reg_valido(in->r1)) break;
            reg_gravar(REG_X, reg_ler(REG_X) + 1);
            atualizar_codigo_condicional(reg_com_sinal(REG_X), reg_com_sinal(in->r1));
            cc_gravar(palavra_status_sw);
            return SIM_OK;

        default:
            /* ----- parte 4: movimentacao e desvios ----- */
            if (mnemonico_movimentacao(in->opcode, &mn))
                return executar_movimentacao(in, mn, msg, n);
            snprintf(msg, n, "instrucao %s nao suportada", nome_opcode(in->opcode));
            return SIM_ERRO;
    }

    snprintf(msg, n, "operando invalido");
    return SIM_ERRO;
}

/* ---------- carregador de programa ---------- */

static int hex_n(const char *s, int n, uint32_t *saida) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        char c = s[i];
        int dig;
        if (c >= '0' && c <= '9') dig = c - '0';
        else if (c >= 'A' && c <= 'F') dig = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') dig = c - 'a' + 10;
        else return 0; /* inclui o fim da string, entao nunca le alem dela */
        v = (v << 4) | (uint32_t)dig;
    }
    *saida = v;
    return 1;
}

/* Programa objeto do livro: H(nome,inicio,tamanho), T(inicio,tamanho em bytes,dados), E(entrada). */
static int carregar_objeto(char *buf, uint8_t *imagem, uint32_t *nova_entrada,
                           char *erro, size_t en) {
    int numero = 0, tem_h = 0, tem_t = 0, tem_e = 0;
    uint32_t inicio = 0, tamanho = 0;

    for (char *linha = strtok(buf, "\r\n"); linha != NULL; linha = strtok(NULL, "\r\n")) {
        numero++;
        while (*linha == ' ' || *linha == '\t') linha++;

        char *w = linha; /* aceita separadores '^' */
        for (char *r = linha; *r; r++)
            if (*r != '^') *w++ = *r;
        *w = '\0';

        size_t tam = strlen(linha);
        while (tam && (linha[tam - 1] == ' ' || linha[tam - 1] == '\t')) linha[--tam] = '\0';
        if (tam == 0) continue;
        if (tem_e || (!tem_h && linha[0] != 'H')) {
            snprintf(erro, en, "ordem de registros invalida (linha %d)", numero);
            return -1;
        }
        if (linha[0] == 'H') {
            if (tem_h || tam != 19 || !hex_n(linha + 7, 6, &inicio) ||
                !hex_n(linha + 13, 6, &tamanho) || tamanho == 0 ||
                !operando_valido(inicio, tamanho)) {
                snprintf(erro, en, "registro H invalido (linha %d)", numero);
                return -1;
            }
            *nova_entrada = inicio;
            tem_h = 1;
        } else if (linha[0] == 'T') {
            uint32_t ini, len;
            if (tam < 9 || !hex_n(linha + 1, 6, &ini) || !hex_n(linha + 7, 2, &len) ||
                tam != 9 + 2 * (size_t)len) {
                snprintf(erro, en, "registro T invalido (linha %d)", numero);
                return -1;
            }
            if (!operando_valido(ini, len) || ini < inicio ||
                ini > inicio + tamanho || len > inicio + tamanho - ini) {
                snprintf(erro, en, "programa nao cabe na memoria de %u bytes (linha %d)",
                         (unsigned)TAM_MEM, numero);
                return -1;
            }
            for (uint32_t i = 0; i < len; i++) {
                uint32_t b;
                if (!hex_n(linha + 9 + 2 * i, 2, &b)) {
                    snprintf(erro, en, "byte invalido no registro T (linha %d)", numero);
                    return -1;
                }
                imagem[ini + i] = (uint8_t)b;
            }
            if (len) tem_t = 1;
        } else if (linha[0] == 'E') {
            uint32_t e = inicio;
            if ((tam != 1 && (tam != 7 || !hex_n(linha + 1, 6, &e))) ||
                e < inicio || e >= inicio + tamanho) {
                snprintf(erro, en, "registro E invalido (linha %d)", numero);
                return -1;
            }
            *nova_entrada = e;
            tem_e = 1;
        } else {
            snprintf(erro, en, "registro %c nao suportado (linha %d)", linha[0], numero);
            return -1;
        }
    }
    if (!tem_h || !tem_t || !tem_e) {
        snprintf(erro, en, "programa objeto incompleto: requer H, T e E");
        return -1;
    }
    return 0;
}

/* ---------- contrato de sicxe.h ---------- */

Maquina *sim_criar(void) {
    Maquina *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    inicializar_memoria();
    entrada = 0;
    m->mem = memoria;
    m->mem_tam = TAM_MEM;
    sincronizar(m);
    return m;
}

void sim_destruir(Maquina *m) {
    free(m); /* m->mem aponta para o vetor global memoria[]: nao se libera */
}

void sim_reset(Maquina *m) {
    for (int i = 0; i < 10; i++) bancoRegistradores[i].valor = 0;
    reg_gravar(REG_PC, entrada); /* volta ao inicio do programa carregado */
    sincronizar(m);
}

int sim_carregar_imagem(Maquina *m, const uint8_t *imagem, size_t tamanho,
                       uint32_t inicio, char *erro, size_t erro_tam) {
    uint8_t copia[TAM_MEM] = {0};
    if (!m || !imagem || !tamanho || tamanho > TAM_MEM || inicio >= tamanho) {
        snprintf(erro, erro_tam, "imagem ou endereco de entrada invalido");
        return -1;
    }
    memcpy(copia, imagem, tamanho); /* origem pode ser a propria memoria */
    inicializar_memoria();
    memcpy(memoria, copia, TAM_MEM);
    entrada = inicio;
    reg_gravar(REG_PC, entrada);
    sincronizar(m);
    if (erro_tam) erro[0] = '\0';
    return 0;
}

int sim_carregar(Maquina *m, const char *caminho, char *erro, size_t erro_tam) {
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        snprintf(erro, erro_tam, "nao foi possivel abrir %s", caminho);
        return -1;
    }
    char *buf = malloc(MAX_ARQUIVO + 1);
    uint8_t *imagem = calloc(TAM_MEM, 1);
    if (!buf || !imagem) {
        free(buf);
        free(imagem);
        fclose(f);
        snprintf(erro, erro_tam, "memoria insuficiente");
        return -1;
    }
    size_t lido = fread(buf, 1, MAX_ARQUIVO, f);
    int falha_leitura = ferror(f);
    int excedeu_limite = fgetc(f) != EOF;
    falha_leitura = falha_leitura || ferror(f);
    fclose(f);
    buf[lido] = '\0';

    uint32_t nova_entrada = 0;
    int r = -1;
    if (falha_leitura || excedeu_limite || lido == 0) {
        snprintf(erro, erro_tam, "arquivo vazio, muito grande ou com erro de leitura");
    } else if (buf[0] == 'H') {
        if (memchr(buf, '\0', lido)) {
            snprintf(erro, erro_tam, "byte nulo em programa objeto textual");
        } else {
            r = carregar_objeto(buf, imagem, &nova_entrada, erro, erro_tam);
        }
    } else if (lido > TAM_MEM) {
        snprintf(erro, erro_tam, "binario excede a memoria de %u bytes", (unsigned)TAM_MEM);
    } else {
        memcpy(imagem, buf, lido);
        r = 0;
    }
    free(buf);

    /* So troca o estado quando todo o arquivo foi validado. */
    if (r == 0) {
        r = sim_carregar_imagem(m, imagem, TAM_MEM, nova_entrada, erro, erro_tam);
    }
    free(imagem);
    return r == 0 ? 0 : -1;
}

int sim_passo(Maquina *m, char *desc, size_t desc_tam) {
    uint32_t pc = reg_ler(REG_PC);
    Instrucao in;
    char msg[120] = "";

    if (!decodificarInstrucao(memoria, sizeof memoria, pc, reg_ler(REG_X), reg_ler(REG_B), &in)) {
        snprintf(desc, desc_tam, "%06X: instrucao invalida ou fora da memoria", (unsigned)pc);
        return SIM_ERRO;
    }

    descrever(&in, pc, desc, desc_tam);
    reg_gravar(REG_PC, in.proximoPC); /* a parte 4 espera o PC ja avancado (JSUB guarda em L) */

    if (executar(&in, msg, sizeof msg) != SIM_OK) {
        reg_gravar(REG_PC, pc); /* O erro continua apontando para a instrucao. */
        snprintf(desc, desc_tam, "%06X: ERRO: %s", (unsigned)pc, msg);
        sincronizar(m);
        return SIM_ERRO;
    }
    sincronizar(m);

    if (reg_ler(REG_PC) == pc) { /* salto para si mesmo = fim do programa */
        size_t len = strlen(desc);
        if (len < desc_tam) snprintf(desc + len, desc_tam - len, "  (fim: salta para si mesmo)");
        return SIM_FIM;
    }
    return SIM_OK;
}
