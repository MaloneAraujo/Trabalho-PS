#ifndef INSTRUCOES_MOVIMENTACAO_DESVIOS_H
#define INSTRUCOES_MOVIMENTACAO_DESVIOS_H

#include <stdbool.h>
#include <stdint.h>

/* Registradores utilizados pela parte de movimentacao e desvios. */
typedef enum {
    SICXE_REG_A,
    SICXE_REG_X,
    SICXE_REG_L,
    SICXE_REG_B,
    SICXE_REG_S,
    SICXE_REG_T,
    SICXE_REG_PC,
    SICXE_REG_SW
} SicxeRegistrador;

typedef enum {
    SICXE_CC_MENOR,
    SICXE_CC_IGUAL,
    SICXE_CC_MAIOR
} SicxeCodigoCondicional;

/* As 22 instrucoes atribuidas a Pessoa 4. */
typedef enum {
    SICXE_LDA,
    SICXE_LDB,
    SICXE_LDCH,
    SICXE_LDL,
    SICXE_LDS,
    SICXE_LDT,
    SICXE_LDX,
    SICXE_STA,
    SICXE_STB,
    SICXE_STCH,
    SICXE_STL,
    SICXE_STS,
    SICXE_STT,
    SICXE_STX,
    SICXE_CLEAR,
    SICXE_RMO,
    SICXE_J,
    SICXE_JEQ,
    SICXE_JGT,
    SICXE_JLT,
    SICXE_JSUB,
    SICXE_RSUB
} SicxeMnemonico;

/*
 * Contrato minimo esperado da CPU criada pelos outros integrantes.
 * contexto aponta para a estrutura real da CPU do projeto final.
 */
typedef struct {
    void *contexto;
    uint32_t (*ler_registrador)(void *contexto, SicxeRegistrador registrador);
    void (*gravar_registrador)(
        void *contexto,
        SicxeRegistrador registrador,
        uint32_t valor
    );
    SicxeCodigoCondicional (*ler_codigo_condicional)(void *contexto);
} SicxeCpu;

/*
 * Contrato minimo esperado da memoria criada pelos outros integrantes.
 * Uma palavra SIC/XE possui 24 bits e ocupa tres bytes consecutivos.
 */
typedef struct {
    void *contexto;
    uint8_t (*ler_byte)(void *contexto, uint32_t endereco);
    void (*gravar_byte)(void *contexto, uint32_t endereco, uint8_t valor);
    uint32_t (*ler_palavra24)(void *contexto, uint32_t endereco);
    void (*gravar_palavra24)(
        void *contexto,
        uint32_t endereco,
        uint32_t valor
    );
} SicxeMemoria;

/*
 * Resultado minimo esperado do futuro decodificador.
 *
 * valor_ou_endereco contem o literal no modo imediato ou o endereco efetivo
 * final nos demais modos. r1 e r2 sao utilizados por CLEAR e RMO.
 */
typedef struct {
    SicxeMnemonico mnemonico;
    uint32_t valor_ou_endereco;
    bool imediata;
    bool possui_r1;
    bool possui_r2;
    SicxeRegistrador r1;
    SicxeRegistrador r2;
} SicxeInstrucao;

typedef struct {
    SicxeCpu *cpu;
    SicxeMemoria *memoria;
} SicxeExecutorMovimentacaoDesvios;

typedef enum {
    SICXE_EXECUCAO_OK = 0,
    SICXE_ERRO_ARGUMENTO = -1,
    SICXE_ERRO_OPERANDO = -2,
    SICXE_ERRO_INSTRUCAO = -3
} SicxeResultadoExecucao;

SicxeResultadoExecucao sicxe_inicializar_executor_movimentacao_desvios(
    SicxeExecutorMovimentacaoDesvios *executor,
    SicxeCpu *cpu,
    SicxeMemoria *memoria
);

SicxeResultadoExecucao sicxe_executar_movimentacao_desvio(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
);

/* Funcoes auxiliares para montar instrucoes durante a integracao. */
SicxeInstrucao sicxe_instrucao_memoria(
    SicxeMnemonico mnemonico,
    uint32_t endereco_efetivo
);

SicxeInstrucao sicxe_instrucao_imediata(
    SicxeMnemonico mnemonico,
    uint32_t valor
);

SicxeInstrucao sicxe_instrucao_um_registrador(
    SicxeMnemonico mnemonico,
    SicxeRegistrador r1
);

SicxeInstrucao sicxe_instrucao_dois_registradores(
    SicxeMnemonico mnemonico,
    SicxeRegistrador r1,
    SicxeRegistrador r2
);

SicxeInstrucao sicxe_instrucao_sem_operando(SicxeMnemonico mnemonico);

#endif
