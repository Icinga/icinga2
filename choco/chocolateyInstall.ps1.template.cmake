$packageName= 'icinga2'
$toolsDir   = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"
$url64 = 'https://packages.icinga.com/windows/Icinga2-v${CHOCO_VERSION_SHORT}-x86_64.msi'

$packageArgs = @{
  packageName   = $packageName
  fileType      = 'msi'
  url64bit      = $url64
  silentArgs    = "/qn /norestart"
  validExitCodes= @(0)
  softwareName  = 'Icinga 2*'
  checksum64    = '%CHOCO_64BIT_CHECKSUM%'
  checksumType64= 'sha256'
}

Install-ChocolateyPackage @packageArgs