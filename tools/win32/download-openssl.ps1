[string]$pwd = Get-Location
$OpenSSL_version = '1.1.0g-1'
$OpenSSL_arch = 'x64'
$OpenSSL_vcbuild = 'vc150'
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

$OpenSSL_zip_location = $pwd + '\vendor\' + $OpenSSL_file
$vendor_path = $pwd + '\vendor'
$OpenSSL_vendor_path = $vendor_path + '\OpenSSL'

if (-not (Test-Path $vendor_path)) {
  mkdir $vendor_path
}

if (Test-Path $OpenSSL_zip_location) {
  Write-Output "OpenSSL archive available at $OpenSSL_zip_location"
} else {
  Write-Output "Downloading OpenSSL binary dist from $OpenSSL_url"

  $progressPreference = 'silentlyContinue'
  Invoke-WebRequest -Uri $OpenSSL_url -OutFile $OpenSSL_zip_location
  if ($lastexitcode -ne 0){ exit $lastexitcode }
  $progressPreference = 'Continue'
  
  if (Test-Path $OpenSSL_vendor_path) {
    Remove-Item -Recurse $OpenSSL_vendor_path
  }
}

if (-not (Test-Path $OpenSSL_vendor_path)) {
  mkdir $OpenSSL_vendor_path

  Write-Output "Extracting ZIP to $OpenSSL_vendor_path"
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  [System.IO.Compression.ZipFile]::ExtractToDirectory($OpenSSL_zip_location, $OpenSSL_vendor_path)
  if ($lastexitcode -ne 0){ exit $lastexitcode }  
} else {
  Write-Output "OpenSSL is already available at $OpenSSL_vendor_path"
}
