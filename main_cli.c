#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "sicxe.h"

/* Uso: simulador [programa.obj] [limite_de_passos] */
int main(int argc, char **argv) {
    const char *arquivo = argc > 1 ? argv[1] : "exemplo.obj";
    unsigned long limite = 10000;
    if (argc > 3) {
        fprintf(stderr, "Uso: %s [programa.obj] [limite_de_passos]\n", argv[0]);
        return 2;
    }
    if (argc == 3) {
        char *fim;
        errno = 0;
        limite = strtoul(argv[2], &fim, 10);
        if (errno || !argv[2][0] || argv[2][0] == '-' || *fim || limite == 0) {
            fprintf(stderr, "O limite deve ser um inteiro positivo.\n");
            return 2;
        }
    }
    Maquina *m = sim_criar();
    if (!m) { fprintf(stderr, "Memoria insuficiente.\n"); return 1; }
    char texto[180];
    if (sim_carregar(m, arquivo, texto, sizeof texto) != 0) {
        fprintf(stderr, "Erro ao carregar: %s\n", texto);
        sim_destruir(m);
        return 1;
    }
    printf("Programa: %s | memoria: %zu bytes | entrada: %06X\n", arquivo, m->mem_tam, (unsigned)m->PC);
    int resultado = 2;
    for (unsigned long passo = 0; passo < limite; passo++) {
        int r = sim_passo(m, texto, sizeof texto);
        printf("%lu  %s\n", passo + 1, texto);
        if (r != SIM_OK) {
            resultado = r == SIM_FIM ? 0 : 1;
            break;
        }
    }
    printf("A=%06X X=%06X L=%06X B=%06X S=%06X T=%06X PC=%06X SW=%06X\n",
           (unsigned)m->A, (unsigned)m->X, (unsigned)m->L, (unsigned)m->B,
           (unsigned)m->S, (unsigned)m->T, (unsigned)m->PC, (unsigned)m->SW);
    if (resultado == 2) fprintf(stderr, "Limite de passos atingido; execucao interrompida.\n");
    sim_destruir(m);
    return resultado;
}
