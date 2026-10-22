/* Regressao de movimentacao, desvios, decodificacao e carregamento.
 * Executar com a pasta do projeto como diretorio atual.
 */
#include <stdio.h>
#include <string.h>
#include "sicxe.h"
#include "memoria.h"
#include "decodificador.h"

static int passou, falhou;
static char descricao[180];

static void conferir(const char *nome, unsigned long long obtido,
                     unsigned long long esperado) {
    int ok = obtido == esperado;
    printf("%s | %s | obtido=0x%llX esperado=0x%llX\n",
           ok ? "OK" : "FALHA", nome, obtido, esperado);
    if (ok) passou++; else falhou++;
}

static void preparar(const unsigned char *codigo, size_t tamanho) {
    inicializar_memoria();
    memcpy(memoria, codigo, tamanho);
}

static int executar(Maquina *m) {
    return sim_passo(m, descricao, sizeof descricao);
}

static void testar_movimentacao(Maquina *m) {
    const unsigned char loads[] = {0x00, 0x04, 0x08, 0x68, 0x6C, 0x74};
    const unsigned char stores[] = {0x0C, 0x10, 0x14, 0x78, 0x7C, 0x84};
    const char *nomes_l[] = {"LDA", "LDX", "LDL", "LDB", "LDS", "LDT"};
    const char *nomes_s[] = {"STA", "STX", "STL", "STB", "STS", "STT"};
    char nome[80];
    for (int i = 0; i < 6; i++) {
        unsigned char codigo[] = {(unsigned char)(loads[i] | 1), 0x00, 0x55};
        preparar(codigo, sizeof codigo);
        snprintf(nome, sizeof nome, "%s imediato", nomes_l[i]);
        int r = executar(m);
        conferir(nome, r == SIM_OK && bancoRegistradores[i].valor == 0x55, 1);
        codigo[0] = stores[i] | 3; codigo[2] = 0x80;
        preparar(codigo, sizeof codigo);
        bancoRegistradores[i].valor = 0x123456;
        snprintf(nome, sizeof nome, "%s palavra big-endian", nomes_s[i]);
        r = executar(m);
        conferir(nome, r == SIM_OK && memoria[0x80] == 0x12 &&
                 memoria[0x81] == 0x34 && memoria[0x82] == 0x56, 1);
    }
    const unsigned char ldch[] = {0x53, 0x00, 0x80};
    preparar(ldch, sizeof ldch);
    bancoRegistradores[REG_A].valor = 0xAABBCC;
    memoria[0x80] = 0xDD;
    conferir("LDCH executa", executar(m) == SIM_OK, 1);
    conferir("LDCH preserva bytes superiores", m->A, 0xAABBDD);
    const unsigned char stch[] = {0x57, 0x00, 0x80};
    preparar(stch, sizeof stch);
    bancoRegistradores[REG_A].valor = 0xAABBCC;
    memoria[0x81] = 0xEE;
    conferir("STCH executa", executar(m) == SIM_OK, 1);
    conferir("STCH grava somente byte baixo", memoria[0x80], 0xCC);
    conferir("STCH preserva byte vizinho", memoria[0x81], 0xEE);
    const unsigned char clear[] = {0xB4, 0x10};
    preparar(clear, sizeof clear); bancoRegistradores[REG_X].valor = 123;
    conferir("CLEAR executa", executar(m) == SIM_OK, 1);
    conferir("CLEAR X", m->X, 0);
    const unsigned char rmo[] = {0xAC, 0x01};
    preparar(rmo, sizeof rmo); bancoRegistradores[REG_A].valor = 0xABCD;
    conferir("RMO executa", executar(m) == SIM_OK, 1);
    conferir("RMO A,X", m->X, 0xABCD);

    const unsigned char jumps[] = {0x33, 0x37, 0x3B};
    const unsigned cc[] = {2, 3, 1};
    const char *nomes[] = {"JEQ", "JGT", "JLT"};
    for (int i = 0; i < 3; i++) {
        unsigned char codigo[] = {jumps[i], 0, 0x80};
        preparar(codigo, sizeof codigo); bancoRegistradores[REG_SW].valor = cc[i];
        int r = executar(m);
        snprintf(nome, sizeof nome, "%s condicao verdadeira", nomes[i]);
        conferir(nome, r == SIM_OK && m->PC == 0x80, 1);
        preparar(codigo, sizeof codigo); bancoRegistradores[REG_SW].valor = cc[i] % 3 + 1;
        r = executar(m);
        snprintf(nome, sizeof nome, "%s condicao falsa", nomes[i]);
        conferir(nome, r == SIM_OK && m->PC == 3, 1);
    }
    const unsigned char jump[] = {0x3F, 0, 0x80};
    preparar(jump, sizeof jump);
    conferir("J incondicional", executar(m) == SIM_OK && m->PC == 0x80, 1);
    const unsigned char jsub[] = {0x4B, 0, 0x80};
    preparar(jsub, sizeof jsub);
    conferir("JSUB guarda PC seguinte e salta", executar(m) == SIM_OK && m->L == 3 && m->PC == 0x80, 1);
    const unsigned char rsub[] = {0x4F, 0, 0};
    preparar(rsub, sizeof rsub); bancoRegistradores[REG_L].valor = 0x80;
    conferir("RSUB restaura PC", executar(m) == SIM_OK && m->PC == 0x80, 1);
}

static void testar_enderecamento(Maquina *m) {
    const unsigned char codigos[][4] = {
        {0x03, 0x00, 0x80, 0}, {0x02, 0x00, 0x60, 0},
        {0x03, 0x80, 0x70, 0}, {0x03, 0x40, 0x10, 0},
        {0x03, 0x20, 0x7D, 0}, {0x03, 0x10, 0x00, 0x80}
    };
    const char *nomes[] = {"LDA direto", "LDA indireto", "LDA indexado", "LDA base-relativo", "LDA PC-relativo", "LDA formato 4"};
    for (int i = 0; i < 6; i++) {
        preparar(codigos[i], sizeof codigos[i]);
        bancoRegistradores[REG_X].valor = 0x10;
        bancoRegistradores[REG_B].valor = 0x70;
        escrever_word_24bits(0x60, 0x80);
        escrever_word_24bits(0x80, 0x654321);
        int r = executar(m);
        conferir(nomes[i], r == SIM_OK && m->A == 0x654321, 1);
    }
    inicializar_memoria();
    memoria[0x100] = 0x03; memoria[0x101] = 0x2F; memoria[0x102] = 0x7D;
    bancoRegistradores[REG_PC].valor = 0x100;
    escrever_word_24bits(0x80, 0x654321);
    conferir("LDA PC-relativo negativo", executar(m) == SIM_OK && m->A == 0x654321, 1);
    const unsigned char fora[] = {0x0F, 0x0F, 0xFF};
    preparar(fora, sizeof fora);
    conferir("STA nao ultrapassa fim da memoria", executar(m) == SIM_ERRO, 1);
}

static void testar_limites(Maquina *m) {
    conferir("Memoria corresponde a MEM_TAMANHO", m->mem_tam, MEM_TAMANHO);
    for (int i = 0; i < 2; i++) {
        unsigned char codigo[] = {(unsigned char)(i ? 0xA8 : 0xA4), (unsigned char)(i ? 0x0F : 0x06)};
        preparar(codigo, sizeof codigo); bancoRegistradores[REG_A].valor = 1;
        conferir(i ? "SHIFTR A,16 aceito" : "SHIFTL A,7 aceito", executar(m) == SIM_OK, 1);
        printf("  diagnostico: %s\n", descricao);
    }
    const unsigned char shl[] = {0xA4, 0x00};
    preparar(shl, sizeof shl); bancoRegistradores[REG_A].valor = 0x800000;
    executar(m);
    conferir("SHIFTL A,1 deve circular bit 23 para bit 0", m->A, 1);
    Instrucao in;
    const unsigned char sic1000[] = {0x00, 0x10, 0x00, 0x00};
    int r = decodificarInstrucao(sic1000, sizeof sic1000, 0, 0, 0, &in);
    conferir("SIC legado LDA 1000 mantem 3 bytes e endereco", r && in.tamanho == 3 && in.enderecoEfetivo == 0x1000, 1);
    const unsigned char sic6000[] = {0x00, 0x60, 0x00, 0x00};
    r = decodificarInstrucao(sic6000, sizeof sic6000, 0, 0, 0, &in);
    conferir("SIC legado LDA 6000 decodifica", r && in.tamanho == 3 && in.enderecoEfetivo == 0x6000, 1);
    const unsigned char extended[] = {0x03, 0x10, 0x00, 0x80};
    r = decodificarInstrucao(extended, sizeof extended, 0, 0, 0, &in);
    conferir("Instrucao estendida informa formato 4", r ? in.formato : 0, 4);
    char erro[160] = "";
    r = sim_carregar(m, "exemplo.obj", erro, sizeof erro);
    if (r == 0) executar(m);
    uint32_t antes_a = m->A, antes_pc = m->PC;
    unsigned char memoria_antes[MEM_TAMANHO];
    memcpy(memoria_antes, memoria, sizeof memoria_antes);
    r = sim_carregar(m, "testes/objeto_invalido.obj", erro, sizeof erro);
    conferir("Objeto invalido retorna erro", r != 0, 1);
    conferir("Falha de carga preserva programa anterior", m->A == antes_a && m->PC == antes_pc && memoria[0] == 1, 1);
    conferir("Falha de carga preserva todos os bytes", memcmp(memoria_antes, memoria, sizeof memoria_antes) == 0, 1);
    printf("  diagnostico carga: %s; A=%u PC=%u\n", erro, m->A, m->PC);
    r = sim_carregar(m, "testes/entrada_invalida.obj", erro, sizeof erro);
    conferir("Entrada E invalida deve ser rejeitada", r != 0, 1);
    r = sim_carregar(m, "testes/sem_registro_e.obj", erro, sizeof erro);
    conferir("Objeto sem registro E rejeitado", r != 0, 1);
    r = sim_carregar(m, "testes/entrada_deslocada.obj", erro, sizeof erro);
    conferir("Carga em endereco nao zero", r == 0 && m->PC == 0x100 && erro[0] == '\0', 1);
    conferir("LDA no endereco 100", executar(m) == SIM_OK && m->A == 5, 1);
    conferir("STA no programa deslocado", executar(m) == SIM_OK && ler_word_24bits(0x80) == 5, 1);
    conferir("Termino no endereco 106", executar(m) == SIM_FIM && m->PC == 0x106, 1);
    sim_reset(m);
    conferir("Reset volta a entrada e preserva memoria", m->PC == 0x100 && m->A == 0 && ler_word_24bits(0x80) == 5, 1);
    r = sim_carregar(m, "testes/separadores.obj", erro, sizeof erro);
    conferir("Formato H T E com separadores", r == 0 && executar(m) == SIM_FIM, 1);
    const unsigned char div_zero[] = {0x25, 0, 0};
    preparar(div_zero, sizeof div_zero);
    bancoRegistradores[REG_A].valor = 5;
    conferir("Erro de execucao preserva PC e A", executar(m) == SIM_ERRO && m->PC == 0 && m->A == 5, 1);
}

int main(void) {
    Maquina *m = sim_criar();
    if (!m) return 2;
    testar_movimentacao(m);
    testar_enderecamento(m);
    testar_limites(m);
    sim_destruir(m);
    printf("RESULTADO: %d passaram; %d falharam.\n", passou, falhou);
    return falhou ? 1 : 0;
}
