$packageName = 'icinga2'
$installerType = 'msi'
$url32 = 'http://packages.icinga.org/windows/Icinga2-v2.4.8-x86.msi'
$url64 = 'http://packages.icinga.org/windows/Icinga2-v2.4.8-x86_64.msi'
$silentArgs = '/qn /norestart'
$validExitCodes = @(0)

Install-ChocolateyPackage "$packageName" "$installerType" "$silentArgs" "$url32" "$url64" -validExitCodes $validExitCodes
