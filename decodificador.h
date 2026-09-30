#ifndef DECODIFICADOR_H
#define DECODIFICADOR_H

#include <stdint.h>
#include <stddef.h>

/* Informacoes de uma instrucao decodificada */

typedef struct {

    uint8_t opcode;

    int formato;
    int tamanho;

    /* Bits de enderecamento */
    int n, i, x, b, p, e;

    /* Registradores do formato 2 */
    int r1, r2;

    /* Campo de endereco ou deslocamento */
    uint32_t endereco;
    int deslocamento;

    /* Resultado do enderecamento */
    uint32_t enderecoEfetivo;
    uint32_t operando;

    int imediato;
    int indireto;

    uint32_t proximoPC;

} Instrucao;


/* Le uma palavra de 24 bits */
int lerPalavra(
    const uint8_t *memoria,
    size_t tamanhoMemoria,
    uint32_t endereco,
    uint32_t *valor
);


/* Identifica o formato */
int identificarFormato(uint8_t primeiroByte);


/* Converte deslocamento de 12 bits */
int converterDeslocamento(int valor);


/* Calcula o endereco efetivo */
int calcularEndereco(
    const uint8_t *memoria,
    size_t tamanhoMemoria,
    uint32_t PC,
    uint32_t X,
    uint32_t B,
    Instrucao *inst
);


/* Decodifica uma instrucao */
int decodificarInstrucao(
    const uint8_t *memoria,
    size_t tamanhoMemoria,
    uint32_t PC,
    uint32_t X,
    uint32_t B,
    Instrucao *inst
);

#endif