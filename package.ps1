param([string]$Compiler, [string]$Python = 'python')
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build.ps1') -Compiler $Compiler
& $Python (Join-Path $PSScriptRoot 'scripts\package.py')
if ($LASTEXITCODE -ne 0) { throw 'Release packaging failed.' }
