if (-not (Test-Path build)) {
  mkdir build
}
if (-not (Test-Path install)) {
  mkdir install
}
if (-not (Test-Path env:CMAKE_PATH)) {
  $env:CMAKE_PATH = 'C:\Program Files\CMake\bin'
}
if (-not ($env:PATH -contains $env:CMAKE_PATH)) {
  $env:PATH = $env:CMAKE_PATH + ';' + $env:PATH
}

[string]$pwd = Get-Location

if (-not (Test-Path env:CMAKE_GENERATOR)) {
  $env:CMAKE_GENERATOR = 'Visual Studio 15 2017 Win64'
}
if (-not (Test-Path env:OPENSSL_ROOT_DIR)) {
  $env:OPENSSL_ROOT_DIR = $pwd + '\vendor\OpenSSL'
}
if (-not (Test-Path env:BOOST_ROOT)) {
  $env:BOOST_ROOT = 'c:\local\boost_1_65_1'
}
if (-not (Test-Path env:BOOST_LIBRARYDIR)) {
  $env:BOOST_LIBRARYDIR = 'c:\local\boost_1_65_1\lib64-msvc-14.1'
}
if (-not (Test-Path env:FLEX_BINARY)) {
  $env:FLEX_BINARY = 'C:\ProgramData\chocolatey\bin\win_flex.exe'
}
if (-not (Test-Path env:BISON_BINARY)) {
  $env:BISON_BINARY = 'C:\ProgramData\chocolatey\bin\win_bison.exe'
}

cd build

& cmake.exe .. `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -G $env:CMAKE_GENERATOR -DCPACK_GENERATOR=WIX `
  -DCMAKE_INSTALL_PREFIX="..\install" `
  -DICINGA2_WITH_MYSQL=OFF -DICINGA2_WITH_PGSQL=OFF `
  -DOPENSSL_ROOT_DIR="$env:OPENSSL_ROOT_DIR" `
  -DBOOST_ROOT="$env:BOOST_ROOT" `
  -DBOOST_LIBRARYDIR="$env:BOOST_LIBRARYDIR" `
  -DFLEX_EXECUTABLE="$env:FLEX_BINARY" `
  -DBISON_EXECUTABLE="$env:BISON_BINARY"

if ($lastexitcode -ne 0) {
  cd ..
  exit $lastexitcode
}

cd ..
