#include "montador.h"
#include "macros.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    Montagem *m;
    FILE *objeto, *listagem = NULL;
    char erro[256];
    int ok, fechou;
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s entrada.asm saida.obj [saida.lst]\n", argv[0]);
        fprintf(stderr, "Por seguranca, arquivos existentes nao sao sobrescritos.\n");
        return 2;
    }
    m = calloc(1, sizeof *m);
    if (!m) { fprintf(stderr, "Memoria insuficiente\n"); return 1; }
    ok = montar_com_macros(argv[1], m, erro, sizeof erro);
    if (!ok) { fprintf(stderr, "%s\n", erro); free(m); return 1; }
    /* Modo exclusivo C11 protege entrada, saidas anteriores e caminhos iguais. */
    objeto = fopen(argv[2], "wx");
    if (!objeto) {
        fprintf(stderr, "Nao foi possivel criar %s (ja existe ou caminho invalido).\n", argv[2]);
        free(m); return 1;
    }
    if (argc == 4) {
        listagem = fopen(argv[3], "wx");
        if (!listagem) {
            fprintf(stderr, "Nao foi possivel criar %s (ja existe ou caminho invalido).\n", argv[3]);
            fclose(objeto); remove(argv[2]); free(m); return 1;
        }
    }
    ok = escrever_objeto(objeto, m);
    fechou = fclose(objeto);
    if (fechou) ok = 0;
    if (listagem) {
        if (!escrever_listagem(listagem, m)) ok = 0;
        if (fclose(listagem)) ok = 0;
    }
    if (!ok) {
        /* Remover somente os arquivos novos criados exclusivamente nesta chamada. */
        remove(argv[2]); if (argc == 4) remove(argv[3]);
        fprintf(stderr, "Falha de escrita; saidas incompletas removidas.\n");
        free(m); return 1;
    }
    printf("Montagem concluida: %u bytes, %u simbolos, entrada %06X.\n",
           m->bytes, m->nsimbolos, m->entrada);
    printf("Objeto: %s\n", argv[2]);
    printf("Assembly expandido: MASMAPRG.ASM\n");
    if (argc == 4) printf("Listagem: %s\n", argv[3]);
    free(m);
    return 0;
}
