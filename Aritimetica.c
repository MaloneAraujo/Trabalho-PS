#include <stdint.h>

int32_t acumulador_a = 0;       //Esse aqui inicia o acumulador padrão em 0 e recebe a maioria dos resultados
double acumulador_f = 0.0;      //Ja esse serve para o de ponto flutuante
int32_t palavra_status_sw = 0;  //E esse vai salvar os resultados de comparações que são esses de baixo

// Constantes para o codigo condicional(CC) que representa o estados das comparações
const int32_t CC_MENOR = -1;
const int32_t CC_IGUAL = 0;
const int32_t CC_MAIOR = 1;

int32_t limitar_24_bits(int32_t valor) {  // basicamente essa parte vai ser oque deixa limitado a 24bits, onde ele verifica se o 24bit é 1
    if (valor & 0x00800000) {            // se ele for então vai enxer oque sobrar com 1
        return valor | 0xFF000000; 
    }
    return valor & 0x00FFFFFF;  // aqui se não for ele só vai zerar tudo que passou do bit 24
}

// ***Daqui para baixo até a proxima marcação vão ser as funções do codigo condicional***

void atualizar_codigo_condicional(int32_t valor_a, int32_t valor_b) { // aqui ele vai fazerer uma comparação de 2 valores e salvar no palavra_status_sw
    if (valor_a < valor_b) {             //ele salva -1 no sw se a for menor
        palavra_status_sw = CC_MENOR;
    } else if (valor_a == valor_b) {  // salva 0 no sw se a for igual a b
        palavra_status_sw = CC_IGUAL;
    } else {                             // e salva 1 no sw se a for maior que b
        palavra_status_sw = CC_MAIOR;
    }
}

void atualizar_codigo_condicional_float(double valor_a, double valor_b) { // nesse ele faz a mesma coisa mas ele recebe valores em formato double
    if (valor_a < valor_b) {
        palavra_status_sw = CC_MENOR;
    } else if (valor_a == valor_b) {
        palavra_status_sw = CC_IGUAL;
    } else {
        palavra_status_sw = CC_MAIOR;
    }
}

// ***Daqui para baixo começa as fuções aritiméticas que usam a memoria*** aqui ele usa o acumulador a e a memoria ram

void fazer_add(int32_t valor_memoria) { // essa função simplismente vai somar o valor do acumulador a com a memoria e limitar a 24bits
    acumulador_a = limitar_24_bits(acumulador_a + valor_memoria);
}

void fazer_sub(int32_t valor_memoria) { // mesma coisa que a anterior, mas trabalha com subtração
    acumulador_a = limitar_24_bits(acumulador_a - valor_memoria);
}

void fazer_mul(int32_t valor_memoria) { // igual, mas multiplica
    acumulador_a = limitar_24_bits(acumulador_a * valor_memoria);
}

void fazer_div(int32_t valor_memoria) { // aqui é a mesma coisa, mas divide e tem o if protegendo caso venha um 0 da memória
    if (valor_memoria != 0) { 
        acumulador_a = limitar_24_bits(acumulador_a / valor_memoria);
    }
}

void fazer_and(int32_t valor_memoria) { // aqui ele vai reslizar uma operação AND bit a bit
    acumulador_a = limitar_24_bits(acumulador_a & valor_memoria);
}

void fazer_or(int32_t valor_memoria) { // aqui ele vai fazer o or bit a bit
    acumulador_a = limitar_24_bits(acumulador_a | valor_memoria);
}

void fazer_comp(int32_t valor_memoria) {// e aqui vai só comparar os valores
    atualizar_codigo_condicional(acumulador_a, valor_memoria);
}

// ***Nesta parte de baixo são as funções que mexem com pontos flutuantes*** aqui se usa o acumulador f e não precisa limitar a 24bits

void fazer_addf(double valor_memoria_float) {
    acumulador_f = acumulador_f + valor_memoria_float;                  // toda essa parte das operações faz a mesma coisa que o de antes, mas agora
}                                                                       //usando outro acumulador, mas a funcionalidade é a mesma

void fazer_subf(double valor_memoria_float) {
    acumulador_f = acumulador_f - valor_memoria_float;
}

void fazer_mulf(double valor_memoria_float) {
    acumulador_f = acumulador_f * valor_memoria_float;
}

void fazer_divf(double valor_memoria_float) {
    if (valor_memoria_float != 0.0) {
        acumulador_f = acumulador_f / valor_memoria_float;
    }
}

void fazer_compf(double valor_memoria_float) {
    atualizar_codigo_condicional_float(acumulador_f, valor_memoria_float);
}

// Daqui para baixo estão as funções que trabalham com 2 registradores

                                                                                //LEMBRETE: quando eu for integrar meu codigo ao dos meus colegas lembrar de ver
                                                                                // o nome que eles colocaram nos registradores e mudar o reg1 e reg2
                                                                                //  *talvez eu posssa mudar para ponteiro oque não geraria esse problema, ver isso depois

int32_t fazer_addr(int32_t reg1, int32_t reg2) {//Soma os 2 registradores e fazer o tratamento de 24bits                      
    return limitar_24_bits(reg2 + reg1);
}

int32_t fazer_subr(int32_t reg1, int32_t reg2) { //Subtrai os 2 registradores, fazer o tratamento e retorna o valor
    return limitar_24_bits(reg2 - reg1);
}

int32_t fazer_mulr(int32_t reg1, int32_t reg2) { //multiplica os registradores e tambem fazer o tratamento e retorna
    return limitar_24_bits(reg2 * reg1);
}

int32_t fazer_divr(int32_t reg1, int32_t reg2) { // divide os registradores e verifica o 0 no denominador, fazer o tratamento e retorna
    if (reg1 != 0) {
        return limitar_24_bits(reg2 / reg1);
    }
    return reg2; 
}

void fazer_compr(int32_t reg1, int32_t reg2) { // compara os regitradores e atualiza o estado
    atualizar_codigo_condicional(reg1, reg2);
}

// ***Aqui em baixo eu faço as funções de deslocamento de bits***

                                                                        //LEMBRAR de ver o negocio do reg1
int32_t fazer_shiftl(int32_t reg1, int quantidade_bits) { // empurra os bits para a esquerda que é multiplicar por 2 pra cada bit deslocado e isso faz
    return limitar_24_bits(reg1 << quantidade_bits);         // o numero crescer então tem que arrumar para os 24bits
}

int32_t fazer_shiftr(int32_t reg1, int quantidade_bits) { // aqui ele desloca para a direita que é dividir por 2 a cada bit deslocado, não aumante o valor então ok
    return reg1 >> quantidade_bits; 
}

//***Aqui fica a parte de conversão entre valores inteiros e flutuantes***

void fazer_fix() {
    acumulador_a = limitar_24_bits((int32_t)acumulador_f); //ele basicamente usa o cast do int32_t para tirar a virgula e salvar como inteiro no acumulador a
}

void fazer_float() {
    acumulador_f = (double)acumulador_a; // aqui ele vai pegar o inteiro converter para decimal com o cast e salvar no acumulador f
}

void fazer_norm() { // aqui fica vazia pq pelo oque eu pesquisei o c ja normaliza por padrão, mas la pede então ta aqui

}