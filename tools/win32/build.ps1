Set-PsDebug -Trace 1

if (-not (Test-Path env:ICINGA2_BUILDPATH)) {
  $env:ICINGA2_BUILDPATH = '.\build'
}

if (-not (Test-Path env:CMAKE_BUILD_TYPE)) {
  $env:CMAKE_BUILD_TYPE = 'RelWithDebInfo'
}

if (-not (Test-Path $env:ICINGA2_BUILDPATH)) {
  Write-Host "Path '$env:ICINGA2_BUILDPATH' does not exist!"
  exit 1
}

if (-not (Test-Path env:CMAKE_PATH)) {
  $env:CMAKE_PATH = 'C:\Program Files\CMake\bin'
}
if (-not ($env:PATH -contains $env:CMAKE_PATH)) {
  $env:PATH = $env:CMAKE_PATH + ';' + $env:PATH
}

cmake.exe --build "$env:ICINGA2_BUILDPATH" --target ALL_BUILD --config $env:CMAKE_BUILD_TYPE
if ($lastexitcode -ne 0) { exit $lastexitcode }

cmake.exe --build "$env:ICINGA2_BUILDPATH" --target PACKAGE --config $env:CMAKE_BUILD_TYPE
if ($lastexitcode -ne 0) { exit $lastexitcode }
