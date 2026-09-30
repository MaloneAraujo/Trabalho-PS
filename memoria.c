#include "memoria.h"


uint8_t memoria[4096];
Registrador bancoRegistradores[10];

void inicializar_memoria() {

    for (int i = 0; i < 4096; i++) {
        memoria[i] = 0;
    }

    for (int i = 0; i < 10; i++) {
        bancoRegistradores[i].valor = 0;
    }
}

uint32_t ler_word_24bits(int endereco) { //FUNÇÃO PARA LER PALAVRA
    if (endereco < 0 || endereco > 4093) return 0;
    return (memoria[endereco] << 16) | 
           (memoria[endereco + 1] << 8) | 
           memoria[endereco + 2];
}

void escrever_word_24bits(int endereco, uint32_t valor) { //FUNÇÃO PARA ESCREVER PALAVRA
    if (endereco < 0 || endereco > 4093) return;

    memoria[endereco]     = (valor >> 16) & 0xFF;
    memoria[endereco + 1] = (valor >> 8)  & 0xFF;
    memoria[endereco + 2] = valor & 0xFF;
}
