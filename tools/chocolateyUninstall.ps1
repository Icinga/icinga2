$instDir = "unset"
$uninstaller = "Uninstall.exe"
$icingaRegistry64bitOS = "hklm:\SOFTWARE\Wow6432Node\Icinga Development Team\ICINGA2"
$icingaRegistry32bitOS = "hklm:\SOFTWARE\Icinga Development Team\ICINGA2"
$found = $false
$validExitCodes = @(0)

if(test-path $icingaRegistry32bitOS) {
  $instDir = (get-itemproperty -literalpath $icingaRegistry32bitOS).'(default)'
  $found = $true
}
elseif(test-path $icingaRegistry64bitOS) {
  $instDir = (get-itemproperty -literalpath $icingaRegistry64bitOS).'(default)'
  $found = $true
}
else {
  Write-Host "Did not find a path in the registry to the Icinga2 folder, did you use the installer?"
} 

if ($found) {
  $packageArgs = "/S ?_="
  $statements = "& `"$instDir\$uninstaller`" $packageArgs`"$instDir`""

  Start-ChocolateyProcessAsAdmin "$statements" -minimized -validExitCodes $validExitCodes
}
