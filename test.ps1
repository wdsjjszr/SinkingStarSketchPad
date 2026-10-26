param([string]$Compiler = (Join-Path $PSScriptRoot '.tools\llvm-mingw-20261006-ucrt-x86_64\bin\clang++.exe'))
$ErrorActionPreference = 'Stop'
foreach ($test in @('model_tests','grid_tests','native_ground_tests','grid_preview_tests','voxel_clip_tests','drawing_tests','shape_input_tests','i18n_tests','input_tests','free_camera_tests','ui_tests')) {
    & $Compiler -std=c++17 -fsyntax-only (Join-Path $PSScriptRoot ('tests\' + $test + '.cpp'))
    if ($LASTEXITCODE -ne 0) { throw ('Static check failed: ' + $test) }
}
$sdk = Join-Path $PSScriptRoot 'third_party\reshade-sdk'
foreach ($source in @('src\addon.cpp','tests\input_addon.cpp','tests\drawing_addon.cpp','tests\free_camera_addon.cpp','tests\free_camera_input_addon.cpp')) {
    & $Compiler -std=c++17 -fsyntax-only -fms-extensions -Wno-ignored-attributes -I $sdk (Join-Path $PSScriptRoot $source)
    if ($LASTEXITCODE -ne 0) { throw ('Static check failed: ' + $source) }
}
Write-Output 'All core and overlay sources passed static compilation checks. No test programs or game sessions were run.'
