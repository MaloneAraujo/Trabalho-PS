#include "interno.h"
#include <ctype.h>
#include <string.h>

static int reservar(Montagem *m, unsigned lc, unsigned tamanho) {
    unsigned i;
    if (lc < m->inicio || lc > MEM_TAMANHO || tamanho > MEM_TAMANHO - lc)
        return erro_montagem(m, "programa fora da memoria (%u bytes)", MEM_TAMANHO);
    for (i = lc; i < lc + tamanho; ++i)
        if (m->ocupado[i]) return erro_montagem(m, "sobreposicao no endereco %06X", i);
    for (i = lc; i < lc + tamanho; ++i) m->ocupado[i] = 1;
    if (lc + tamanho > m->fim) m->fim = lc + tamanho;
    return 1;
}

int primeiro_passo(FILE *fonte, Montagem *m) {
    unsigned lc = 0, numero = 0;
    int terminou = 0, tem_conteudo = 0;
    char texto[ASM_TEXTO];
    while (fgets(texto, sizeof texto, fonte)) {
        Linha *l;
        Diretiva d;
        const Operacao *op;
        int32_t v = 0;
        int rel = 0;
        size_t n = strlen(texto);
        m->linha_atual = ++numero;
        if (n == sizeof texto - 1 && texto[n-1] != '\n')
            return erro_montagem(m, "linha muito longa (maximo %u caracteres)", ASM_TEXTO - 2);
        while (n && (texto[n-1] == '\n' || texto[n-1] == '\r')) texto[--n] = '\0';
        /* Aceita BOM UTF-8 apenas no inicio do arquivo. */
        if (numero == 1 && n >= 3 && (unsigned char)texto[0] == 0xEF &&
            (unsigned char)texto[1] == 0xBB && (unsigned char)texto[2] == 0xBF)
            memmove(texto, texto + 3, n - 2);
        if (m->nlinhas == ASM_LINHAS) return erro_montagem(m, "limite de linhas excedido");
        l = &m->linhas[m->nlinhas++];
        l->numero = numero; l->endereco = lc;
        strcpy(l->fonte, texto);
        if (!analisar_linha(m, l)) return 0;
        if (!*l->operacao) continue;
        if (terminou) return erro_montagem(m, "conteudo depois de END");
        d = buscar_diretiva(l->operacao);
        if (d == DIR_START) {
            if (tem_conteudo) return erro_montagem(m, "START deve ser a primeira operacao");
            if (!avaliar(m, l->operando, lc, 16, &v, &rel)) return 0;
            if (v < 0 || v >= (int32_t)MEM_TAMANHO || rel)
                return erro_montagem(m, "endereco START invalido");
            if (strlen(l->rotulo) > 6) return erro_montagem(m, "nome do programa deve ter ate 6 caracteres");
            lc = m->inicio = m->fim = (unsigned)v; l->endereco = lc;
            if (*l->rotulo) strcpy(m->nome, l->rotulo);
            if (!definir_simbolo(m, l->rotulo, v, 1)) return 0;
            tem_conteudo = 1;
            continue;
        }
        tem_conteudo = 1;
        if (d == DIR_END || d == DIR_BASE || d == DIR_NOBASE || d == DIR_ORG) {
            if (*l->rotulo) return erro_montagem(m, "esta diretiva nao aceita rotulo");
        }
        if (d == DIR_EQU) {
            if (!*l->rotulo) return erro_montagem(m, "EQU exige um rotulo");
            if (!avaliar(m, l->operando, lc, 10, &v, &rel) ||
                !definir_simbolo(m, l->rotulo, v, rel)) return 0;
            continue;
        }
        if (!definir_simbolo(m, l->rotulo, (int32_t)lc, 1)) return 0;
        switch (d) {
        case DIR_END: terminou = 1; break;
        case DIR_ORG:
            if (!avaliar(m, l->operando, lc, 10, &v, &rel)) return 0;
            if (v < (int32_t)m->inicio || v >= (int32_t)MEM_TAMANHO)
                return erro_montagem(m, "ORG fora do intervalo do programa");
            lc = (unsigned)v;
            break;
        case DIR_NOBASE:
            if (*l->operando) return erro_montagem(m, "NOBASE nao recebe operando");
            break;
        case DIR_BASE:
            if (!*l->operando) return erro_montagem(m, "BASE exige operando");
            break;
        case DIR_BYTE:
            if (!bytes_constante(m, l->operando, NULL, &l->tamanho)) return 0;
            break;
        case DIR_WORD: l->tamanho = 3; break;
        case DIR_RESB: case DIR_RESW:
            if (!avaliar(m, l->operando, lc, 10, &v, &rel)) return 0;
            if (v < 0 || rel || v > (int32_t)(MEM_TAMANHO / (d == DIR_RESW ? 3u : 1u)))
                return erro_montagem(m, "quantidade de reserva invalida");
            l->tamanho = (unsigned)v * (d == DIR_RESW ? 3u : 1u);
            break;
        case DIR_NENHUMA:
            op = buscar_operacao(l->operacao);
            if (!op) return erro_montagem(m, "instrucao desconhecida");
            if (l->operacao[0] == '+' && op->formato != 3)
                return erro_montagem(m, "formato 4 nao permitido para %s", op->nome);
            l->tamanho = l->operacao[0] == '+' ? 4u : op->formato;
            l->instrucao = 1;
            break;
        default: return erro_montagem(m, "diretiva inesperada");
        }
        if (l->tamanho) {
            if (!reservar(m, lc, l->tamanho)) return 0;
            lc += l->tamanho;
        }
    }
    if (ferror(fonte)) return erro_montagem(m, "falha ao ler arquivo fonte");
    if (!terminou) return erro_montagem(m, "diretiva END ausente");
    return 1;
}

static void sem_espacos(char *destino, const char *origem) {
    while (*origem) {
        if (!isspace((unsigned char)*origem)) *destino++ = *origem;
        ++origem;
    }
    *destino = '\0';
}

static int formato2(Montagem *m, const Linha *l, const Operacao *op, uint8_t *b) {
    char args[ASM_OPERANDO], *segundo;
    int r1, r2 = 0, rel;
    int32_t v;
    strcpy(args, l->operando);
    segundo = strchr(args, ',');
    if (segundo) *segundo++ = '\0';
    /* Espacos ao redor da virgula sao permitidos, dentro do nome nao. */
    {
        size_t n = strlen(args);
        while (n && isspace((unsigned char)args[n-1])) args[--n] = '\0';
        if (segundo) while (isspace((unsigned char)*segundo)) ++segundo;
    }
    r1 = numero_registrador(args);
    if (r1 < 0) return erro_montagem(m, "registrador invalido: %s", args);
    if (op->tipo == OPER_REG) {
        if (segundo) return erro_montagem(m, "%s exige apenas um registrador", op->nome);
    } else {
        if (!segundo || !*segundo || strchr(segundo, ','))
            return erro_montagem(m, "%s exige dois operandos", op->nome);
        if (op->tipo == OPER_SHIFT) {
            if (!avaliar(m, segundo, l->endereco, 10, &v, &rel)) return 0;
            if (rel || v < 1 || v > 16) return erro_montagem(m, "deslocamento deve estar entre 1 e 16");
            r2 = (int)v - 1;
        } else {
            r2 = numero_registrador(segundo);
            if (r2 < 0) return erro_montagem(m, "registrador invalido: %s", segundo);
        }
    }
    b[0] = (uint8_t)op->opcode; b[1] = (uint8_t)((r1 << 4) | r2);
    return 1;
}

static int formato34(Montagem *m, const Linha *l, const Operacao *op,
                     int base_ativa, int32_t base, uint8_t *b) {
    char args[ASM_OPERANDO], *p, *virgula;
    int32_t alvo, deslocamento;
    int relativo, ni = 3;
    unsigned flags = l->tamanho == 4 ? 1u : 0u, campo;
    /* Nao remover espacos internos: '1 2' nao pode virar o numero 12. */
    strcpy(args, l->operando); p = args;
    if (op->tipo == OPER_ZERO) {
        if (*p) return erro_montagem(m, "RSUB nao recebe operando");
        b[0] = 0x4F; b[1] = (uint8_t)(flags << 4); b[2] = 0;
        if (l->tamanho == 4) b[3] = 0;
        return 1;
    }
    if (*p == '#') { ni = 1; ++p; }
    else if (*p == '@') { ni = 2; ++p; }
    virgula = strchr(p, ',');
    if (virgula) {
        char indice[ASM_OPERANDO];
        *virgula++ = '\0'; sem_espacos(indice, virgula);
        if (ni != 3 || strcmp(indice, "X")) return erro_montagem(m, "indexacao exige operando simples seguido de ,X");
        flags |= 8;
    }
    if (!avaliar(m, p, l->endereco, 10, &alvo, &relativo)) return 0;
    if (alvo < 0 || alvo > 0xFFFFF) return erro_montagem(m, "operando fora de 0..0xFFFFF; use WORD para negativos");
    if (l->tamanho == 4) campo = (unsigned)alvo;
    else if (ni == 1 && !relativo) {
        if (alvo > 4095) return erro_montagem(m, "constante imediata exige formato 4 (+%s)", op->nome);
        campo = (unsigned)alvo;
    } else {
        deslocamento = alvo - (int32_t)(l->endereco + 3);
        if (relativo && deslocamento >= -2048 && deslocamento <= 2047) {
            flags |= 2; campo = (unsigned)deslocamento & 0xFFF;
        } else if (base_ativa && alvo >= base && alvo - base <= 4095) {
            flags |= 4; campo = (unsigned)(alvo - base);
        } else if (alvo <= 4095) campo = (unsigned)alvo;
        else return erro_montagem(m, "endereco nao alcancavel: use BASE ou +%s", op->nome);
    }
    b[0] = (uint8_t)(op->opcode | (unsigned)ni);
    if (l->tamanho == 4) {
        b[1] = (uint8_t)((flags << 4) | (campo >> 16));
        b[2] = (uint8_t)(campo >> 8); b[3] = (uint8_t)campo;
    } else {
        b[1] = (uint8_t)((flags << 4) | (campo >> 8)); b[2] = (uint8_t)campo;
    }
    return 1;
}

int segundo_passo(Montagem *m) {
    unsigned i, j;
    int base_ativa = 0, rel = 0;
    int32_t base = 0, v;
    m->entrada = m->inicio;
    for (i = 0; i < m->nlinhas; ++i) {
        Linha *l = &m->linhas[i];
        Diretiva d = buscar_diretiva(l->operacao);
        uint8_t b[ASM_OPERANDO] = {0};
        unsigned n = 0;
        m->linha_atual = l->numero;
        if (!*l->operacao) continue;
        switch (d) {
        case DIR_BASE:
            if (!avaliar(m, l->operando, l->endereco, 10, &base, &rel)) return 0;
            if (base < 0 || base > 0xFFFFF) return erro_montagem(m, "BASE fora do intervalo");
            base_ativa = 1; break;
        case DIR_NOBASE: base_ativa = 0; break;
        case DIR_END:
            if (*l->operando) {
                if (!avaliar(m, l->operando, l->endereco, 10, &v, &rel)) return 0;
                if (v < 0 || v >= (int32_t)MEM_TAMANHO) return erro_montagem(m, "entrada END invalida");
                m->entrada = (unsigned)v;
            }
            break;
        case DIR_BYTE:
            if (!bytes_constante(m, l->operando, b, &n)) return 0;
            break;
        case DIR_WORD:
            if (!avaliar(m, l->operando, l->endereco, 10, &v, &rel)) return 0;
            if (v < -8388608 || v > 16777215) return erro_montagem(m, "WORD fora de 24 bits");
            b[0] = (uint8_t)((uint32_t)v >> 16); b[1] = (uint8_t)((uint32_t)v >> 8);
            b[2] = (uint8_t)v; n = 3; break;
        case DIR_NENHUMA: {
            const Operacao *op = buscar_operacao(l->operacao);
            if (!op) return erro_montagem(m, "instrucao desconhecida");
            if (op->formato == 2) {
                if (!formato2(m, l, op, b)) return 0;
            } else if (!formato34(m, l, op, base_ativa, base, b)) return 0;
            n = l->tamanho;
            break;
        }
        default: break;
        }
        for (j = 0; j < n; ++j) {
            m->imagem[l->endereco + j] = b[j]; m->emitido[l->endereco + j] = 1;
        }
        m->bytes += n;
    }
    if (!m->bytes) return erro_montagem(m, "programa nao gera codigo nem dados");
    /* Nao aceitar END apontando para dados ou para o meio de uma instrucao. */
    for (i = 0; i < m->nlinhas; ++i)
        if (m->linhas[i].instrucao && m->linhas[i].endereco == m->entrada) return 1;
    return erro_montagem(m, "END deve apontar para o inicio de uma instrucao");
}

int montar(FILE *fonte, Montagem *m) {
    if (!fonte || !m) return 0;
    memset(m, 0, sizeof *m);
    strcpy(m->nome, "PROG");
    return primeiro_passo(fonte, m) && segundo_passo(m);
}
