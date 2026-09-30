
#include <stdio.h>
#include <stdlib.h>
#include "decodificador.h"

int main() {

    size_t tamanho = 4096;

    uint8_t *memoria = malloc(tamanho);

    Instrucao inst;

    uint32_t PC = 100;
    uint32_t X = 0;
    uint32_t B = 0;


    if (memoria == NULL) {
        printf("Erro ao alocar memoria!\n");
        return 1;
    }


    /* Exemplo: LDA #10 */

    memoria[100] = 0x01;
    memoria[101] = 0x00;
    memoria[102] = 0x0A;


    if (decodificarInstrucao(
        memoria,
        tamanho,
        PC,
        X,
        B,
        &inst
    )) {

        printf("Instrucao decodificada!\n");

        printf("Opcode: %02X\n", inst.opcode);
        printf("Formato: %d\n", inst.formato);
        printf("Tamanho: %d bytes\n", inst.tamanho);

        printf("nixbpe: %d%d%d%d%d%d\n",
            inst.n, inst.i, inst.x,
            inst.b, inst.p, inst.e
        );

        printf("Operando: %u\n", inst.operando);
        printf("Proximo PC: %u\n", inst.proximoPC);
    }

    else {
        printf("Erro na decodificacao!\n");
    }


    free(memoria);

    return 0;
}