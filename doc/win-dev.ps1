Set-PSDebug -Trace 1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

function ThrowOnNativeFailure {
	if (-not $?) {
		throw 'Native failure'
	}
}


$VsVersion = 2019
$MsvcVersion = '14.2'
$BoostVersion = @(1, 79, 0)
$OpensslVersion = '1_1_1q'

switch ($Env:BITS) {
	32 { }
	64 { }
	default {
		$Env:BITS = 64
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

	for ($trial = 1;; ++$trial) {
		try {
			Invoke-WebRequest -Uri $Url -OutFile $ExeFile -UseBasicParsing
		} catch {
			if ($trial -ge 2) {
				throw
			}

			continue
		}

		break
	}

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
        "visualstudio${VsVersion}-workload-netcoretools" `
        "visualstudio${VsVersion}-workload-vctools" `
        "visualstudio${VsVersion}-workload-manageddesktop" `
        "visualstudio${VsVersion}-workload-nativedesktop" `
        "visualstudio${VsVersion}-workload-universal" `
        "visualstudio${VsVersion}buildtools" `
        git `
        cmake `
        winflexbison3 `
        windows-sdk-8.1 `
        wixtoolset
    ThrowOnNativeFailure
} else {
    choco install -y winflexbison3
    ThrowOnNativeFailure
}


Install-Exe -Url "https://packages.icinga.com/windows/dependencies/boost_$($BoostVersion -join '_')-msvc-${MsvcVersion}-${Env:BITS}.exe" -Dir "C:\local\boost_$($BoostVersion -join '_')-Win${Env:BITS}"

Install-Exe -Url "https://packages.icinga.com/windows/dependencies/Win${Env:BITS}OpenSSL-${OpensslVersion}.exe" -Dir "C:\local\OpenSSL_${OpensslVersion}-Win${Env:BITS}"
