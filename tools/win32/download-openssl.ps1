$ErrorActionPreference = "Stop"

$OpenSSL_version = '1.1.0g-1'

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

$OpenSSL_fileversion = $OpenSSL_version.Replace('.', '_').Split('-')[0]
$OpenSSL_file = [string]::Format(
  'openssl-{0}-binary-icinga-{1}-{2}.zip',
  $OpenSSL_fileversion,
  $OpenSSL_arch,
  $OpenSSL_vcbuild
)
$OpenSSL_url = [string]::Format(
  'https://github.com/Icinga/openssl-windows/releases/download/v{0}/{1}',
  $OpenSSL_version,
  $OpenSSL_file
)

if (-not (Test-Path env:ICINGA2_BUILDPATH)) {
  $env:ICINGA2_BUILDPATH = '.\build'
}

$vendor_path = $env:ICINGA2_BUILDPATH + '\vendor'
$OpenSSL_zip_location = $env:ICINGA2_BUILDPATH + '\vendor\' + $OpenSSL_file
$OpenSSL_vendor_path = "$vendor_path\OpenSSL-$OpenSSL_arch-$OpenSSL_vcbuild"

# Tune Powershell TLS protocols
$AllProtocols = [System.Net.SecurityProtocolType]'Tls11,Tls12'
[System.Net.ServicePointManager]::SecurityProtocol = $AllProtocols

if (-not (Test-Path $env:ICINGA2_BUILDPATH)) {
  mkdir $env:ICINGA2_BUILDPATH | out-null
}
if (-not (Test-Path $vendor_path)) {
  mkdir $vendor_path | out-null
}

if (Test-Path $OpenSSL_zip_location) {
  Write-Output "OpenSSL archive available at $OpenSSL_zip_location"
} else {
  Write-Output "Downloading OpenSSL binary dist from $OpenSSL_url"

  $progressPreference = 'silentlyContinue'
  Invoke-WebRequest -Uri $OpenSSL_url -OutFile $OpenSSL_zip_location
  $progressPreference = 'Continue'

  if (Test-Path $OpenSSL_vendor_path) {
    Remove-Item -Recurse $OpenSSL_vendor_path
  }
}

if (-not (Test-Path $OpenSSL_vendor_path)) {
  mkdir $OpenSSL_vendor_path | out-null

  Write-Output "Extracting ZIP to $OpenSSL_vendor_path"
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $pwd = Get-Location
  [System.IO.Compression.ZipFile]::ExtractToDirectory(
    (Join-Path -path $pwd -childpath $OpenSSL_zip_location),
    (Join-Path -path $pwd -childpath $OpenSSL_vendor_path)
  )
  if ($lastexitcode -ne 0){ exit $lastexitcode }
} else {
  Write-Output "OpenSSL is already available at $OpenSSL_vendor_path"
}
