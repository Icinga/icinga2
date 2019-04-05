Set-PsDebug -Trace 1

if (-not (Test-Path env:ICINGA2_BUILDPATH)) {
  $env:ICINGA2_BUILDPATH = 'build'
}

if (-not (Test-Path env:CMAKE_BUILD_TYPE)) {
  $env:CMAKE_BUILD_TYPE = 'RelWithDebInfo'
}

[string]$pwd = Get-Location

if (-not (Test-Path $env:ICINGA2_BUILDPATH)) {
  Write-Host "Path '$pwd\$env:ICINGA2_BUILDPATH' does not exist!"
  exit 1
}

if (-not (Test-Path env:CMAKE_PATH)) {
  $env:CMAKE_PATH = 'C:\Program Files\CMake\bin'
}
if (-not ($env:PATH -contains $env:CMAKE_PATH)) {
  $env:PATH = $env:CMAKE_PATH + ';' + $env:PATH
}

cd "$env:ICINGA2_BUILDPATH"

ctest.exe -C "${env:CMAKE_BUILD_TYPE}" -T test -O $env:ICINGA2_BUILDPATH/Test.xml --output-on-failure
if ($lastexitcode -ne 0) {
  cd ..
  exit $lastexitcode
}

cd ..
