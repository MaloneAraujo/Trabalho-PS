$ErrorActionPreference = 'Stop'
$raizTeste = Split-Path $PSScriptRoot -Parent
$montadorTeste = Join-Path $raizTeste 'montador.exe'
$simuladorTeste = Join-Path $raizTeste 'simulador.exe'
$fonteTeste = Join-Path $raizTeste 'exemplos\soma.asm'
$invalidoTeste = Join-Path $PSScriptRoot 'entrada_invalida.asm'
$pastaTeste = Join-Path ([IO.Path]::GetTempPath()) ('sicxe-cli-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $pastaTeste | Out-Null
$objTeste = Join-Path $pastaTeste 'soma.obj'
$lstTeste = Join-Path $pastaTeste 'soma.lst'
$ruimTeste = Join-Path $pastaTeste 'ruim.obj'
$rollbackTeste = Join-Path $pastaTeste 'rollback.obj'
$igualTeste = Join-Path $pastaTeste 'igual.obj'
$script:passouCli = 0

function Verificar-Cli([bool]$ok, [string]$nome) {
    if (!$ok) { throw "FALHOU: $nome" }
    $script:passouCli++
}
function Rodar-Cli([string]$programa, [string[]]$argumentos, [int]$codigo) {
    # Start-Process preserva stderr sem trata-lo como erro do PowerShell 5.
    $argsCitados = ($argumentos | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $opcoes = @{ FilePath=$programa; Wait=$true; PassThru=$true; WindowStyle='Hidden';
        RedirectStandardOutput=(Join-Path $pastaTeste 'stdout.txt');
        RedirectStandardError=(Join-Path $pastaTeste 'stderr.txt') }
    if ($argsCitados) { $opcoes.ArgumentList = $argsCitados }
    $processo = Start-Process @opcoes
    Verificar-Cli ($processo.ExitCode -eq $codigo) "codigo de saida esperado $codigo"
}
try {
    $hashFonte = (Get-FileHash -LiteralPath $fonteTeste).Hash
    Rodar-Cli $montadorTeste @() 2
    Rodar-Cli $montadorTeste @((Join-Path $pastaTeste 'ausente.asm'), $ruimTeste) 1
    Rodar-Cli $montadorTeste @($invalidoTeste, $ruimTeste) 1
    Verificar-Cli (!(Test-Path -LiteralPath $ruimTeste)) 'erro de montagem nao cria objeto'
    Rodar-Cli $montadorTeste @($fonteTeste, $fonteTeste) 1
    Verificar-Cli ((Get-FileHash -LiteralPath $fonteTeste).Hash -eq $hashFonte) 'fonte preservado'
    Rodar-Cli $montadorTeste @($fonteTeste, $rollbackTeste, $fonteTeste) 1
    Verificar-Cli (!(Test-Path -LiteralPath $rollbackTeste)) 'rollback ao falhar listagem'
    Rodar-Cli $montadorTeste @($fonteTeste, $igualTeste, $igualTeste) 1
    Verificar-Cli (!(Test-Path -LiteralPath $igualTeste)) 'saidas iguais nao deixam arquivo parcial'
    Rodar-Cli $montadorTeste @($fonteTeste, $objTeste, $lstTeste) 0
    Verificar-Cli ((Test-Path -LiteralPath $objTeste) -and (Test-Path -LiteralPath $lstTeste)) 'objeto e listagem criados'
    $hashObj = (Get-FileHash -LiteralPath $objTeste).Hash
    Rodar-Cli $montadorTeste @($fonteTeste, $objTeste) 1
    Verificar-Cli ((Get-FileHash -LiteralPath $objTeste).Hash -eq $hashObj) 'objeto anterior preservado'
    Rodar-Cli $simuladorTeste @($objTeste) 0
    Verificar-Cli ((Get-Content -LiteralPath (Join-Path $pastaTeste 'stdout.txt') -Raw) -match 'A=000008') 'assembly executa soma'
    Rodar-Cli $montadorTeste @($fonteTeste, (Join-Path $pastaTeste 'ausente\saida.obj')) 1
    Write-Output "CLI: $script:passouCli verificacoes passaram."
} finally {
    # Somente arquivos conhecidos dentro da pasta exclusiva deste teste; sem recursao.
    foreach ($nome in @('soma.obj','soma.lst','ruim.obj','rollback.obj','igual.obj','stdout.txt','stderr.txt')) {
        $arquivoTeste = Join-Path $pastaTeste $nome
        if (Test-Path -LiteralPath $arquivoTeste) { Remove-Item -LiteralPath $arquivoTeste }
    }
    Remove-Item -LiteralPath $pastaTeste
}
