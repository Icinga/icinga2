$packageName= 'icinga2'
$toolsDir   = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"
$url = 'https://packages.icinga.com/windows/Icinga2-v${CHOCO_VERSION_SHORT}-x86.msi'
$url64 = 'https://packages.icinga.com/windows/Icinga2-v${CHOCO_VERSION_SHORT}-x86_64.msi'

$packageArgs = @{
  packageName   = $packageName
  fileType      = 'msi'
  url           = $url
  url64bit      = $url64
  silentArgs    = "/qn /norestart"
  validExitCodes= @(0)
  softwareName  = 'Icinga 2*'
  checksum      = '' # TODO: Add checksum
  checksumType  = 'sha256'
  checksum64    = '' # TODO: Add checksum
  checksumType64= 'sha256'
}

Install-ChocolateyPackage @packageArgs