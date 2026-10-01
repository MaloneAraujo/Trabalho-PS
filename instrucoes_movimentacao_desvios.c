#include "instrucoes_movimentacao_desvios.h"

#include <stddef.h>

#define SICXE_MASCARA_24_BITS UINT32_C(0x00FFFFFF)
#define SICXE_MASCARA_BYTE UINT32_C(0x000000FF)
#define SICXE_MASCARA_BYTES_SUPERIORES UINT32_C(0x00FFFF00)

static uint32_t limitar_24_bits(uint32_t valor) {
    return valor & SICXE_MASCARA_24_BITS;
}

static uint32_t ler_registrador(
    const SicxeExecutorMovimentacaoDesvios *executor,
    SicxeRegistrador registrador
) {
    return limitar_24_bits(
        executor->cpu->ler_registrador(executor->cpu->contexto, registrador)
    );
}

static void gravar_registrador(
    SicxeExecutorMovimentacaoDesvios *executor,
    SicxeRegistrador registrador,
    uint32_t valor
) {
    executor->cpu->gravar_registrador(
        executor->cpu->contexto,
        registrador,
        limitar_24_bits(valor)
    );
}

static uint32_t ler_operando_palavra(
    const SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    if (instrucao->imediata) {
        return limitar_24_bits(instrucao->valor_ou_endereco);
    }

    return limitar_24_bits(
        executor->memoria->ler_palavra24(
            executor->memoria->contexto,
            instrucao->valor_ou_endereco
        )
    );
}

static uint8_t ler_operando_byte(
    const SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    if (instrucao->imediata) {
        return (uint8_t)(instrucao->valor_ou_endereco & SICXE_MASCARA_BYTE);
    }

    return executor->memoria->ler_byte(
        executor->memoria->contexto,
        instrucao->valor_ou_endereco
    );
}

static void carregar_palavra(
    SicxeExecutorMovimentacaoDesvios *executor,
    SicxeRegistrador destino,
    const SicxeInstrucao *instrucao
) {
    gravar_registrador(executor, destino, ler_operando_palavra(executor, instrucao));
}

static void carregar_byte_em_a(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    uint32_t acumulador = ler_registrador(executor, SICXE_REG_A);
    uint32_t byte_lido = ler_operando_byte(executor, instrucao);

    /* LDCH altera somente os oito bits da direita do acumulador A. */
    uint32_t resultado =
        (acumulador & SICXE_MASCARA_BYTES_SUPERIORES) |
        (byte_lido & SICXE_MASCARA_BYTE);

    gravar_registrador(executor, SICXE_REG_A, resultado);
}

static SicxeResultadoExecucao armazenar_palavra(
    SicxeExecutorMovimentacaoDesvios *executor,
    SicxeRegistrador origem,
    const SicxeInstrucao *instrucao
) {
    if (instrucao->imediata) {
        return SICXE_ERRO_OPERANDO;
    }

    executor->memoria->gravar_palavra24(
        executor->memoria->contexto,
        instrucao->valor_ou_endereco,
        ler_registrador(executor, origem)
    );
    return SICXE_EXECUCAO_OK;
}

static SicxeResultadoExecucao armazenar_byte_de_a(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    if (instrucao->imediata) {
        return SICXE_ERRO_OPERANDO;
    }

    executor->memoria->gravar_byte(
        executor->memoria->contexto,
        instrucao->valor_ou_endereco,
        (uint8_t)(ler_registrador(executor, SICXE_REG_A) & SICXE_MASCARA_BYTE)
    );
    return SICXE_EXECUCAO_OK;
}

static void saltar(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    gravar_registrador(executor, SICXE_REG_PC, instrucao->valor_ou_endereco);
}

static void saltar_se(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao,
    SicxeCodigoCondicional condicao_necessaria
) {
    SicxeCodigoCondicional condicao_atual =
        executor->cpu->ler_codigo_condicional(executor->cpu->contexto);

    if (condicao_atual == condicao_necessaria) {
        saltar(executor, instrucao);
    }
}

static void chamar_subrotina(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    /* O ciclo de busca deve avancar PC antes de executar JSUB. */
    uint32_t endereco_retorno = ler_registrador(executor, SICXE_REG_PC);
    gravar_registrador(executor, SICXE_REG_L, endereco_retorno);
    saltar(executor, instrucao);
}

static void retornar_subrotina(SicxeExecutorMovimentacaoDesvios *executor) {
    gravar_registrador(
        executor,
        SICXE_REG_PC,
        ler_registrador(executor, SICXE_REG_L)
    );
}

SicxeResultadoExecucao sicxe_inicializar_executor_movimentacao_desvios(
    SicxeExecutorMovimentacaoDesvios *executor,
    SicxeCpu *cpu,
    SicxeMemoria *memoria
) {
    if (
        executor == NULL ||
        cpu == NULL ||
        memoria == NULL ||
        cpu->ler_registrador == NULL ||
        cpu->gravar_registrador == NULL ||
        cpu->ler_codigo_condicional == NULL ||
        memoria->ler_byte == NULL ||
        memoria->gravar_byte == NULL ||
        memoria->ler_palavra24 == NULL ||
        memoria->gravar_palavra24 == NULL
    ) {
        return SICXE_ERRO_ARGUMENTO;
    }

    executor->cpu = cpu;
    executor->memoria = memoria;
    return SICXE_EXECUCAO_OK;
}

SicxeResultadoExecucao sicxe_executar_movimentacao_desvio(
    SicxeExecutorMovimentacaoDesvios *executor,
    const SicxeInstrucao *instrucao
) {
    if (executor == NULL || executor->cpu == NULL ||
        executor->memoria == NULL || instrucao == NULL) {
        return SICXE_ERRO_ARGUMENTO;
    }

    switch (instrucao->mnemonico) {
        case SICXE_LDA:
            carregar_palavra(executor, SICXE_REG_A, instrucao);
            break;
        case SICXE_LDB:
            carregar_palavra(executor, SICXE_REG_B, instrucao);
            break;
        case SICXE_LDL:
            carregar_palavra(executor, SICXE_REG_L, instrucao);
            break;
        case SICXE_LDS:
            carregar_palavra(executor, SICXE_REG_S, instrucao);
            break;
        case SICXE_LDT:
            carregar_palavra(executor, SICXE_REG_T, instrucao);
            break;
        case SICXE_LDX:
            carregar_palavra(executor, SICXE_REG_X, instrucao);
            break;
        case SICXE_LDCH:
            carregar_byte_em_a(executor, instrucao);
            break;

        case SICXE_STA:
            return armazenar_palavra(executor, SICXE_REG_A, instrucao);
        case SICXE_STB:
            return armazenar_palavra(executor, SICXE_REG_B, instrucao);
        case SICXE_STL:
            return armazenar_palavra(executor, SICXE_REG_L, instrucao);
        case SICXE_STS:
            return armazenar_palavra(executor, SICXE_REG_S, instrucao);
        case SICXE_STT:
            return armazenar_palavra(executor, SICXE_REG_T, instrucao);
        case SICXE_STX:
            return armazenar_palavra(executor, SICXE_REG_X, instrucao);
        case SICXE_STCH:
            return armazenar_byte_de_a(executor, instrucao);

        case SICXE_CLEAR:
            if (!instrucao->possui_r1) {
                return SICXE_ERRO_OPERANDO;
            }
            gravar_registrador(executor, instrucao->r1, 0);
            break;

        case SICXE_RMO:
            if (!instrucao->possui_r1 || !instrucao->possui_r2) {
                return SICXE_ERRO_OPERANDO;
            }
            gravar_registrador(
                executor,
                instrucao->r2,
                ler_registrador(executor, instrucao->r1)
            );
            break;

        case SICXE_J:
            saltar(executor, instrucao);
            break;
        case SICXE_JEQ:
            saltar_se(executor, instrucao, SICXE_CC_IGUAL);
            break;
        case SICXE_JGT:
            saltar_se(executor, instrucao, SICXE_CC_MAIOR);
            break;
        case SICXE_JLT:
            saltar_se(executor, instrucao, SICXE_CC_MENOR);
            break;
        case SICXE_JSUB:
            chamar_subrotina(executor, instrucao);
            break;
        case SICXE_RSUB:
            retornar_subrotina(executor);
            break;

        default:
            return SICXE_ERRO_INSTRUCAO;
    }

    return SICXE_EXECUCAO_OK;
}

static SicxeInstrucao criar_instrucao_base(SicxeMnemonico mnemonico) {
    SicxeInstrucao instrucao = {0};
    instrucao.mnemonico = mnemonico;
    return instrucao;
}

SicxeInstrucao sicxe_instrucao_memoria(
    SicxeMnemonico mnemonico,
    uint32_t endereco_efetivo
) {
    SicxeInstrucao instrucao = criar_instrucao_base(mnemonico);
    instrucao.valor_ou_endereco = limitar_24_bits(endereco_efetivo);
    return instrucao;
}

SicxeInstrucao sicxe_instrucao_imediata(
    SicxeMnemonico mnemonico,
    uint32_t valor
) {
    SicxeInstrucao instrucao = criar_instrucao_base(mnemonico);
    instrucao.valor_ou_endereco = limitar_24_bits(valor);
    instrucao.imediata = true;
    return instrucao;
}

SicxeInstrucao sicxe_instrucao_um_registrador(
    SicxeMnemonico mnemonico,
    SicxeRegistrador r1
) {
    SicxeInstrucao instrucao = criar_instrucao_base(mnemonico);
    instrucao.r1 = r1;
    instrucao.possui_r1 = true;
    return instrucao;
}

SicxeInstrucao sicxe_instrucao_dois_registradores(
    SicxeMnemonico mnemonico,
    SicxeRegistrador r1,
    SicxeRegistrador r2
) {
    SicxeInstrucao instrucao = criar_instrucao_base(mnemonico);
    instrucao.r1 = r1;
    instrucao.r2 = r2;
    instrucao.possui_r1 = true;
    instrucao.possui_r2 = true;
    return instrucao;
}

SicxeInstrucao sicxe_instrucao_sem_operando(SicxeMnemonico mnemonico) {
    return criar_instrucao_base(mnemonico);
}
