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

cd build

ctest.exe -C RelWithDebInfo -T test -O build/Test.xml --output-on-failure
if ($lastexitcode -ne 0) {
  cd ..
  exit $lastexitcode
}

cd ..