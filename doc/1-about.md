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

There are many ways to contribute to Icinga -- whether it be sending patches,
testing, reporting bugs, or reviewing and updating the documentation. Every
contribution is appreciated!

Please get in touch with the Icinga team at https://www.icinga.org/community/.

If you want to help update this documentation, please read
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
* When reporting a bug, please include the details described in the [Troubleshooting](15-troubleshooting.md#troubleshooting-information-required) chapter (version, configs, logs, etc.).

## <a id="whats-new"></a> What's New

### What's New in Version 2.5.2

#### Bugfixes

* Bug [12527](https://dev.icinga.org/issues/12527 "Bug 12527") (DB IDO): Newly added group member tables in the IDO database are not updated
* Bug [12529](https://dev.icinga.org/issues/12529 "Bug 12529") (Checker): Icinga 2 sends SOFT recovery notifications

### What's New in Version 2.5.1

#### Bugfixes

* Bug [12517](https://dev.icinga.org/issues/12517 "Bug 12517") (Notifications): Icinga 2 sends recovery notifications for SOFT NOT-OK states

### What's New in Version 2.5.0

#### Changes

* InfluxdbWriter feature
* API
    * New endpoints: /v1/variables and /v1/templates (GET requests), /v1/action/generate-ticket (POST request)
    * State/type filters for notifications/users are now string values (PUT, POST, GET requests)
* Configuration
    * TimePeriod excludes/includes attributes
    * DateTime object for formatting time strings
    * New prototype methods: Array#filter, Array#unique, Array#map, Array#reduce
    * icinga2.conf now includes plugins-contrib, manubulon, windows-plugins, nscp by default (ITL CheckCommand definitions)
    * Performance improvements (config compiler and validation)
* CLI
    * 'icinga2 object list' formats state/type filters as string values
    * Compiled config files are now visible with "notice" debug level (hidden by default)
    * CA serial file now uses a hash value (HA cluster w/ 2 CA directories)
* Cluster
    * There is a known issue with >2 endpoints inside a zone. Icinga 2 will now log a warning.
    * Support for accepted ciphers and minimum TLS version
    * Connection and error logging has been improved.
* DB IDO
    * Schema upgrade required (2.5.0.sql)
    * Incremental config dump (performance boost)
    * `categories` attribute is now an array. Previous method is deprecated and to be removed.
    * DbCatLog is not enabled by default anymore.
    * SSL support for MySQL
* New packages
    * vim-icinga2 for syntax highlighting
    * libicinga2 (Debian), icinga2-libs (RPM) for Icinga Studio packages

#### Feature

* Feature [7355](https://dev.icinga.org/issues/7355 "Feature 7355") (libicinga): Exclude option for TimePeriod definitions
* Feature [8401](https://dev.icinga.org/issues/8401 "Feature 8401") (Packages): Package for syntax highlighting
* Feature [9184](https://dev.icinga.org/issues/9184 "Feature 9184") (Perfdata): Add timestamp support for GelfWriter
* Feature [9264](https://dev.icinga.org/issues/9264 "Feature 9264") (ITL): Extend CheckCommand definitions for nscp-local
* Feature [9725](https://dev.icinga.org/issues/9725 "Feature 9725") (DB IDO): Add SSL support for the IdoMysqlConnection feature
* Feature [9839](https://dev.icinga.org/issues/9839 "Feature 9839") (Configuration): Implement support for formatting date/time
* Feature [9858](https://dev.icinga.org/issues/9858 "Feature 9858") (Perfdata): Gelf module: expose 'perfdata' fields for 'CHECK_RESULT' events
* Feature [10140](https://dev.icinga.org/issues/10140 "Feature 10140") (libicinga): Remove the deprecated IcingaStatusWriter feature
* Feature [10480](https://dev.icinga.org/issues/10480 "Feature 10480") (Perfdata): Add InfluxDbWriter feature
* Feature [10553](https://dev.icinga.org/issues/10553 "Feature 10553") (Documentation): Update SELinux documentation
* Feature [10669](https://dev.icinga.org/issues/10669 "Feature 10669") (ITL): Add IPv4/IPv6 support to the rest of the monitoring-plugins
* Feature [10722](https://dev.icinga.org/issues/10722 "Feature 10722") (ITL): icinga2.conf: Include plugins-contrib, manubulon, windows-plugins, nscp by default
* Feature [10816](https://dev.icinga.org/issues/10816 "Feature 10816") (libbase): Add name attribute for WorkQueue class
* Feature [10952](https://dev.icinga.org/issues/10952 "Feature 10952") (Packages): Provide packages for icinga-studio on Fedora
* Feature [11063](https://dev.icinga.org/issues/11063 "Feature 11063") (API): Implement SSL cipher configuration support for the API feature
* Feature [11290](https://dev.icinga.org/issues/11290 "Feature 11290") (API): ApiListener: Force server's preferred cipher
* Feature [11292](https://dev.icinga.org/issues/11292 "Feature 11292") (API): ApiListener: Make minimum TLS version configurable
* Feature [11359](https://dev.icinga.org/issues/11359 "Feature 11359") (ITL): Add "retries" option to check_snmp command
* Feature [11419](https://dev.icinga.org/issues/11419 "Feature 11419") (Configuration): Config parser should not log names of included files by default
* Feature [11423](https://dev.icinga.org/issues/11423 "Feature 11423") (libicinga): Cleanup downtimes created by ScheduleDowntime
* Feature [11445](https://dev.icinga.org/issues/11445 "Feature 11445") (Configuration): Allow strings in state/type filters
* Feature [11599](https://dev.icinga.org/issues/11599 "Feature 11599") (Documentation): Documentation review
* Feature [11612](https://dev.icinga.org/issues/11612 "Feature 11612") (Configuration): Improve performance for field accesses
* Feature [11623](https://dev.icinga.org/issues/11623 "Feature 11623") (Installation): Add script for automatically cherry-picking commits for minor versions
* Feature [11659](https://dev.icinga.org/issues/11659 "Feature 11659") (Configuration): Remove the (unused) 'inherits' keyword
* Feature [11706](https://dev.icinga.org/issues/11706 "Feature 11706") (API): Improve logging for HTTP API requests
* Feature [11739](https://dev.icinga.org/issues/11739 "Feature 11739") (Packages): Windows Installer: Remove dependency on KB2999226 package
* Feature [11772](https://dev.icinga.org/issues/11772 "Feature 11772") (Cluster): Add lag threshold for cluster-zone check
* Feature [11837](https://dev.icinga.org/issues/11837 "Feature 11837") (Documentation): Use HTTPS for debmon.org links in the documentation
* Feature [11869](https://dev.icinga.org/issues/11869 "Feature 11869") (ITL): Add CIM port parameter for esxi_hardware CheckCommand
* Feature [11875](https://dev.icinga.org/issues/11875 "Feature 11875") (Tests): Add debugging mode for Utility::GetTime
* Feature [11931](https://dev.icinga.org/issues/11931 "Feature 11931") (ITL): Adding option to access ifName for manubulon snmp-interface check command
* Feature [11941](https://dev.icinga.org/issues/11941 "Feature 11941") (API): Support for enumerating available templates via the API
* Feature [11955](https://dev.icinga.org/issues/11955 "Feature 11955") (API): Implement support for getting a list of global variables from the API
* Feature [11967](https://dev.icinga.org/issues/11967 "Feature 11967") (DB IDO): Update DB IDO schema version to 1.14.1
* Feature [11968](https://dev.icinga.org/issues/11968 "Feature 11968") (DB IDO): Enhance IDO check with schema version info
* Feature [11970](https://dev.icinga.org/issues/11970 "Feature 11970") (ITL): add check command for plugin check_apache_status
* Feature [12006](https://dev.icinga.org/issues/12006 "Feature 12006") (ITL): Add check command definitions for kdc and rbl
* Feature [12018](https://dev.icinga.org/issues/12018 "Feature 12018") (Graphite): Add acknowledgement type to Graphite, InfluxDB, OpenTSDB metadata
* Feature [12024](https://dev.icinga.org/issues/12024 "Feature 12024") (DB IDO): Change Ido*Connection 'categories' attribute to an array
* Feature [12041](https://dev.icinga.org/issues/12041 "Feature 12041") (API): API: Add missing downtime_depth attribute
* Feature [12061](https://dev.icinga.org/issues/12061 "Feature 12061") (ITL): Add check command definition for db2_health
* Feature [12106](https://dev.icinga.org/issues/12106 "Feature 12106") (DB IDO): Do not populate logentries table by default
* Feature [12116](https://dev.icinga.org/issues/12116 "Feature 12116") (Cluster): Enhance client disconnect message for "No data received on new API connection."
* Feature [12189](https://dev.icinga.org/issues/12189 "Feature 12189") (ITL): Add support for "-A" command line switch to CheckCommand "snmp-process"
* Feature [12194](https://dev.icinga.org/issues/12194 "Feature 12194") (Cluster): Improve log message for connecting nodes without configured Endpoint object
* Feature [12201](https://dev.icinga.org/issues/12201 "Feature 12201") (Cluster): Improve error messages for failed certificate validation
* Feature [12215](https://dev.icinga.org/issues/12215 "Feature 12215") (Cluster): Include IP address and port in the "New connection" log message
* Feature [12221](https://dev.icinga.org/issues/12221 "Feature 12221") (ITL): A lot of missing parameters for (latest) mysql_health
* Feature [12222](https://dev.icinga.org/issues/12222 "Feature 12222") (Cluster): Log a warning if there are more than 2 zone endpoint members
* Feature [12234](https://dev.icinga.org/issues/12234 "Feature 12234") (CLI): Add history for icinga2 console
* Feature [12247](https://dev.icinga.org/issues/12247 "Feature 12247") (Configuration): Add map/reduce and filter functionality for the Array class
* Feature [12254](https://dev.icinga.org/issues/12254 "Feature 12254") (API): Remove obsolete debug log message
* Feature [12256](https://dev.icinga.org/issues/12256 "Feature 12256") (ITL): Add check command definition for check_graphite
* Feature [12287](https://dev.icinga.org/issues/12287 "Feature 12287") (Cluster): Enhance TLS handshake error messages with connection information
* Feature [12304](https://dev.icinga.org/issues/12304 "Feature 12304") (Notifications): Add the notification type into the log message
* Feature [12314](https://dev.icinga.org/issues/12314 "Feature 12314") (ITL): Add command definition for check_mysql_query
* Feature [12327](https://dev.icinga.org/issues/12327 "Feature 12327") (API): Support for determining the Icinga 2 version via the API
* Feature [12329](https://dev.icinga.org/issues/12329 "Feature 12329") (libicinga): Implement process_check_result script method for the Checkable class
* Feature [12336](https://dev.icinga.org/issues/12336 "Feature 12336") (libbase): Improve logging for the WorkQueue class
* Feature [12338](https://dev.icinga.org/issues/12338 "Feature 12338") (Configuration): Move internal script functions into the 'Internal' namespace
* Feature [12386](https://dev.icinga.org/issues/12386 "Feature 12386") (Documentation): Rewrite Client and Cluster chapter and; add service monitoring chapter
* Feature [12389](https://dev.icinga.org/issues/12389 "Feature 12389") (libbase): Include compiler name/version and build host name in --version
* Feature [12392](https://dev.icinga.org/issues/12392 "Feature 12392") (ITL): Add custom variables for all check_swap arguments
* Feature [12393](https://dev.icinga.org/issues/12393 "Feature 12393") (libbase): Implement support for marking functions as deprecated
* Feature [12407](https://dev.icinga.org/issues/12407 "Feature 12407") (CLI): Implement support for inspecting variables with LLDB/GDB
* Feature [12408](https://dev.icinga.org/issues/12408 "Feature 12408") (Configuration): Implement support for namespaces
* Feature [12412](https://dev.icinga.org/issues/12412 "Feature 12412") (Documentation): Add URL and short description for Monitoring Plugins inside the ITL documentation
* Feature [12424](https://dev.icinga.org/issues/12424 "Feature 12424") (ITL): Add perfsyntax parameter to nscp-local-counter CheckCommand
* Feature [12426](https://dev.icinga.org/issues/12426 "Feature 12426") (Configuration): Implement comparison operators for the Array class
* Feature [12433](https://dev.icinga.org/issues/12433 "Feature 12433") (API): Add API action for generating a PKI ticket
* Feature [12434](https://dev.icinga.org/issues/12434 "Feature 12434") (DB IDO): Remove unused code from the IDO classes
* Feature [12435](https://dev.icinga.org/issues/12435 "Feature 12435") (DB IDO): Incremental updates for the IDO database
* Feature [12448](https://dev.icinga.org/issues/12448 "Feature 12448") (libbase): Improve performance for type lookups
* Feature [12450](https://dev.icinga.org/issues/12450 "Feature 12450") (Cluster): Improve performance for Endpoint config validation
* Feature [12457](https://dev.icinga.org/issues/12457 "Feature 12457") (libbase): Remove unnecessary Dictionary::Contains calls
* Feature [12468](https://dev.icinga.org/issues/12468 "Feature 12468") (ITL): Add interfacetable CheckCommand options --trafficwithpkt and --snmp-maxmsgsize
* Feature [12477](https://dev.icinga.org/issues/12477 "Feature 12477") (Documentation): Development docs: Add own section for gdb backtrace from a running process
* Feature [12481](https://dev.icinga.org/issues/12481 "Feature 12481") (libbase): Remove some unused #includes

#### Bugfixes

* Bug [7354](https://dev.icinga.org/issues/7354 "Bug 7354") (libicinga): Disable immediate hard state after first checkresult
* Bug [9242](https://dev.icinga.org/issues/9242 "Bug 9242") (Cluster): Custom notification external commands do not work in a master-master setup
* Bug [9848](https://dev.icinga.org/issues/9848 "Bug 9848") (libbase): Function::Invoke should optionally register ScriptFrame
* Bug [10061](https://dev.icinga.org/issues/10061 "Bug 10061") (DB IDO): IDO: icinga_host/service_groups alias columns are TEXT columns
* Bug [10066](https://dev.icinga.org/issues/10066 "Bug 10066") (DB IDO): Missing indexes for icinga_endpoints* and icinga_zones* tables in DB IDO schema
* Bug [10069](https://dev.icinga.org/issues/10069 "Bug 10069") (DB IDO): IDO: check_source should not be a TEXT field
* Bug [10070](https://dev.icinga.org/issues/10070 "Bug 10070") (DB IDO): IDO: there is no usable object index on icinga_{scheduleddowntime,comments}
* Bug [10075](https://dev.icinga.org/issues/10075 "Bug 10075") (libbase): Race condition in CreatePipeOverlapped
* Bug [10363](https://dev.icinga.org/issues/10363 "Bug 10363") (Notifications): Notification times w/ empty begin/end specifications prevent sending notifications
* Bug [10570](https://dev.icinga.org/issues/10570 "Bug 10570") (API): /v1 returns HTML even if JSON is requested
* Bug [10903](https://dev.icinga.org/issues/10903 "Bug 10903") (Perfdata): GELF multi-line output
* Bug [10937](https://dev.icinga.org/issues/10937 "Bug 10937") (Configuration): High CPU usage with self-referenced parent zone config
* Bug [11182](https://dev.icinga.org/issues/11182 "Bug 11182") (DB IDO): IDO: entry_time of all comments is set to the date and time when Icinga 2 was restarted
* Bug [11196](https://dev.icinga.org/issues/11196 "Bug 11196") (Cluster): High load when pinning command endpoint on HA cluster
* Bug [11483](https://dev.icinga.org/issues/11483 "Bug 11483") (libicinga): Numbers are not properly formatted in runtime macro strings
* Bug [11562](https://dev.icinga.org/issues/11562 "Bug 11562") (Notifications): last_problem_notification should be synced in HA cluster
* Bug [11590](https://dev.icinga.org/issues/11590 "Bug 11590") (Notifications): notification interval = 0 not honoured in HA clusters
* Bug [11622](https://dev.icinga.org/issues/11622 "Bug 11622") (Configuration): Don't allow flow control keywords outside of other flow control constructs
* Bug [11648](https://dev.icinga.org/issues/11648 "Bug 11648") (Packages): Reload permission error with SELinux
* Bug [11650](https://dev.icinga.org/issues/11650 "Bug 11650") (Packages): RPM update starts disabled icinga2 service
* Bug [11688](https://dev.icinga.org/issues/11688 "Bug 11688") (DB IDO): Outdated downtime/comments not removed from IDO database (restart)
* Bug [11730](https://dev.icinga.org/issues/11730 "Bug 11730") (libicinga): Icinga 2 client gets killed during network scans
* Bug [11782](https://dev.icinga.org/issues/11782 "Bug 11782") (Packages): Incorrect filter in pick.py
* Bug [11793](https://dev.icinga.org/issues/11793 "Bug 11793") (Documentation): node setup: Add a note for --endpoint syntax for client-master connection
* Bug [11817](https://dev.icinga.org/issues/11817 "Bug 11817") (Installation): Windows: Error with repository handler (missing /var/lib/icinga2/api/repository path)
* Bug [11823](https://dev.icinga.org/issues/11823 "Bug 11823") (DB IDO): Volatile check results for OK->OK transitions are logged into DB IDO statehistory
* Bug [11825](https://dev.icinga.org/issues/11825 "Bug 11825") (libicinga): Problems with check scheduling for HARD state changes (standalone/command_endpoint)
* Bug [11832](https://dev.icinga.org/issues/11832 "Bug 11832") (Tests): Boost tests are missing a dependency on libmethods
* Bug [11847](https://dev.icinga.org/issues/11847 "Bug 11847") (Documentation): Missing quotes for API action URL
* Bug [11851](https://dev.icinga.org/issues/11851 "Bug 11851") (Notifications): Downtime notifications do not pass author and comment
* Bug [11862](https://dev.icinga.org/issues/11862 "Bug 11862") (libicinga): SOFT OK-state after returning from a soft state
* Bug [11887](https://dev.icinga.org/issues/11887 "Bug 11887") (ITL): Add "fuse.gvfsd-fuse" to the list of excluded file systems for check_disk
* Bug [11890](https://dev.icinga.org/issues/11890 "Bug 11890") (Configuration): Config validation should not delete comments/downtimes w/o reference
* Bug [11894](https://dev.icinga.org/issues/11894 "Bug 11894") (Configuration): Incorrect custom variable name in the hosts.conf example config
* Bug [11898](https://dev.icinga.org/issues/11898 "Bug 11898") (libicinga): last SOFT state should be hard (max_check_attempts)
* Bug [11899](https://dev.icinga.org/issues/11899 "Bug 11899") (libicinga): Flapping Notifications dependent on state change
* Bug [11903](https://dev.icinga.org/issues/11903 "Bug 11903") (Documentation): Fix systemd client command formatting
* Bug [11905](https://dev.icinga.org/issues/11905 "Bug 11905") (Documentation): Improve "Endpoint" documentation
* Bug [11926](https://dev.icinga.org/issues/11926 "Bug 11926") (API): Trying to delete an object protected by a permissions filter, ends up deleting all objects that match the filter instead
* Bug [11933](https://dev.icinga.org/issues/11933 "Bug 11933") (DB IDO): SOFT state changes with the same state are not logged
* Bug [11962](https://dev.icinga.org/issues/11962 "Bug 11962") (DB IDO): Overflow in current_notification_number column in DB IDO MySQL
* Bug [11991](https://dev.icinga.org/issues/11991 "Bug 11991") (Documentation): Incorrect URL for API examples in the documentation
* Bug [11993](https://dev.icinga.org/issues/11993 "Bug 11993") (DB IDO): Comment/Downtime delete queries are slow
* Bug [12003](https://dev.icinga.org/issues/12003 "Bug 12003") (libbase): Hang in TlsStream::Handshake
* Bug [12008](https://dev.icinga.org/issues/12008 "Bug 12008") (Documentation): Add a note about creating Zone/Endpoint objects with the API
* Bug [12016](https://dev.icinga.org/issues/12016 "Bug 12016") (Configuration): ConfigWriter::EmitScope incorrectly quotes dictionary keys
* Bug [12022](https://dev.icinga.org/issues/12022 "Bug 12022") (Configuration): Icinga crashes when using include_recursive in an object definition
* Bug [12029](https://dev.icinga.org/issues/12029 "Bug 12029") (Documentation): Migration docs still show unsupported CHANGE_*MODATTR external commands
* Bug [12044](https://dev.icinga.org/issues/12044 "Bug 12044") (Packages): Icinga fails to build with OpenSSL 1.1.0
* Bug [12046](https://dev.icinga.org/issues/12046 "Bug 12046") (Documentation): Typo in Manubulon CheckCommand documentation
* Bug [12067](https://dev.icinga.org/issues/12067 "Bug 12067") (Documentation): Documentation: Setting up Plugins section is broken
* Bug [12077](https://dev.icinga.org/issues/12077 "Bug 12077") (Documentation): Add a note to the docs that API POST updates to custom attributes/groups won't trigger re-evaluation
* Bug [12085](https://dev.icinga.org/issues/12085 "Bug 12085") (DB IDO): deadlock in ido reconnect
* Bug [12092](https://dev.icinga.org/issues/12092 "Bug 12092") (API): Icinga incorrectly disconnects all endpoints if one has a wrong certificate
* Bug [12098](https://dev.icinga.org/issues/12098 "Bug 12098") (Configuration): include_recursive should gracefully handle inaccessible files
* Bug [12099](https://dev.icinga.org/issues/12099 "Bug 12099") (Packages): Build fails with Visual Studio 2013
* Bug [12100](https://dev.icinga.org/issues/12100 "Bug 12100") (libbase): Ensure to clear the SSL error queue before calling SSL_{read,write,do_handshake}
* Bug [12107](https://dev.icinga.org/issues/12107 "Bug 12107") (DB IDO): Add missing index on state history for DB IDO cleanup
* Bug [12135](https://dev.icinga.org/issues/12135 "Bug 12135") (ITL): ITL: check_iftraffic64.pl default values, wrong postfix value in CheckCommand
* Bug [12144](https://dev.icinga.org/issues/12144 "Bug 12144") (Documentation): pkg-config is not listed as a build requirement in INSTALL.md
* Bug [12147](https://dev.icinga.org/issues/12147 "Bug 12147") (DB IDO): IDO module starts threads before daemonize
* Bug [12179](https://dev.icinga.org/issues/12179 "Bug 12179") (Cluster): Duplicate messages for command_endpoint w/ master and satellite
* Bug [12180](https://dev.icinga.org/issues/12180 "Bug 12180") (Cluster): CheckerComponent sometimes fails to schedule checks in time
* Bug [12193](https://dev.icinga.org/issues/12193 "Bug 12193") (Cluster): Increase cluster reconnect interval
* Bug [12199](https://dev.icinga.org/issues/12199 "Bug 12199") (API): Fix URL encoding for '&'
* Bug [12204](https://dev.icinga.org/issues/12204 "Bug 12204") (Documentation): Improve author information about check_yum
* Bug [12210](https://dev.icinga.org/issues/12210 "Bug 12210") (DB IDO): Do not clear {host,service,contact}group_members tables on restart
* Bug [12216](https://dev.icinga.org/issues/12216 "Bug 12216") (libicinga): icinga check reports "-1" for minimum latency and execution time and only uptime has a number but 0
* Bug [12217](https://dev.icinga.org/issues/12217 "Bug 12217") (Documentation): Incorrect documentation about apply rules in zones.d directories
* Bug [12219](https://dev.icinga.org/issues/12219 "Bug 12219") (Documentation): Missing explanation for three level clusters with CSR auto-signing
* Bug [12225](https://dev.icinga.org/issues/12225 "Bug 12225") (libicinga): Icinga stats min_execution_time and max_execution_time are invalid
* Bug [12227](https://dev.icinga.org/issues/12227 "Bug 12227") (Perfdata): Incorrect escaping / formatting of perfdata to InfluxDB
* Bug [12237](https://dev.icinga.org/issues/12237 "Bug 12237") (Installation): Increase default systemd timeout
* Bug [12257](https://dev.icinga.org/issues/12257 "Bug 12257") (Notifications): Notification interval mistimed
* Bug [12259](https://dev.icinga.org/issues/12259 "Bug 12259") (Documentation): Incorrect API permission name for /v1/status in the documentation
* Bug [12267](https://dev.icinga.org/issues/12267 "Bug 12267") (Notifications): Multiple notifications when master fails
* Bug [12274](https://dev.icinga.org/issues/12274 "Bug 12274") (ITL): -q option for check_ntp_time is wrong
* Bug [12288](https://dev.icinga.org/issues/12288 "Bug 12288") (DB IDO): Change the way outdated comments/downtimes are deleted on restart
* Bug [12293](https://dev.icinga.org/issues/12293 "Bug 12293") (Notifications): Missing notification for recovery during downtime
* Bug [12302](https://dev.icinga.org/issues/12302 "Bug 12302") (Cluster): Remove obsolete README files in tools/syntax
* Bug [12310](https://dev.icinga.org/issues/12310 "Bug 12310") (Notifications): Notification sent too fast when one master fails
* Bug [12318](https://dev.icinga.org/issues/12318 "Bug 12318") (Configuration): Icinga doesn't delete temporary icinga2.debug file when config validation fails
* Bug [12331](https://dev.icinga.org/issues/12331 "Bug 12331") (libbase): Fix building Icinga with -fvisibility=hidden
* Bug [12333](https://dev.icinga.org/issues/12333 "Bug 12333") (Notifications): Incorrect downtime notification events
* Bug [12334](https://dev.icinga.org/issues/12334 "Bug 12334") (libbase): Handle I/O errors while writing the Icinga state file more gracefully
* Bug [12390](https://dev.icinga.org/issues/12390 "Bug 12390") (libbase): Disallow casting "" to an Object
* Bug [12391](https://dev.icinga.org/issues/12391 "Bug 12391") (libbase): Don't violate POSIX by ensuring that the argument to usleep(3) is less than 1000000
* Bug [12395](https://dev.icinga.org/issues/12395 "Bug 12395") (libicinga): Flexible downtimes should be removed after trigger_time+duration
* Bug [12401](https://dev.icinga.org/issues/12401 "Bug 12401") (DB IDO): Fixed downtime start does not update actual_start_time
* Bug [12402](https://dev.icinga.org/issues/12402 "Bug 12402") (Notifications): Notification resent, even if interval = 0
* Bug [12404](https://dev.icinga.org/issues/12404 "Bug 12404") (Notifications): Add log message if notifications are forced (i.e. filters are not checked)
* Bug [12409](https://dev.icinga.org/issues/12409 "Bug 12409") (Configuration): 'use' keyword cannot be used with templates
* Bug [12416](https://dev.icinga.org/issues/12416 "Bug 12416") (Documentation): The description for the http_certificate attribute doesn't have the right default value
* Bug [12417](https://dev.icinga.org/issues/12417 "Bug 12417") (DB IDO): IDO does duplicate config updates
* Bug [12418](https://dev.icinga.org/issues/12418 "Bug 12418") (DB IDO): IDO marks objects as inactive on shutdown
* Bug [12422](https://dev.icinga.org/issues/12422 "Bug 12422") (CLI): pki sign-csr does not log where it is writing the certificate file
* Bug [12425](https://dev.icinga.org/issues/12425 "Bug 12425") (libicinga): CompatUtility::GetCheckableNotificationStateFilter is returning an incorrect value
* Bug [12428](https://dev.icinga.org/issues/12428 "Bug 12428") (DB IDO): Fix the "ido" check command for use with command_endpoint
* Bug [12430](https://dev.icinga.org/issues/12430 "Bug 12430") (DB IDO): ido CheckCommand returns returns "Could not connect to database server" when HA enabled
* Bug [12432](https://dev.icinga.org/issues/12432 "Bug 12432") (Cluster): Only allow sending command_endpoint checks to directly connected child zones
* Bug [12438](https://dev.icinga.org/issues/12438 "Bug 12438") (libbase): Replace GetType()->GetName() calls with GetReflectionType()->GetName()
* Bug [12442](https://dev.icinga.org/issues/12442 "Bug 12442") (Documentation): Missing documentation for "legacy-timeperiod" template
* Bug [12452](https://dev.icinga.org/issues/12452 "Bug 12452") (Installation): Remove unused functions from icinga-installer
* Bug [12453](https://dev.icinga.org/issues/12453 "Bug 12453") (libbase): Use hash-based serial numbers for new certificates
* Bug [12454](https://dev.icinga.org/issues/12454 "Bug 12454") (API): API: action schedule-downtime requires a duration also when fixed is true
* Bug [12458](https://dev.icinga.org/issues/12458 "Bug 12458") (DB IDO): Insert fails for the icinga_scheduleddowntime table due to duplicate key
* Bug [12459](https://dev.icinga.org/issues/12459 "Bug 12459") (DB IDO): Query for customvariablestatus incorrectly updates the host's/service's insert ID
* Bug [12460](https://dev.icinga.org/issues/12460 "Bug 12460") (Cluster): DB IDO started before daemonizing (no systemd)
* Bug [12461](https://dev.icinga.org/issues/12461 "Bug 12461") (DB IDO): IDO query fails due to key contraint violation for the icinga_customvariablestatus table
* Bug [12464](https://dev.icinga.org/issues/12464 "Bug 12464") (API): API: events for DowntimeTriggered does not provide needed information
* Bug [12473](https://dev.icinga.org/issues/12473 "Bug 12473") (Documentation): Docs: API example uses wrong attribute name
* Bug [12474](https://dev.icinga.org/issues/12474 "Bug 12474") (libmethods): ClrCheck is null on *nix
* Bug [12475](https://dev.icinga.org/issues/12475 "Bug 12475") (Cluster): Incorrect certificate validation error message
* Bug [12487](https://dev.icinga.org/issues/12487 "Bug 12487") (Configuration): Memory leak when using closures
* Bug [12488](https://dev.icinga.org/issues/12488 "Bug 12488") (Documentation): Typo in Notification object documentation
