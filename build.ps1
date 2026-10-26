param([string]$Compiler,[string]$Sdk)
$ErrorActionPreference = 'Stop'
$localCompiler = Join-Path $PSScriptRoot '.tools\llvm-mingw-20261006-ucrt-x86_64\bin\clang++.exe'
if (-not $Compiler -and (Test-Path -LiteralPath $localCompiler -PathType Leaf)) { $Compiler = $localCompiler }
if (-not $Compiler) {
    $command = Get-Command x86_64-w64-mingw32-clang++ -ErrorAction SilentlyContinue
    if ($command) { $Compiler = $command.Source }
}
if (-not $Compiler -or -not (Test-Path -LiteralPath $Compiler)) { throw 'Pass -Compiler with the path to the x64 LLVM-MinGW clang++.exe.' }
if (-not $Sdk) { $Sdk = Join-Path $PSScriptRoot 'third_party\reshade-sdk' }
$target = (& $Compiler -dumpmachine | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $target -notmatch '^x86_64-.*(mingw|windows-gnu)') { throw ('An x64 MinGW compiler is required. Detected: ' + $target) }
$out = Join-Path $PSScriptRoot 'build'
[IO.Directory]::CreateDirectory($out) | Out-Null
& $Compiler -std=c++17 -O2 -shared -static -fms-extensions -Wno-ignored-attributes -I $Sdk (Join-Path $PSScriptRoot 'src\addon.cpp') -o (Join-Path $out 'SinkingStarSketch.addon64') -luser32 -lkernel32
if ($LASTEXITCODE -ne 0) { throw 'Native add-on build failed.' }
Write-Output ('Built: ' + (Join-Path $out 'SinkingStarSketch.addon64'))
