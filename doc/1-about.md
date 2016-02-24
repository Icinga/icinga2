# <a id="about-icinga2"></a> About Icinga 2

## <a id="what-is-icinga2"></a> What is Icinga 2?

Icinga 2 is an open source monitoring system which checks the availability of
your network resources, notifies users of outages, and generates performance
data for reporting.

Scalable and extensible, Icinga 2 can monitor large, complex environments across
multiple locations.

## <a id="licensing"></a> Licensing

Icinga 2 and the Icinga 2 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
LICENSE file included in the source package.

## <a id="support"></a> Support

Support for Icinga 2 is available in a number of ways. Please have a look at
the [support overview page](https://support.icinga.org).

## <a id="contribute"></a> Contribute

There are many ways to contribute to Icinga - whether it be sending patches,
testing, reporting bugs, or reviewing and updating the documentation. Every
contribution is appreciated!

Please get in touch with the Icinga team at https://www.icinga.org/community/.

If you want to help update this documentation please read
[this howto](https://wiki.icinga.org/display/community/Update+the+Icinga+2+documentation).

### <a id="development-info"></a> Icinga 2 Development

You can follow Icinga 2's development closely by checking
out these resources:

* [Development Bug Tracker](https://dev.icinga.org/projects/i2): [How to report a bug?](https://www.icinga.org/icinga/faq/)
* Git Repositories: [main mirror on icinga.org](https://git.icinga.org/?p=icinga2.git;a=summary) [release mirror at github.com](https://github.com/Icinga/icinga2)
* [Git Checkins Mailinglist](https://lists.icinga.org/mailman/listinfo/icinga-checkins)
* [Development](https://lists.icinga.org/mailman/listinfo/icinga-devel) and [Users](https://lists.icinga.org/mailman/listinfo/icinga-users) Mailinglists
* [#icinga-devel on irc.freenode.net](http://webchat.freenode.net/?channels=icinga-devel) including a Git Commit Bot

For general support questions, please refer to the [community support channels](https://support.icinga.org).

### <a id="how-to-report-bug-feature-requests"></a> How to Report a Bug or Feature Request

More details in the [Icinga FAQ](https://www.icinga.org/icinga/faq/).

* [Register](https://accounts.icinga.org/register) an Icinga account.
* Create a new issue at the [Icinga 2 Development Tracker](https://dev.icinga.org/projects/i2).
* When reporting a bug, please include the details described in the [Troubleshooting](16-troubleshooting.md#troubleshooting-information-required) chapter (version, configs, logs, etc).

## <a id="whats-new"></a> What's New

### What's New in Version 2.4.3

#### Bugfixes

* Bug [11211](https://dev.icinga.org/issues/11211 "Bug 11211"): Permission problem after running icinga2 node wizard
* Bug [11212](https://dev.icinga.org/issues/11212 "Bug 11212"): Wrong permissions for files in /var/cache/icinga2/*

### What's New in Version 2.4.2

#### Changes

* ITL
    * Additional arguments for check_disk
    * Fix incorrect path for the check_hpasm plugin
    * New command: check_iostat
    * Fix incorrect variable names for the check_impi plugin
* Cluster
    * Improve cluster performance
    * Fix connection handling problems (multiple connections for the same endpoint)
* Performance improvements for the DB IDO modules
* Lots and lots of various other bugfixes
* Documentation updates

#### Feature

* Feature [10660](https://dev.icinga.org/issues/10660 "Feature 10660"): Add CMake flag for disabling the unit tests
* Feature [10777](https://dev.icinga.org/issues/10777 "Feature 10777"): Add check_iostat to ITL
* Feature [10787](https://dev.icinga.org/issues/10787 "Feature 10787"): Add "-x" parameter in command definition for disk-windows CheckCommand
* Feature [10807](https://dev.icinga.org/issues/10807 "Feature 10807"): Raise a config error for "Checkable" objects in global zones
* Feature [10857](https://dev.icinga.org/issues/10857 "Feature 10857"): DB IDO: Add a log message when the connection handling is completed
* Feature [10860](https://dev.icinga.org/issues/10860 "Feature 10860"): Log DB IDO query queue stats
* Feature [10880](https://dev.icinga.org/issues/10880 "Feature 10880"): "setting up check plugins" section should be enhanced with package manager examples
* Feature [10920](https://dev.icinga.org/issues/10920 "Feature 10920"): Add Timeout parameter to snmpv3 check
* Feature [10947](https://dev.icinga.org/issues/10947 "Feature 10947"): Add example how to use custom functions in attributes
* Feature [10964](https://dev.icinga.org/issues/10964 "Feature 10964"): Troubleshooting: Explain how to fetch the executed command
* Feature [10988](https://dev.icinga.org/issues/10988 "Feature 10988"): Support TLSv1.1 and TLSv1.2 for the cluster transport encryption
* Feature [11037](https://dev.icinga.org/issues/11037 "Feature 11037"): Add String#trim
* Feature [11138](https://dev.icinga.org/issues/11138 "Feature 11138"): Checkcommand Disk : Option Freespace-ignore-reserved

#### Bugfixes

* Bug [7287](https://dev.icinga.org/issues/7287 "Bug 7287"): Re-checks scheduling w/ retry_interval
* Bug [8714](https://dev.icinga.org/issues/8714 "Bug 8714"): Add priority queue for disconnect/programstatus update events
* Bug [8976](https://dev.icinga.org/issues/8976 "Bug 8976"): DB IDO: notification_id for contact notifications is out of range
* Bug [10226](https://dev.icinga.org/issues/10226 "Bug 10226"): Icinga2 reload timeout results in killing old and new process because of systemd
* Bug [10449](https://dev.icinga.org/issues/10449 "Bug 10449"): Livestatus log query - filter "class" yields empty results
* Bug [10458](https://dev.icinga.org/issues/10458 "Bug 10458"): Incorrect SQL command for creating the user of the PostgreSQL DB for the IDO
* Bug [10460](https://dev.icinga.org/issues/10460 "Bug 10460"): A PgSQL DB for the IDO can't be created w/ UTF8
* Bug [10497](https://dev.icinga.org/issues/10497 "Bug 10497"): check_memory and check_swap plugins do unit conversion and rounding before percentage calculations resulting in imprecise percentages
* Bug [10544](https://dev.icinga.org/issues/10544 "Bug 10544"): check_network performance data in invalid format
* Bug [10554](https://dev.icinga.org/issues/10554 "Bug 10554"): Non-UTF8 characters from plugins causes IDO to fail
* Bug [10655](https://dev.icinga.org/issues/10655 "Bug 10655"): API queries cause memory leaks
* Bug [10700](https://dev.icinga.org/issues/10700 "Bug 10700"): Crash in ExternalCommandListener
* Bug [10711](https://dev.icinga.org/issues/10711 "Bug 10711"): Zone::CanAccessObject is very expensive
* Bug [10713](https://dev.icinga.org/issues/10713 "Bug 10713"): ApiListener::ReplayLog can block with a lot of clients
* Bug [10714](https://dev.icinga.org/issues/10714 "Bug 10714"): API is not working on wheezy
* Bug [10724](https://dev.icinga.org/issues/10724 "Bug 10724"): Remove the local zone name question in node wizard
* Bug [10728](https://dev.icinga.org/issues/10728 "Bug 10728"): node wizard does not remember user defined port
* Bug [10736](https://dev.icinga.org/issues/10736 "Bug 10736"): Missing num_hosts_pending in /v1/status/CIB
* Bug [10739](https://dev.icinga.org/issues/10739 "Bug 10739"): Crash on startup with incorrect directory permissions
* Bug [10744](https://dev.icinga.org/issues/10744 "Bug 10744"): build of icinga2 with gcc 4.4.7 segfaulting with ido
* Bug [10745](https://dev.icinga.org/issues/10745 "Bug 10745"): ITL check command possibly mistyped variable names
* Bug [10748](https://dev.icinga.org/issues/10748 "Bug 10748"): Missing path in mkdir() exceptions
* Bug [10760](https://dev.icinga.org/issues/10760 "Bug 10760"): Disallow lambda expressions where side-effect-free expressions are not allowed
* Bug [10765](https://dev.icinga.org/issues/10765 "Bug 10765"): Avoid duplicate config and status updates on startup
* Bug [10773](https://dev.icinga.org/issues/10773 "Bug 10773"): chcon partial context error in safe-reload prevents reload
* Bug [10779](https://dev.icinga.org/issues/10779 "Bug 10779"): Wrong postgresql-setup initdb command for RHEL7
* Bug [10780](https://dev.icinga.org/issues/10780 "Bug 10780"): The hpasm check command is using the PluginDir constant
* Bug [10784](https://dev.icinga.org/issues/10784 "Bug 10784"): Incorrect information in --version on Linux
* Bug [10806](https://dev.icinga.org/issues/10806 "Bug 10806"): Missing SUSE repository for monitoring plugins documentation
* Bug [10817](https://dev.icinga.org/issues/10817 "Bug 10817"): Failed IDO query for icinga_downtimehistory
* Bug [10818](https://dev.icinga.org/issues/10818 "Bug 10818"): Use NodeName in null and random checks
* Bug [10819](https://dev.icinga.org/issues/10819 "Bug 10819"): Cluster config sync ignores zones.d from API packages
* Bug [10824](https://dev.icinga.org/issues/10824 "Bug 10824"): Windows build fails with latest git master
* Bug [10825](https://dev.icinga.org/issues/10825 "Bug 10825"): Missing documentation for API packages zones.d config sync
* Bug [10826](https://dev.icinga.org/issues/10826 "Bug 10826"): Build error with older CMake versions on VERSION_LESS compare
* Bug [10828](https://dev.icinga.org/issues/10828 "Bug 10828"): Relative path in include_zones does not work
* Bug [10829](https://dev.icinga.org/issues/10829 "Bug 10829"): IDO breaks when writing to icinga_programstatus with latest snapshots
* Bug [10830](https://dev.icinga.org/issues/10830 "Bug 10830"): Config validation doesn't fail when templates are used as object names
* Bug [10852](https://dev.icinga.org/issues/10852 "Bug 10852"): Formatting problem in "Advanced Filter" chapter
* Bug [10855](https://dev.icinga.org/issues/10855 "Bug 10855"): Implement support for re-ordering groups of IDO queries
* Bug [10862](https://dev.icinga.org/issues/10862 "Bug 10862"): Evaluate if CanExecuteQuery/FieldToEscapedString lead to exceptions on !m_Connected
* Bug [10867](https://dev.icinga.org/issues/10867 "Bug 10867"): "repository add" cli command writes invalid "type" attribute
* Bug [10883](https://dev.icinga.org/issues/10883 "Bug 10883"): Icinga2 crashes in IDO when removing a comment
* Bug [10890](https://dev.icinga.org/issues/10890 "Bug 10890"): Remove superfluous #ifdef
* Bug [10891](https://dev.icinga.org/issues/10891 "Bug 10891"): is_active in IDO is only re-enabled on "every second" restart
* Bug [10908](https://dev.icinga.org/issues/10908 "Bug 10908"): Typos in the "troubleshooting" section of the documentation
* Bug [10923](https://dev.icinga.org/issues/10923 "Bug 10923"): API actions: Decide whether fixed: false is the right default
* Bug [10931](https://dev.icinga.org/issues/10931 "Bug 10931"): Exception stack trace on icinga2 client when the master reloads the configuration
* Bug [10932](https://dev.icinga.org/issues/10932 "Bug 10932"): Cluster config sync: Ensure that /var/lib/icinga2/api/zones/* exists
* Bug [10935](https://dev.icinga.org/issues/10935 "Bug 10935"): Logrotate on systemd distros should use systemctl not service
* Bug [10948](https://dev.icinga.org/issues/10948 "Bug 10948"): Icinga state file corruption with temporary file creation
* Bug [10956](https://dev.icinga.org/issues/10956 "Bug 10956"): Compiler warnings in lib/remote/base64.cpp
* Bug [10959](https://dev.icinga.org/issues/10959 "Bug 10959"): Better explaination for array values in "disk" CheckCommand docs
* Bug [10963](https://dev.icinga.org/issues/10963 "Bug 10963"): high load and memory consumption on icinga2 agent v2.4.1
* Bug [10968](https://dev.icinga.org/issues/10968 "Bug 10968"): Race condition when using systemd unit file
* Bug [10974](https://dev.icinga.org/issues/10974 "Bug 10974"): Modified attributes do not work for the IcingaApplication object w/ external commands
* Bug [10979](https://dev.icinga.org/issues/10979 "Bug 10979"): Mistake in mongodb command definition (mongodb_replicaset)
* Bug [10981](https://dev.icinga.org/issues/10981 "Bug 10981"): Incorrect name in AUTHORS
* Bug [10989](https://dev.icinga.org/issues/10989 "Bug 10989"): Escaped sequences not properly generated with 'node update-config'
* Bug [10991](https://dev.icinga.org/issues/10991 "Bug 10991"): Stream buffer size is 512 bytes, could be raised
* Bug [10998](https://dev.icinga.org/issues/10998 "Bug 10998"): Incorrect IdoPgSqlConnection Example in Documentation
* Bug [11006](https://dev.icinga.org/issues/11006 "Bug 11006"): Segfault in ApiListener::ConfigUpdateObjectAPIHandler
* Bug [11014](https://dev.icinga.org/issues/11014 "Bug 11014"): Check event duplication with parallel connections involved
* Bug [11019](https://dev.icinga.org/issues/11019 "Bug 11019"): next_check noise in the IDO
* Bug [11020](https://dev.icinga.org/issues/11020 "Bug 11020"): Master reloads with agents generate false alarms
* Bug [11065](https://dev.icinga.org/issues/11065 "Bug 11065"): Deleting an object via API does not disable it in DB IDO
* Bug [11074](https://dev.icinga.org/issues/11074 "Bug 11074"): Partially missing escaping in doc/7-icinga-template-library.md
* Bug [11075](https://dev.icinga.org/issues/11075 "Bug 11075"): Outdated link to icingaweb2-module-nagvis
* Bug [11083](https://dev.icinga.org/issues/11083 "Bug 11083"): Ensure that config sync updates are always sent on reconnect
* Bug [11085](https://dev.icinga.org/issues/11085 "Bug 11085"): Crash in ConfigItem::RunWithActivationContext
* Bug [11088](https://dev.icinga.org/issues/11088 "Bug 11088"): API queries on non-existant objects cause exception
* Bug [11096](https://dev.icinga.org/issues/11096 "Bug 11096"): Windows build fails on InterlockedIncrement type
* Bug [11103](https://dev.icinga.org/issues/11103 "Bug 11103"): Problem with hostgroup_members table cleanup
* Bug [11111](https://dev.icinga.org/issues/11111 "Bug 11111"): Clean up unused variables a bit
* Bug [11118](https://dev.icinga.org/issues/11118 "Bug 11118"): Cluster WQ thread dies after fork()
* Bug [11122](https://dev.icinga.org/issues/11122 "Bug 11122"): Connections are not cleaned up properly
* Bug [11132](https://dev.icinga.org/issues/11132 "Bug 11132"): YYYY-MM-DD time specs are parsed incorrectly
* Bug [11178](https://dev.icinga.org/issues/11178 "Bug 11178"): Documentation: Unescaped pipe character in tables
* Bug [11179](https://dev.icinga.org/issues/11179 "Bug 11179"): CentOS 5 doesn't support epoll_create1
* Bug [11204](https://dev.icinga.org/issues/11204 "Bug 11204"): "node setup" tries to chown() files before they're created

### What's New in Version 2.4.1

#### Changes

* ITL
    * Add running_kernel_use_sudo option for the running_kernel check
* Configuration
    * Add global constants: `PlatformName`. `PlatformVersion`, `PlatformKernel` and `PlatformKernelVersion`
* CLI
    * Use NodeName and ZoneName constants for 'node setup' and 'node wizard' 

#### Feature

* Feature [10622](https://dev.icinga.org/issues/10622 "Feature 10622"): Add by_ssh_options argument for the check_by_ssh plugin
* Feature [10693](https://dev.icinga.org/issues/10693 "Feature 10693"): Add running_kernel_use_sudo option for the running_kernel check
* Feature [10716](https://dev.icinga.org/issues/10716 "Feature 10716"): Use NodeName and ZoneName constants for 'node setup' and 'node wizard'

#### Bugfixes

* Bug [10528](https://dev.icinga.org/issues/10528 "Bug 10528"): Documentation example in "Access Object Attributes at Runtime" doesn't work correctly
* Bug [10615](https://dev.icinga.org/issues/10615 "Bug 10615"): Build fails on SLES 11 SP3 with GCC 4.8
* Bug [10632](https://dev.icinga.org/issues/10632 "Bug 10632"): "node wizard" does not ask user to verify SSL certificate
* Bug [10641](https://dev.icinga.org/issues/10641 "Bug 10641"): API setup command incorrectly overwrites existing certificates
* Bug [10643](https://dev.icinga.org/issues/10643 "Bug 10643"): Icinga 2 crashes when ScheduledDowntime objects are used
* Bug [10645](https://dev.icinga.org/issues/10645 "Bug 10645"): Documentation for schedule-downtime is missing required paremeters
* Bug [10648](https://dev.icinga.org/issues/10648 "Bug 10648"): lib/base/process.cpp SIGSEGV on Debian squeeze / RHEL 6
* Bug [10661](https://dev.icinga.org/issues/10661 "Bug 10661"): Incorrect web inject URL in documentation
* Bug [10663](https://dev.icinga.org/issues/10663 "Bug 10663"): Incorrect redirect for stderr in /usr/lib/icinga2/prepare-dirs
* Bug [10667](https://dev.icinga.org/issues/10667 "Bug 10667"): Indentation in command-plugins.conf
* Bug [10677](https://dev.icinga.org/issues/10677 "Bug 10677"): node wizard checks for /var/lib/icinga2/ca directory but not the files
* Bug [10690](https://dev.icinga.org/issues/10690 "Bug 10690"): CLI command 'repository add' doesn't work
* Bug [10692](https://dev.icinga.org/issues/10692 "Bug 10692"): Fix typos in the documentation
* Bug [10708](https://dev.icinga.org/issues/10708 "Bug 10708"): Windows setup wizard crashes when InstallDir registry key is not set
* Bug [10710](https://dev.icinga.org/issues/10710 "Bug 10710"): Incorrect path for icinga2 binary in development documentation
* Bug [10720](https://dev.icinga.org/issues/10720 "Bug 10720"): Remove --master_zone from --help because it is currently not implemented

### What's New in Version 2.4.0

#### Changes

* API
    * RESTful API with basic auth or client certificates
    * Filters, types, permissions
    * configuration package management
    * query/create/modify/delete config objects at runtime
    * status queries for global stats
    * actions (e.g. acknowledge all service problems)
    * event streams
* ITL and Plugin Check Command definitions
    * The 'running_kernel' check command was moved to the plugins-contrib section. You have to update your config to include 'plugins-contrib'
* Configuration
    * The global constants Enable* and Vars have been removed. Use the IcingaApplication object attributes instead.
* Features
    * New Graphite tree. Please check the documentation how enable the legacy schema.
    * IcingaStatusWriter feature has been deprecated and will be removed in future versions.
    * Modified attributes are not exposed as bit mask to external interfaces anymore (api related changes). External commands like CHANGE_*_MODATTR have been removed.

#### Feature

* Feature [7709](https://dev.icinga.org/issues/7709 "Feature 7709"): Validators should be implemented in (auto-generated) native code
* Feature [8093](https://dev.icinga.org/issues/8093 "Feature 8093"): Add icinga, cluster, cluster-zone check information to the ApiListener status handler
* Feature [8149](https://dev.icinga.org/issues/8149 "Feature 8149"): graphite writer should pass "-" in host names and "." in perf data
* Feature [8666](https://dev.icinga.org/issues/8666 "Feature 8666"): Allow some of the Array and Dictionary methods to be inlined by the compiler
* Feature [8688](https://dev.icinga.org/issues/8688 "Feature 8688"): Add embedded DB IDO version health check
* Feature [8689](https://dev.icinga.org/issues/8689 "Feature 8689"): Add support for current and current-1 db ido schema version
* Feature [8690](https://dev.icinga.org/issues/8690 "Feature 8690"): 'icinga2 console' should serialize temporary attributes (rather than just config + state)
* Feature [8738](https://dev.icinga.org/issues/8738 "Feature 8738"): Implement support for CLIENT_MULTI_STATEMENTS
* Feature [8741](https://dev.icinga.org/issues/8741 "Feature 8741"): Deprecate IcingaStatusWriter feature
* Feature [8775](https://dev.icinga.org/issues/8775 "Feature 8775"): Move the base command templates into libmethods
* Feature [8776](https://dev.icinga.org/issues/8776 "Feature 8776"): Implement support for libedit
* Feature [8791](https://dev.icinga.org/issues/8791 "Feature 8791"): Refactor the startup process
* Feature [8832](https://dev.icinga.org/issues/8832 "Feature 8832"): Implement constructor-style casts
* Feature [8842](https://dev.icinga.org/issues/8842 "Feature 8842"): Add support for the C++11 keyword 'override'
* Feature [8867](https://dev.icinga.org/issues/8867 "Feature 8867"): Use DebugHint information when reporting validation errors
* Feature [8890](https://dev.icinga.org/issues/8890 "Feature 8890"): Move implementation code from thpp files into separate files
* Feature [8922](https://dev.icinga.org/issues/8922 "Feature 8922"): Avoid unnecessary dictionary lookups
* Feature [9044](https://dev.icinga.org/issues/9044 "Feature 9044"): Remove the ScopeCurrent constant
* Feature [9068](https://dev.icinga.org/issues/9068 "Feature 9068"): Implement sandbox mode for the config parser
* Feature [9074](https://dev.icinga.org/issues/9074 "Feature 9074"): Basic API framework
* Feature [9076](https://dev.icinga.org/issues/9076 "Feature 9076"): Reflection support for the API
* Feature [9077](https://dev.icinga.org/issues/9077 "Feature 9077"): Implement filters for the API
* Feature [9078](https://dev.icinga.org/issues/9078 "Feature 9078"): Event stream support for the API
* Feature [9079](https://dev.icinga.org/issues/9079 "Feature 9079"): Implement status queries for the API
* Feature [9080](https://dev.icinga.org/issues/9080 "Feature 9080"): Add commands (actions) for the API
* Feature [9081](https://dev.icinga.org/issues/9081 "Feature 9081"): Add modified attribute support for the API
* Feature [9082](https://dev.icinga.org/issues/9082 "Feature 9082"): Runtime configuration for the API
* Feature [9083](https://dev.icinga.org/issues/9083 "Feature 9083"): Configuration file management for the API
* Feature [9084](https://dev.icinga.org/issues/9084 "Feature 9084"): Enable the ApiListener by default
* Feature [9085](https://dev.icinga.org/issues/9085 "Feature 9085"): Certificate-based authentication for the API
* Feature [9086](https://dev.icinga.org/issues/9086 "Feature 9086"): Password-based authentication for the API
* Feature [9087](https://dev.icinga.org/issues/9087 "Feature 9087"): Create default administrative user
* Feature [9088](https://dev.icinga.org/issues/9088 "Feature 9088"): API permissions
* Feature [9091](https://dev.icinga.org/issues/9091 "Feature 9091"): API status queries
* Feature [9093](https://dev.icinga.org/issues/9093 "Feature 9093"): Changelog for modified attributes
* Feature [9095](https://dev.icinga.org/issues/9095 "Feature 9095"): Disallow changes for certain config attributes at runtime
* Feature [9096](https://dev.icinga.org/issues/9096 "Feature 9096"): Dependency tracking for objects
* Feature [9098](https://dev.icinga.org/issues/9098 "Feature 9098"): Update modules to support adding and removing objects at runtime
* Feature [9099](https://dev.icinga.org/issues/9099 "Feature 9099"): Implement support for writing configuration files
* Feature [9100](https://dev.icinga.org/issues/9100 "Feature 9100"): Multiple sources for zone configuration tree
* Feature [9101](https://dev.icinga.org/issues/9101 "Feature 9101"): Commands for adding and removing objects
* Feature [9102](https://dev.icinga.org/issues/9102 "Feature 9102"): Support validating configuration changes
* Feature [9103](https://dev.icinga.org/issues/9103 "Feature 9103"): Staging for configuration validation
* Feature [9104](https://dev.icinga.org/issues/9104 "Feature 9104"): Implement config file management commands
* Feature [9105](https://dev.icinga.org/issues/9105 "Feature 9105"): API Documentation
* Feature [9175](https://dev.icinga.org/issues/9175 "Feature 9175"): Move 'running_kernel' check command to plugins-contrib 'operating system' section
* Feature [9286](https://dev.icinga.org/issues/9286 "Feature 9286"): DB IDO/Livestatus: Add zone object table w/ endpoint members
* Feature [9414](https://dev.icinga.org/issues/9414 "Feature 9414"): "-Wno-deprecated-register" compiler option breaks builds on SLES 11
* Feature [9447](https://dev.icinga.org/issues/9447 "Feature 9447"): Implement support for HTTP
* Feature [9448](https://dev.icinga.org/issues/9448 "Feature 9448"): Define RESTful url schema
* Feature [9461](https://dev.icinga.org/issues/9461 "Feature 9461"): New Graphite schema
* Feature [9470](https://dev.icinga.org/issues/9470 "Feature 9470"): Implement URL parser
* Feature [9471](https://dev.icinga.org/issues/9471 "Feature 9471"): Implement ApiUser type
* Feature [9594](https://dev.icinga.org/issues/9594 "Feature 9594"): Implement base64 de- and encoder
* Feature [9614](https://dev.icinga.org/issues/9614 "Feature 9614"): Register ServiceOK, ServiceWarning, HostUp, etc. as constants
* Feature [9647](https://dev.icinga.org/issues/9647 "Feature 9647"): Move url to /lib/remote from /lib/base
* Feature [9689](https://dev.icinga.org/issues/9689 "Feature 9689"): Add exceptions for Utility::MkDir{,P}
* Feature [9693](https://dev.icinga.org/issues/9693 "Feature 9693"): Add Array::FromVector() method
* Feature [9698](https://dev.icinga.org/issues/9698 "Feature 9698"): Implement support for X-HTTP-Method-Override
* Feature [9704](https://dev.icinga.org/issues/9704 "Feature 9704"): String::Trim() should return a new string rather than modifying the current string
* Feature [9705](https://dev.icinga.org/issues/9705 "Feature 9705"): Add real path sanity checks to provided file paths
* Feature [9723](https://dev.icinga.org/issues/9723 "Feature 9723"): Documentation for config management API
* Feature [9768](https://dev.icinga.org/issues/9768 "Feature 9768"): Update the url parsers behaviour
* Feature [9777](https://dev.icinga.org/issues/9777 "Feature 9777"): Make Comments and Downtime types available as ConfigObject type in the API
* Feature [9794](https://dev.icinga.org/issues/9794 "Feature 9794"): Setting global variables with i2tcl doesn't work
* Feature [9849](https://dev.icinga.org/issues/9849 "Feature 9849"): Validation for modified attributes
* Feature [9850](https://dev.icinga.org/issues/9850 "Feature 9850"): Re-implement events for attribute changes
* Feature [9851](https://dev.icinga.org/issues/9851 "Feature 9851"): Remove GetModifiedAttributes/SetModifiedAttributes
* Feature [9852](https://dev.icinga.org/issues/9852 "Feature 9852"): Implement support for . in modify_attribute
* Feature [9859](https://dev.icinga.org/issues/9859 "Feature 9859"): Implement global modified attributes
* Feature [9866](https://dev.icinga.org/issues/9866 "Feature 9866"): Implement support for attaching GDB to the Icinga process on crash
* Feature [9914](https://dev.icinga.org/issues/9914 "Feature 9914"): Rename DynamicObject/DynamicType to ConfigObject/ConfigType
* Feature [9919](https://dev.icinga.org/issues/9919 "Feature 9919"): Allow comments when parsing JSON
* Feature [9921](https://dev.icinga.org/issues/9921 "Feature 9921"): Implement the 'base' field for the Type class
* Feature [9926](https://dev.icinga.org/issues/9926 "Feature 9926"): Ensure that runtime config objects are persisted on disk
* Feature [9927](https://dev.icinga.org/issues/9927 "Feature 9927"): Figure out how to sync dynamically created objects inside the cluster
* Feature [9929](https://dev.icinga.org/issues/9929 "Feature 9929"): Add override keyword for all relevant methods
* Feature [9930](https://dev.icinga.org/issues/9930 "Feature 9930"): Document Object#clone
* Feature [9931](https://dev.icinga.org/issues/9931 "Feature 9931"): Implement Object#clone and rename Array/Dictionary#clone to shallow_clone
* Feature [9933](https://dev.icinga.org/issues/9933 "Feature 9933"): Implement support for indexers in ConfigObject::RestoreAttribute
* Feature [9935](https://dev.icinga.org/issues/9935 "Feature 9935"): Implement support for restoring modified attributes
* Feature [9937](https://dev.icinga.org/issues/9937 "Feature 9937"): Add package attribute for ConfigObject and set its origin
* Feature [9940](https://dev.icinga.org/issues/9940 "Feature 9940"): Implement support for filter_vars
* Feature [9944](https://dev.icinga.org/issues/9944 "Feature 9944"): Add String::ToLower/ToUpper
* Feature [9946](https://dev.icinga.org/issues/9946 "Feature 9946"): Remove debug messages in HttpRequest class
* Feature [9953](https://dev.icinga.org/issues/9953 "Feature 9953"): Rename config/modules to config/packages
* Feature [9960](https://dev.icinga.org/issues/9960 "Feature 9960"): Implement ignore_on_error keyword
* Feature [10017](https://dev.icinga.org/issues/10017 "Feature 10017"): Use an AST node for the 'library' keyword
* Feature [10038](https://dev.icinga.org/issues/10038 "Feature 10038"): Add plural_name field to /v1/types
* Feature [10039](https://dev.icinga.org/issues/10039 "Feature 10039"): URL class improvements
* Feature [10042](https://dev.icinga.org/issues/10042 "Feature 10042"): Implement a demo API client: Icinga Studio
* Feature [10060](https://dev.icinga.org/issues/10060 "Feature 10060"): Implement joins for status queries
* Feature [10116](https://dev.icinga.org/issues/10116 "Feature 10116"): Add global status handler for the API
* Feature [10186](https://dev.icinga.org/issues/10186 "Feature 10186"): Make ConfigObject::{G,S}etField() method public
* Feature [10194](https://dev.icinga.org/issues/10194 "Feature 10194"): Sanitize error status codes and messages
* Feature [10202](https://dev.icinga.org/issues/10202 "Feature 10202"): Add documentation for api-users.conf and app.conf
* Feature [10209](https://dev.icinga.org/issues/10209 "Feature 10209"): Rename statusqueryhandler to objectqueryhandler
* Feature [10212](https://dev.icinga.org/issues/10212 "Feature 10212"): Move /v1/<type> to /v1/objects/<type>
* Feature [10243](https://dev.icinga.org/issues/10243 "Feature 10243"): Provide keywords to retrieve the current file name at parse time
* Feature [10257](https://dev.icinga.org/issues/10257 "Feature 10257"): Change object version to timestamps for diff updates on config sync
* Feature [10329](https://dev.icinga.org/issues/10329 "Feature 10329"): Pretty-print arrays and dictionaries when converting them to strings
* Feature [10368](https://dev.icinga.org/issues/10368 "Feature 10368"): Document that modified attributes require accept_config for cluster/clients
* Feature [10374](https://dev.icinga.org/issues/10374 "Feature 10374"): Add check command nginx_status
* Feature [10383](https://dev.icinga.org/issues/10383 "Feature 10383"): DB IDO should provide its connected state via /v1/status
* Feature [10385](https://dev.icinga.org/issues/10385 "Feature 10385"): Add 'support' tracker to changelog.py
* Feature [10387](https://dev.icinga.org/issues/10387 "Feature 10387"): Use the API for "icinga2 console"
* Feature [10388](https://dev.icinga.org/issues/10388 "Feature 10388"): Log a warning message on unauthorized http request
* Feature [10392](https://dev.icinga.org/issues/10392 "Feature 10392"): Original attributes list in IDO
* Feature [10393](https://dev.icinga.org/issues/10393 "Feature 10393"): Hide internal attributes
* Feature [10394](https://dev.icinga.org/issues/10394 "Feature 10394"): Add getter for endpoint 'connected' attribute
* Feature [10407](https://dev.icinga.org/issues/10407 "Feature 10407"): Remove api.cpp, api.hpp
* Feature [10409](https://dev.icinga.org/issues/10409 "Feature 10409"): Add documentation for apply+for in the language reference chapter
* Feature [10423](https://dev.icinga.org/issues/10423 "Feature 10423"): Ability to set port on SNMP Checks
* Feature [10431](https://dev.icinga.org/issues/10431 "Feature 10431"): Add the name for comments/downtimes next to legacy_id to DB IDO
* Feature [10441](https://dev.icinga.org/issues/10441 "Feature 10441"): Rewrite man page
* Feature [10479](https://dev.icinga.org/issues/10479 "Feature 10479"): Use ZoneName variable for parent_zone in node update-config
* Feature [10482](https://dev.icinga.org/issues/10482 "Feature 10482"): Documentation: Reorganize Livestatus and alternative frontends
* Feature [10503](https://dev.icinga.org/issues/10503 "Feature 10503"): Missing parameters for check jmx4perl
* Feature [10507](https://dev.icinga.org/issues/10507 "Feature 10507"): Add check command negate
* Feature [10509](https://dev.icinga.org/issues/10509 "Feature 10509"): Change GetLastStateUp/Down to host attributes
* Feature [10511](https://dev.icinga.org/issues/10511 "Feature 10511"): Add check command mysql
* Feature [10513](https://dev.icinga.org/issues/10513 "Feature 10513"): Add ipv4/ipv6 only to tcp and http CheckCommand
* Feature [10522](https://dev.icinga.org/issues/10522 "Feature 10522"): Change output format for 'icinga2 console'
* Feature [10547](https://dev.icinga.org/issues/10547 "Feature 10547"): Icinga 2 script debugger
* Feature [10548](https://dev.icinga.org/issues/10548 "Feature 10548"): Implement CSRF protection for the API
* Feature [10549](https://dev.icinga.org/issues/10549 "Feature 10549"): Change 'api setup' into a manual step while configuring the API
* Feature [10551](https://dev.icinga.org/issues/10551 "Feature 10551"): Change object query result set
* Feature [10566](https://dev.icinga.org/issues/10566 "Feature 10566"): Enhance programmatic examples for the API docs
* Feature [10574](https://dev.icinga.org/issues/10574 "Feature 10574"): Mention wxWidget (optional) requirement in INSTALL.md
* Feature [10575](https://dev.icinga.org/issues/10575 "Feature 10575"): Documentation for /v1/console
* Feature [10576](https://dev.icinga.org/issues/10576 "Feature 10576"): Explain variable names for joined objects in filter expressions
* Feature [10577](https://dev.icinga.org/issues/10577 "Feature 10577"): Documentation for the script debugger
* Feature [10591](https://dev.icinga.org/issues/10591 "Feature 10591"): Explain DELETE for config stages/packages
* Feature [10630](https://dev.icinga.org/issues/10630 "Feature 10630"): Update wxWidgets documentation for Icinga Studio

#### Bugfixes

* Bug [8822](https://dev.icinga.org/issues/8822 "Bug 8822"): Update OpenSSL for the Windows builds
* Bug [8823](https://dev.icinga.org/issues/8823 "Bug 8823"): Don't allow users to instantiate the StreamLogger class
* Bug [8830](https://dev.icinga.org/issues/8830 "Bug 8830"): Make default notifications include users from host.vars.notification.mail.users
* Bug [8865](https://dev.icinga.org/issues/8865 "Bug 8865"): Failed assertion in IdoMysqlConnection::FieldToEscapedString
* Bug [8907](https://dev.icinga.org/issues/8907 "Bug 8907"): Validation fails even though field is not required
* Bug [8924](https://dev.icinga.org/issues/8924 "Bug 8924"): Specify pidfile for status_of_proc in the init script
* Bug [8952](https://dev.icinga.org/issues/8952 "Bug 8952"): Crash in VMOps::FunctionCall
* Bug [8989](https://dev.icinga.org/issues/8989 "Bug 8989"): pgsql driver does not have latest mysql changes synced
* Bug [9015](https://dev.icinga.org/issues/9015 "Bug 9015"): Compiler warnings with latest HEAD 5ac5f98
* Bug [9027](https://dev.icinga.org/issues/9027 "Bug 9027"): PostgreSQL schema sets default timestamps w/o time zone
* Bug [9053](https://dev.icinga.org/issues/9053 "Bug 9053"): icinga demo module can not be built
* Bug [9188](https://dev.icinga.org/issues/9188 "Bug 9188"): Remove incorrect 'ignore where' expression from 'ssh' apply example
* Bug [9455](https://dev.icinga.org/issues/9455 "Bug 9455"): Fix incorrect datatype for the check_source column in icinga_statehistory table
* Bug [9547](https://dev.icinga.org/issues/9547 "Bug 9547"): Wrong vars changed handler in api events
* Bug [9576](https://dev.icinga.org/issues/9576 "Bug 9576"): Overflow in freshness_threshold column (smallint) w/ DB IDO MySQL
* Bug [9590](https://dev.icinga.org/issues/9590 "Bug 9590"): 'node wizard/setup' should always generate new CN certificates
* Bug [9703](https://dev.icinga.org/issues/9703 "Bug 9703"): Problem with child nodes in http url registry
* Bug [9735](https://dev.icinga.org/issues/9735 "Bug 9735"): Broken cluster config sync w/o include_zones
* Bug [9778](https://dev.icinga.org/issues/9778 "Bug 9778"): Accessing field ID 0 ("prototype") fails
* Bug [9793](https://dev.icinga.org/issues/9793 "Bug 9793"): Operator - should not work with "" and numbers
* Bug [9795](https://dev.icinga.org/issues/9795 "Bug 9795"): ScriptFrame's 'Self' attribute gets corrupted when an expression throws an exception
* Bug [9813](https://dev.icinga.org/issues/9813 "Bug 9813"): win32 build: S_ISDIR is undefined
* Bug [9843](https://dev.icinga.org/issues/9843 "Bug 9843"): console autocompletion should take into account parent classes' prototypes
* Bug [9868](https://dev.icinga.org/issues/9868 "Bug 9868"): Crash in ScriptFrame::~ScriptFrame
* Bug [9872](https://dev.icinga.org/issues/9872 "Bug 9872"): Color codes in console prompt break line editing
* Bug [9876](https://dev.icinga.org/issues/9876 "Bug 9876"): Crash during cluster log replay
* Bug [9879](https://dev.icinga.org/issues/9879 "Bug 9879"): Missing conf.d or zones.d cause parse failure
* Bug [9911](https://dev.icinga.org/issues/9911 "Bug 9911"): Do not let API users create objects with invalid names
* Bug [9966](https://dev.icinga.org/issues/9966 "Bug 9966"): Fix formatting in mkclass
* Bug [9968](https://dev.icinga.org/issues/9968 "Bug 9968"): Implement support for '.' when persisting modified attributes
* Bug [9987](https://dev.icinga.org/issues/9987 "Bug 9987"): Crash in ConfigCompiler::RegisterZoneDir
* Bug [10008](https://dev.icinga.org/issues/10008 "Bug 10008"): Don't parse config files for branches not taken
* Bug [10012](https://dev.icinga.org/issues/10012 "Bug 10012"): Unused variable 'dobj' in configobject.tcpp
* Bug [10024](https://dev.icinga.org/issues/10024 "Bug 10024"): HTTP keep-alive does not work with .NET WebClient
* Bug [10027](https://dev.icinga.org/issues/10027 "Bug 10027"): Filtering by name doesn't work
* Bug [10034](https://dev.icinga.org/issues/10034 "Bug 10034"): Unused variable console_type in consolecommand.cpp
* Bug [10041](https://dev.icinga.org/issues/10041 "Bug 10041"): build failure: demo module
* Bug [10048](https://dev.icinga.org/issues/10048 "Bug 10048"): Error handling in HttpClient/icinga-studio
* Bug [10110](https://dev.icinga.org/issues/10110 "Bug 10110"): Add object_id where clause for icinga_downtimehistory
* Bug [10180](https://dev.icinga.org/issues/10180 "Bug 10180"): API actions do not follow REST guidelines
* Bug [10198](https://dev.icinga.org/issues/10198 "Bug 10198"): Detect infinite recursion in user scripts
* Bug [10210](https://dev.icinga.org/issues/10210 "Bug 10210"): Move the Collection status handler to /v1/status
* Bug [10211](https://dev.icinga.org/issues/10211 "Bug 10211"): PerfdataValue is not properly serialised in status queries
* Bug [10224](https://dev.icinga.org/issues/10224 "Bug 10224"): URL parser is cutting off last character
* Bug [10234](https://dev.icinga.org/issues/10234 "Bug 10234"): ASCII NULs don't work in string values
* Bug [10238](https://dev.icinga.org/issues/10238 "Bug 10238"): Use a temporary file for modified-attributes.conf updates
* Bug [10241](https://dev.icinga.org/issues/10241 "Bug 10241"): Properly encode URLs in Icinga Studio
* Bug [10249](https://dev.icinga.org/issues/10249 "Bug 10249"): Config Sync shouldn't send updates for objects the client doesn't have access to
* Bug [10253](https://dev.icinga.org/issues/10253 "Bug 10253"): /v1/objects/<type> returns an HTTP error when there are no objects of that type
* Bug [10255](https://dev.icinga.org/issues/10255 "Bug 10255"): Config sync does not set endpoint syncing and plays disconnect-sync ping-pong
* Bug [10256](https://dev.icinga.org/issues/10256 "Bug 10256"): ConfigWriter::EmitValue should format floating point values properly
* Bug [10326](https://dev.icinga.org/issues/10326 "Bug 10326"): icinga2 repository host add does not work
* Bug [10350](https://dev.icinga.org/issues/10350 "Bug 10350"): Remove duplicated text in section "Apply Notifications to Hosts and Services"
* Bug [10355](https://dev.icinga.org/issues/10355 "Bug 10355"): Version updates are not working properly
* Bug [10360](https://dev.icinga.org/issues/10360 "Bug 10360"): Icinga2 API performance regression
* Bug [10371](https://dev.icinga.org/issues/10371 "Bug 10371"): Ensure that modified attributes work with clients with local config and no zone attribute
* Bug [10386](https://dev.icinga.org/issues/10386 "Bug 10386"): restore_attribute does not work in clusters
* Bug [10403](https://dev.icinga.org/issues/10403 "Bug 10403"): Escaping $ not documented
* Bug [10406](https://dev.icinga.org/issues/10406 "Bug 10406"): Misleading wording in generated zones.conf
* Bug [10410](https://dev.icinga.org/issues/10410 "Bug 10410"): OpenBSD: hang during ConfigItem::ActivateItems() in daemon startup
* Bug [10417](https://dev.icinga.org/issues/10417 "Bug 10417"): 'which' isn't available in a minimal CentOS container
* Bug [10422](https://dev.icinga.org/issues/10422 "Bug 10422"): Changing a group's attributes causes duplicate rows in the icinga_*group_members table
* Bug [10433](https://dev.icinga.org/issues/10433 "Bug 10433"): 'dig_lookup' custom attribute for the 'dig' check command isn't optional
* Bug [10436](https://dev.icinga.org/issues/10436 "Bug 10436"): Custom variables aren't removed from the IDO database
* Bug [10439](https://dev.icinga.org/issues/10439 "Bug 10439"): "Command options" is empty when executing icinga2 without any argument.
* Bug [10440](https://dev.icinga.org/issues/10440 "Bug 10440"): Improve --help output for the --log-level option
* Bug [10455](https://dev.icinga.org/issues/10455 "Bug 10455"): Improve error handling during log replay
* Bug [10456](https://dev.icinga.org/issues/10456 "Bug 10456"): Incorrect attribute name in the documentation
* Bug [10457](https://dev.icinga.org/issues/10457 "Bug 10457"): Don't allow scripts to access FANoUserView attributes in sandbox mode
* Bug [10461](https://dev.icinga.org/issues/10461 "Bug 10461"): Line continuation is broken in 'icinga2 console'
* Bug [10466](https://dev.icinga.org/issues/10466 "Bug 10466"): Crash in IndexerExpression::GetReference when attempting to set an attribute on an object other than the current one
* Bug [10473](https://dev.icinga.org/issues/10473 "Bug 10473"): IDO tries to execute empty UPDATE queries
* Bug [10491](https://dev.icinga.org/issues/10491 "Bug 10491"): Unique constraint violation with multiple comment inserts in DB IDO
* Bug [10495](https://dev.icinga.org/issues/10495 "Bug 10495"): Incorrect JSON-RPC message causes Icinga 2 to crash
* Bug [10498](https://dev.icinga.org/issues/10498 "Bug 10498"): IcingaStudio: Accessing non-ConfigObjects causes ugly exception
* Bug [10501](https://dev.icinga.org/issues/10501 "Bug 10501"): Plural name rule not treating edge case correcly
* Bug [10504](https://dev.icinga.org/issues/10504 "Bug 10504"): Increase the default timeout for OS checks
* Bug [10508](https://dev.icinga.org/issues/10508 "Bug 10508"): Figure out whether we need the Checkable attributes state_raw, last_state_raw, hard_state_raw
* Bug [10510](https://dev.icinga.org/issues/10510 "Bug 10510"): CreatePipeOverlapped is not thread-safe
* Bug [10512](https://dev.icinga.org/issues/10512 "Bug 10512"): Mismatch on {comment,downtime}_id vs internal name in the API
* Bug [10517](https://dev.icinga.org/issues/10517 "Bug 10517"): Circular reference between *Connection and TlsStream objects
* Bug [10518](https://dev.icinga.org/issues/10518 "Bug 10518"): Crash in ConfigWriter::GetKeywords
* Bug [10527](https://dev.icinga.org/issues/10527 "Bug 10527"): Fix indentation for Dictionary::ToString
* Bug [10529](https://dev.icinga.org/issues/10529 "Bug 10529"): Change session_token to integer timestamp
* Bug [10535](https://dev.icinga.org/issues/10535 "Bug 10535"): Spaces do not work in command arguments
* Bug [10538](https://dev.icinga.org/issues/10538 "Bug 10538"): Crash in ConfigWriter::EmitIdentifier
* Bug [10539](https://dev.icinga.org/issues/10539 "Bug 10539"): Don't validate custom attributes that aren't strings
* Bug [10540](https://dev.icinga.org/issues/10540 "Bug 10540"): Async mysql queries aren't logged in the debug log
* Bug [10545](https://dev.icinga.org/issues/10545 "Bug 10545"): Broken build - unresolved external symbol "public: void __thiscall icinga::ApiClient::ExecuteScript...
* Bug [10555](https://dev.icinga.org/issues/10555 "Bug 10555"): Don't try to use --gc-sections on Solaris
* Bug [10556](https://dev.icinga.org/issues/10556 "Bug 10556"): Update OpenSSL for the Windows builds
* Bug [10558](https://dev.icinga.org/issues/10558 "Bug 10558"): There's a variable called 'string' in filter expressions
* Bug [10559](https://dev.icinga.org/issues/10559 "Bug 10559"): Autocompletion doesn't work in the debugger
* Bug [10560](https://dev.icinga.org/issues/10560 "Bug 10560"): 'api setup' should create a user even when api feature is already enabled
* Bug [10561](https://dev.icinga.org/issues/10561 "Bug 10561"): 'remove-comment' action does not support filters
* Bug [10562](https://dev.icinga.org/issues/10562 "Bug 10562"): Documentation should not reference real host names
* Bug [10563](https://dev.icinga.org/issues/10563 "Bug 10563"): /v1/console should only use a single permission
* Bug [10568](https://dev.icinga.org/issues/10568 "Bug 10568"): Improve location information for errors in API filters
* Bug [10569](https://dev.icinga.org/issues/10569 "Bug 10569"): Icinga 2 API Docs
* Bug [10578](https://dev.icinga.org/issues/10578 "Bug 10578"): API call doesn't fail when trying to use a template that doesn't exist
* Bug [10580](https://dev.icinga.org/issues/10580 "Bug 10580"): Detailed error message is missing when object creation via API fails
* Bug [10583](https://dev.icinga.org/issues/10583 "Bug 10583"): modify_attribute: object cannot be cloned
* Bug [10588](https://dev.icinga.org/issues/10588 "Bug 10588"): Documentation for /v1/types
* Bug [10596](https://dev.icinga.org/issues/10596 "Bug 10596"): Deadlock in MacroProcessor::EvaluateFunction
* Bug [10601](https://dev.icinga.org/issues/10601 "Bug 10601"): Don't allow users to set state attributes via PUT
* Bug [10602](https://dev.icinga.org/issues/10602 "Bug 10602"): API overwrites (and then deletes) config file when trying to create an object that already exists
* Bug [10604](https://dev.icinga.org/issues/10604 "Bug 10604"): Group memberships are not updated for runtime created objects
* Bug [10629](https://dev.icinga.org/issues/10629 "Bug 10629"): Download URL for NSClient++ is incorrect
* Bug [10637](https://dev.icinga.org/issues/10637 "Bug 10637"): Utility::FormatErrorNumber fails when error message uses arguments
