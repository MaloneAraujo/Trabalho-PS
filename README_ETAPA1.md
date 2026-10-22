# Simulador SIC/XE - primeira etapa em C

Esta etapa executa o nucleo pelo terminal: memoria, registradores, decodificacao, aritmetica, movimentacao e desvios. Nao inclui interface grafica e nao depende de Raylib. Os requisitos da segunda etapa ainda serao definidos.

## Rodar no Windows

O pacote inclui `simulador.exe`, compilado para Windows x64. Abra um terminal nesta pasta e execute:

```powershell
.\simulador.exe exemplo.obj
```

Para compilar a partir dos fontes, tenha GCC no PATH:

```powershell
.\compilar.ps1 -Testar -Executar
```

Se a politica local bloquear scripts, execute explicitamente:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1 -Testar -Executar
```

Esse comando vale somente para o processo iniciado; nao altera a configuracao permanente do PowerShell.

Para repetir tambem os dois testes de overflow:

```powershell
.\compilar.ps1 -Testar -Executar -VerificarOverflow
```

## GCC e Make

Em um ambiente com GCC e Make:

```sh
make
make teste
make executar
```

No MinGW, o nome do comando pode ser `mingw32-make`. A compilacao foi validada neste Windows; nao foi executada em Linux.

Sem Make ou PowerShell, a linha equivalente e:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -I. main_cli.c memoria.c decodificador.c Aritimetica.c instrucoes_movimentacao_desvios.c sim_real.c -lm -o simulador
```

## O que o exemplo demonstra

O programa `exemplo.obj` usa somente instrucoes do conjunto obrigatorio:

1. LDA carrega 5 e ADD soma 3, deixando A = 8.
2. STA grava a palavra 8 no endereco hexadecimal 40.
3. COMP compara A com 8; JEQ pula uma carga de 99.
4. JSUB salva o retorno em L e entra na subrotina.
5. LDB carrega 7; RSUB retorna.
6. J salta para si mesmo, convencao usada para terminar.

Resultado esperado: nove passos, A=`000008`, B=`000007`, L=`000015`, PC=`000015`, SW=`000002` e palavra em 40=`000008`.

O primeiro argumento seleciona o arquivo. O segundo, opcional, limita os passos:

```powershell
.\simulador.exe exemplo.obj 100
```

O limite padrao e 10.000. O processo retorna 0 ao terminar normalmente, 1 em erro de carga/execucao e 2 para uso invalido ou limite atingido.

## Organizacao do codigo

| Arquivo | Responsabilidade |
| --- | --- |
| `main_cli.c` | Le os argumentos, carrega o programa e executa passos ate terminar |
| `sicxe.h` | Contrato publico: `Maquina` e funcoes `sim_*` |
| `sicxe_config.h` | Capacidade real de memoria e mascaras de registradores |
| `memoria.c/.h` | Memoria de 4096 bytes, banco de registradores e palavras de 24 bits |
| `decodificador.c/.h` | Opcode, formato, flags e endereco efetivo |
| `Aritimetica.c/.h` | Operacoes inteiras, logicas, comparacoes e deslocamentos |
| `instrucoes_movimentacao_desvios.c/.h` | Cargas, armazenamento, CLEAR/RMO, saltos e subrotinas |
| `sim_real.c` | Adaptacao entre os modulos, carregamento e ciclo de execucao |
| `teste_sim_real.c` | Testa o programa de exemplo de ponta a ponta |
| `testes/teste_integracao.c` | Testa movimentacao, desvios, limites, decodificacao e carregamento |
| `testes/teste_instrucoes.c` | Testa aritmetica, 18 modos de enderecamento e as 32 contagens de shift |

## Estruturas e contratos

- `Registrador`: guarda os bits de cada registrador; os identificadores sao os numeros de SIC/XE (A=0, X=1, L=2, B=3, S=4, T=5, F=6, PC=8, SW=9).
- `Instrucao`: resultado do decodificador, incluindo opcode, formato, tamanho, flags n/i/x/b/p/e, registradores, endereco efetivo e proximo PC.
- `Maquina`: visao dos registradores e ponteiro para a memoria global. Esta versao suporta uma maquina por vez; criar outra reinicializa o mesmo estado.
- `SicxeCpu` e `SicxeMemoria`: callbacks usados pela parte de movimentacao e desvios; `sim_real.c` fornece esses acessos.

`sim_criar` prepara o estado; `sim_carregar` valida e carrega; `sim_passo` executa uma instrucao; `sim_reset` zera os registradores e volta a entrada preservando a memoria; `sim_destruir` libera a visao da maquina. A atualizacao de PC ocorre antes da instrucao, para JSUB salvar o retorno correto. Em erro de execucao, PC volta a apontar para a instrucao que falhou.

## Decisoes desta etapa

- Memoria enderecada por bytes, palavras de tres bytes em ordem big-endian, capacidade de 4 KiB. Isso atende ao minimo de 1 KiB e segue os formatos de instrucao e operacoes de byte do enunciado.
- Inteiros limitados a 24 bits, com complemento de dois. Multiplicacoes usam intermediario de 64 bits antes de manter os 24 bits baixos.
- CC ocupa os dois bits inferiores de SW: 1=menor, 2=igual, 3=maior, 0=sem comparacao. Outros bits de SW sao preservados nas comparacoes.
- SHIFTL faz rotacao de 24 bits; SHIFTR preserva o sinal. As contagens aceitas pelo formato 2 vao de 1 a 16.
- Um salto para o proprio PC significa fim. Outros ciclos sao interrompidos pelo limite do terminal.
- A primeira etapa valida as 38 instrucoes obrigatorias. As rotinas adicionais de ponto flutuante herdadas do pacote continuam no codigo, mas usam uma aproximacao e **nao fazem parte da funcionalidade validada**; nao sao usadas no exemplo nem devem ser usadas para demonstrar esta etapa. Nao houve implementacao nova desses extras.
- Nao foram adicionados interface, montador, ligador ou recursos da segunda etapa.

## Formato de entrada

Objeto absoluto textual H/T/E, com valores hexadecimais:

- H: nome com seis caracteres, inicio com seis digitos, tamanho com seis digitos.
- T: endereco com seis digitos, quantidade de bytes com dois digitos e os bytes.
- E: entrada com seis digitos, ou somente E para usar o inicio declarado.

Separadores `^` sao aceitos. Sao exigidos H, algum dado T e E, nessa ordem. Enderecos precisam caber na memoria e na faixa declarada pelo H. Registros M de relocacao e outros registros nao sao suportados. Arquivos vazios, truncados ou invalidos retornam erro; uma carga malsucedida preserva o estado anterior.

Foi mantida a leitura de bytes binarios crus do pacote original: se o primeiro byte nao for H, os bytes sao carregados a partir de zero. Essa deteccao automatica e simples; um binario cujo primeiro byte seja 0x48 sera interpretado como texto H/T/E. Para esta etapa, prefira o formato textual documentado.

## Correcoes realizadas

Contagens de SHIFTL/SHIFTR; rotacao e sinal dos shifts; intermediarios de MUL/MULR; modo SIC legado; identificacao do formato 4; constante unica de memoria; validacao completa de H/T/E; preservacao do estado em falhas de carga; PC preservado em erro de execucao. O exemplo e os comandos de compilacao foram adaptados para uso sem interface.

O projeto original recebido permanece separado, sem essas alteracoes. Esta pasta e a versao de trabalho da primeira etapa.

## Validacao realizada

Ambiente: Windows x64, GCC 16.2.0, C11. Compilacao sem avisos com `-Wall -Wextra -Wpedantic -O2`, pelo script PowerShell e pelo Makefile.

- 12 verificacoes do exemplo, 57 de integracao e 72 de instrucoes: **141 verificacoes passaram, sem falhas**.
- Dois casos de multiplicacao com `-ftrapv`: MUL e MULR passaram sem encerrar por overflow.
- Oito cenarios do terminal passaram: exemplo, entrada em endereco nao zero, separadores H/T/E, objeto invalido, arquivo inexistente, limite de passos, limite negativo e argumentos excedentes.
- O exemplo terminou em nove passos com A=8 e B=7, conforme esperado.

Esses testes validam os cenarios descritos, sem pretensao de cobrir todas as combinacoes possiveis. A interface grafica e os requisitos futuros nao fazem parte desta entrega.
