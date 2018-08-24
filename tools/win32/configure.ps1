if (-not (Test-Path env:ICINGA2_BUILDPATH)) {
  $env:ICINGA2_BUILDPATH = '.\build'
}

if (-not (Test-Path "$env:ICINGA2_BUILDPATH")) {
  mkdir "$env:ICINGA2_BUILDPATH" | out-null
}
if (-not (Test-Path "$env:ICINGA2_BUILDPATH\install")) {
  mkdir "$env:ICINGA2_BUILDPATH\install" | out-null
}
if (-not (Test-Path env:CMAKE_PATH)) {
  $env:CMAKE_PATH = 'C:\Program Files\CMake\bin'
}
if (-not ($env:PATH -contains $env:CMAKE_PATH)) {
  $env:PATH = $env:CMAKE_PATH + ';' + $env:PATH
}

if (-not (Test-Path env:CMAKE_GENERATOR)) {
  $env:CMAKE_GENERATOR = 'Visual Studio 15 2017 Win64'
}
if (-not (Test-Path env:OPENSSL_ROOT_DIR)) {
  if (Test-Path env:VSCMD_ARG_TGT_ARCH) {
    $OpenSSL_arch = $env:VSCMD_ARG_TGT_ARCH
  } else {
    throw "Missing env variable VSCMD_ARG_TGT_ARCH"
  }

  if (Test-Path env:VSCMD_VER) {
    $VSmajor = $env:VSCMD_VER -replace "\..*$", ""
    $OpenSSL_vcbuild = "vc${VSmajor}0"
  } else {
    throw "Missing env variable VSCMD_VER"
  }

  $env:OPENSSL_ROOT_DIR = "$env:ICINGA2_BUILDPATH\vendor\OpenSSL-$OpenSSL_arch-$OpenSSL_vcbuild"
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

$sourcePath = Get-Location

cd "$env:ICINGA2_BUILDPATH"

#-DCMAKE_INSTALL_PREFIX="C:\Program Files\Icinga2" `

& cmake.exe "$sourcePath" `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -G $env:CMAKE_GENERATOR -DCPACK_GENERATOR=WIX `
  -DICINGA2_WITH_MYSQL=OFF -DICINGA2_WITH_PGSQL=OFF `
  -DOPENSSL_ROOT_DIR="$env:OPENSSL_ROOT_DIR" `
  -DBOOST_ROOT="$env:BOOST_ROOT" `
  -DBOOST_LIBRARYDIR="$env:BOOST_LIBRARYDIR" `
  -DFLEX_EXECUTABLE="$env:FLEX_BINARY" `
  -DBISON_EXECUTABLE="$env:BISON_BINARY"

cd "$sourcePath"

if ($lastexitcode -ne 0) {
  exit $lastexitcode
}
