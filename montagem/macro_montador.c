#include "macros.h"
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define EXPANDIDO "MASMAPRG.ASM"
#define TEMPORARIO "MASMAPRG.ASM.tmp"

static int falhar(Montagem *m, char *erro, size_t tamanho, const char *mensagem) {
    if (m) snprintf(m->erro, sizeof m->erro, "%s", mensagem);
    if (erro && tamanho) snprintf(erro, tamanho, "%s", mensagem);
    return 0;
}

static int publicar(void) {
#ifdef _WIN32
    /* rename() no Windows nao substitui um destino existente.
     * Nao removemos a versao anterior antes de a nova estar pronta. */
    return MoveFileExA(TEMPORARIO, EXPANDIDO, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(TEMPORARIO, EXPANDIDO) == 0;
#endif
}

int montar_com_macros(const char *arquivoEntrada, Montagem *m,
                      char *erro, size_t tamanhoErro) {
    FILE *entrada, *saida;
    char detalhe[256] = "";
    int ok, falhaEntrada, falhaSaida;
    if (erro && tamanhoErro) erro[0] = '\0';
    if (!m || !arquivoEntrada || !arquivoEntrada[0])
        return falhar(m, erro, tamanhoErro, "Argumentos invalidos para montar_com_macros.");
    m->erro[0] = '\0';
    entrada = fopen(arquivoEntrada, "r");
    if (!entrada)
        return falhar(m, erro, tamanhoErro, "Nao foi possivel abrir o Assembly de entrada.");
    /* Exclusivo: nunca truncar um arquivo temporario preexistente nem a entrada. */
    saida = fopen(TEMPORARIO, "wx");
    if (!saida) {
        fclose(entrada);
        return falhar(m, erro, tamanhoErro,
                      "Nao foi possivel criar MASMAPRG.ASM.tmp (ja existe ou diretorio sem acesso).");
    }
    ok = processar_macros(entrada, saida, detalhe, sizeof detalhe);
    detalhe[sizeof detalhe - 1] = '\0';
    falhaEntrada = ferror(entrada);
    falhaSaida = ferror(saida);
    if (fclose(entrada)) falhaEntrada = 1;
    if (fclose(saida)) falhaSaida = 1;
    if (!ok || falhaEntrada || falhaSaida) {
        remove(TEMPORARIO);
        if (!ok && detalhe[0]) return falhar(m, erro, tamanhoErro, detalhe);
        return falhar(m, erro, tamanhoErro, !ok ? "Falha no processamento de macros."
                      : "Falha de leitura ou escrita durante o processamento de macros.");
    }
    /* A entrada ja foi fechada: ela pode ser o proprio MASMAPRG.ASM. */
    if (!publicar()) {
        remove(TEMPORARIO);
        return falhar(m, erro, tamanhoErro, "Nao foi possivel publicar MASMAPRG.ASM.");
    }
    entrada = fopen(EXPANDIDO, "r");
    if (!entrada)
        return falhar(m, erro, tamanhoErro, "Nao foi possivel reabrir MASMAPRG.ASM.");
    ok = montar(entrada, m);
    /* Copiar antes de falhar(), pois a mensagem original pertence a m. */
    snprintf(detalhe, sizeof detalhe, "%s", m->erro);
    falhaEntrada = ferror(entrada);
    if (fclose(entrada)) falhaEntrada = 1;
    if (!ok)
        return falhar(m, erro, tamanhoErro,
                      detalhe[0] ? detalhe : "Falha na montagem do Assembly expandido.");
    if (falhaEntrada)
        return falhar(m, erro, tamanhoErro, "Falha de leitura de MASMAPRG.ASM.");
    return 1;
}
