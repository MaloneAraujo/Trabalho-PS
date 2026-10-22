#include "montador.h"

/* Objeto absoluto H/T/E. Reservas nao geram T; maximo de 30 bytes por T.
 * O carregador existente zera a memoria antes de copiar os registros T. */
int escrever_objeto(FILE *saida, const Montagem *m) {
    unsigned p = m->inicio, i, n;
    if (!saida || *m->erro) return 0;
    fprintf(saida, "H%-6s%06X%06X\n", m->nome, m->inicio, m->fim - m->inicio);
    while (p < m->fim) {
        if (!m->emitido[p]) { ++p; continue; }
        for (n = 0; n < 30 && p + n < m->fim && m->emitido[p+n]; ++n) {}
        fprintf(saida, "T%06X%02X", p, n);
        for (i = 0; i < n; ++i) fprintf(saida, "%02X", m->imagem[p+i]);
        fputc('\n', saida); p += n;
    }
    fprintf(saida, "E%06X\n", m->entrada);
    return !ferror(saida);
}

int escrever_listagem(FILE *saida, const Montagem *m) {
    unsigned i, j;
    if (!saida || *m->erro) return 0;
    fprintf(saida, "LINHA ENDERECO CODIGO / FONTE\n");
    for (i = 0; i < m->nlinhas; ++i) {
        const Linha *l = &m->linhas[i];
        fprintf(saida, "%5u %06X  ", l->numero, l->endereco);
        for (j = 0; j < l->tamanho && m->emitido[l->endereco+j]; ++j)
            fprintf(saida, "%02X", m->imagem[l->endereco+j]);
        fprintf(saida, "  %s\n", l->fonte);
    }
    fprintf(saida, "\nTABELA DE SIMBOLOS (A=absoluto, R=endereco)\n");
    for (i = 0; i < m->nsimbolos; ++i)
        fprintf(saida, "%-31s %08X %c\n", m->simbolos[i].nome,
                (unsigned)m->simbolos[i].valor, m->simbolos[i].relativo ? 'R' : 'A');
    return !ferror(saida);
}
