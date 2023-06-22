Set-PsDebug -Trace 1

# Specify default targets for VS 2019 for developers.

if (-not (Test-Path env:ICINGA2_BUILDPATH)) {
  $env:ICINGA2_BUILDPATH = '.\debug'
}
if (-not (Test-Path "$env:ICINGA2_BUILDPATH")) {
  mkdir "$env:ICINGA2_BUILDPATH" | out-null
}

if (-not (Test-Path env:CMAKE_BUILD_TYPE)) {
  $env:CMAKE_BUILD_TYPE = 'Debug'
}
if (-not (Test-Path env:ICINGA2_INSTALLPATH)) {
  $env:ICINGA2_INSTALLPATH = 'C:\Program Files\Icinga2-debug'
}
if (-not (Test-Path env:CMAKE_PATH)) {
  $env:CMAKE_PATH = 'C:\Program Files\CMake\bin'
}
if (-not ($env:PATH -contains $env:CMAKE_PATH)) {
  $env:PATH = $env:CMAKE_PATH + ';' + $env:PATH
}
if (-not (Test-Path env:CMAKE_GENERATOR)) {
  $env:CMAKE_GENERATOR = 'Visual Studio 16 2019'
}
if (-not (Test-Path env:CMAKE_GENERATOR_PLATFORM)) {
  $env:CMAKE_GENERATOR_PLATFORM = 'x64'
}
if (-not (Test-Path env:OPENSSL_ROOT_DIR)) {
  $env:OPENSSL_ROOT_DIR = 'c:\local\OpenSSL-Win64'
}
if (-not (Test-Path env:BOOST_ROOT)) {
  $env:BOOST_ROOT = 'c:\local\boost_1_82_0'
}
if (-not (Test-Path env:BOOST_LIBRARYDIR)) {
  $env:BOOST_LIBRARYDIR = 'c:\local\boost_1_82_0\lib64-msvc-14.2'
}
if (-not (Test-Path env:FLEX_BINARY)) {
  $env:FLEX_BINARY = 'C:\ProgramData\chocolatey\bin\win_flex.exe'
}
if (-not (Test-Path env:BISON_BINARY)) {
  $env:BISON_BINARY = 'C:\ProgramData\chocolatey\bin\win_bison.exe'
}

$sourcePath = Get-Location

cd "$env:ICINGA2_BUILDPATH"

# Invalidate cache in case something in the build environment changed
if (Test-Path CMakeCache.txt) {
  Remove-Item -Force CMakeCache.txt | Out-Null
}

& cmake.exe "$sourcePath" `
  -DCMAKE_BUILD_TYPE="$env:CMAKE_BUILD_TYPE" `
  -G "$env:CMAKE_GENERATOR" -A "$env:CMAKE_GENERATOR_PLATFORM" -DCPACK_GENERATOR=WIX `
  -DCMAKE_INSTALL_PREFIX="$env:ICINGA2_INSTALLPATH" `
  -DOPENSSL_ROOT_DIR="$env:OPENSSL_ROOT_DIR" `
  -DBOOST_LIBRARYDIR="$env:BOOST_LIBRARYDIR" `
  -DBOOST_INCLUDEDIR="$env:BOOST_ROOT" `
  -DFLEX_EXECUTABLE="$env:FLEX_BINARY" `
  -DBISON_EXECUTABLE="$env:BISON_BINARY"

cd "$sourcePath"

if ($lastexitcode -ne 0) {
  exit $lastexitcode
}
