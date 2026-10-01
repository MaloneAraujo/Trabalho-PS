#include <stdio.h>
#include "memoria.h"

int main() {

    inicializar_hardware();
    printf("START HARDWARE\n");

    int endereco_alvo = 500;
    uint32_t valor_original = 10;

    printf("ESCREVENDO: %d END: %d\n", valor_original, endereco_alvo);
    escrever_palavra_24bits(endereco_alvo, valor_original);


    uint32_t valor_lido = ler_palavra_24bits(endereco_alvo);
    printf("VALOR LIDO: %d\n", valor_lido);

    if (valor_lido == valor_original) {
        printf("\n[SUCESSO]\n");
    } else {
        printf("\n[ERRO]\n");
    }

    return 0;
}