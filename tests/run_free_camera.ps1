param(
    [Parameter(Mandatory=$true)][string]$Compiler,
    [Parameter(Mandatory=$true)][string]$Runtime,
    [string]$Font,
    [switch]$InputSuite,
    [switch]$Cached,
    [switch]$LateDescriptor,
    [switch]$GpuUpload,
    [switch]$LargerCamera,
    [switch]$BulkDescriptors,
    [switch]$DefaultCamera
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$case = Join-Path $root ('build\free-camera-test-' + [guid]::NewGuid().ToString('N').Substring(0,8))
[IO.Directory]::CreateDirectory((Join-Path $case 'SinkingStarSketch-assets')) | Out-Null
Copy-Item -LiteralPath $Runtime -Destination (Join-Path $case 'dxgi.dll')
if ($Font) { Copy-Item -LiteralPath $Font -Destination (Join-Path $case 'SinkingStarSketch-assets\NotoSansCJKsc-Regular.otf') }
Copy-Item -LiteralPath (Join-Path $root 'config\ReShade.ini') -Destination $case
if ($Font) { $cfg=Join-Path $case 'ReShade.ini'; (Get-Content -LiteralPath $cfg -Raw).Replace('Font=', 'Font=.\SinkingStarSketch-assets\NotoSansCJKsc-Regular.otf') | Set-Content -LiteralPath $cfg -Encoding utf8 }
& $Compiler -std=c++17 -O2 -static -municode -mwindows (Join-Path $PSScriptRoot 'dx12_host.cpp') -o (Join-Path $case 'SketchDX12Test.exe') -ld3d12 -ldxgi -ld3dcompiler -luser32 -lole32 -luuid
if ($LASTEXITCODE -ne 0) { throw 'Camera host build failed.' }
$fixture = if ($InputSuite) { 'free_camera_input_addon.cpp' } else { 'free_camera_addon.cpp' }
& $Compiler -std=c++17 -O2 -shared -static -fms-extensions -Wno-ignored-attributes -I (Join-Path $root 'third_party\reshade-sdk') (Join-Path $PSScriptRoot $fixture) -o (Join-Path $case 'SinkingStarSketch.addon64') -luser32 -lkernel32
if ($LASTEXITCODE -ne 0) { throw 'Camera fixture add-on build failed.' }
$testArgs = @('--smoke','--grid','--free-camera')
if ($InputSuite) { $testArgs += '--camera-input' }
if ($Cached) { $testArgs += '--camera-cached' }
if ($LateDescriptor) { $testArgs += '--late-descriptor' }
if ($GpuUpload) { $testArgs += '--gpu-upload' }
if ($LargerCamera) { $testArgs += '--larger-camera' }
if ($BulkDescriptors) { $testArgs += '--bulk-descriptors' }
if ($DefaultCamera) { $testArgs += '--default-camera' }
$testProcess = Start-Process -FilePath (Join-Path $case 'SketchDX12Test.exe') -ArgumentList $testArgs -WorkingDirectory $case -WindowStyle Hidden -PassThru
if (-not $testProcess.WaitForExit(25000)) { Stop-Process -Id $testProcess.Id; throw 'Camera fixture timed out.' }
if ($testProcess.ExitCode -ne 0) { Get-Content (Join-Path $case 'dx12-test-error.txt') -ErrorAction SilentlyContinue; throw 'Camera fixture failed.' }
if (Test-Path (Join-Path $case 'free-camera-test-error.txt')) { throw (Get-Content (Join-Path $case 'free-camera-test-error.txt') -Raw) }
if ($InputSuite) {
    if (Test-Path (Join-Path $case 'free-camera-input-error.txt')) { throw (Get-Content (Join-Path $case 'free-camera-input-error.txt') -Raw) }
    $results = Get-Content (Join-Path $case 'free-camera-input-result.txt')
} else {
    $results = Get-Content (Join-Path $case 'free-camera-render-results.txt')
    if ($results.Count -ne 5) { throw 'Missing GPU camera verification stages.' }
}
$results
Get-Content (Join-Path $case 'dx12-test-result.txt')
Write-Output ('Fixture output: ' + $case)
