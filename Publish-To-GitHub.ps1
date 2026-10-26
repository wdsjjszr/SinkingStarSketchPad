# Push committed source only; this does not build assets or create a Release.
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$expectedRemote = 'https://github.com/wdsjjszr/SinkingStarSketchPad.git'
$origin = (& git remote get-url origin | Out-String).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Could not read origin.' }
if ($origin -ne $expectedRemote) { throw ('Unexpected origin; no files pushed: ' + $origin) }
& git push -u origin main
if ($LASTEXITCODE -ne 0) { throw 'Source push failed.' }
Write-Output 'Source pushed: https://github.com/wdsjjszr/SinkingStarSketchPad'
Write-Output 'Build the Release ZIP with .\package.ps1; see docs/RELEASE_zh-CN.md.'
