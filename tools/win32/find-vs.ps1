# Query a single `vswhere` property for the selected VS install. The auto-detection can be
# overriden by setting the VS_INSTALL_PATH environment variable.
# By default, the latest install that has the C++ (x86/x64) toolset will be selected.
# If `vswhere` itself cannot be found or if it did not successfully return the requested
# property an error will be thrown.
function Get-VSProperty($property) {
  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    throw "Could not find vswhere.exe at: ${vswhere}"
  }

  if (Test-Path env:VS_INSTALL_PATH) {
    $selector = @('-path', $env:VS_INSTALL_PATH)
  } else {
    $selector = @('-latest', '-products', '*',
                  '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64')
  }

  $value = & $vswhere @selector -property $property
  if ($LastExitCode -ne 0 -or [string]::IsNullOrEmpty($value)) {
    throw "vswhere could not determine '${property}' for the selected Visual Studio installation"
  }

  return $value
}

# Root directory of the VS installation for locating the vcvars*.bat file.
function Get-VSInstallPath {
  return Get-VSProperty 'installationPath'
}

# CMake can auto-detect the latest installed VS version, but that doesn't have to align with the
# version we have selected above, for example when using the VS_INSTALL_PATH override.
# Therefore it is generated from the same `vswhere` result that is used to detect the VS path.
# An error is thrown if the VS version we have selected is not supported by CMake.
function Get-VSCMakeGenerator {
  $major = (Get-VSProperty 'installationVersion').Split('.')[0]

  # Get the supported VS versions from CMake and check match it against the one we have.
  $generator = & cmake.exe --help |
    Select-String -Pattern "(Visual Studio $major\b[^=]*)=" |
    ForEach-Object { $_.Matches.Groups[1].Value.Trim() } |
    Select-Object -First 1

  if (-not $generator) {
    throw "Installed CMake has no 'Visual Studio $major' generator (CMake too old for this VS?); set CMAKE_GENERATOR to override"
  }

  return $generator
}
