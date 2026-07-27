# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds the per-instance WiX authoring for the MSI package.
#
# The installer supports a build-time configurable number of side-by-side instances. Instance 1 is
# the product as it has always been shipped (same UpgradeCode, product name, directories and service
# name), so existing installations keep upgrading normally. Instances 2..N are MSI instance
# transforms embedded into the very same MSI and are selected either interactively or with
#
#   msiexec /i Icinga2.msi TRANSFORMS=:Instance2 MSINEWINSTANCE=1
#
# The variables computed here are substituted into icinga2.wixpatch.cmake.in.

set(ICINGA2_MSI_INSTANCES "3" CACHE STRING
  "Number of side-by-side instances supported by the Windows MSI package")
set(ICINGA2_MSI_INSTANCE_UPGRADE_GUIDS "" CACHE STRING
  "UpgradeCodes for instances 2..N (semicolon separated). Derived deterministically if empty.")

# The identity of instance 1, i.e. of the product as it has always been shipped. The root
# CMakeLists.txt feeds these into CPACK_PACKAGE_INSTALL_DIRECTORY and CPACK_WIX_UPGRADE_GUID.
# The spelling is the one earlier packages shipped, so that the default installation directory
# stays byte for byte what it has always been.
set(ICINGA2_MSI_INSTALL_DIRECTORY "ICINGA2")
set(ICINGA2_MSI_UPGRADE_GUID "52F2BEAA-4DF0-4C3E-ABDC-C0F61DE4DF8A")

if(NOT MSVC)
  return()
endif()

if(NOT ICINGA2_MSI_INSTANCES MATCHES "^[1-9]$")
  message(FATAL_ERROR "ICINGA2_MSI_INSTANCES must be an integer between 1 and 9, got '${ICINGA2_MSI_INSTANCES}'")
endif()

math(EXPR ICINGA2_MSI_EXTRA_INSTANCES "${ICINGA2_MSI_INSTANCES} - 1")
list(LENGTH ICINGA2_MSI_INSTANCE_UPGRADE_GUIDS _icinga2_msi_guid_count)

if(_icinga2_msi_guid_count GREATER 0 AND _icinga2_msi_guid_count LESS ICINGA2_MSI_EXTRA_INSTANCES)
  message(FATAL_ERROR
    "ICINGA2_MSI_INSTANCES=${ICINGA2_MSI_INSTANCES} needs ${ICINGA2_MSI_EXTRA_INSTANCES} UpgradeCodes, "
    "but ICINGA2_MSI_INSTANCE_UPGRADE_GUIDS only provides ${_icinga2_msi_guid_count}.")
endif()

# The UpgradeCode identifies an instance for its entire lifetime: changing how it is derived orphans
# every installation of that instance, as the next package no longer recognizes it as an upgrade.
# The namespace is the product's original UpgradeCode, the names are stable strings.
#
# The Program Files folder property differs between the 32 and the 64 bit package; ProgramFiles64Folder
# is not available in a 32 bit package.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_icinga2_msi_program_files "ProgramFiles64Folder")
else()
  set(_icinga2_msi_program_files "ProgramFilesFolder")
endif()

set(_tpl_transform [==[
      <Instance Id="@_id@" ProductCode="*" UpgradeCode="{@_upgrade@}" ProductName="@_product@" />
]==])

# Everything an already installed instance recorded about itself. These searches are the only way the
# instance picker knows whether an instance exists, and they restore the settings of an instance that
# is being upgraded (a major upgrade is a fresh install as far as property defaults are concerned).
set(_tpl_search [==[
    <Property Id="ICINGA_INSTANCE_@_n@_PATH">
      <RegistrySearch Id="IcingaInstance@_n@Path" Root="HKLM" Key="@_settings_key@" Name="InstallLocation" Type="raw" />
    </Property>
    <Property Id="ICINGA_INSTANCE_@_n@_SERVICE">
      <RegistrySearch Id="IcingaInstance@_n@Service" Root="HKLM" Key="@_settings_key@" Name="ServiceName" Type="raw" />
    </Property>
    <Property Id="ICINGA_INSTANCE_@_n@_DATADIR">
      <RegistrySearch Id="IcingaInstance@_n@DataDir" Root="HKLM" Key="@_settings_key@" Name="DataDir" Type="raw" />
    </Property>
    <Property Id="ICINGA_INSTANCE_@_n@_VERSION">
      <RegistrySearch Id="IcingaInstance@_n@Version" Root="HKLM" Key="@_settings_key@" Name="Version" Type="raw" />
    </Property>
    <Property Id="ICINGA_INSTANCE_@_n@_INFO" Value="not installed" />
    <SetProperty Id="ICINGA_INSTANCE_@_n@_INFO" Action="IcingaInfo@_n@Version" Sequence="ui"
        Value="installed, version [ICINGA_INSTANCE_@_n@_VERSION]" After="AppSearch">ICINGA_INSTANCE_@_n@_VERSION</SetProperty>
]==])

# Restore what the instance recorded, unless the value was given on the command line.
set(_tpl_restore [==[
    <SetProperty Id="INSTALL_ROOT" Action="IcingaRestore@_n@InstallRoot" Sequence="both"
        Value="[ICINGA_INSTANCE_@_n@_PATH]" After="AppSearch"><![CDATA[INSTANCEID = "@_id@" AND NOT INSTALL_ROOT AND ICINGA_INSTANCE_@_n@_PATH]]></SetProperty>
    <SetProperty Id="ICINGA_SERVICE_NAME" Action="IcingaRestore@_n@Service" Sequence="both"
        Value="[ICINGA_INSTANCE_@_n@_SERVICE]" After="AppSearch"><![CDATA[INSTANCEID = "@_id@" AND NOT ICINGA_SERVICE_NAME AND ICINGA_INSTANCE_@_n@_SERVICE]]></SetProperty>
    <SetProperty Id="ICINGA_DATA_DIR" Action="IcingaRestore@_n@DataDir" Sequence="both"
        Value="[ICINGA_INSTANCE_@_n@_DATADIR]" After="AppSearch"><![CDATA[INSTANCEID = "@_id@" AND NOT ICINGA_DATA_DIR AND ICINGA_INSTANCE_@_n@_DATADIR]]></SetProperty>
    <SetProperty Id="ICINGA_INSTANCE_INSTALLED" Action="IcingaRestore@_n@Installed" Sequence="both"
        Value="1" After="AppSearch"><![CDATA[INSTANCEID = "@_id@" AND ICINGA_INSTANCE_@_n@_PATH]]></SetProperty>
]==])

# Build time defaults, applied only where nothing was restored or passed in. Every instance resolves
# its service name, data directory and event log source explicitly, because those three end up in the
# instance marker key, in instance.ini and in the conflict check of the options page - leaving them
# empty for the default instance would make it invisible to all three.
set(_tpl_default [==[
    <SetProperty Id="ICINGA_SERVICE_NAME" Action="IcingaDefault@_n@Service" Sequence="both"
        Value="@_service@" Before="CostFinalize"><![CDATA[INSTANCEID = "@_id@" AND NOT ICINGA_SERVICE_NAME]]></SetProperty>
    <SetProperty Id="ICINGA_DATA_DIR" Action="IcingaDefault@_n@DataDir" Sequence="both"
        Value="[CommonAppDataFolder]@_data_dir@" Before="CostFinalize"><![CDATA[INSTANCEID = "@_id@" AND NOT ICINGA_DATA_DIR]]></SetProperty>
    <SetProperty Id="ICINGA_EVENTLOG_SOURCE" Action="IcingaDefault@_n@EventLog" Sequence="both"
        Value="@_eventlog@" Before="CostFinalize"><![CDATA[INSTANCEID = "@_id@"]]></SetProperty>
]==])

# Instance 1 keeps its directory defaults from the Directory table so that its authoring stays what it
# has always been; the additional instances redirect all three of them. A directory property only
# redirects its tree when it is set before costing.
set(_tpl_default_dirs [==[
    <SetProperty Id="INSTALL_ROOT" Action="IcingaDefault@_n@InstallRoot" Sequence="both"
        Value="[@_program_files@]@_install_dir@\" Before="CostFinalize"><![CDATA[INSTANCEID = "@_id@" AND NOT INSTALL_ROOT]]></SetProperty>
    <SetProperty Id="ICINGA_BROWSE_DIR" Action="IcingaDefault@_n@BrowseDir" Sequence="both"
        Value="[CommonAppDataFolder]@_data_dir@\" Before="CostFinalize"><![CDATA[INSTANCEID = "@_id@"]]></SetProperty>
    <SetProperty Id="PROGRAM_MENU_FOLDER" Action="IcingaDefault@_n@MenuFolder" Sequence="both"
        Value="[ProgramMenuFolder]@_menu_folder@\" Before="CostFinalize"><![CDATA[INSTANCEID = "@_id@"]]></SetProperty>
]==])

# Component/@Guid="*" derives the GUID from the resolved key path, so a literal per-instance registry
# key is what makes these components - and only these - unique per instance and therefore removable
# per instance. Everything CPack generates shares its GUIDs across all instances; see the comment in
# icinga2.wixpatch.cmake.in above XtraCleanup.
set(_tpl_component [==[
      <Component Id="IcingaInstanceMarker@_n@" Guid="*" Directory="INSTALL_ROOT">
        <Condition><![CDATA[INSTANCEID = "@_id@"]]></Condition>
        <RegistryKey Root="HKLM" Key="@_settings_key@" Action="createAndRemoveOnUninstall">
          <RegistryValue Name="ProductCode" Type="string" Value="[ProductCode]" KeyPath="yes" />
          <RegistryValue Name="InstallLocation" Type="string" Value="[INSTALL_ROOT]" />
          <RegistryValue Name="ServiceName" Type="string" Value="[ICINGA_SERVICE_NAME]" />
          <RegistryValue Name="DataDir" Type="string" Value="[ICINGA_DATA_DIR]" />
          <RegistryValue Name="EventLogSource" Type="string" Value="[ICINGA_EVENTLOG_SOURCE]" />
          <RegistryValue Name="Version" Type="string" Value="[ProductVersion]" />
        </RegistryKey>
        <RemoveFile Id="IcingaRemoveInstanceIni@_n@" Name="instance.ini" On="uninstall" Directory="INSTALL_ROOT" />
      </Component>
]==])

# Write the path to eventprovider.dll to the registry so that the Event Viewer is able to find the
# message definitions and properly displays our log messages. The source name has to be unique per
# instance, otherwise uninstalling one instance breaks the event log of the others.
#
# See also: https://docs.microsoft.com/en-us/windows/win32/eventlog/reporting-an-event
set(_tpl_eventlog_component [==[
      <Component Id="@_eventlog_component@" Guid="*" Directory="INSTALL_ROOT">
        <Condition><![CDATA[INSTANCEID = "@_id@"]]></Condition>
        <RegistryKey Root="HKLM" Key="SYSTEM\CurrentControlSet\Services\EventLog\Application\@_eventlog@" Action="createAndRemoveOnUninstall">
          <RegistryValue Name="EventMessageFile" Type="string" Value="[#CM_FP_sbin.eventprovider.dll]" />
        </RegistryKey>
      </Component>
]==])

# One row on the instance overview per instance: a label with the recorded state and a button that
# maintains the instance (repair, upgrade or remove). Both only exist for installed instances - the
# conditions are evaluated when the dialog is created, long after AppSearch. Individual radio
# buttons cannot be hidden, which is why the rows are built from stand-alone controls.
set(_tpl_row [==[
            <Control Id="InstanceLabel@_n@" Type="Text" X="20" Y="@_label_y@" Width="250" Height="12" NoPrefix="yes"
                Text="@_label@: [ICINGA_INSTANCE_@_n@_INFO]">
                <Condition Action="hide"><![CDATA[NOT (@_installed_cond@)]]></Condition>
                <Condition Action="show"><![CDATA[@_installed_cond@]]></Condition>
            </Control>
            <Control Id="InstanceManage@_n@" Type="PushButton" X="280" Y="@_row_y@" Width="70" Height="16" Text="Manage...">
                <Condition Action="hide"><![CDATA[NOT (@_installed_cond@)]]></Condition>
                <Condition Action="show"><![CDATA[@_installed_cond@]]></Condition>
@_manage_publish@            </Control>
]==])

# Maintaining instance 1 simply continues this session: it runs without a transform, so it already
# is instance 1, and AppSearch/FindRelatedProducts have classified it as maintenance or upgrade.
set(_tpl_manage_return [==[
                <Publish Event="EndDialog" Value="Return">1</Publish>
]==])

# Every other instance needs its transform, which can only be applied while the database is being
# opened, so the installer restarts for it. No MSINEWINSTANCE: the instance already exists, and
# passing it anyway would create a second product with the same instance id.
set(_tpl_manage_relaunch [==[
                <Publish Property="ICINGA_RELAUNCH_INSTANCE" Value="@_id@" Order="1">1</Publish>
                <Publish Property="ICINGA_RELAUNCH_NEW" Value="{}" Order="2">1</Publish>
                <Publish Event="SpawnDialog" Value="IcingaRestartDlg" Order="3">1</Publish>
                <Publish Event="DoAction" Value="IcingaRelaunch" Order="4">1</Publish>
                <Publish Event="EndDialog" Value="Exit" Order="5">1</Publish>
]==])

# Which instance a new installation would get: the first one that is not installed. Each condition
# is self-contained (all lower instances installed, this one not), so the relative order of the
# SetProperty actions does not matter. UI sequence only - the overview page is the only consumer.
set(_tpl_next_free [==[
    <SetProperty Id="ICINGA_NEXT_FREE_INSTANCE" Action="IcingaNextFree@_n@" Sequence="ui"
        Value="@_id@" After="AppSearch"><![CDATA[@_first_free_cond@]]></SetProperty>
    <SetProperty Id="ICINGA_NEXT_FREE_SERVICE" Action="IcingaNextFreeService@_n@" Sequence="ui"
        Value="@_service@" After="AppSearch"><![CDATA[@_first_free_cond@]]></SetProperty>
]==])

# Refuse to reuse the service name or the data directory of a different instance: two instances
# sharing a data directory would corrupt each other's state.
set(_tpl_conflict [==[(ICINGA_INSTANCE_@_n@_SERVICE AND ICINGA_SERVICE_NAME ~= ICINGA_INSTANCE_@_n@_SERVICE AND INSTANCEID <> "@_id@") OR (ICINGA_INSTANCE_@_n@_DATADIR AND ICINGA_DATA_DIR ~= ICINGA_INSTANCE_@_n@_DATADIR AND INSTANCEID <> "@_id@") OR ]==])

set(ICINGA2_WIX_INSTANCE_TRANSFORMS "")
set(ICINGA2_WIX_INSTANCE_SEARCHES "")
set(ICINGA2_WIX_INSTANCE_DEFAULTS "")
set(ICINGA2_WIX_INSTANCE_COMPONENTS "")
set(ICINGA2_WIX_INSTANCE_ROWS "")
set(ICINGA2_WIX_INSTANCE_STATE "")
set(ICINGA2_WIX_INSTANCE_CONFLICT "")

# "Is instance n installed?" as an MSI condition, and its accumulations across all instances.
# Instances 2..N leave a marker key behind; instance 1 additionally has to account for
# installations made by packages from before instance support, which have no marker - but the
# product itself is visible to Windows Installer, as this session runs without a transform and
# therefore carries instance 1's identity.
set(_icinga2_msi_any_installed "")
set(_icinga2_msi_lower_installed "")

set(_icinga2_msi_row_spacing 18)
set(_icinga2_msi_first_row_y 58)

foreach(_n RANGE 1 ${ICINGA2_MSI_INSTANCES})
  set(_id "Instance${_n}")
  set(_settings_key "SOFTWARE\\Icinga GmbH\\Icinga 2\\Instances\\${_id}")
  set(_program_files "${_icinga2_msi_program_files}")

  if(_n EQUAL 1)
    set(_installed_cond "ICINGA_INSTANCE_1_PATH OR Installed OR WIX_UPGRADE_DETECTED OR WIX_DOWNGRADE_DETECTED")
  else()
    set(_installed_cond "ICINGA_INSTANCE_${_n}_PATH")
  endif()

  set(_first_free_cond "NOT (${_installed_cond})")

  if(_icinga2_msi_lower_installed)
    string(APPEND _first_free_cond " AND ${_icinga2_msi_lower_installed}")
    string(APPEND _icinga2_msi_any_installed " OR ")
    string(APPEND _icinga2_msi_lower_installed " AND ")
  endif()

  string(APPEND _icinga2_msi_any_installed "(${_installed_cond})")
  string(APPEND _icinga2_msi_lower_installed "(${_installed_cond})")

  math(EXPR _row_y "${_icinga2_msi_first_row_y} + (${_n} - 1) * ${_icinga2_msi_row_spacing}")
  math(EXPR _label_y "${_row_y} + 3")

  if(_n EQUAL 1)
    set(_product "Icinga 2")
    set(_service "icinga2")
    set(_eventlog "Icinga 2")
    set(_install_dir "${ICINGA2_MSI_INSTALL_DIRECTORY}")
    set(_data_dir "icinga2")
    set(_menu_folder "")
    set(_label "Instance 1 - ${_service} (default)")
    # Keep the component id and the registry key of the pre-existing authoring so that its generated
    # GUID does not change and upgrades of installations made by earlier packages stay intact.
    set(_eventlog_component "EventProviderRegistryEntry")
    set(_upgrade "${ICINGA2_MSI_UPGRADE_GUID}")
    set(_manage_publish "${_tpl_manage_return}")
  else()
    math(EXPR _guid_index "${_n} - 2")

    if(_icinga2_msi_guid_count GREATER 0)
      list(GET ICINGA2_MSI_INSTANCE_UPGRADE_GUIDS ${_guid_index} _upgrade)
      string(TOUPPER "${_upgrade}" _upgrade)
      string(REGEX REPLACE "^{|}$" "" _upgrade "${_upgrade}")
    else()
      string(UUID _upgrade NAMESPACE "${ICINGA2_MSI_UPGRADE_GUID}"
        NAME "icinga2-msi-instance-${_n}" TYPE SHA1 UPPER)
    endif()

    set(_product "Icinga 2 (Instance ${_n})")
    set(_service "icinga2-${_n}")
    set(_eventlog "Icinga 2 Instance ${_n}")
    set(_install_dir "${ICINGA2_MSI_INSTALL_DIRECTORY}-${_n}")
    set(_data_dir "icinga2-${_n}")
    set(_menu_folder "Icinga 2 Instance ${_n}")
    set(_label "Instance ${_n} - ${_service}")
    set(_eventlog_component "EventProviderRegistryEntry${_n}")

    string(CONFIGURE "${_tpl_transform}" _out @ONLY)
    string(APPEND ICINGA2_WIX_INSTANCE_TRANSFORMS "${_out}")

    string(CONFIGURE "${_tpl_default_dirs}" _out @ONLY)
    string(APPEND ICINGA2_WIX_INSTANCE_DEFAULTS "${_out}")

    string(CONFIGURE "${_tpl_manage_relaunch}" _manage_publish @ONLY)
  endif()

  string(CONFIGURE "${_tpl_search}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_SEARCHES "${_out}")

  string(CONFIGURE "${_tpl_restore}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_DEFAULTS "${_out}")

  string(CONFIGURE "${_tpl_default}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_DEFAULTS "${_out}")

  string(CONFIGURE "${_tpl_component}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_COMPONENTS "${_out}")

  string(CONFIGURE "${_tpl_eventlog_component}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_COMPONENTS "${_out}")

  string(CONFIGURE "${_tpl_row}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_ROWS "${_out}")

  string(CONFIGURE "${_tpl_next_free}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_STATE "${_out}")

  string(CONFIGURE "${_tpl_conflict}" _out @ONLY)
  string(APPEND ICINGA2_WIX_INSTANCE_CONFLICT "${_out}")

  message(STATUS "MSI instance ${_n}: ${_product}, service ${_service}, upgrade code {${_upgrade}}")
endforeach()

if(ICINGA2_MSI_INSTANCES EQUAL 1)
  # Without additional instances there is nothing to pick and nothing to transform. The picker dialog
  # stays in the package but is never shown, so the installer behaves exactly as it did before.
  set(ICINGA2_WIX_INSTANCE_TRANSFORMS "")
  set(ICINGA2_WIX_MULTI_INSTANCE_PROPERTY "")
else()
  set(ICINGA2_WIX_INSTANCE_TRANSFORMS
    "    <InstanceTransforms Property=\"INSTANCEID\">\n${ICINGA2_WIX_INSTANCE_TRANSFORMS}    </InstanceTransforms>\n")
  set(ICINGA2_WIX_MULTI_INSTANCE_PROPERTY
    "    <Property Id=\"ICINGA_MULTI_INSTANCE\" Value=\"1\" />\n")
endif()

# The trailing "OR " of the last conflict term keeps the generated condition composable.
string(APPEND ICINGA2_WIX_INSTANCE_CONFLICT "0")

# Whether anything is installed at all is what decides whether the instance overview is shown: on a
# clean machine setup goes straight to a fresh installation of the default instance.
string(APPEND ICINGA2_WIX_INSTANCE_STATE
  "    <SetProperty Id=\"ICINGA_ANY_INSTANCE\" Action=\"IcingaAnyInstance\" Sequence=\"ui\"
        Value=\"1\" After=\"AppSearch\"><![CDATA[${_icinga2_msi_any_installed}]]></SetProperty>\n")

# The explanatory note gets whatever is left between the instance list and the button row; with many
# instances there is nothing left, so it is dropped rather than drawn over the buttons. Which of the
# two texts appears depends on whether a free instance is left.
math(EXPR _icinga2_msi_note_y "${_icinga2_msi_first_row_y} + ${ICINGA2_MSI_INSTANCES} * ${_icinga2_msi_row_spacing} + 6")
math(EXPR _icinga2_msi_note_height "230 - ${_icinga2_msi_note_y}")

if(_icinga2_msi_note_height GREATER 34)
  set(_icinga2_msi_note_height 34)
endif()

if(_icinga2_msi_note_height LESS 12)
  set(ICINGA2_WIX_INSTANCE_NOTE "")
else()
  set(_tpl_note [==[
            <Control Id="InstanceNote" Type="Text" X="20" Y="@_icinga2_msi_note_y@" Width="330" Height="@_icinga2_msi_note_height@" NoPrefix="yes"
                Text="Every instance is a separate product with its own installation directory, data directory and Windows service. A new instance will be installed as service '[ICINGA_NEXT_FREE_SERVICE]'.">
                <Condition Action="hide"><![CDATA[NOT ICINGA_NEXT_FREE_INSTANCE]]></Condition>
                <Condition Action="show"><![CDATA[ICINGA_NEXT_FREE_INSTANCE]]></Condition>
            </Control>
            <Control Id="InstanceFullNote" Type="Text" X="20" Y="@_icinga2_msi_note_y@" Width="330" Height="@_icinga2_msi_note_height@" NoPrefix="yes"
                Text="All instances supported by this package are already installed.">
                <Condition Action="hide"><![CDATA[ICINGA_NEXT_FREE_INSTANCE]]></Condition>
                <Condition Action="show"><![CDATA[NOT ICINGA_NEXT_FREE_INSTANCE]]></Condition>
            </Control>
]==])
  string(CONFIGURE "${_tpl_note}" ICINGA2_WIX_INSTANCE_NOTE @ONLY)
endif()
