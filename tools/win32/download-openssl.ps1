[string]$pwd = Get-Location
$OpenSSL_version = '1.1.0g'
$OpenSSL_arch = 'x64'
$OpenSSL_vcbuild = 'vc150'
$OpenSSL_fileversion = $OpenSSL_version.Replace('.', '_')
$OpenSSL_file = [string]::Format(
  'openssl-{0}-binary-icinga-{1}-{2}.zip',
  $OpenSSL_fileversion,
  $OpenSSL_arch,
  $OpenSSL_vcbuild
)

# TODO: from GitHub Release!
#$OpenSSL_url = ''
$OpenSSL_url = 'https://ci.appveyor.com/api/buildjobs/4hwjjxgvvyeplrjd/artifacts/' + $OpenSSL_file

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
}

Write-Output "Extracting ZIP to $OpenSSL_vendor_path"
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($OpenSSL_zip_location, $OpenSSL_vendor_path)
if ($lastexitcode -ne 0){ exit $lastexitcode }

exit 0
