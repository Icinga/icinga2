$packageName = 'icinga2'
$installerType = 'msi'
$url32 = 'https://packages.icinga.com/windows/Icinga2-v${SPEC_VERSION}-x86.msi'
$url64 = 'https://packages.icinga.com/windows/Icinga2-v${SPEC_VERSION}-x86_64.msi'
$silentArgs = '/qn /norestart'
$validExitCodes = @(0)

Install-ChocolateyPackage "$packageName" "$installerType" "$silentArgs" "$url32" "$url64" -validExitCodes $validExitCodes
