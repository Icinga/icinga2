Set-PSDebug -Trace 1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

function ThrowOnNativeFailure {
	if (-not $?) {
		throw 'Native failure'
	}
}


# Keep the empty lines below to prevent merge conflicts during backports!

$VsVersion = 2022

$MsvcVersion = '14.3'

$BoostVersion = @(1, 92, 0)

$OpensslVersion = '3_5_9'

$WinFlexBisonVersion = '2.5.24'

switch ($Env:BITS) {
	32 { }
	64 { }
	default {
		$Env:BITS = 64
	}
}


function Download-File {
	param (
		[string]$Url,
		[string]$OutFile
	)

	for ($trial = 1;; ++$trial) {
		try {
			Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing
		} catch {
			if ($trial -ge 2) {
				throw
			}

			continue
		}

		break
	}
}

function Install-Exe {
	param (
		[string]$Url,
		[string]$Dir
	)

	$TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().Guid)
	$ExeFile = Join-Path $TempDir inst.exe

	New-Item -ItemType Directory -Path $TempDir

	Download-File -Url $Url -OutFile $ExeFile

	Start-Process -Wait -FilePath $ExeFile -ArgumentList @('/VERYSILENT', '/INSTALL', '/PASSIVE', '/NORESTART', "/DIR=${Dir}")
	ThrowOnNativeFailure

	Remove-Item -Recurse -Path $TempDir
}


try {
	Get-Command choco
} catch {
	Invoke-Expression (New-Object Net.WebClient).DownloadString('https://chocolatey.org/install.ps1')
	ThrowOnNativeFailure

	$RegEnv = 'Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment'
	$ChocoPath = ";$(Join-Path $Env:AllUsersProfile chocolatey\bin)"

	Set-ItemProperty -Path $RegEnv -Name Path -Value ((Get-ItemProperty -Path $RegEnv -Name Path).Path + $ChocoPath)
	$Env:Path += $ChocoPath
}

# GitHub Actions uses an image that comes with most dependencies preinstalled. Don't install them twice.
if (-not $Env:GITHUB_ACTIONS) {
    choco install -y `
        "visualstudio${VsVersion}community" `
        "visualstudio${VsVersion}-workload-vctools" `
        "visualstudio${VsVersion}-workload-manageddesktop" `
        "visualstudio${VsVersion}-workload-nativedesktop" `
        "visualstudio${VsVersion}-workload-universal" `
        "visualstudio${VsVersion}buildtools" `
        git `
        cmake `
        winflexbison3 `
        netfx-4.6-devpack `
        windows-sdk-8.1 `
        wixtoolset
    ThrowOnNativeFailure
}

# Disable the progress bar for downloads from the Web, which will speed up the entire download process
$Global:ProgressPreference = 'SilentlyContinue';

if ($Env:GITHUB_ACTIONS) {
	$ZipFile = Join-Path ([System.IO.Path]::GetTempPath()) "$([System.Guid]::NewGuid().Guid).zip"

	Download-File -Url "https://packages.icinga.com/windows/dependencies/win_flex_bison-${WinFlexBisonVersion}.zip" -OutFile $ZipFile
	Expand-Archive -Path $ZipFile -DestinationPath (Join-Path $Env:AllUsersProfile chocolatey\bin)

	Remove-Item -Path $ZipFile
}

Install-Exe -Url "https://packages.icinga.com/windows/dependencies/boost_$($BoostVersion -join '_')-msvc-${MsvcVersion}-${Env:BITS}.exe" -Dir "C:\local\boost_$($BoostVersion -join '_')-Win${Env:BITS}"

Install-Exe -Url "https://packages.icinga.com/windows/dependencies/Win${Env:BITS}OpenSSL-${OpensslVersion}.exe" -Dir "C:\local\OpenSSL_${OpensslVersion}-Win${Env:BITS}"
