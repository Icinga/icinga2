# Build Icinga 2 on Windows

The Icinga Project is providing Windows MSI packages under https://packages.icinga.com/windows/

> **Note:**
> This is a developer documentation on how to build Icinga 2 on Windows!

Also see [INSTALL.md](INSTALL.md) for Linux build instructions.

## Requirements

* 32 or 64-bit system
* Visual Studio >= 14 2015
* CMake >= 2.6
* OpenSSL >= 1.0.1
* Flex and Bison

## Install Requirements

**Visual Studio**

Download from [visualstudio.com](https://www.visualstudio.com/en/downloads/)

The Community Edition is available for free, and is what we use to build.

Workloads to install:
* C++ Desktop
* .NET Desktop

**OpenSSL for Icinga**

See our [openssl-windows GitHub project](https://github.com/Icinga/openssl-windows).

You will need to install a binary dist version to 'C:\\Program Files\\OpenSSL'.

There is a Powershell script to help you downloading: `.\tools\win32\download-openssl.ps1`

**Chocolatey**

A simple package manager for Windows, please see [install instructions](https://chocolatey.org/install).

**Git**

Best to use Chocolatey, see [package details](https://chocolatey.org/packages/git).

```
choco install git
```

**Flex / Bison**

Best to use Chocolatey, see [package details](https://chocolatey.org/packages/winflexbison3).

```
choco install winflexbison3
```

**CMake**

Best to use Chocolatey, see [package details](https://chocolatey.org/packages/cmake)
or download from: [cmake.org](https://cmake.org/download/)

```
choco install cmake
```

**WIX**

Best to use Chocolatey, see [package details](https://chocolatey.org/packages/wixtoolset).

```
choco install wixtoolset
```

**Boost**

Download third party Windows binaries from: [boost.org](http://www.boost.org/users/download/)

For example: `https://dl.bintray.com/boostorg/release/1.65.1/binaries/boost_1_65_1-msvc-14.1-64.exe`

*Warnings:*
* Must match your Visual Studio version!
* CMake might not support the latest Boost version (we used CMake 3.10 and Boost 1_65_1)

Run the installer exe.

## Build Icinga 2

Run with VC Native x64 Command Prompt:

```
powershell .\tools\win32\configure.ps1
powershell .\tools\win32\build.ps1
powershell .\tools\win32\test.ps1
```

See these scripts for details.

## AppVeyor

We are building [Icinga 2 with AppVeyor](https://ci.appveyor.com/project/icinga/icinga2) for testing and CI integration.

Please check `appveyor.yml` for our instructions.
