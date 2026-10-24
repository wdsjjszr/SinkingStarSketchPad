param([Parameter(Mandatory=$true)][string]$Compiler)
$ErrorActionPreference = 'Stop'
foreach ($test in @('model_tests','grid_tests','native_ground_tests','grid_preview_tests','i18n_tests','input_tests','free_camera_tests')) {
    & $Compiler -std=c++17 -fsyntax-only (Join-Path $PSScriptRoot ('tests\' + $test + '.cpp'))
    if ($LASTEXITCODE -ne 0) { throw ('Static check failed: ' + $test) }
}
Write-Output 'All core sources passed static compilation checks. No test programs or game sessions were run.'
