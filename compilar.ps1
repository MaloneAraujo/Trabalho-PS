param([switch]$Testar, [switch]$Executar, [switch]$VerificarOverflow, [switch]$SemInterface)
$ErrorActionPreference = 'Stop'
$gccEtapa1 = (Get-Command gcc -ErrorAction Stop).Source
$nucleoEtapa1 = @('memoria.c','decodificador.c','Aritimetica.c','instrucoes_movimentacao_desvios.c','sim_real.c')
$opcoesEtapa1 = @('-std=c11','-Wall','-Wextra','-Wpedantic','-O2','-I.')
$fontesMontador = @('montagem/parser.c','montagem/tabelas.c','montagem/passos.c','montagem/objeto.c')

function Compilar-Etapa1([string]$fonte, [string]$saida, [string[]]$extras = @()) {
    & $gccEtapa1 @opcoesEtapa1 @extras $fonte @nucleoEtapa1 '-lm' '-o' $saida
    if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar $fonte" }
}
function Executar-Etapa1([string]$arquivo, [string[]]$argumentos = @()) {
    & (Join-Path $PSScriptRoot $arquivo) @argumentos
    if ($LASTEXITCODE -ne 0) { throw "Falha em $arquivo (codigo $LASTEXITCODE)" }
}

Push-Location $PSScriptRoot
try {
    Compilar-Etapa1 'main_cli.c' 'simulador.exe'
    & $gccEtapa1 @opcoesEtapa1 'montagem/main.c' @fontesMontador '-o' 'montador.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar montador' }
    if (!$SemInterface) {
        & $gccEtapa1 @opcoesEtapa1 '-Ivendor/raylib/include' 'main.c' 'memview.c' 'programa.c' @fontesMontador @nucleoEtapa1 'vendor/raylib/lib/libraylib.a' '-lopengl32' '-lgdi32' '-lwinmm' '-lm' '-mwindows' '-o' 'simulador-gui.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar interface' }
    }
    if ($Testar) {
        Compilar-Etapa1 'teste_sim_real.c' 'teste_sim_real.exe'
        Compilar-Etapa1 'testes/teste_integracao.c' 'teste_integracao.exe'
        Compilar-Etapa1 'testes/teste_instrucoes.c' 'teste_instrucoes.exe'
        Executar-Etapa1 'teste_sim_real.exe' @('exemplo.obj')
        Executar-Etapa1 'teste_integracao.exe'
        Executar-Etapa1 'teste_instrucoes.exe'
        & $gccEtapa1 @opcoesEtapa1 'testes/teste_montador.c' @fontesMontador @nucleoEtapa1 '-lm' '-o' 'teste_montador.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar teste do montador' }
        Executar-Etapa1 'teste_montador.exe'
        & $gccEtapa1 @opcoesEtapa1 'testes/teste_programa.c' 'programa.c' @fontesMontador @nucleoEtapa1 '-lm' '-o' 'teste_programa.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar testes da ponte grafica' }
        Executar-Etapa1 'teste_programa.exe'
        & (Join-Path $PSScriptRoot 'testes/teste_cli.ps1')
    }
    if ($VerificarOverflow) {
        Compilar-Etapa1 'testes/teste_instrucoes.c' 'teste_overflow.exe' @('-O0','-ftrapv')
        Executar-Etapa1 'teste_overflow.exe' @('overflow-mul')
        Executar-Etapa1 'teste_overflow.exe' @('overflow-mulr')
    }
    if ($Executar) { Executar-Etapa1 'simulador.exe' @('exemplo.obj') }
    if (!$SemInterface) { Write-Output 'Interface: simulador-gui.exe' }
    Write-Output 'Terminal: simulador.exe e montador.exe'
} finally {
    Pop-Location
}
