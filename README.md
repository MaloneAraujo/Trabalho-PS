# SIC/XE - simulador e montador em C

## Baixar para o grupo

No GitHub, use **Code > Download ZIP** e extraia a pasta inteira.
No Windows x64, abra `simulador-gui.exe` dentro da pasta extraida.
Mantenha a pasta `exemplos` junto ao executavel. Nao e necessario instalar GCC
para usar os executaveis incluidos; GCC e necessario apenas para recompilar.

O [roteiro do video da equipe](docs/roteiro_video_sicxe_equipe.pdf) indica as
falas, os arquivos e as linhas de codigo para a apresentacao.

## Sobre o projeto

Versao integrada com a interface Raylib original do grupo. Abra `simulador-gui.exe`
para usar a janela: ela monta `.asm` em memoria e tambem carrega `.obj`.
Veja [INTERFACE.md](INTERFACE.md) para controles e validacao visual.
O montador absoluto em dois passos e as ferramentas de terminal continuam disponiveis.
As pastas das etapas anteriores foram preservadas; esta e uma copia independente.

## Executar no Windows

Os executaveis Windows x64 estao incluidos no pacote. Abra o PowerShell nesta pasta:

```powershell
.\montador.exe exemplos\soma.asm minha-soma.obj minha-soma.lst
.\simulador.exe minha-soma.obj
```

O primeiro comando traduz o assembly, gera o objeto e uma listagem com enderecos,
bytes e tabela de simbolos. O segundo executa o programa: ao final, `A=000008`,
`B=000007`, `L=000015` e `PC=000015`. O programa termina com `J FIM` apontando
para si mesmo, convencao de parada ja adotada pelo simulador.

O montador NAO sobrescreve arquivos existentes. Em outra execucao, escolha nomes
novos para as saidas. O terceiro argumento (`.lst`) e opcional. Um erro no fonte
nao cria saidas; uma falha ao criar a listagem remove apenas o objeto novo daquela
chamada. Codigos de saida: 0=sucesso, 1=erro de montagem/arquivo, 2=uso incorreto.

Os exemplos tambem incluem objetos e listagens prontos, para comparacao:

```powershell
.\simulador.exe exemplos\soma.obj
.\simulador.exe exemplos\enderecamento.obj
.\simulador.exe exemplos\base_org.obj
```

## Compilar e testar

Requer GCC com C11. No Windows com GCC no PATH:

```powershell
.\compilar.ps1 -Testar -VerificarOverflow
```

Se a politica local bloquear o script, a alternativa abaixo vale somente para
o processo iniciado, sem alterar a configuracao permanente do Windows:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\compilar.ps1 -Testar -VerificarOverflow
```

Com GNU Make:

```text
make
make teste
```

No MinGW, use `mingw32-make`. Os testes da CLI em PowerShell tambem podem ser
executados separadamente: `powershell -NoProfile -ExecutionPolicy Bypass -File testes/teste_cli.ps1`.
O alvo `make teste` executa as baterias em C; o script `compilar.ps1 -Testar`
executa essas baterias e os testes de CLI. `-Executar` roda o exemplo da etapa 1.
Fontes C padrao e Makefile preparados para outros sistemas; Linux nao foi testado.

## Como o montador funciona

O capitulo 10 fornecido pelo grupo, especialmente as secoes 10.4.2 e 10.5 e as
Figuras 10.2/10.3, fundamenta o algoritmo: MOT, POT, tabela de simbolos e contador
de localizacao. Seus exemplos sao para a familia 68000; nao foram copiados como
instrucoes SIC/XE. Aqui `BYTE`/`WORD` inicializam dados e `RESB`/`RESW` reservam
espaco, exercendo os papeis de `DC` e `DS` do texto. Uma WORD SIC/XE tem 3 bytes.

1. **Passo 1:** separa rotulo/operacao/operando, consulta MOT/POT, calcula tamanhos,
   atualiza o contador de localizacao e insere simbolos. Guarda linhas intermediarias
   em memoria, evitando reabrir o fonte. Detecta duplicatas e sobreposicoes.
2. **Passo 2:** consulta os simbolos, resolve referencias futuras, escolhe os bits
   de enderecamento e gera instrucoes/dados. Valida o endereco de entrada.
3. **Saida:** escreve H (nome, inicio e tamanho), T (ate 30 bytes contiguos) e E
   (entrada). Reservas e lacunas quebram registros T; nao emitem bytes.
4. **Execucao:** o carregador existente le H/T/E, zera a memoria e copia os bytes.
   Assim, as areas reservadas com RESB/RESW ficam zeradas nesta integracao.

### Organizacao e funcoes

| Arquivo | Responsabilidade e funcoes principais |
| --- | --- |
| `montagem/main.c` | CLI: abre o fonte, chama `montar`, cria saidas exclusivas e trata erros de escrita. |
| `montagem/montador.h` | Contrato publico: `Montagem`, `Linha`, `Simbolo`, `montar` e os escritores. |
| `montagem/interno.h` | Contratos internos e tipos de instrucoes/diretivas. |
| `montagem/tabelas.c` | `buscar_operacao` consulta a MOT; `buscar_diretiva` consulta a POT; `numero_registrador` codifica registradores. |
| `montagem/parser.c` | `analisar_linha` separa campos/comentarios; `definir_simbolo` insere sem duplicar; `avaliar` resolve expressoes; `bytes_constante` converte BYTE; `erro_montagem` inclui numero da linha. |
| `montagem/passos.c` | `primeiro_passo` calcula enderecos e simbolos; `segundo_passo` gera bytes; `formato2`/`formato34` codificam operandos; `montar` reinicializa o contexto e coordena os dois passos. |
| `montagem/objeto.c` | `escrever_objeto` emite H/T/E; `escrever_listagem` mostra fonte, enderecos, bytes e simbolos. |
| `testes/teste_montador.c` | Testes de codificacao, erros, diretivas e execucao dos objetos no simulador real. |
| `testes/teste_cli.ps1` | Testes dos comandos, codigos de saida e preservacao de arquivos existentes. |

Os modulos do simulador continuam na raiz: `memoria`, `decodificador`, `Aritimetica`,
`instrucoes_movimentacao_desvios`, `sim_real` e `main_cli`. Nesta integracao, `sicxe.h`
e `sim_real.c` ganharam `sim_carregar_imagem`; `programa.c` liga o montador a essa API.
As atribuicoes originais do grupo constam no `LEIA-ME.txt`.
O historico tecnico da etapa 1 esta em `README_ETAPA1.md`.

Para integrar por C, aloque `Montagem` com `calloc` (estrutura grande), passe o
`FILE*` para `montar` e confira o retorno. Em erro, leia `m->erro`; em sucesso,
chame os escritores. Retornos da API: 1=sucesso, 0=erro. Feche os arquivos e libere
o contexto. O montador nao usa estado global mutavel e nao modifica a maquina.

## Linguagem aceita

Linhas: `[ROTULO] OPERACAO [OPERANDO]`. Rotulo pode terminar em `:`. Operacoes e
simbolos ignoram maiusculas/minusculas; conteudo de `C'...'` preserva os bytes.
Indentacao e opcional. Nomes comecam com letra ou `_`, seguidos de letras, digitos
ou `_`. Um nome igual ao de uma operacao deve usar `:` para funcionar como rotulo.
Comentarios: linha iniciada por `.` ou `;`; comentario ao final com `;`, fora de aspas.

38 instrucoes inteiras do nucleo:

```text
ADD ADDR AND CLEAR COMP COMPR DIV DIVR J JEQ JGT JLT JSUB
LDA LDB LDCH LDL LDS LDT LDX MUL MULR OR RMO RSUB SHIFTL SHIFTR
STA STB STCH STL STS STT STX SUB SUBR TIX TIXR
```

- Formato 2: registradores `A X L B S T PC SW`; `F` nao e aceito nesta versao.
  `CLEAR`/`TIXR` recebem um registrador; operacoes de dois registradores usam virgula.
  `SHIFTL`/`SHIFTR` recebem contagens 1..16, codificadas como contagem menos 1.
- Formatos 3 e 4: simples `VALOR`, imediato `#5`, indireto `@PONTEIRO`, indexado
  `TABELA,X`. `+LDA` solicita formato 4; nao ha promocao automatica de tamanho.
  Indexacao nao pode ser combinada com imediato/indireto.
- No formato 3: constante imediata absoluta usa 12 bits; enderecos simbolicos
  tentam PC relativo (-2048..2047), depois BASE (0..4095), depois absoluto (0..4095).
  Constantes imediatas maiores exigem `+`; valores negativos devem estar em WORD.
- `BASE` so informa o montador: o programa precisa carregar o registrador B,
  por exemplo `+LDB #DADOS`. `NOBASE` desativa essa escolha.

| Diretiva | Efeito |
| --- | --- |
| `START endereco` | Opcional, primeira operacao; endereco hexadecimal. Nome do programa tem ate 6 caracteres. Sem START, inicio=0 e nome=PROG. |
| `END entrada` | Obrigatoria; entrada opcional, padrao=inicio. Deve apontar para o inicio de uma instrucao; so comentarios/linhas vazias podem vir depois. |
| `BYTE C'texto'` / `BYTE X'00FF'` | Emite bytes; hexadecimal precisa de pares de digitos. Strings nao vazias, sem escapes/aspas internas; texto UTF-8 ocupa seus bytes, nao um byte por letra acentuada. |
| `WORD expressao` | Emite uma palavra big-endian de 24 bits; intervalo -8388608..16777215. |
| `RESB n` / `RESW n` | Reserva n bytes ou n palavras de 3 bytes; n absoluto e nao negativo. |
| `EQU expressao` | Exige rotulo; define constante/endereco sem emitir bytes. |
| `ORG expressao` | Move o contador de localizacao sem emitir bytes; operandos obrigatorios, sobreposicoes rejeitadas. |
| `BASE expressao` / `NOBASE` | Ativa/desativa o enderecamento relativo ao registrador B. |

Numeros sao decimais, exceto START (hexadecimal). Prefixos `0x` ou `$` permitem
hexadecimal nas demais expressoes. Suporta `+`, `-` e `*` como localizacao atual;
nao suporta multiplicacao, divisao ou parenteses. Endereco menos endereco gera
constante absoluta; somar dois enderecos e rejeitado. EQU/ORG/RESB/RESW precisam
de valores conhecidos no passo 1. Operandos de instrucoes, WORD, BASE e END podem
referenciar rotulos definidos adiante. Os calculos sao verificados contra overflow.

## Limites e escopo

- Objeto ABSOLUTO de um unico programa: deve ser carregado nos enderecos montados.
  Nao gera registros M de relocacao nem faz ligacao entre modulos.
- Nao implementa macros, `MACRO/ENDM`, `EXTREF/EXTDEF`, `CSECT`, `USE`, `LTORG`,
  literais com `=`, `DC`/`DS` do 68000, `ORG` sem operando nem listas de WORD.
  Essas funcionalidades aparecem no contexto geral de montadores, mas nao estao
  prometidas por esta implementacao inicial. Operacoes desconhecidas sao erros.
- Sem instrucoes de ponto flutuante, E/S, privilegiadas, STSW ou formato 1;
  o conjunto aqui e o das 38 instrucoes obrigatorias inteiras validadas no nucleo.
- Memoria do projeto: 4096 bytes. Codigo, dados e reservas precisam caber nela.
  Um operando de formato 4 pode representar ate 20 bits; isso nao aumenta a memoria
  do simulador. Acesso efetivo fora dela continua sendo erro de execucao.
- Maximo: 2048 linhas (incluindo comentarios), 1024 simbolos, rotulos com 31
  caracteres, operando com 255 caracteres e linhas com ate 510 caracteres.
  A listagem intermediaria fica no heap; limites excedidos geram mensagem.
- Esta entrega implementa o caminho basico demonstrado pelo capitulo, adaptado ao
  projeto. Um enunciado especifico da segunda entrega pode exigir ampliar o escopo.

## Validacao realizada

Windows x64, GCC 16.2.0, C11, `-Wall -Wextra -Wpedantic -O2`, sem avisos:

- 141 verificacoes originais do simulador: 12 + 57 + 72.
- 114 verificacoes do montador: bytes conhecidos das 38 instrucoes, modos de
  enderecamento, diretivas, erros, limites, H/T/E, listagem e integracao real.
- Tres exemplos `.asm` montados pela CLI e executados ate o fim no simulador.
- Dois casos adicionais de multiplicacao do nucleo com `-ftrapv` passaram.
- 17 verificacoes da CLI: erros, retorno e nao sobrescrita de entradas/saidas.

Total: 272 verificacoes nas quatro baterias em C e na CLI, mais os dois casos
adicionais de overflow.

Os testes nao sao uma prova de cobertura completa de toda a arquitetura SIC/XE.
Nao houve commit nem publicacao no GitHub nesta etapa.
