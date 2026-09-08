# why that env handling, see
# https://help.appveyor.com/discussions/questions/18777-how-to-use-vcvars64bat-from-powershell#comment_44999171

Set-PsDebug -Trace 1

. "$PSScriptRoot\find-vs.ps1"

$SOURCE = Get-Location

if (Test-Path env:ICINGA2_BUILDPATH) {
  $BUILD = $env:ICINGA2_BUILDPATH
} else {
  $BUILD = "${SOURCE}\Build"
}

if (-not (Test-Path $BUILD)) {
  mkdir $BUILD | Out-Null
}

if (Test-Path env:BITS) {
  $bits = $env:BITS
} else {
  $bits = 64
}

$vcvars = "$(Get-VSInstallPath)\VC\Auxiliary\Build\vcvars${bits}.bat"
if (-not (Test-Path $vcvars)) {
  throw "Could not get Build environment script at location: ${vcvars}"
}

cmd.exe /c "call `"${vcvars}`" && set > `"${BUILD}\vcvars.txt`""
if ($LastExitCode -ne 0) {
  throw "Could not load Build environment from: ${vcvars}"
}

# Load environment for PowerShell
Get-Content "${BUILD}\vcvars.txt" | Foreach-Object {
  if ($_ -match "^(VSCMD.*?)=(.*)$") {
	Set-Content ("env:" + $matches[1]) $matches[2]
  }
}
