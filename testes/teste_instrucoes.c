/* Testes de aritmetica, modos de enderecamento e contagens de deslocamento.
 * Complementa os testes de movimentacao e integracao.
 */
#include <stdio.h>
#include <string.h>
#include "sicxe.h"
#include "memoria.h"
#include "decodificador.h"

static int acertos, falhas;
static char descricao[160];

static void conferir(const char *nome, int condicao) {
    printf("%s | %s\n", condicao ? "OK" : "FALHA", nome);
    if (condicao) acertos++; else falhas++;
}

static void preparar(const unsigned char *codigo, size_t n) {
    inicializar_memoria();
    memcpy(memoria, codigo, n);
}

typedef struct {
    const char *nome;
    unsigned char codigo[3];
    unsigned a, x, resultado, cc;
    int reg_resultado, retorno;
} CasoAritmetico;

static void aritmetica(Maquina *m) {
    const CasoAritmetico casos[] = {
        {"ADD",   {0x19,0,3}, 5,0,8,0,REG_A,SIM_OK},
        {"SUB",   {0x1D,0,3}, 5,0,2,0,REG_A,SIM_OK},
        {"MUL",   {0x21,0,3}, 5,0,15,0,REG_A,SIM_OK},
        {"DIV",   {0x25,0,3}, 15,0,5,0,REG_A,SIM_OK},
        {"AND",   {0x41,0,3}, 5,0,1,0,REG_A,SIM_OK},
        {"OR",    {0x45,0,3}, 5,0,7,0,REG_A,SIM_OK},
        {"COMP",  {0x29,0,5}, 5,0,5,2,REG_A,SIM_OK},
        {"ADDR",  {0x90,0x01,0}, 3,5,8,0,REG_X,SIM_OK},
        {"SUBR",  {0x94,0x01,0}, 3,5,2,0,REG_X,SIM_OK},
        {"MULR",  {0x98,0x01,0}, 3,5,15,0,REG_X,SIM_OK},
        {"DIVR",  {0x9C,0x01,0}, 3,15,5,0,REG_X,SIM_OK},
        {"COMPR", {0xA0,0x01,0}, 3,5,3,1,REG_A,SIM_OK},
        {"SHIFTL contagem 1", {0xA4,0x00,0}, 3,0,6,0,REG_A,SIM_OK},
        {"SHIFTR contagem 1", {0xA8,0x00,0}, 6,0,3,0,REG_A,SIM_OK},
        {"TIX",   {0x2D,0,5}, 0,4,5,2,REG_X,SIM_OK},
        {"TIXR",  {0xB8,0x00,0}, 5,4,5,2,REG_X,SIM_OK},
        {"ADD com limite de 24 bits", {0x19,0,1}, 0x7FFFFF,0,0x800000,0,REG_A,SIM_OK},
        {"COMP com A negativo", {0x29,0,0}, 0xFFFFFF,0,0xFFFFFF,1,REG_A,SIM_OK},
        {"SHIFTR preserva sinal", {0xA8,0,0}, 0x800000,0,0xC00000,0,REG_A,SIM_OK},
        {"DIV por zero rejeitado", {0x25,0,0}, 5,0,5,0,REG_A,SIM_ERRO},
        {"DIVR por zero rejeitado", {0x9C,0x01,0}, 0,5,5,0,REG_X,SIM_ERRO}
    };
    for (size_t i = 0; i < sizeof casos / sizeof casos[0]; i++) {
        const CasoAritmetico *c = &casos[i];
        preparar(c->codigo, sizeof c->codigo);
        bancoRegistradores[REG_A].valor = c->a;
        bancoRegistradores[REG_X].valor = c->x;
        int r = sim_passo(m, descricao, sizeof descricao);
        conferir(c->nome, r == c->retorno && bancoRegistradores[c->reg_resultado].valor == c->resultado && (m->SW & 3) == c->cc);
    }
}

typedef struct {
    const char *nome;
    unsigned char codigo[4];
    unsigned destino;
    int imediato, indireto, tamanho;
} CasoEndereco;

static void enderecamento(void) {
    const CasoEndereco casos[] = {
        {"direto 12 bits", {0x03,0x00,0x80,0},0x80,0,0,3},
        {"direto 20 bits", {0x03,0x10,0x00,0x80},0x80,0,0,4},
        {"direto PC relativo", {0x03,0x20,0x7D,0},0x80,0,0,3},
        {"direto B relativo", {0x03,0x40,0x10,0},0x80,0,0,3},
        {"direto indexado 12 bits", {0x03,0x80,0x70,0},0x80,0,0,3},
        {"direto indexado 20 bits", {0x03,0x90,0x00,0x70},0x80,0,0,4},
        {"direto PC relativo indexado", {0x03,0xA0,0x6D,0},0x80,0,0,3},
        {"direto B relativo indexado", {0x03,0xC0,0x00,0},0x80,0,0,3},
        {"SIC legado endereco baixo", {0x00,0x00,0x80,0},0x80,0,0,3},
        {"SIC legado indexado baixo", {0x00,0x80,0x70,0},0x80,0,0,3},
        {"indireto 12 bits", {0x02,0x00,0x80,0},0x90,0,1,3},
        {"indireto 20 bits", {0x02,0x10,0x00,0x80},0x90,0,1,4},
        {"indireto PC relativo", {0x02,0x20,0x7D,0},0x90,0,1,3},
        {"indireto B relativo", {0x02,0x40,0x10,0},0x90,0,1,3},
        {"imediato 12 bits", {0x01,0x00,0x80,0},0x80,1,0,3},
        {"imediato 20 bits", {0x01,0x10,0x00,0x80},0x80,1,0,4},
        {"imediato PC relativo", {0x01,0x20,0x7D,0},0x80,1,0,3},
        {"imediato B relativo", {0x01,0x40,0x10,0},0x80,1,0,3}
    };
    for (size_t i = 0; i < sizeof casos / sizeof casos[0]; i++) {
        const CasoEndereco *c = &casos[i];
        preparar(c->codigo, sizeof c->codigo);
        escrever_word_24bits(0x80,0x90);
        Instrucao in;
        int r = decodificarInstrucao(memoria, sizeof memoria,0,0x10,0x70,&in);
        conferir(c->nome,r && in.enderecoEfetivo == c->destino && in.tamanho == c->tamanho &&
                 in.imediato == c->imediato && in.indireto == c->indireto &&
                 (!c->imediato || in.operando == c->destino));
    }
}

static void contagens(Maquina *m) {
    for (int direita = 0; direita <= 1; direita++) {
        for (unsigned n = 1; n <= 16; n++) {
            const unsigned char codigo[] = {(unsigned char)(direita ? 0xA8 : 0xA4),(unsigned char)(n-1)};
            preparar(codigo,sizeof codigo);
            bancoRegistradores[REG_A].valor = direita ? 0x10000 : 1;
            int r = sim_passo(m,descricao,sizeof descricao);
            unsigned esperado = direita ? 0x10000u >> n : 1u << n;
            char nome[64];
            snprintf(nome,sizeof nome,"%s A,%u",direita ? "SHIFTR" : "SHIFTL",n);
            conferir(nome,r == SIM_OK && m->A == esperado);
        }
    }
}

int main(int argc, char **argv) {
    Maquina *m = sim_criar();
    if (!m) return 2;
    if (argc > 1) {
        /* Executar com -ftrapv confirma o overflow intermediario sem depender
         * de um resultado aparentemente correto produzido por wraparound. */
        int registradores = strcmp(argv[1],"overflow-mulr") == 0;
        const unsigned char codigo[] = {(unsigned char)(registradores ? 0x98 : 0x23),(unsigned char)(registradores ? 0x01 : 0x00),0x80};
        preparar(codigo,sizeof codigo);
        bancoRegistradores[REG_A].valor = 0x7FFFFF;
        bancoRegistradores[REG_X].valor = 0x7FFFFF;
        escrever_word_24bits(0x80,0x7FFFFF);
        puts(registradores ? "Executando MULR de dois inteiros de 24 bits" : "Executando MUL de dois inteiros de 24 bits");
        fflush(stdout);
        int r = sim_passo(m,descricao,sizeof descricao);
        printf("retorno=%d valor=%llX\n",r,(unsigned long long)bancoRegistradores[registradores ? REG_X : REG_A].valor);
        sim_destruir(m);
        return r == SIM_OK ? 0 : 1;
    }
    conferir("memoria minima de 1 KiB",m->mem_tam >= 1024);
    aritmetica(m);
    enderecamento();
    contagens(m);
    sim_destruir(m);
    printf("RESULTADO INSTRUCOES: %d passaram; %d falharam.\n",acertos,falhas);
    return falhas ? 1 : 0;
}
