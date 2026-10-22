/*
 * teste_sim_real.c - testa as partes 1 a 4 JUNTAS, pelo contrato de sicxe.h (sem interface grafica).
 * Compilar e rodar:  make teste
 */
#include <stdio.h>
#include <string.h>

#include "memoria.h"
#include "sicxe.h"

static int falhas = 0;

#define CONFERE(cond, texto)                                   \
    do {                                                       \
        if (cond) {                                            \
            printf("  ok   %s\n", texto);                      \
        } else {                                               \
            printf("  FALHOU: %s\n", texto);                   \
            falhas++;                                          \
        }                                                      \
    } while (0)

static int passo(Maquina *m) {
    char desc[160];
    int r = sim_passo(m, desc, sizeof desc);
    printf("%d  %s\n", r, desc);
    return r;
}

int main(int argc, char **argv) {
    const char *arquivo = argc > 1 ? argv[1] : "exemplo.obj";
    char erro[120] = "";

    Maquina *m = sim_criar();
    if (!m) return 1;

    printf("carregando %s\n", arquivo);
    CONFERE(sim_carregar(m, arquivo, erro, sizeof erro) == 0, "programa carregado");
    if (erro[0]) printf("  erro: %s\n", erro);
    CONFERE(m->PC == 0, "PC no endereco de entrada");

    passo(m); /* LDA #5 */
    CONFERE(m->A == 5, "LDA #5 -> A = 5");
    passo(m); /* ADD #3 */
    CONFERE(m->A == 8, "ADD #3 -> A = 8");
    passo(m); /* STA 40 */
    CONFERE(memoria[0x42] == 8, "STA 40 -> memoria[42] = 8");
    passo(m); /* COMP #8 */
    CONFERE((m->SW & 3) == 2, "COMP #8 -> codigo condicional = igual");
    passo(m); /* JEQ 12 */
    CONFERE(m->PC == 0x12, "JEQ salta para 000012 (pula o LDA #99)");
    passo(m); /* JSUB 1B */
    CONFERE(m->L == 0x15 && m->PC == 0x1B, "JSUB guarda o retorno em L e salta");
    passo(m); /* LDB #7 */
    CONFERE(m->B == 7, "LDB #7 -> B = 7");
    passo(m); /* RSUB */
    CONFERE(m->PC == 0x15, "RSUB volta para 000015");
    CONFERE(passo(m) == SIM_FIM, "J 000015 (salta para si mesmo) termina o programa");

    sim_reset(m);
    CONFERE(m->A == 0 && m->PC == 0, "sim_reset zera registradores e volta o PC");

    sim_destruir(m);
    printf(falhas ? "\n%d FALHA(S)\n" : "\nTUDO OK\n", falhas);
    return falhas ? 1 : 0;
}
