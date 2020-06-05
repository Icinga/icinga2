Set-PsDebug -Trace 1

if(-not (Test-Path "$($env:ProgramData)\chocolatey\choco.exe")) {
	throw "Could not find Choco executable. Abort."
}

if (-not (Test-Path env:ICINGA2_BUILDPATH)) {
  $env:ICINGA2_BUILDPATH = '.\build'
}

if(-not (Test-Path "$($env:ICINGA2_BUILDPATH)\choco\chocolateyInstall.ps1.template")) {
	throw "Could not find Chocolatey install script template. Abort."
}

$chocoInstallScriptTemplatePath = "$($env:ICINGA2_BUILDPATH)\choco\chocolateyInstall.ps1.template"
$chocoInstallScript = Get-Content $chocoInstallScriptTemplatePath

if(-not (Test-Path "$($env:ICINGA2_BUILDPATH)\*-x86.msi")) {
	throw "Could not find Icinga 2 32 bit MSI package. Abort."
}

$hashMSIpackage32 =  Get-FileHash "$($env:ICINGA2_BUILDPATH)\*-x86.msi"
Write-Output "File Hash for 32 bit MSI package: $($hashMSIpackage32.Hash)."

if(-not (Test-Path "$($env:ICINGA2_BUILDPATH)\*-x86_64.msi")) {
	throw "Could not find Icinga 2 64 bit MSI package. Abort."
}

$hashMSIpackage64 =  Get-FileHash "$($env:ICINGA2_BUILDPATH)\*-x86_64.msi"
Write-Output "File Hash for 32 bit MSI package: $($hashMSIpackage64.Hash)"

$chocoInstallScript = $chocoInstallScript.Replace("%CHOCO_32BIT_CHECKSUM%", "$($hashMSIpackage32.Hash)")
$chocoInstallScript = $chocoInstallScript.Replace("%CHOCO_64BIT_CHECKSUM%", "$($hashMSIpackage64.Hash)")
Write-Output $chocoInstallScript

Set-Content -Path "$($env:ICINGA2_BUILDPATH)\choco\chocolateyInstall.ps1" -Value $chocoInstallScript

cd "$($env:ICINGA2_BUILDPATH)\choco"
& "$($env:ProgramData)\chocolatey\choco.exe" "pack"
cd "..\.."

Move-Item -Path "$($env:ICINGA2_BUILDPATH)\choco\*.nupkg" -Destination "$($env:ICINGA2_BUILDPATH)"