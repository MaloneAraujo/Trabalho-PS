// Bibliotecas
#include "raylib.h"
#include <stdio.h>

// Dimensão total da memória simulada
#define MEMORY_SIZE 1024
#define BYTES_PER_ROW 16

int main(void) {
    // Inicialização da janela
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "SIC/XE - Visualização de Memória (Pessoa 6)");
    // Taxa de atualização
    SetTargetFPS(60);

    // Estruturas simuladas (Deverão vir do Grupo de Execução)
    unsigned char memory[MEMORY_SIZE] = {0};
    
    // Dados fictícios para visualização
    memory[0] = 0x14; memory[1] = 0x10; memory[2] = 0x33; // Instrução fictícia 1
    memory[3] = 0x48; memory[4] = 0x20; memory[5] = 0x00; // Instrução fictícia 2
    
    int pc = 0; // Program Counter inicial

    while (!WindowShouldClose()) {
        // Simulação de execução para teste da interface gráfica
        // Avança o PC em 3 bytes (1 palavra) ao pressionar a seta direita
        if (IsKeyPressed(KEY_RIGHT)) pc = (pc + 3) % MEMORY_SIZE;
        if (IsKeyPressed(KEY_LEFT)) pc = (pc - 3 + MEMORY_SIZE) % MEMORY_SIZE;

        // Construção visual inicial
        BeginDrawing();
        //Limpa o fundo
        ClearBackground(RAYWHITE);    

        // Cabeçalho da Interface
        DrawText("Monitoramento de Memória SIC/XE", 20, 20, 20, DARKGRAY);
        
        char pcStr[32];
        sprintf(pcStr, "PC Atual: 0x%04X", pc);
        DrawText(pcStr, 20, 50, 20, MAROON);

        // Renderização da Grade de Memória (Mostrando apenas os primeiros 256 bytes por espaço visual)
        int startY = 100;
        int startX = 20;
        
        for (int i = 0; i < 256; i += BYTES_PER_ROW) {
            // Desenha a coluna de endereços (ex: "0x0000:")
            char addrStr[16];
            sprintf(addrStr, "0x%04X:", i);
            DrawText(addrStr, startX, startY + (i / BYTES_PER_ROW) * 20, 20, DARKBLUE);
            
            // Desenha os bytes da linha atual
            for (int j = 0; j < BYTES_PER_ROW; j++) {
                int memIdx = i + j;
                char byteStr[4];
                sprintf(byteStr, "%02X", memory[memIdx]);
                
                Color textColor = BLACK;
                
                // verifica se a posição atual (memIdx) se encontra no intervalo exato da instrução apontada
                if (memIdx >= pc && memIdx < pc + 3) {
                    // Desenha um fundo de destaque atrás do byte
                    DrawRectangle(startX + 80 + j * 30 - 2, startY + (i / BYTES_PER_ROW) * 20 - 2, 26, 22, LIGHTGRAY);
                    textColor = RED; // Muda a cor da fonte do byte atual
                }
                
                // Renderiza o byte hexadecimal
                DrawText(byteStr, startX + 80 + j * 30, startY + (i / BYTES_PER_ROW) * 20, 20, textColor);
            }
        }
        
        // Rodapé de instruções
        DrawText("Use as setas ESQUERDA/DIREITA para simular a mudança do PC", 20, 550, 16, GRAY);

        // Processo visual encerra e atualiza a interface mostrada
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
