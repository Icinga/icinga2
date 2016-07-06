<CPackWiXPatch>
  <CPackWiXFragment Id="#PRODUCT">
    <Property Id="ALLUSERS">1</Property>
    <Property Id="MSIRESTARTMANAGERCONTROL">Disable</Property>

    <CustomAction Id="XtraUpgradeNSIS" BinaryKey="icinga2_installer" ExeCommand="upgrade-nsis" Execute="deferred" Impersonate="no" />
    <CustomAction Id="XtraInstall" FileKey="CM_FP_sbin.icinga2_installer.exe" ExeCommand="install" Execute="deferred" Impersonate="no" />
    <CustomAction Id="XtraUninstall" FileKey="CM_FP_sbin.icinga2_installer.exe" ExeCommand="uninstall" Execute="deferred" Impersonate="no" />

    <Binary Id="icinga2_installer" SourceFile="$<TARGET_FILE:icinga-installer>" />
    
    <InstallExecuteSequence>
      <!-- Removed for builds with VS2013, kept intact for VS2015 --> 
      <!--
      <Custom Action='CheckForUCRT' Before='LaunchConditions'>
        <![CDATA[Not REMOVE="ALL" AND Not PREVIOUSFOUND AND UCRTINSTALLED = ""]]>
      </Custom>
      -->
      <Custom Action="XtraUpgradeNSIS" After="InstallInitialize">$CM_CP_sbin.icinga2_installer.exe&gt;2</Custom>
      <Custom Action="XtraInstall" Before="InstallFinalize">$CM_CP_sbin.icinga2_installer.exe&gt;2</Custom>
      <Custom Action="XtraUninstall" Before="RemoveExistingProducts">$CM_CP_sbin.icinga2_installer.exe=2</Custom>
    </InstallExecuteSequence>

    <Property Id="WIXUI_EXITDIALOGOPTIONALCHECKBOXTEXT" Value="Run Icinga 2 setup wizard" />

    <Property Id="WixShellExecTarget" Value="[#CM_FP_sbin.Icinga2SetupAgent.exe]" />
    <CustomAction Id="LaunchIcinga2Wizard"
        BinaryKey="WixCA"
        DllEntry="WixShellExec"
        Impersonate="no" />

    <UI>
        <Publish Dialog="ExitDialog" 
            Control="Finish" 
            Event="DoAction" 
            Value="LaunchIcinga2Wizard">WIXUI_EXITDIALOGOPTIONALCHECKBOX = 1 and NOT Installed</Publish>
    </UI>

    <!-- Removed for builds with VS2013, kept intact for VS2015 --> 
    <!--
    <Property Id="UCRTINSTALLED" Secure="yes">
      <DirectorySearch Id="searchSystem2" Path="[SystemFolder]" Depth="0">
        <FileSearch Id="UCRT_FileSearch"
                    Name="ucrtbase.dll"
                    MinVersion="6.2.10585.0"/>
      </DirectorySearch>
    </Property>

    <CustomAction Id="CheckForUCRT" Error="Installation cannot continue because the Microsoft Universal C Runtime is not installed. Please see https://support.microsoft.com/en-us/kb/2999226 for more details." />
    -->
  </CPackWiXFragment>
</CPackWiXPatch>
