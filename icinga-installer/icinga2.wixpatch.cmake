<CPackWiXPatch>
  <CPackWiXFragment Id="#PRODUCT">
    <Property Id="ALLUSERS">1</Property>
    <Property Id="MSIRESTARTMANAGERCONTROL">Disable</Property>

    <PropertyRef Id="WIX_IS_NETFRAMEWORK_46_OR_LATER_INSTALLED" />
    <Condition Message='This application requires .NET Framework 4.6 or higher. Please install the .NET Framework then run this installer again.'>
      <![CDATA[Installed OR WIX_IS_NETFRAMEWORK_46_OR_LATER_INSTALLED]]>
    </Condition>

    <CustomAction Id="XtraUpgradeNSIS" BinaryKey="icinga2_installer" ExeCommand="upgrade-nsis" Execute="deferred" Impersonate="no" />
    <CustomAction Id="XtraInstall" FileKey="CM_FP_sbin.icinga2_installer.exe" ExeCommand="install" Execute="deferred" Impersonate="no" />
    <CustomAction Id="XtraUninstall" FileKey="CM_FP_sbin.icinga2_installer.exe" ExeCommand="uninstall" Execute="deferred" Impersonate="no" />

    <Binary Id="icinga2_installer" SourceFile="$<TARGET_FILE:icinga-installer>" />

    <InstallExecuteSequence>
      <Custom Action="XtraUpgradeNSIS" After="InstallInitialize">$CM_CP_sbin.icinga2_installer.exe&gt;2 AND NOT SUPPRESS_XTRA</Custom>
      <Custom Action="XtraInstall" Before="InstallFinalize">$CM_CP_sbin.icinga2_installer.exe&gt;2 AND NOT SUPPRESS_XTRA</Custom>
      <Custom Action="XtraUninstall" Before="RemoveExistingProducts">$CM_CP_sbin.icinga2_installer.exe=2 AND NOT SUPPRESS_XTRA</Custom>
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
  </CPackWiXFragment>
</CPackWiXPatch>
