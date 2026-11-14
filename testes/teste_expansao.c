/* Teste isolado da parte 4. Dubles minimos, NAO implementacoes dos colegas. */
#include "montagem/macros_interno.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static unsigned verificacoes;
#define CHECK(x) do { ++verificacoes; if (!(x)) { fprintf(stderr,"FALHOU linha %d: %s\n",__LINE__,#x); exit(1); } } while (0)
int mc_erro(McContexto *c, const char *f, ...) {
    va_list a; va_start(a,f); vsnprintf(c->erro,c->tamanho_erro,f,a); va_end(a); return 0;
}
int mc_igual(const char *a, const char *b) {
    while (*a && *b && toupper((unsigned char)*a)==toupper((unsigned char)*b)) { ++a; ++b; }
    return !*a && !*b;
}
int mc_parse(McContexto *c, const char *s, McLinha *l) {
    (void)c; memset(l,0,sizeof *l);
    if (strstr(s,"MCDEFN")) strcpy(l->operacao,"MCDEFN");
    else if (strstr(s,"MCEND")) strcpy(l->operacao,"MCEND");
    return 1;
}
int main(void) {
    McContexto *c = calloc(1,sizeof *c);
    McMacro soma = {0}, dupla = {0}, vazia = {0}, interna = {0};
    char *corpo_soma[] = {"LDA #(1,0)","ADD #(1,1)","STA #(1,0)"};
    char *corpo_dupla[] = {"SOMA #(1,0), #(1,1)","SOMA #(1,0), #(1,1)","L.SER J L.SER"};
    char *corpo_interno[] = {"I MCDEFN X", "L.SER LDA #(2,0)", "MCEND", "F.SER J F.SER"};
    char args[MC_PARAMS][MC_TEXTO] = {{0}}, linha[MC_TEXTO], erro[256];
    CHECK(c != NULL); c->erro=erro; c->tamanho_erro=sizeof erro;
    strcpy(soma.nome,"SOMA"); soma.nparametros=2; soma.corpo=corpo_soma; soma.nlinhas=3;
    strcpy(dupla.nome,"DUPLA"); dupla.nparametros=2; dupla.corpo=corpo_dupla; dupla.nlinhas=3;
    strcpy(args[0],"TOTAL"); strcpy(args[1],"#1");
    CHECK(mc_empilhar(c,&dupla,(const char (*)[MC_TEXTO])args,2));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"SOMA TOTAL, #1"));
    CHECK(mc_empilhar(c,&soma,(const char (*)[MC_TEXTO])args,2));
    CHECK(!mc_empilhar(c,&dupla,(const char (*)[MC_TEXTO])args,2));
    CHECK(strstr(erro,"recursao") != NULL);
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"LDA TOTAL"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"ADD #1"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"STA TOTAL"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"SOMA TOTAL, #1"));
    CHECK(mc_empilhar(c,&soma,(const char (*)[MC_TEXTO])args,2));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"LDA TOTAL"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"ADD #1"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"STA TOTAL"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"L1 J L1"));
    CHECK(mc_proxima_expansao(c,linha)==0 && c->topo==0);
    CHECK(!mc_empilhar(c,&soma,(const char (*)[MC_TEXTO])args,1));
    strcpy(vazia.nome,"VAZIA");
    CHECK(mc_empilhar(c,&vazia,(const char (*)[MC_TEXTO])args,0));
    CHECK(mc_proxima_expansao(c,linha)==0);
    strcpy(interna.nome,"EXTERNA"); interna.corpo=corpo_interno; interna.nlinhas=4;
    CHECK(mc_empilhar(c,&interna,(const char (*)[MC_TEXTO])args,0));
    CHECK(mc_proxima_expansao(c,linha)==1 && strstr(linha,"MCDEFN"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"L.SER LDA #(1,0)"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"MCEND"));
    CHECK(mc_proxima_expansao(c,linha)==1 && !strcmp(linha,"F5 J F5"));
    CHECK(mc_proxima_expansao(c,linha)==0);
    c->topo=MC_NIVEIS;
    CHECK(!mc_empilhar(c,&vazia,(const char (*)[MC_TEXTO])args,0));
    c->topo=0; c->serial=MC_LINHAS;
    CHECK(!mc_empilhar(c,&vazia,(const char (*)[MC_TEXTO])args,0));
    free(c);
    printf("Parte 4 isolada: %u verificacoes passaram.\n",verificacoes);
    return 0;
}
