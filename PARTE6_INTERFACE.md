# Parte 6 - Interface grafica e compilacao (Vitor)

## Alteracoes

- `programa.c`: carrega `.asm` por `montar_com_macros`, o mesmo integrador do
  terminal. O fluxo e fonte -> macros -> MASMAPRG.ASM -> montagem -> imagem
  de memoria -> simulador. `.obj` continua sendo carregado diretamente.
- `Makefile` e `compilar.ps1`: ligam os modulos ativos de macros e incluem os
  testes individuais, o teste integrado do grupo e o teste da ponte da GUI.
- `testes/teste_interface_macros.c`: verifica carga, passos, resultado, reinicio,
  recarga, erros sem alterar a maquina e compatibilidade com `.obj`.
- `simulador.exe`, `montador.exe` e `simulador-gui.exe`: recompilados no Windows.
- `.gitignore`: ignora arquivos intermediarios e executaveis de testes.

O repositorio de teste estava sem os fontes do montador original, a Raylib e
exemplos/testes referenciados pelos scripts. Apenas os arquivos ausentes foram
recuperados de `MaloneAraujo/Trabalho-PS`, commit
`77ed9dc95dfcc35f6def189c18122073bca223ef`. Os modulos de macros enviados pelo
grupo foram preservados.

## Contrato de definicoes

O controlador `montagem/macros.c` e o modulo `montagem/macros_definicao.c`
trabalham de forma integrada no fluxo de definicoes aninhadas, substituicao
posicional `#(d,i)` e gerenciamento da pilha de niveis `d`, compilados juntos
em `Makefile` e `compilar.ps1`.

## Executar

```powershell
.\compilar.ps1
.\simulador-gui.exe exemplos\macros_aninhadas.asm
```

Tambem e possivel arrastar o `.asm` para a janela. Passo, Executar, Reiniciar e
Recarregar continuam utilizando o estado do simulador existente.

```powershell
.\compilar.ps1 -Testar -VerificarOverflow
mingw32-make all
mingw32-make teste
```

## Validacao realizada em 13/11/2026

- Compilacao completa por PowerShell e por `mingw32-make -B all`: sucesso.
- Ponte da interface com macros: 24 verificacoes, zero falhas. O exemplo do
  grupo terminou com A=10 e RESULT=10; erros preservaram memoria e registradores.
- Ponte anterior: 19; parser de macros: 45; tabela: 13; expansao: 27;
  integracao parcial de macros: 14 verificacoes, todas sem falhas.
- Teste anterior de integracao do simulador: 57 verificacoes sem falhas em uma
  execucao. O teste basico do simulador tambem passou.
- Montador pelo terminal gerou objeto do exemplo com macros; o simulador
  executou esse objeto em 5 passos e terminou com A=00000A.
- O Controle de Aplicativos do Windows bloqueou intermitentemente executaveis
  de testes. Por isso a suite completa nao terminou em uma unica execucao;
  nao se declara aprovacao dos testes restantes nem do teste de overflow.
- A janela grafica nao foi validada visualmente nesta entrega. A validacao
  automatizada cobre sua ponte de carga/execucao, sem interagir com os botoes.
