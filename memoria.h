#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdint.h>

#define REG_A   0 //DEFN REGS
#define REG_X   1
#define REG_L   2
#define REG_B   3
#define REG_S   4
#define REG_T   5
#define REG_F   6
#define REG_PC  8
#define REG_SW  9

typedef struct {
    uint64_t valor;
} Registrador;

extern uint8_t memoria[4096];
extern Registrador bancoRegistradores[10]; //RESERVA MEMORIA

void inicializar_memoria();
uint32_t ler_word_24bits(int endereco);
void escrever_word_24bits(int endereco, uint32_t valor);

#endif
