/*
 * memview.c - painel grafico da memoria (versao inicial, pode ser melhorada na parte 6).
 *
 * Mostra a memoria como uma grade de bytes em hexadecimal:
 *   - celula amarela: byte que mudou no ultimo passo
 *   - contorno azul : byte apontado pelo PC
 *   - bytes zerados aparecem em cinza
 */
#include "memview.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINHA_H        22
#define CABECALHO_H    24
#define COL_ENDERECO_W 80
#define CEL_W          34
#define FONTE          18

static uint8_t *copia = NULL; /* memoria antes do ultimo passo */
static size_t copia_tam = 0;
static int tem_copia = 0;

void memview_antes_do_passo(const Maquina *m) {
    if (copia_tam != m->mem_tam) {
        tem_copia = 0;
        free(copia);
        copia = malloc(m->mem_tam);
        copia_tam = copia ? m->mem_tam : 0;
    }
    if (copia) {
        memcpy(copia, m->mem, m->mem_tam);
        tem_copia = 1;
    }
}

void memview_esquecer_mudancas(void) {
    tem_copia = 0;
}

void memview_liberar(void) {
    free(copia);
    copia = NULL;
    copia_tam = 0;
    tem_copia = 0;
}

int memview_linhas_visiveis(Rectangle area) {
    int n = (int)((area.height - CABECALHO_H) / LINHA_H);
    return n > 0 ? n : 0;
}

void memview_desenhar(const Maquina *m, Rectangle area, uint32_t inicio) {
    const Color fundo = {24, 26, 33, 255};
    const Color cinza = {110, 115, 130, 255};
    const Color amarelo = {255, 235, 130, 255};
    const Color azul = {80, 160, 255, 255};

    DrawRectangleRec(area, fundo);
    DrawRectangleLinesEx(area, 1, GRAY);

    int x0 = (int)area.x + 8;
    int y0 = (int)area.y + 4;
    DrawText("ENDERECO", x0, y0, 14, LIGHTGRAY);
    for (int c = 0; c < MEMVIEW_COLUNAS; c++) {
        char cab[4];
        snprintf(cab, sizeof cab, "%02X", c);
        DrawText(cab, x0 + COL_ENDERECO_W + c * CEL_W + 6, y0, 14, LIGHTGRAY);
    }

    int linhas = memview_linhas_visiveis(area);
    for (int l = 0; l < linhas; l++) {
        uint32_t base = inicio + (uint32_t)l * MEMVIEW_COLUNAS;
        if (base >= m->mem_tam) break;

        int y = (int)area.y + CABECALHO_H + l * LINHA_H;
        char end[12];
        snprintf(end, sizeof end, "%06X", (unsigned)base);
        DrawText(end, x0, y + 2, FONTE, SKYBLUE);

        for (int c = 0; c < MEMVIEW_COLUNAS; c++) {
            uint32_t addr = base + (uint32_t)c;
            if (addr >= m->mem_tam) break;

            int x = x0 + COL_ENDERECO_W + c * CEL_W;
            uint8_t v = m->mem[addr];
            int mudou = tem_copia && copia && copia[addr] != v;

            if (mudou) DrawRectangle(x, y, CEL_W - 2, LINHA_H - 2, amarelo);
            if (addr == (m->PC & MASK24)) DrawRectangleLines(x, y, CEL_W - 2, LINHA_H - 2, azul);

            char byte[4];
            snprintf(byte, sizeof byte, "%02X", v);
            Color cor = mudou ? BLACK : (v == 0 ? cinza : WHITE);
            DrawText(byte, x + 6, y + 2, FONTE, cor);
        }
    }
}
