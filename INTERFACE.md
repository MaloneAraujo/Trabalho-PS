# Interface grafica original, integrada ao montador

Extraia TODO o ZIP em uma pasta e abra **simulador-gui.exe**. Nao execute diretamente
de dentro do ZIP. O exemplo `exemplos/soma.asm` e localizado ao lado do executavel,
montado e carregado automaticamente. Nao e necessario instalar a Raylib para executar.

## Controles

- **Passo** ou **Espaco**: executa uma instrucao e pausa a execucao automatica.
- **Executar/Pausar**: avanca uma instrucao a cada 0,15 segundo ou pausa.
- **Reiniciar**: restaura a imagem originalmente carregada, incluindo memoria,
  registradores e endereco inicial. Nao rele o arquivo do disco.
- **Recarregar** ou **F5**: rele o arquivo atual; se for `.asm`, monta novamente.
- Arraste um arquivo **.asm** ou **.obj** para a janela para trocar de programa.
  Assembly e montado em memoria, sem criar ou sobrescrever objetos no disco.
- Campo de memoria: apague o endereco com Backspace, digite hexadecimal e Enter.
  Intervalo valido nesta versao: `000000` a `000FFF` (4096 bytes).
- Roda do mouse sobre a memoria: navega pelas linhas de 16 bytes.
- Amarelo: valor alterado no ultimo passo; contorno azul: byte apontado pelo PC.
- Depois de finalizar ou falhar, use Reiniciar ou Recarregar antes de executar.

Tambem e possivel escolher o arquivo por argumento:

```powershell
.\simulador-gui.exe exemplos\enderecamento.asm
.\simulador-gui.exe exemplos\soma.obj
.\simulador-gui.exe exemplos\contagem.asm
```

`contagem.asm` e mais longo para demonstrar pausa. O exemplo de soma termina com
`A=000008`, `B=000007`, `L=000015`, `PC=000015`, em 9 passos. O salto para a propria
instrucao e a convencao de termino do simulador, nao uma instrucao HALT da arquitetura.
O log mostra as seis entradas mais recentes; mensagens muito longas ficam recortadas.

## O que foi reaproveitado e alterado

- `main.c`, `memview.c` e `memview.h` vieram do projeto original enviado pelo grupo.
  Layout de registradores, memoria e log mantido.
- `programa.c/.h`: ponte sem dependencia grafica. `.asm` passa pelo montador;
  demais arquivos passam pelo carregador existente. Uma falha preserva a maquina.
- `sim_carregar_imagem`, em `sim_real.c` (contrato em `sicxe.h`): aplica uma
  imagem validada, permitindo montagem em memoria e reinicio completo.
- `main.c`: abre assembly, guarda a imagem inicial, impede continuar apos fim/erro,
  mostra estado e contador, permite recarregar e usa fila de eventos de teclado
  para nao perder pressionamentos curtos entre quadros.
- `memview.c`: libera a copia de memoria ao encerrar e invalida copia antiga se a
  alocacao mudar. Navegacao de memoria alinhada em linhas de 16 bytes.
- O `sim_reset` original continua com sua semantica de zerar registradores e
  preservar memoria. O botao Reiniciar usa a nova carga de imagem para restaurar tudo.

Nao foi adicionado editor de codigo nem dialogo de selecao de arquivos. Para gerar
`.obj` e `.lst` permanentes, use a CLI do montador, documentada no README.

## Compilar

Windows x64 com GCC/MinGW no PATH:

```powershell
.\compilar.ps1 -Testar -VerificarOverflow
```

O script gera `simulador-gui.exe`, `simulador.exe` e `montador.exe`. Use
`-SemInterface` se quiser compilar apenas o terminal. Feche a janela antes de recompilar.
Com MinGW Make: `mingw32-make OS=Windows_NT` e `mingw32-make teste OS=Windows_NT`.

Raylib 5.5 Windows x64 esta em `vendor/raylib`, ligada estaticamente; veja ali a
licenca e a origem oficial. Em Linux, `make` usa Raylib via pkg-config; `make terminal`
nao depende da Raylib. Linux nao foi validado.

## Validacao em 21/10/2026

- Compilacao Windows x64 com GCC 16.2.0, C11 e avisos habilitados, sem avisos.
- 291 verificacoes automatizadas aprovadas: 141 do simulador, 114 do montador,
  19 da ponte de carregamento e 17 da CLI. Mais 2 casos de overflow com `-ftrapv`.
- Janela real inspecionada: carregamento inicial de assembly, Passo, destaques de
  registradores/memoria, execucao ate o fim, restauracao completa, pausa com contador
  estavel, Espaco, F5, botao Recarregar e rolagem de memoria.
- Arrastar arquivos e editar o campo hexadecimal nao foram validados por automacao
  de interface. A carga `.obj` e os erros de carga/montagem foram validados em C.