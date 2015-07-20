$packageName = 'icinga2'
$installerType = 'exe'
$url = 'http://packages.icinga.org/windows/Icinga2-v2.3.8.exe'
$silentArgs = '/S'
$validExitCodes = @(0)

Install-ChocolateyPackage "$packageName" "$installerType" "$silentArgs" "$url" -validExitCodes $validExitCodes
