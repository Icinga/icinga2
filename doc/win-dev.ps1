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
$BoostVersion = @(1, 71, 0)
$OpensslVersion = '1_1_1k'

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


choco install -y "visualstudio${VsVersion}community"
ThrowOnNativeFailure

choco install -y "visualstudio${VsVersion}-workload-netcoretools"
ThrowOnNativeFailure

choco install -y "visualstudio${VsVersion}-workload-vctools"
ThrowOnNativeFailure

choco install -y "visualstudio${VsVersion}-workload-manageddesktop"
ThrowOnNativeFailure

choco install -y "visualstudio${VsVersion}-workload-nativedesktop"
ThrowOnNativeFailure

choco install -y "visualstudio${VsVersion}-workload-universal"
ThrowOnNativeFailure

choco install -y "visualstudio${VsVersion}buildtools"
ThrowOnNativeFailure


choco install -y git
ThrowOnNativeFailure

choco install -y cmake
ThrowOnNativeFailure

choco install -y winflexbison3
ThrowOnNativeFailure

choco install -y windows-sdk-8.1
ThrowOnNativeFailure

choco install -y wixtoolset
ThrowOnNativeFailure


Install-Exe -Url "https://packages.icinga.com/windows/dependencies/boost_$($BoostVersion -join '_')-msvc-${MsvcVersion}-${Env:BITS}.exe" -Dir "C:\local\boost_$($BoostVersion -join '_')-Win${Env:BITS}"

Install-Exe -Url "https://packages.icinga.com/windows/dependencies/Win${Env:BITS}OpenSSL-${OpensslVersion}.exe" -Dir "C:\local\OpenSSL_${OpensslVersion}-Win${Env:BITS}"
