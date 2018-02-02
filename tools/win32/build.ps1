[string]$pwd = Get-Location

if (-not (Test-Path build)) {
  Write-Host "Path '$pwd\build' does not exist!"
  exit 1
}

if (-not (Test-Path env:CMAKE_PATH)) {
  $env:CMAKE_PATH = 'C:\Program Files\CMake\bin'
}
if (-not ($env:PATH -contains $env:CMAKE_PATH)) {
  $env:PATH = $env:CMAKE_PATH + ';' + $env:PATH
}

cmake.exe --build build --target PACKAGE --config RelWithDebInfo
if ($lastexitcode -ne 0) { exit $lastexitcode }