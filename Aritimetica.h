#ifndef ARITIMETICA_H
#define ARITIMETICA_H

#include <stdint.h>


extern int32_t acumulador_a;            //as variaveis ja explicadas no codigo
extern double acumulador_f;
extern int32_t palavra_status_sw;

// Constantes do codigo condicional(CC) que são os estados
extern const int32_t CC_MENOR;
extern const int32_t CC_IGUAL;
extern const int32_t CC_MAIOR;

// funções para limitar em 24 bits e ver o estado
int32_t limitar_24_bits(int32_t valor);
void atualizar_codigo_condicional(int32_t valor_a, int32_t valor_b);
void atualizar_codigo_condicional_float(double valor_a, double valor_b);

// funções das contas aritimeticas que usam a memoria
void fazer_add(int32_t valor_memoria);
void fazer_sub(int32_t valor_memoria);
void fazer_mul(int32_t valor_memoria);
void fazer_div(int32_t valor_memoria);
void fazer_and(int32_t valor_memoria);
void fazer_or(int32_t valor_memoria);
void fazer_comp(int32_t valor_memoria);

// funções que fazem as contas com ponto flutuante
void fazer_addf(double valor_memoria_float);
void fazer_subf(double valor_memoria_float);
void fazer_mulf(double valor_memoria_float);
void fazer_divf(double valor_memoria_float);
void fazer_compf(double valor_memoria_float);

// funções que fazem as contas com 2 regisradores
int32_t fazer_addr(int32_t reg1, int32_t reg2);
int32_t fazer_subr(int32_t reg1, int32_t reg2);
int32_t fazer_mulr(int32_t reg1, int32_t reg2);
int32_t fazer_divr(int32_t reg1, int32_t reg2);
void fazer_compr(int32_t reg1, int32_t reg2);

// funções que fazem os deslocamentes tanto para esquerda quanto para direita
int32_t fazer_shiftl(int32_t reg1, int quantidade_bits);
int32_t fazer_shiftr(int32_t reg1, int quantidade_bits);

// funções que fazem a conversão entre inteiros e flutuantes
void fazer_fix(void);
void fazer_float(void);
void fazer_norm(void);

#endif