$packageName = 'icinga2'
$installerType = 'msi'
$url32 = 'https://packages.icinga.com/windows/Icinga2-v${ICINGA2_VERSION_SAFE}-x86.msi'
$url64 = 'https://packages.icinga.com/windows/Icinga2-v${ICINGA2_VERSION_SAFE}-x86_64.msi'
$silentArgs = '/qn /norestart'
$validExitCodes = @(0)

Install-ChocolateyPackage "$packageName" "$installerType" "$silentArgs" "$url32" "$url64" -validExitCodes $validExitCodes
