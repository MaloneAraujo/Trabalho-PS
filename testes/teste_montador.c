#include "montagem/montador.h"
#include "sicxe.h"
#include <stdlib.h>
#include <string.h>

static unsigned passou, falhou;
static void verificar(int ok, const char *nome) {
    if (ok) ++passou;
    else { ++falhou; printf("FALHOU: %s\n", nome); }
}
static int texto(Montagem *m, const char *s) {
    FILE *f = tmpfile();
    int ok;
    if (!f) { verificar(0, "tmpfile"); return 0; }
    fputs(s, f); rewind(f); ok = montar(f, m); fclose(f); return ok;
}
static void invalido(Montagem *m, const char *s, const char *motivo) {
    verificar(!texto(m, s) && strstr(m->erro, motivo), motivo);
}
static void bytes(Montagem *m, const char *linha, const char *hex) {
    char fonte[1024];
    unsigned i, v;
    int ok;
    snprintf(fonte, sizeof fonte, "P START 0\nINICIO %s\n END INICIO\n", linha);
    ok = texto(m, fonte);
    if (ok) {
        ok = m->bytes == strlen(hex) / 2;
        for (i = 0; i < strlen(hex) / 2; ++i) {
            sscanf(hex + i * 2, "%2x", &v);
            if (m->imagem[i] != v) ok = 0;
        }
    }
    verificar(ok, linha);
}

static void integracao(Montagem *m, const char *arquivo, uint32_t a, uint32_t b,
                       uint32_t x, uint32_t s, unsigned endereco, uint8_t byte) {
    const char *tmp = "testes/temporario-montador.obj";
    FILE *f = fopen(arquivo, "r");
    Maquina *maq;
    char erro[256];
    int r = SIM_OK, ok;
    unsigned passos = 0;
    if (!f) { verificar(0, arquivo); return; }
    ok = montar(f, m); fclose(f); verificar(ok, arquivo);
    if (!ok) { puts(m->erro); return; }
    f = fopen(tmp, "wx");
    if (!f) { verificar(0, "arquivo temporario exclusivo"); return; }
    ok = escrever_objeto(f, m);
    if (fclose(f)) ok = 0;
    verificar(ok, "escrita objeto");
    maq = sim_criar();
    if (!maq) { verificar(0, "sim_criar"); remove(tmp); return; }
    ok = !sim_carregar(maq, tmp, erro, sizeof erro);
    verificar(ok, "carregar objeto montado");
    if (ok) {
        while (r == SIM_OK && passos++ < 100) r = sim_passo(maq, erro, sizeof erro);
        verificar(r == SIM_FIM, "programa montado termina");
        verificar(maq->A == a && maq->B == b && maq->X == x && maq->S == s,
                  "registradores apos executar assembly");
        verificar(maq->mem[endereco] == byte, "memoria apos executar assembly");
    }
    sim_destruir(maq); remove(tmp);
}

int main(void) {
    Montagem *m = calloc(1, sizeof *m);
    unsigned i;
    static const struct { const char *asm_, *hex; } casos[] = {
        {"ADD #1","190001"},{"ADDR A,X","9001"},{"AND #1","410001"},
        {"CLEAR X","B410"},{"COMP #1","290001"},{"COMPR A,S","A004"},
        {"DIV #1","250001"},{"DIVR S,T","9C45"},{"J 10","3F000A"},
        {"JEQ 10","33000A"},{"JGT 10","37000A"},{"JLT 10","3B000A"},
        {"JSUB 10","4B000A"},{"LDA #1","010001"},{"LDB #1","690001"},
        {"LDCH #1","510001"},{"LDL #1","090001"},{"LDS #1","6D0001"},
        {"LDT #1","750001"},{"LDX #1","050001"},{"MUL #1","210001"},
        {"MULR T,A","9850"},{"OR #1","450001"},{"RMO A,L","AC02"},
        {"RSUB","4F0000"},{"SHIFTL A,1","A400"},{"SHIFTR S,16","A84F"},
        {"STA 10","0F000A"},{"STB 10","7B000A"},{"STCH 10","57000A"},
        {"STL 10","17000A"},{"STS 10","7F000A"},{"STT 10","87000A"},
        {"STX 10","13000A"},{"SUB #1","1D0001"},{"SUBR A,S","9404"},
        {"TIX 10","2F000A"},{"TIXR X","B810"},
        {"+LDA #65535","0110FFFF"},{"+JSUB 4095","4B100FFF"},
        {"+RSUB","4F100000"},{"LDA @10","02000A"},
        {"LDA 10,X","03800A"},{"J INICIO","3F2FFD"}
    };
    if (!m) return 1;
    for (i = 0; i < sizeof casos / sizeof casos[0]; ++i) bytes(m, casos[i].asm_, casos[i].hex);
    verificar(texto(m, "P START 100\nI LDA V\n J I\nV WORD -1\n END I\n") &&
        m->inicio == 0x100 && m->imagem[0x101] == 0x20 && m->imagem[0x102] == 3 &&
        m->imagem[0x106] == 255 && m->imagem[0x108] == 255, "START hex, referencia futura e WORD negativo");
    verificar(texto(m, "I LDA #K\n J I\nK EQU 3+2\n END I\n") && m->imagem[1] == 0 &&
        m->imagem[2] == 5, "EQU absoluto em imediato nao usa PC");
    verificar(texto(m, "I LDA #V\nV RSUB\n END I\n") && m->imagem[1] == 0x20 &&
        m->imagem[2] == 0, "endereco imediato simbolico usa PC");
    verificar(texto(m, "i: lda #1 ; teste\n fim j fim\n msg byte C'a; b'\n end i\n") &&
        m->imagem[6] == 'a' && m->imagem[7] == ';' && m->imagem[8] == ' ', "case, rotulo com dois pontos e comentario");
    verificar(texto(m, ". comentario com ' solta\nI RSUB\nB BYTE X'00ffA2'\n END I\n") &&
        m->imagem[4] == 255 && m->imagem[5] == 0xA2, "BYTE hexadecimal e comentario ponto");
    verificar(texto(m, "I RSUB\nA RESB 2\nB RESW 2\nK EQU *-A\n END I\n") &&
        m->fim == 11 && m->bytes == 3 && m->simbolos[3].valor == 8, "reservas e EQU com LC");
    verificar(texto(m, "I LDA V\n ORG 2050\nV WORD 0\n END I\n") &&
        m->imagem[1] == 0x27 && m->imagem[2] == 0xFF, "PC relativo limite +2047");
    verificar(texto(m, "I RSUB\n ORG 2045\nF J I\n END F\n") &&
        m->imagem[2046] == 0x28 && m->imagem[2047] == 0, "PC relativo limite -2048");
    verificar(texto(m, "I LDA V\n ORG 2051\nV WORD 0\n END I\n") &&
        m->imagem[1] == 8 && m->imagem[2] == 3, "fora do PC usa absoluto de 12 bits");
    verificar(texto(m, " BASE V\nI LDA V\n NOBASE\n LDA V\n ORG 3000\nV WORD 42\n END I\n") &&
        m->imagem[1] == 0x40 && m->imagem[2] == 0 && m->imagem[4] == 0x0B,
        "BASE e NOBASE");
    verificar(texto(m, "P START FFD\nI RSUB\n END I\n") && m->inicio == 4093, "START hexadecimal sem prefixo");
    verificar(texto(m, "P START 0xFFD\nI RSUB\n END I\n") && m->fim == MEM_TAMANHO,
        "limite superior da memoria");
    invalido(m,"I LDA FALTA\n END I\n","simbolo indefinido");
    invalido(m,"I RSUB\nI RSUB\n END I\n","duplicado");
    invalido(m,"I FOO 1\n END I\n","desconhecida");
    invalido(m,"I +CLEAR X\n END I\n","formato 4");
    invalido(m,"I CLEAR F\n END I\n","registrador invalido");
    invalido(m,"I ADDR A\n END I\n","dois operandos");
    invalido(m,"I CLEAR A,X\n END I\n","apenas um");
    invalido(m,"I SHIFTL A,0\n END I\n","entre 1 e 16");
    invalido(m,"I SHIFTR A,17\n END I\n","entre 1 e 16");
    invalido(m,"I LDA #1,X\n END I\n","indexacao");
    invalido(m,"I LDA @1,X\n END I\n","indexacao");
    invalido(m,"I LDA #4096\n END I\n","formato 4");
    invalido(m,"I +LDA #1048576\n END I\n","0xFFFFF");
    invalido(m,"I LDA #1 2\n END I\n","expressao invalida");
    invalido(m,"I SHIFTL A,1 6\n END I\n","expressao invalida");
    invalido(m,"I CLEAR S W\n END I\n","registrador invalido");
    invalido(m,"I RSUB X\n END I\n","nao recebe");
    invalido(m,"I RSUB\nD BYTE X'F'\n END I\n","pares");
    invalido(m,"I RSUB\nD BYTE X'GG'\n END I\n","digito");
    invalido(m,"I RSUB\nD BYTE C'abc\n END I\n","aspas");
    invalido(m,"I RSUB\nD WORD 16777216\n END I\n","24 bits");
    invalido(m,"I RSUB\nD RESB -1\n END I\n","reserva");
    invalido(m,"I RSUB\nD RESW 999999\n END I\n","reserva");
    invalido(m,"I RSUB\n ORG 0\nD WORD 1\n END I\n","sobreposicao");
    invalido(m,"I RSUB\n ORG 4095\nD WORD 1\n END I\n","fora da memoria");
    invalido(m,"I RSUB\n ORG 4096\n END I\n","ORG fora");
    invalido(m,"K EQU LATER\nI RSUB\nLATER EQU 1\n END I\n","indefinido");
    invalido(m,"I RSUB\n END I+1\n","inicio de uma instrucao");
    invalido(m,"I RSUB\n END I\n RSUB\n","depois de END");
    invalido(m,"I RSUB\n","END ausente");
    invalido(m," END\n","nao gera");
    invalido(m,"P START 0\n START 0\nI RSUB\n END I\n","primeira operacao");
    invalido(m,"I RSUB\nK EQU 2147483647+1\n END I\n","overflow");
    invalido(m,"I RSUB\nK EQU I+I\n END I\n","relativa invalida");
    invalido(m,"I RSUB\n NOBASE 1\n END I\n","NOBASE");
    invalido(m,"I RSUB\n BASE\n END I\n","BASE exige");
    /* Verifica segmentacao do objeto, reservas e listagem com simbolos. */
    if (texto(m,"P START 0\nI RSUB\nB BYTE C'abcdefghijklmnopqrstuvwxyz0123456789'\nR RESB 2\nD WORD 5\n END I\n")) {
        FILE *f = tmpfile(); char buf[2048]; size_t n;
        if (f) {
            verificar(escrever_objeto(f,m), "escritor HTE"); rewind(f);
            n = fread(buf,1,sizeof buf-1,f); buf[n] = 0; fclose(f);
            verificar(strstr(buf,"HP     00000000002C\n") && strstr(buf,"T0000001E") &&
                      strstr(buf,"T00001E09") && strstr(buf,"T00002903") && strstr(buf,"E000000"),
                      "HTE: 30 bytes, lacuna e comprimento");
        } else verificar(0,"tmpfile objeto");
        f = tmpfile();
        if (f) {
            verificar(escrever_listagem(f,m), "listagem gerada"); rewind(f);
            n = fread(buf,1,sizeof buf-1,f); buf[n] = 0; fclose(f);
            verificar(strstr(buf,"TABELA DE SIMBOLOS") && strstr(buf,"4F0000"), "listagem contem codigo e simbolos");
        } else verificar(0,"tmpfile listagem");
    } else verificar(0,"montagem para registros");
    integracao(m,"exemplos/soma.asm",8,7,0,0,32,8);
    integracao(m,"exemplos/enderecamento.asm",0xFF42,0,1,0x4200FF,0x11E,0xFF);
    integracao(m,"exemplos/base_org.asm",42,3000,0,0,3002,42);
    printf("Montador: %u verificacoes passaram; %u falharam.\n", passou, falhou);
    free(m); return falhou ? 1 : 0;
}
