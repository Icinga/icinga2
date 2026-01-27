<?xml version="1.0" encoding="utf-8"?>
<!-- Do not remove this test for UTF-8: if ??? doesn?t appear as greek uppercase omega letter enclosed in quotation marks, you should use an editor that supports UTF-8, not this one. -->
<!--package xmlns="http://schemas.microsoft.com/packaging/2010/07/nuspec.xsd"-->
<package xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <metadata>
    <!-- Read this before publishing packages to chocolatey.org: https://github.com/chocolatey/chocolatey/wiki/CreatePackages -->
    <id>icinga2</id>
    <title>Icinga 2</title>
    <version>${ICINGA2_VERSION_SAFE}</version>
    <authors>Icinga GmbH</authors>
    <owners>Icinga GmbH</owners>
    <summary>icinga2 - Monitoring Agent for Windows</summary>
    <description>Icinga is an open source monitoring platform which notifies users about host and service outages.</description>
    <projectUrl>https://icinga.com/</projectUrl>
    <tags>icinga2 icinga agent monitoring admin</tags>
    <licenseUrl>https://github.com/Icinga/icinga2/blob/master/LICENSE.md</licenseUrl>
    <releaseNotes>https://github.com/Icinga/icinga2/blob/master/ChangeLog</releaseNotes>
    <docsUrl>https://icinga.com/docs/icinga2/latest/</docsUrl>
    <bugTrackerUrl>https://github.com/Icinga/icinga2/issues</bugTrackerUrl>
    <packageSourceUrl>https://github.com/Icinga/icinga2</packageSourceUrl>
    <projectSourceUrl>https://github.com/Icinga/icinga2</projectSourceUrl>
    <requireLicenseAcceptance>false</requireLicenseAcceptance>
    <iconUrl>https://raw.githubusercontent.com/Icinga/icinga2/master/icinga-app/icinga.ico</iconUrl>
    <dependencies>
      <dependency id='netfx-4.6.2' />
    </dependencies>
  </metadata>
  <files>
    <file src="${CMAKE_CURRENT_BINARY_DIR}/chocolateyInstall.ps1" target="tools" />
    <file src="${CMAKE_CURRENT_SOURCE_DIR}/chocolateyUninstall.ps1" target="tools" />
  </files>
</package>
