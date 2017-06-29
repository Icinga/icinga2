# Icinga 2.x CHANGELOG

## What's New

### What's New in Version 2.6.3

#### Changes

This is a bugfix release which addresses a number of bugs we've found since
2.6.2 was released. It also contains a number of improvements for the Icinga
documentation.

#### Feature

* Feature 4955 (Documentation): Review CheckCommand documentation including external URLs
* Feature 5057 (Documentation): Update Security section in the Distributed Monitoring chapter
* Feature 5055 (Documentation): mysql_socket attribute missing in the documentation for the mysql CheckCommand
* Feature 5035 (Documentation): Docs: Typo in Distributed Monitoring chapter
* Feature 5029 (Documentation): Advanced topics: Wrong acknowledgement notification filter
* Feature 5030 (Documentation): Advanced topics: Mention the API and explain stick acks, fixed/flexible downtimes
* Feature 3133 (Documentation): [dev.icinga.com #9583] Add practical examples for apply expressions
* Feature 4996 (Documentation): documentation: mixed up host names in 6-distributed-monitoring.md
* Feature 4980 (Documentation): Add OpenBSD and AlpineLinux package repositories to the documentation
* Feature 4954 (Documentation): Add an example for /v1/actions/process-check-result which uses filter/type

#### Bugfixes

* Bug 5080 (IDO): Missing index use can cause icinga_downtimehistory queries to hang indefinitely
* Bug 4603 (IDO): [dev.icinga.com #12597] With too many comments, Icinga reload process won't finish reconnecting to database
* Bug 4989 (Check Execution): Icinga daemon runs with nice 5 after reload
* Bug 4930 (Cluster): Change "Discarding 'config update object'" log messages to notice log level

### What's New in Version 2.6.2

#### Changes

This is a bugfix release which addresses a crash that can occur when removing
configuration files for objects which have been deleted via the API.

#### Bugfixes

* Bug 4952 (API): Icinga crashes while trying to remove configuration files for objects which no longer exist

### What's New in Version 2.6.1

#### Changes

This release addresses a number of bugs we have identified in version 2.6.0.
The documentation changes
reflect our recent move to GitHub.

#### Feature

* Feature 4950 (Documentation): doc/6-distributed-monitoring.md: Fix typo
* Feature 4934 (Documentation): Update contribution section for GitHub
* Feature 4923 (Documentation): [dev.icinga.com #14011] Migration to Github
* Feature 4917 (Documentation): [dev.icinga.com #13969] Incorrect license file mentioned in README.md
* Feature 4916 (Documentation): [dev.icinga.com #13967] Add travis-ci build status logo to README.md
* Feature 4813 (libicinga): [dev.icinga.com #13345] Include argument name for log message about incorrect set_if values
* Feature 4908 (Documentation): [dev.icinga.com #13897] Move domain to icinga.com
* Feature 4803 (Documentation): [dev.icinga.com #13277] Update Repositories in Docs
* Feature 4885 (Documentation): [dev.icinga.com #13671] SLES 12 SP2 libboost_thread package requires libboost_chrono
* Feature 4868 (Documentation): [dev.icinga.com #13569] Add more build details to INSTALL.md
* Feasture 4869 (Documentation): [dev.icinga.com #13571] Update RELEASE.md

#### Bugfixes

* Bug 4950 (IDO): IDO schema update is not compatible to MySQL 5.7
* Bug 4882 (libbase): [dev.icinga.com #13655] Crash - Error: parse error: premature EOF bug High libbase
* Bug 4867 (libbase) [dev.icinga.com #13567] SIGPIPE shutdown on config reload
* Bug 4874 (IDO) [dev.icinga.com #13617] IDO: Timestamps in PostgreSQL may still have a time zone offset
* Bug 4877 (IDO) [dev.icinga.com #13633] IDO MySQL schema not working on MySQL 5.7
* Bug 4870 (Packages): [dev.icinga.com #13573] SLES11 SP4 dependency on Postgresql >= 8.4

### What's New in Version 2.6.0

#### Changes

* Client/Satellite setup
 * The "bottom up" client configuration mode has been deprecated. Check [#13255](https://dev.icinga.com/issues/13255) for additional details and migration.
* Linux/Unix daemon
 * Ensure that Icinga 2 does not leak file descriptors to executed commands. 
 * There are 2 processes started instead of previously just one process.
* Windows client
 * Package bundles NSClient++ 0.5.0. ITL CheckCommands have been updated too.
 * Allow to configure the user account for the Icinga 2 service. This is useful if several checks require administrator permissions (e.g. check_update.exe)
 * Bugfixes for check plugins
* Cluster and API
 * Provide location information for objects and templates in the API
 * Improve log message for ignored config updates
 * Fix cluster resync problem with API created objects (hosts, downtimes, etc.)
 * Fix that API-created objects in a global zone are not synced to child endpoints
* Notifications
 * Several bugfixes for downtime, custom and flapping notifications
* New ITL CheckCommands: logstash, glusterfs, iostats
* Package builds require a compiler which supports C++11 features (gcc-c++ >= 4.7, clang++)
* DB IDO
 * Schema upgrade required (2.6.0.sql)
 * This update fixes timestamp columns required by Icinga Web 2 and might take a while. Please ensure to schedule a maintenance task for your database upgrade.

#### Feature

* Feature 12566 (API): Provide location information for objects and templates in the API
* Feature 13255 (Cluster): Deprecate cluster/client mode "bottom up" w/ repository.d and node update-config
* Feature 12844 (Cluster): Check whether nodes are synchronizing the API log before putting them into UNKNOWN
* Feature 12623 (Cluster): Improve log message for ignored config updates
* Feature 12635 (Configuration): Suppress compiler warnings for auto-generated code
* Feature 12575 (Configuration): Implement support for default templates
* Feature 12554 (Configuration): Implement a command-line argument for "icinga2 console" to allow specifying a script file
* Feature 12544 (Configuration): Remove unused method: ApplyRule::DiscardRules
* Feature 10675 (Configuration): Command line option for config syntax validation
* Feature 13491 (Documentation): Update README.md and correct project URLs
* Feature 13457 (Documentation): Add a note for boolean values in the disk CheckCommand section
* Feature 13455 (Documentation): Troubleshooting: Add examples for fetching the executed command line
* Feature 13443 (Documentation): Update Windows screenshots in the client documentation
* Feature 13437 (Documentation): Add example for concurrent_checks in CheckerComponent object type
* Feature 13395 (Documentation): Add a note about removing "conf.d" on the client for "top down command endpoint" setups
* Feature 13327 (Documentation): Update API and Library Reference chapters
* Feature 13319 (Documentation): Add a note about pinning checks w/ command_endpoint
* Feature 13297 (Documentation): Add a note about default template import to the CheckCommand object
* Feature 13199 (Documentation): Doc: Swap packages.icinga.com w/ DebMon
* Feature 12834 (Documentation): Add more Timeperiod examples in the documentation
* Feature 12832 (Documentation): Add an example of multi-parents configuration for the Migration chapter
* Feature 12587 (Documentation): Update service monitoring and distributed docs
* Feature 12449 (Documentation): Add information about function 'range'
* Feature 13449 (ITL): Add tempdir attribute to postgres CheckCommand
* Feature 13435 (ITL): Add sudo option to mailq CheckCommand
* Feature 13433 (ITL): Add verbose parameter to http CheckCommand
* Feature 13431 (ITL): Add timeout option to mysql_health CheckCommand
* Feature 12762 (ITL): Add a radius CheckCommand for the radius check provide by nagios-plugins
* Feature 12755 (ITL): Add CheckCommand definition for check_logstash
* Feature 12739 (ITL): Add timeout option to oracle_health CheckCommand
* Feature 12613 (ITL): Add CheckCommand definition for check_iostats
* Feature 12516 (ITL): ITL - check_vmware_esx - specify a datacenter/vsphere server for esx/host checks
* Feature 12040 (ITL): Add CheckCommand definition for check_glusterfs
* Feature 12576 (Installation): Use raw string literals in mkembedconfig
* Feature 12564 (Installation): Improve detection for the -flto compiler flag
* Feature 12552 (Installation): Set versions for all internal libraries
* Feature 12537 (Installation): Update cmake config to require a compiler that supports C++11
* Feature 9119 (Installation): Make the user account configurable for the Windows service
* Feature 12733 (Packages): Windows Installer should include NSClient++ 0.5.0
* Feature 12679 (Plugins): Review windows plugins performance output
* Feature 13225 (Tests): Add unit test for notification state/type filter checks
* Feature 12530 (Tests): Implement unit tests for state changes
* Feature 12562 (libbase): Use lambda functions for INITIALIZE_ONCE
* Feature 12561 (libbase): Use 'auto' for iterator declarations
* Feature 12555 (libbase): Implement an rvalue constructor for the String and Value classes
* Feature 12538 (libbase): Replace BOOST_FOREACH with range-based for loops
* Feature 12536 (libbase): Add -fvisibility=hidden to the default compiler flags
* Feature 12510 (libbase): Implement an environment variable to keep Icinga from closing FDs on startup
* Feature 12509 (libbase): Avoid unnecessary string copies
* Feature 12507 (libbase): Remove deprecated functions
* Feature 9182 (libbase): Better message for apply errors
* Feature 12578 (libicinga): Make sure that libmethods is automatically loaded even when not using the ITL

#### Bugfixes

* Bug 12860 (API): Icinga crashes while deleting a config file which doesn't exist anymore
* Bug 12667 (API): Crash in HttpRequest::Parse while processing HTTP request
* Bug 12621 (API): Invalid API filter error messages
* Bug 11541 (API): Objects created in a global zone are not synced to child endpoints
* Bug 11329 (API): API requests from execute-script action are too verbose
* Bug 13419 (CLI): Wrong help string for node setup cli command argument --master_host
* Bug 12741 (CLI): Parse error: "premature EOF" when running "icinga2 node update-config"
* Bug 12596 (CLI): Last option highlighted as the wrong one, even when it is not the culprit
* Bug 13151 (Cluster): Crash w/ SendNotifications cluster handler and check result with empty perfdata
* Bug 11684 (Cluster): Cluster resync problem with API created objects
* Bug 10897 (Compat): SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME command missing
* Bug 10896 (Compat): SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME command missing
* Bug 12749 (Configuration): Configuration validation fails when setting tls_protocolmin to TLSv1.2
* Bug 12633 (Configuration): Validation does not highlight the correct attribute
* Bug 12571 (Configuration): Debug hints for dictionary expressions are nested incorrectly
* Bug 12556 (Configuration): Config validation shouldnt allow 'endpoints = [ "" ]'
* Bug 13221 (DB IDO): PostgreSQL: Don't use timestamp with timezone for UNIX timestamp columns
* Bug 12558 (DB IDO): Getting error during schema update
* Bug 12514 (DB IDO): Don't link against libmysqlclient_r
* Bug 10502 (DB IDO): MySQL 5.7.9, Incorrect datetime value Error
* Bug 13519 (Documentation): "2.1.4. Installation Paths" should contain systemd paths
* Bug 13517 (Documentation): Update "2.1.3. Enabled Features during Installation" - outdated "feature list"
* Bug 13515 (Documentation): Update package instructions for Fedora
* Bug 13411 (Documentation): Missing API headers for X-HTTP-Method-Override
* Bug 13407 (Documentation): Fix example in PNP template docs
* Bug 13267 (Documentation): Docs: Typo in "CLI commands" chapter
* Bug 12933 (Documentation): Docs: wrong heading level for commands.conf and groups.conf
* Bug 12831 (Documentation): Typo in the documentation
* Bug 12822 (Documentation): Fix some spelling mistakes
* Bug 12725 (Documentation): Add documentation for logrotation for the mainlog feature
* Bug 12681 (Documentation): Corrections for distributed monitoring chapter
* Bug 12664 (Documentation): Docs: Migrating Notification example tells about filters instead of types
* Bug 12662 (Documentation): GDB example in the documentation isn't working
* Bug 12594 (Documentation): Typo in distributed monitoring docs
* Bug 12577 (Documentation): Fix help output for update-links.py
* Bug 12995 (Graphite): Performance data writer for Graphite : Values without fraction limited to 2147483647 (7FFFFFFF)
* Bug 12849 (ITL): Default values for check_swap are incorrect
* Bug 12838 (ITL): snmp_miblist variable to feed the -m option of check_snmp is missing in the snmpv3 CheckCommand object
* Bug 12747 (ITL): Problem passing arguments to nscp-local CheckCommand objects
* Bug 12588 (ITL): Default disk plugin check should not check inodes
* Bug 12586 (ITL): Manubulon: Add missing procurve memory flag
* Bug 12573 (ITL): Fix code style violations in the ITL
* Bug 12570 (ITL): Incorrect help text for check_swap
* Bug 12535 (Installation): logrotate file is not properly generated when the logrotate binary resides in /usr/bin
* Bug 13205 (Notifications): Recovery notifications sent for Not-Problem notification type if notified before
* Bug 12892 (Notifications): Flapping notifications sent for soft state changes
* Bug 12670 (Notifications): Forced custom notification is setting "force_next_notification": true permanently
* Bug 12560 (Notifications): Don't send Flapping* notifications when downtime is active
* Bug 12549 (Notifications): Fixed downtimes scheduled for a future date trigger DOWNTIMESTART notifications
* Bug 12276 (Perfdata): InfluxdbWriter does not write state other than 0
* Bug 12155 (Plugins): check_network performance data in invalid format - ingraph
* Bug 10489 (Plugins): Windows Agent: performance data of check_perfmon
* Bug 10487 (Plugins): Windows Agent: Performance data values for check_perfmon.exe are invalid sometimes
* Bug 9831 (Plugins): Implement support for resolving DNS hostnames in check_ping.exe
* Bug 12940 (libbase): SIGALRM handling may be affected by recent commit
* Bug 12718 (libbase): Crash in ClusterEvents::SendNotificationsAPIHandler
* Bug 12545 (libbase): Add missing initializer for WorkQueue::m_NextTaskID
* Bug 12534 (libbase): Fix compiler warnings
* Bug 8900 (libbase): File descriptors are leaked to child processes which makes SELinux unhappy
* Bug 13275 (libicinga): Icinga tries to delete Downtime objects that were statically configured
* Bug 13103 (libicinga): Config validation crashes when using command_endpoint without also having an ApiListener object
* Bug 12602 (libicinga): Remove unused last_in_downtime field
* Bug 12592 (libicinga): Unexpected state changes with max_check_attempts = 2
* Bug 12511 (libicinga): Don't update TimePeriod ranges for inactive objects

### What's New in Version 2.5.4

#### Bugfixes

* Bug 11932 (Checker): many check commands executed at same time when master reload

### What's New in Version 2.5.3

#### Changes

This release addresses an issue with PostgreSQL support for the IDO database module.

#### Bugfixes

* Bug 12533 (DB IDO): ido pgsql migration from 2.4.0 to 2.5.0 : wrong size for config_hash

### What's New in Version 2.5.2

#### Bugfixes

* Bug 12527 (DB IDO): Newly added group member tables in the IDO database are not updated
* Bug 12529 (Checker): Icinga 2 sends SOFT recovery notifications

### What's New in Version 2.5.1

#### Bugfixes

* Bug 12517 (Notifications): Icinga 2 sends recovery notifications for SOFT NOT-OK states

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

* Feature 7355 (libicinga): Exclude option for TimePeriod definitions
* Feature 8401 (Packages): Package for syntax highlighting
* Feature 9184 (Perfdata): Add timestamp support for GelfWriter
* Feature 9264 (ITL): Extend CheckCommand definitions for nscp-local
* Feature 9725 (DB IDO): Add SSL support for the IdoMysqlConnection feature
* Feature 9839 (Configuration): Implement support for formatting date/time
* Feature 9858 (Perfdata): Gelf module: expose 'perfdata' fields for 'CHECK_RESULT' events
* Feature 10140 (libicinga): Remove the deprecated IcingaStatusWriter feature
* Feature 10480 (Perfdata): Add InfluxDbWriter feature
* Feature 10553 (Documentation): Update SELinux documentation
* Feature 10669 (ITL): Add IPv4/IPv6 support to the rest of the monitoring-plugins
* Feature 10722 (ITL): icinga2.conf: Include plugins-contrib, manubulon, windows-plugins, nscp by default
* Feature 10816 (libbase): Add name attribute for WorkQueue class
* Feature 10952 (Packages): Provide packages for icinga-studio on Fedora
* Feature 11063 (API): Implement SSL cipher configuration support for the API feature
* Feature 11290 (API): ApiListener: Force server's preferred cipher
* Feature 11292 (API): ApiListener: Make minimum TLS version configurable
* Feature 11359 (ITL): Add "retries" option to check_snmp command
* Feature 11419 (Configuration): Config parser should not log names of included files by default
* Feature 11423 (libicinga): Cleanup downtimes created by ScheduleDowntime
* Feature 11445 (Configuration): Allow strings in state/type filters
* Feature 11599 (Documentation): Documentation review
* Feature 11612 (Configuration): Improve performance for field accesses
* Feature 11623 (Installation): Add script for automatically cherry-picking commits for minor versions
* Feature 11659 (Configuration): Remove the (unused) 'inherits' keyword
* Feature 11706 (API): Improve logging for HTTP API requests
* Feature 11739 (Packages): Windows Installer: Remove dependency on KB2999226 package
* Feature 11772 (Cluster): Add lag threshold for cluster-zone check
* Feature 11837 (Documentation): Use HTTPS for debmon.org links in the documentation
* Feature 11869 (ITL): Add CIM port parameter for esxi_hardware CheckCommand
* Feature 11875 (Tests): Add debugging mode for Utility::GetTime
* Feature 11931 (ITL): Adding option to access ifName for manubulon snmp-interface check command
* Feature 11941 (API): Support for enumerating available templates via the API
* Feature 11955 (API): Implement support for getting a list of global variables from the API
* Feature 11967 (DB IDO): Update DB IDO schema version to 1.14.1
* Feature 11968 (DB IDO): Enhance IDO check with schema version info
* Feature 11970 (ITL): add check command for plugin check_apache_status
* Feature 12006 (ITL): Add check command definitions for kdc and rbl
* Feature 12018 (Graphite): Add acknowledgement type to Graphite, InfluxDB, OpenTSDB metadata
* Feature 12024 (DB IDO): Change Ido*Connection 'categories' attribute to an array
* Feature 12041 (API): API: Add missing downtime_depth attribute
* Feature 12061 (ITL): Add check command definition for db2_health
* Feature 12106 (DB IDO): Do not populate logentries table by default
* Feature 12116 (Cluster): Enhance client disconnect message for "No data received on new API connection."
* Feature 12189 (ITL): Add support for "-A" command line switch to CheckCommand "snmp-process"
* Feature 12194 (Cluster): Improve log message for connecting nodes without configured Endpoint object
* Feature 12201 (Cluster): Improve error messages for failed certificate validation
* Feature 12215 (Cluster): Include IP address and port in the "New connection" log message
* Feature 12221 (ITL): A lot of missing parameters for (latest) mysql_health
* Feature 12222 (Cluster): Log a warning if there are more than 2 zone endpoint members
* Feature 12234 (CLI): Add history for icinga2 console
* Feature 12247 (Configuration): Add map/reduce and filter functionality for the Array class
* Feature 12254 (API): Remove obsolete debug log message
* Feature 12256 (ITL): Add check command definition for check_graphite
* Feature 12287 (Cluster): Enhance TLS handshake error messages with connection information
* Feature 12304 (Notifications): Add the notification type into the log message
* Feature 12314 (ITL): Add command definition for check_mysql_query
* Feature 12327 (API): Support for determining the Icinga 2 version via the API
* Feature 12329 (libicinga): Implement process_check_result script method for the Checkable class
* Feature 12336 (libbase): Improve logging for the WorkQueue class
* Feature 12338 (Configuration): Move internal script functions into the 'Internal' namespace
* Feature 12386 (Documentation): Rewrite Client and Cluster chapter and; add service monitoring chapter
* Feature 12389 (libbase): Include compiler name/version and build host name in --version
* Feature 12392 (ITL): Add custom variables for all check_swap arguments
* Feature 12393 (libbase): Implement support for marking functions as deprecated
* Feature 12407 (CLI): Implement support for inspecting variables with LLDB/GDB
* Feature 12408 (Configuration): Implement support for namespaces
* Feature 12412 (Documentation): Add URL and short description for Monitoring Plugins inside the ITL documentation
* Feature 12424 (ITL): Add perfsyntax parameter to nscp-local-counter CheckCommand
* Feature 12426 (Configuration): Implement comparison operators for the Array class
* Feature 12433 (API): Add API action for generating a PKI ticket
* Feature 12434 (DB IDO): Remove unused code from the IDO classes
* Feature 12435 (DB IDO): Incremental updates for the IDO database
* Feature 12448 (libbase): Improve performance for type lookups
* Feature 12450 (Cluster): Improve performance for Endpoint config validation
* Feature 12457 (libbase): Remove unnecessary Dictionary::Contains calls
* Feature 12468 (ITL): Add interfacetable CheckCommand options --trafficwithpkt and --snmp-maxmsgsize
* Feature 12477 (Documentation): Development docs: Add own section for gdb backtrace from a running process
* Feature 12481 (libbase): Remove some unused #includes

#### Bugfixes

* Bug 7354 (libicinga): Disable immediate hard state after first checkresult
* Bug 9242 (Cluster): Custom notification external commands do not work in a master-master setup
* Bug 9848 (libbase): Function::Invoke should optionally register ScriptFrame
* Bug 10061 (DB IDO): IDO: icinga_host/service_groups alias columns are TEXT columns
* Bug 10066 (DB IDO): Missing indexes for icinga_endpoints* and icinga_zones* tables in DB IDO schema
* Bug 10069 (DB IDO): IDO: check_source should not be a TEXT field
* Bug 10070 (DB IDO): IDO: there is no usable object index on icinga_{scheduleddowntime,comments}
* Bug 10075 (libbase): Race condition in CreatePipeOverlapped
* Bug 10363 (Notifications): Notification times w/ empty begin/end specifications prevent sending notifications
* Bug 10570 (API): /v1 returns HTML even if JSON is requested
* Bug 10903 (Perfdata): GELF multi-line output
* Bug 10937 (Configuration): High CPU usage with self-referenced parent zone config
* Bug 11182 (DB IDO): IDO: entry_time of all comments is set to the date and time when Icinga 2 was restarted
* Bug 11196 (Cluster): High load when pinning command endpoint on HA cluster
* Bug 11483 (libicinga): Numbers are not properly formatted in runtime macro strings
* Bug 11562 (Notifications): last_problem_notification should be synced in HA cluster
* Bug 11590 (Notifications): notification interval = 0 not honoured in HA clusters
* Bug 11622 (Configuration): Don't allow flow control keywords outside of other flow control constructs
* Bug 11648 (Packages): Reload permission error with SELinux
* Bug 11650 (Packages): RPM update starts disabled icinga2 service
* Bug 11688 (DB IDO): Outdated downtime/comments not removed from IDO database (restart)
* Bug 11730 (libicinga): Icinga 2 client gets killed during network scans
* Bug 11782 (Packages): Incorrect filter in pick.py
* Bug 11793 (Documentation): node setup: Add a note for --endpoint syntax for client-master connection
* Bug 11817 (Installation): Windows: Error with repository handler (missing /var/lib/icinga2/api/repository path)
* Bug 11823 (DB IDO): Volatile check results for OK->OK transitions are logged into DB IDO statehistory
* Bug 11825 (libicinga): Problems with check scheduling for HARD state changes (standalone/command_endpoint)
* Bug 11832 (Tests): Boost tests are missing a dependency on libmethods
* Bug 11847 (Documentation): Missing quotes for API action URL
* Bug 11851 (Notifications): Downtime notifications do not pass author and comment
* Bug 11862 (libicinga): SOFT OK-state after returning from a soft state
* Bug 11887 (ITL): Add "fuse.gvfsd-fuse" to the list of excluded file systems for check_disk
* Bug 11890 (Configuration): Config validation should not delete comments/downtimes w/o reference
* Bug 11894 (Configuration): Incorrect custom variable name in the hosts.conf example config
* Bug 11898 (libicinga): last SOFT state should be hard (max_check_attempts)
* Bug 11899 (libicinga): Flapping Notifications dependent on state change
* Bug 11903 (Documentation): Fix systemd client command formatting
* Bug 11905 (Documentation): Improve "Endpoint" documentation
* Bug 11926 (API): Trying to delete an object protected by a permissions filter, ends up deleting all objects that match the filter instead
* Bug 11933 (DB IDO): SOFT state changes with the same state are not logged
* Bug 11962 (DB IDO): Overflow in current_notification_number column in DB IDO MySQL
* Bug 11991 (Documentation): Incorrect URL for API examples in the documentation
* Bug 11993 (DB IDO): Comment/Downtime delete queries are slow
* Bug 12003 (libbase): Hang in TlsStream::Handshake
* Bug 12008 (Documentation): Add a note about creating Zone/Endpoint objects with the API
* Bug 12016 (Configuration): ConfigWriter::EmitScope incorrectly quotes dictionary keys
* Bug 12022 (Configuration): Icinga crashes when using include_recursive in an object definition
* Bug 12029 (Documentation): Migration docs still show unsupported CHANGE_*MODATTR external commands
* Bug 12044 (Packages): Icinga fails to build with OpenSSL 1.1.0
* Bug 12046 (Documentation): Typo in Manubulon CheckCommand documentation
* Bug 12067 (Documentation): Documentation: Setting up Plugins section is broken
* Bug 12077 (Documentation): Add a note to the docs that API POST updates to custom attributes/groups won't trigger re-evaluation
* Bug 12085 (DB IDO): deadlock in ido reconnect
* Bug 12092 (API): Icinga incorrectly disconnects all endpoints if one has a wrong certificate
* Bug 12098 (Configuration): include_recursive should gracefully handle inaccessible files
* Bug 12099 (Packages): Build fails with Visual Studio 2013
* Bug 12100 (libbase): Ensure to clear the SSL error queue before calling SSL_{read,write,do_handshake}
* Bug 12107 (DB IDO): Add missing index on state history for DB IDO cleanup
* Bug 12135 (ITL): ITL: check_iftraffic64.pl default values, wrong postfix value in CheckCommand
* Bug 12144 (Documentation): pkg-config is not listed as a build requirement in INSTALL.md
* Bug 12147 (DB IDO): IDO module starts threads before daemonize
* Bug 12179 (Cluster): Duplicate messages for command_endpoint w/ master and satellite
* Bug 12180 (Cluster): CheckerComponent sometimes fails to schedule checks in time
* Bug 12193 (Cluster): Increase cluster reconnect interval
* Bug 12199 (API): Fix URL encoding for '&'
* Bug 12204 (Documentation): Improve author information about check_yum
* Bug 12210 (DB IDO): Do not clear {host,service,contact}group_members tables on restart
* Bug 12216 (libicinga): icinga check reports "-1" for minimum latency and execution time and only uptime has a number but 0
* Bug 12217 (Documentation): Incorrect documentation about apply rules in zones.d directories
* Bug 12219 (Documentation): Missing explanation for three level clusters with CSR auto-signing
* Bug 12225 (libicinga): Icinga stats min_execution_time and max_execution_time are invalid
* Bug 12227 (Perfdata): Incorrect escaping / formatting of perfdata to InfluxDB
* Bug 12237 (Installation): Increase default systemd timeout
* Bug 12257 (Notifications): Notification interval mistimed
* Bug 12259 (Documentation): Incorrect API permission name for /v1/status in the documentation
* Bug 12267 (Notifications): Multiple notifications when master fails
* Bug 12274 (ITL): -q option for check_ntp_time is wrong
* Bug 12288 (DB IDO): Change the way outdated comments/downtimes are deleted on restart
* Bug 12293 (Notifications): Missing notification for recovery during downtime
* Bug 12302 (Cluster): Remove obsolete README files in tools/syntax
* Bug 12310 (Notifications): Notification sent too fast when one master fails
* Bug 12318 (Configuration): Icinga doesn't delete temporary icinga2.debug file when config validation fails
* Bug 12331 (libbase): Fix building Icinga with -fvisibility=hidden
* Bug 12333 (Notifications): Incorrect downtime notification events
* Bug 12334 (libbase): Handle I/O errors while writing the Icinga state file more gracefully
* Bug 12390 (libbase): Disallow casting "" to an Object
* Bug 12391 (libbase): Don't violate POSIX by ensuring that the argument to usleep(3) is less than 1000000
* Bug 12395 (libicinga): Flexible downtimes should be removed after trigger_time+duration
* Bug 12401 (DB IDO): Fixed downtime start does not update actual_start_time
* Bug 12402 (Notifications): Notification resent, even if interval = 0
* Bug 12404 (Notifications): Add log message if notifications are forced (i.e. filters are not checked)
* Bug 12409 (Configuration): 'use' keyword cannot be used with templates
* Bug 12416 (Documentation): The description for the http_certificate attribute doesn't have the right default value
* Bug 12417 (DB IDO): IDO does duplicate config updates
* Bug 12418 (DB IDO): IDO marks objects as inactive on shutdown
* Bug 12422 (CLI): pki sign-csr does not log where it is writing the certificate file
* Bug 12425 (libicinga): CompatUtility::GetCheckableNotificationStateFilter is returning an incorrect value
* Bug 12428 (DB IDO): Fix the "ido" check command for use with command_endpoint
* Bug 12430 (DB IDO): ido CheckCommand returns returns "Could not connect to database server" when HA enabled
* Bug 12432 (Cluster): Only allow sending command_endpoint checks to directly connected child zones
* Bug 12438 (libbase): Replace GetType()->GetName() calls with GetReflectionType()->GetName()
* Bug 12442 (Documentation): Missing documentation for "legacy-timeperiod" template
* Bug 12452 (Installation): Remove unused functions from icinga-installer
* Bug 12453 (libbase): Use hash-based serial numbers for new certificates
* Bug 12454 (API): API: action schedule-downtime requires a duration also when fixed is true
* Bug 12458 (DB IDO): Insert fails for the icinga_scheduleddowntime table due to duplicate key
* Bug 12459 (DB IDO): Query for customvariablestatus incorrectly updates the host's/service's insert ID
* Bug 12460 (Cluster): DB IDO started before daemonizing (no systemd)
* Bug 12461 (DB IDO): IDO query fails due to key contraint violation for the icinga_customvariablestatus table
* Bug 12464 (API): API: events for DowntimeTriggered does not provide needed information
* Bug 12473 (Documentation): Docs: API example uses wrong attribute name
* Bug 12474 (libmethods): ClrCheck is null on *nix
* Bug 12475 (Cluster): Incorrect certificate validation error message
* Bug 12487 (Configuration): Memory leak when using closures
* Bug 12488 (Documentation): Typo in Notification object documentation

### What's New in Version 2.4.10

#### Bugfixes

* Bug 11812 (Checker): Checker component doesn't execute any checks for command_endpoint

### What's New in Version 2.4.9

#### Changes

This release fixes a number of issues introduced in 2.4.8.

#### Bugfixes

* Bug 11801 (Perfdata): Error: Function call 'rename' for file '/var/spool/icinga2/tmp/service-perfdata' failed with error code 2, 'No such file or directory'
* Bug 11804 (Configuration): Segfault when trying to start 2.4.8
* Bug 11807 (Compat): Command Pipe thread 100% CPU Usage

### What's New in Version 2.4.8

#### Changes

* Bugfixes
* Support for limiting the maximum number of concurrent checks (new configuration option) 
* HA-aware features now wait for connected cluster nodes in the same zone (e.g. DB IDO)
* The 'icinga' check now alerts on failed reloads

#### Feature

* Feature 8137 (Checker): Maximum concurrent service checks
* Feature 9236 (Perfdata): PerfdataWriter: Better failure handling for file renames across file systems
* Feature 9997 (libmethods): "icinga" check should have state WARNING when the last reload failed
* Feature 10581 (ITL): Provide icingacli in the ITL
* Feature 11556 (libbase): Add support for subjectAltName in SSL certificates
* Feature 11651 (CLI): Implement SNI support for the CLI commands
* Feature 11720 (ITL): 'disk' CheckCommand: Exclude 'cgroup' and 'tracefs' by default
* Feature 11748 (Cluster): Remove unused cluster commands
* Feature 11765 (Cluster): Only activate HARunOnce objects once there's a cluster connection
* Feature 11768 (Documentation): Add the category to the generated changelog

#### Bugfixes

* Bug 9989 (Configuration): Service apply without name possible
* Bug 10426 (libicinga): Icinga crashes with a segfault on receiving a lot of check results for nonexisting hosts/services
* Bug 10717 (Configuration): Comments and downtimes of deleted checkable objects are not deleted
* Bug 11046 (Cluster): Icinga2 agent gets stuck after disconnect and won't relay messages
* Bug 11112 (Compat): Empty author/text attribute for comment/downtimes external commands causing crash
* Bug 11147 (libicinga): "day -X" time specifications are parsed incorrectly
* Bug 11158 (libicinga): Crash with empty ScheduledDowntime 'ranges' attribute
* Bug 11374 (API): Icinga2 API: deleting service with cascade=1 does not delete dependant notification
* Bug 11390 (Compat): Command pipe overloaded: Can't send external Icinga command to the local command file
* Bug 11396 (API): inconsistent API /v1/objects/* response for PUT requests
* Bug 11589 (libicinga): notification sent out during flexible downtime
* Bug 11645 (Documentation): Incorrect chapter headings for Object#to_string and Object#type
* Bug 11646 (Configuration): Wrong log severity causes segfault
* Bug 11686 (API): Icinga Crash with the workflow Create_Host-> Downtime for the Host ->  Delete Downtime -> Remove Host
* Bug 11711 (libicinga): Expired downtimes are not removed
* Bug 11714 (libbase): Crash in UnameHelper
* Bug 11742 (Documentation): Missing documentation for event commands w/ execution bridge
* Bug 11757 (API): API: Missing error handling for invalid JSON request body
* Bug 11767 (DB IDO): Ensure that program status updates are immediately updated in DB IDO
* Bug 11779 (API): Incorrect variable names for joined fields in filters

### What's New in Version 2.4.7

#### Bugfixes

* Bug 11639: Crash in IdoMysqlConnection::ExecuteMultipleQueries

### What's New in Version 2.4.6

#### Feature

* Feature 11638: Update RELEASE.md

#### Bugfixes

* Bug 11628: Docs: Zone attribute 'endpoints' is an array
* Bug 11634: Icinga 2 fails to build on Ubuntu Xenial
* Bug 11635: Failed assertion in IdoPgsqlConnection::FieldToEscapedString

### What's New in Version 2.4.5

#### Changes

* Windows Installer changed from NSIS to MSI
* New configuration attribute for hosts and services: check_timeout (overrides the CheckCommand's timeout when set)
* ITL updates
* Lots of bugfixes

#### Feature

* Feature 9283: Implement support for overriding check command timeout
* Feature 9618: Add Windows setup wizard screenshots
* Feature 11098: Add --method parameter for check_{oracle,mysql,mssql}_health CheckCommands
* Feature 11194: Add --units, --rate and --rate-multiplier support for the snmpv3 check command
* Feature 11399: Update .mailmap for Markus Frosch
* Feature 11437: Add silent install / reference to NSClient++ to documentation
* Feature 11449: Build 64-bit packages for Windows
* Feature 11473: Update NSClient++ to version 0.4.4.19
* Feature 11474: Install 64-bit version of NSClient++ on 64-bit versions of Windows
* Feature 11585: Make sure to update the agent wizard banner
* Feature 11587: Update chocolatey uninstall script for the MSI package

#### Bugfixes

* Bug 9249: logrotate fails since the "su" directive was removed
* Bug 10624: Add application manifest for the Windows agent wizard
* Bug 10843: DB IDO: downtime is not in effect after restart
* Bug 11106: Too many assign where filters cause stack overflow
* Bug 11224: Socket Exceptions (Operation not permitted) while reading from API
* Bug 11227: Downtimes and Comments are not synced to child zones
* Bug 11258: Incorrect base URL in the icinga-rpm-release packages for Fedora
* Bug 11336: Use retry_interval instead of check_interval for first OK -> NOT-OK state change
* Bug 11347: Symlink subfolders not followed/considered for config files
* Bug 11382: Downtimes are not always activated/expired on restart
* Bug 11384: Remove dependency for .NET 3.5 from the chocolatey package
* Bug 11387: IDO: historical contact notifications table column notification_id is off-by-one
* Bug 11402: Explain how to use functions for wildcard matches for arrays and/or dictionaries in assign where expressions
* Bug 11407: Docs: Remove the migration script chapter
* Bug 11434: Config validation for Notification objects should check whether the state filters are valid
* Bug 11435: Icinga 2 Windows Agent does not honor install path during upgrade
* Bug 11438: Remove semi-colons in the auto-generated configs
* Bug 11439: Update the CentOS installation documentation
* Bug 11440: Docs: Cluster manual SSL generation formatting is broken
* Bug 11455: ConfigSync broken from 2.4.3. to 2.4.4 under Windows
* Bug 11462: Error compiling icinga2 targeted for x64 on Windows
* Bug 11475: FatalError() returns when called before Application.Run
* Bug 11482: API User gets wrongly authenticated (client_cn and no password)
* Bug 11484: Overwriting global type variables causes crash in ConfigItem::Commit()
* Bug 11494: Update documentation URL for Icinga Web 2
* Bug 11522: Make the socket event engine configurable
* Bug 11534: DowntimesExpireTimerHandler crashes Icinga2 with <unknown function>
* Bug 11542: make install overwrites configuration files
* Bug 11559: Segfault during config validation if host exists, service does not exist any longer and downtime expires
* Bug 11564: Incorrect link in the documentation
* Bug 11567: Navigation attributes are missing in /v1/objects/<type>
* Bug 11574: Package fails to build on *NIX
* Bug 11577: Compiler warning in NotifyActive
* Bug 11582: icinga2 crashes when a command_endpoint is set, but the api feature is not active
* Bug 11586: icinga2-installer.exe doesn't wait until NSIS uninstall.exe exits
* Bug 11592: Remove instance_name from Ido*Connection example
* Bug 11610: Windows installer does not copy "features-enabled" on upgrade
* Bug 11617: Vim Syntax Highlighting does not work with assign where

### What's New in Version 2.4.4

#### Feature

* Feature 10358: ITL: Allow to enforce specific SSL versions using the http check command
* Feature 11205: Add "query" option to check_postgres command.

#### Bugfixes

* Bug 9642: Flapping notifications are sent for hosts/services which are in a downtime
* Bug 9969: Problem notifications while Flapping is active
* Bug 10225: Host notification type is PROBLEM but should be RECOVERY
* Bug 10231: MkDirP not working on Windows
* Bug 10766: DB IDO: User notification type filters are incorrect
* Bug 10770: Status code 200 even if an object could not be deleted.
* Bug 10795: http check's URI is really just Path
* Bug 10976: Explain how to join hosts/services for /v1/objects/comments
* Bug 11107: ITL: Missing documentation for nwc_health "mode" parameter
* Bug 11159: Common name in node wizard isn't case sensitive
* Bug 11208: CMake does not find MySQL libraries on Windows
* Bug 11209: Wrong log message for trusted cert in node setup command
* Bug 11240: DEL_DOWNTIME_BY_HOST_NAME does not accept optional arguments
* Bug 11248: Active checks are executed even though passive results are submitted
* Bug 11257: Incorrect check interval when passive check results are used
* Bug 11273: Services status updated multiple times within check_interval even though no retry was triggered
* Bug 11289: epoll_ctl might cause oops on Ubuntu trusty
* Bug 11320: Volatile transitions from HARD NOT-OK->NOT-OK do not trigger notifications
* Bug 11328: Typo in API docs
* Bug 11331: Update build requirements for SLES 11 SP4
* Bug 11349: 'icinga2 feature list' fails when all features are disabled
* Bug 11350: Docs: Add API examples for creating services and check commands
* Bug 11352: Segmentation fault during 'icinga2 daemon -C'
* Bug 11369: Chocolatey package is missing uninstall function
* Bug 11385: Update development docs to use 'thread apply all bt full'

### What's New in Version 2.4.3

#### Bugfixes

* Bug 11211: Permission problem after running icinga2 node wizard
* Bug 11212: Wrong permissions for files in /var/cache/icinga2/*

### What's New in Version 2.4.2

#### Changes

* ITL
    Additional arguments for check_disk
    Fix incorrect path for the check_hpasm plugin
    New command: check_iostat
    Fix incorrect variable names for the check_impi plugin
* Cluster
    Improve cluster performance
    Fix connection handling problems (multiple connections for the same endpoint)
* Performance improvements for the DB IDO modules
* Lots and lots of various other bugfixes
* Documentation updates

#### Feature

* Feature 10660: Add CMake flag for disabling the unit tests
* Feature 10777: Add check_iostat to ITL
* Feature 10787: Add "-x" parameter in command definition for disk-windows CheckCommand
* Feature 10807: Raise a config error for "Checkable" objects in global zones
* Feature 10857: DB IDO: Add a log message when the connection handling is completed
* Feature 10860: Log DB IDO query queue stats
* Feature 10880: "setting up check plugins" section should be enhanced with package manager examples
* Feature 10920: Add Timeout parameter to snmpv3 check
* Feature 10947: Add example how to use custom functions in attributes
* Feature 10964: Troubleshooting: Explain how to fetch the executed command
* Feature 10988: Support TLSv1.1 and TLSv1.2 for the cluster transport encryption
* Feature 11037: Add String#trim
* Feature 11138: Checkcommand Disk : Option Freespace-ignore-reserved

#### Bugfixes

* Bug 7287: Re-checks scheduling w/ retry_interval
* Bug 8714: Add priority queue for disconnect/programstatus update events
* Bug 8976: DB IDO: notification_id for contact notifications is out of range
* Bug 10226: Icinga2 reload timeout results in killing old and new process because of systemd
* Bug 10449: Livestatus log query - filter "class" yields empty results
* Bug 10458: Incorrect SQL command for creating the user of the PostgreSQL DB for the IDO
* Bug 10460: A PgSQL DB for the IDO can't be created w/ UTF8
* Bug 10497: check_memory and check_swap plugins do unit conversion and rounding before percentage calculations resulting in imprecise percentages
* Bug 10544: check_network performance data in invalid format
* Bug 10554: Non-UTF8 characters from plugins causes IDO to fail
* Bug 10655: API queries cause memory leaks
* Bug 10700: Crash in ExternalCommandListener
* Bug 10711: Zone::CanAccessObject is very expensive
* Bug 10713: ApiListener::ReplayLog can block with a lot of clients
* Bug 10714: API is not working on wheezy
* Bug 10724: Remove the local zone name question in node wizard
* Bug 10728: node wizard does not remember user defined port
* Bug 10736: Missing num_hosts_pending in /v1/status/CIB
* Bug 10739: Crash on startup with incorrect directory permissions
* Bug 10744: build of icinga2 with gcc 4.4.7 segfaulting with ido
* Bug 10745: ITL check command possibly mistyped variable names
* Bug 10748: Missing path in mkdir() exceptions
* Bug 10760: Disallow lambda expressions where side-effect-free expressions are not allowed
* Bug 10765: Avoid duplicate config and status updates on startup
* Bug 10773: chcon partial context error in safe-reload prevents reload
* Bug 10779: Wrong postgresql-setup initdb command for RHEL7
* Bug 10780: The hpasm check command is using the PluginDir constant
* Bug 10784: Incorrect information in --version on Linux
* Bug 10806: Missing SUSE repository for monitoring plugins documentation
* Bug 10817: Failed IDO query for icinga_downtimehistory
* Bug 10818: Use NodeName in null and random checks
* Bug 10819: Cluster config sync ignores zones.d from API packages
* Bug 10824: Windows build fails with latest git master
* Bug 10825: Missing documentation for API packages zones.d config sync
* Bug 10826: Build error with older CMake versions on VERSION_LESS compare
* Bug 10828: Relative path in include_zones does not work
* Bug 10829: IDO breaks when writing to icinga_programstatus with latest snapshots
* Bug 10830: Config validation doesn't fail when templates are used as object names
* Bug 10852: Formatting problem in "Advanced Filter" chapter
* Bug 10855: Implement support for re-ordering groups of IDO queries
* Bug 10862: Evaluate if CanExecuteQuery/FieldToEscapedString lead to exceptions on !m_Connected
* Bug 10867: "repository add" cli command writes invalid "type" attribute
* Bug 10883: Icinga2 crashes in IDO when removing a comment
* Bug 10890: Remove superfluous #ifdef
* Bug 10891: is_active in IDO is only re-enabled on "every second" restart
* Bug 10908: Typos in the "troubleshooting" section of the documentation
* Bug 10923: API actions: Decide whether fixed: false is the right default
* Bug 10931: Exception stack trace on icinga2 client when the master reloads the configuration
* Bug 10932: Cluster config sync: Ensure that /var/lib/icinga2/api/zones/* exists
* Bug 10935: Logrotate on systemd distros should use systemctl not service
* Bug 10948: Icinga state file corruption with temporary file creation
* Bug 10956: Compiler warnings in lib/remote/base64.cpp
* Bug 10959: Better explaination for array values in "disk" CheckCommand docs
* Bug 10963: high load and memory consumption on icinga2 agent v2.4.1
* Bug 10968: Race condition when using systemd unit file
* Bug 10974: Modified attributes do not work for the IcingaApplication object w/ external commands
* Bug 10979: Mistake in mongodb command definition (mongodb_replicaset)
* Bug 10981: Incorrect name in AUTHORS
* Bug 10989: Escaped sequences not properly generated with 'node update-config'
* Bug 10991: Stream buffer size is 512 bytes, could be raised
* Bug 10998: Incorrect IdoPgSqlConnection Example in Documentation
* Bug 11006: Segfault in ApiListener::ConfigUpdateObjectAPIHandler
* Bug 11014: Check event duplication with parallel connections involved
* Bug 11019: next_check noise in the IDO
* Bug 11020: Master reloads with agents generate false alarms
* Bug 11065: Deleting an object via API does not disable it in DB IDO
* Bug 11074: Partially missing escaping in doc/7-icinga-template-library.md
* Bug 11075: Outdated link to icingaweb2-module-nagvis
* Bug 11083: Ensure that config sync updates are always sent on reconnect
* Bug 11085: Crash in ConfigItem::RunWithActivationContext
* Bug 11088: API queries on non-existant objects cause exception
* Bug 11096: Windows build fails on InterlockedIncrement type
* Bug 11103: Problem with hostgroup_members table cleanup
* Bug 11111: Clean up unused variables a bit
* Bug 11118: Cluster WQ thread dies after fork()
* Bug 11122: Connections are not cleaned up properly
* Bug 11132: YYYY-MM-DD time specs are parsed incorrectly
* Bug 11178: Documentation: Unescaped pipe character in tables
* Bug 11179: CentOS 5 doesn't support epoll_create1
* Bug 11204: "node setup" tries to chown() files before they're created

### What's New in Version 2.4.1

#### Changes

* ITL
    * Add running_kernel_use_sudo option for the running_kernel check
* Configuration
    * Add global constants: `PlatformName`. `PlatformVersion`, `PlatformKernel` and `PlatformKernelVersion`
* CLI
    * Use NodeName and ZoneName constants for 'node setup' and 'node wizard' 

#### Feature

* Feature 10622: Add by_ssh_options argument for the check_by_ssh plugin
* Feature 10693: Add running_kernel_use_sudo option for the running_kernel check
* Feature 10716: Use NodeName and ZoneName constants for 'node setup' and 'node wizard'

#### Bugfixes

* Bug 10528: Documentation example in "Access Object Attributes at Runtime" doesn't work correctly
* Bug 10615: Build fails on SLES 11 SP3 with GCC 4.8
* Bug 10632: "node wizard" does not ask user to verify SSL certificate
* Bug 10641: API setup command incorrectly overwrites existing certificates
* Bug 10643: Icinga 2 crashes when ScheduledDowntime objects are used
* Bug 10645: Documentation for schedule-downtime is missing required paremeters
* Bug 10648: lib/base/process.cpp SIGSEGV on Debian squeeze / RHEL 6
* Bug 10661: Incorrect web inject URL in documentation
* Bug 10663: Incorrect redirect for stderr in /usr/lib/icinga2/prepare-dirs
* Bug 10667: Indentation in command-plugins.conf
* Bug 10677: node wizard checks for /var/lib/icinga2/ca directory but not the files
* Bug 10690: CLI command 'repository add' doesn't work
* Bug 10692: Fix typos in the documentation
* Bug 10708: Windows setup wizard crashes when InstallDir registry key is not set
* Bug 10710: Incorrect path for icinga2 binary in development documentation
* Bug 10720: Remove --master_zone from --help because it is currently not implemented

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

* Feature 7709: Validators should be implemented in (auto-generated) native code
* Feature 8093: Add icinga, cluster, cluster-zone check information to the ApiListener status handler
* Feature 8149: graphite writer should pass "-" in host names and "." in perf data
* Feature 8666: Allow some of the Array and Dictionary methods to be inlined by the compiler
* Feature 8688: Add embedded DB IDO version health check
* Feature 8689: Add support for current and current-1 db ido schema version
* Feature 8690: 'icinga2 console' should serialize temporary attributes (rather than just config + state)
* Feature 8738: Implement support for CLIENT_MULTI_STATEMENTS
* Feature 8741: Deprecate IcingaStatusWriter feature
* Feature 8775: Move the base command templates into libmethods
* Feature 8776: Implement support for libedit
* Feature 8791: Refactor the startup process
* Feature 8832: Implement constructor-style casts
* Feature 8842: Add support for the C++11 keyword 'override'
* Feature 8867: Use DebugHint information when reporting validation errors
* Feature 8890: Move implementation code from thpp files into separate files
* Feature 8922: Avoid unnecessary dictionary lookups
* Feature 9044: Remove the ScopeCurrent constant
* Feature 9068: Implement sandbox mode for the config parser
* Feature 9074: Basic API framework
* Feature 9076: Reflection support for the API
* Feature 9077: Implement filters for the API
* Feature 9078: Event stream support for the API
* Feature 9079: Implement status queries for the API
* Feature 9080: Add commands (actions) for the API
* Feature 9081: Add modified attribute support for the API
* Feature 9082: Runtime configuration for the API
* Feature 9083: Configuration file management for the API
* Feature 9084: Enable the ApiListener by default
* Feature 9085: Certificate-based authentication for the API
* Feature 9086: Password-based authentication for the API
* Feature 9087: Create default administrative user
* Feature 9088: API permissions
* Feature 9091: API status queries
* Feature 9093: Changelog for modified attributes
* Feature 9095: Disallow changes for certain config attributes at runtime
* Feature 9096: Dependency tracking for objects
* Feature 9098: Update modules to support adding and removing objects at runtime
* Feature 9099: Implement support for writing configuration files
* Feature 9100: Multiple sources for zone configuration tree
* Feature 9101: Commands for adding and removing objects
* Feature 9102: Support validating configuration changes
* Feature 9103: Staging for configuration validation
* Feature 9104: Implement config file management commands
* Feature 9105: API Documentation
* Feature 9175: Move 'running_kernel' check command to plugins-contrib 'operating system' section
* Feature 9286: DB IDO/Livestatus: Add zone object table w/ endpoint members
* Feature 9414: "-Wno-deprecated-register" compiler option breaks builds on SLES 11
* Feature 9447: Implement support for HTTP
* Feature 9448: Define RESTful url schema
* Feature 9461: New Graphite schema
* Feature 9470: Implement URL parser
* Feature 9471: Implement ApiUser type
* Feature 9594: Implement base64 de- and encoder
* Feature 9614: Register ServiceOK, ServiceWarning, HostUp, etc. as constants
* Feature 9647: Move url to /lib/remote from /lib/base
* Feature 9689: Add exceptions for Utility::MkDir{,P}
* Feature 9693: Add Array::FromVector() method
* Feature 9698: Implement support for X-HTTP-Method-Override
* Feature 9704: String::Trim() should return a new string rather than modifying the current string
* Feature 9705: Add real path sanity checks to provided file paths
* Feature 9723: Documentation for config management API
* Feature 9768: Update the url parsers behaviour
* Feature 9777: Make Comments and Downtime types available as ConfigObject type in the API
* Feature 9794: Setting global variables with i2tcl doesn't work
* Feature 9849: Validation for modified attributes
* Feature 9850: Re-implement events for attribute changes
* Feature 9851: Remove GetModifiedAttributes/SetModifiedAttributes
* Feature 9852: Implement support for . in modify_attribute
* Feature 9859: Implement global modified attributes
* Feature 9866: Implement support for attaching GDB to the Icinga process on crash
* Feature 9914: Rename DynamicObject/DynamicType to ConfigObject/ConfigType
* Feature 9919: Allow comments when parsing JSON
* Feature 9921: Implement the 'base' field for the Type class
* Feature 9926: Ensure that runtime config objects are persisted on disk
* Feature 9927: Figure out how to sync dynamically created objects inside the cluster
* Feature 9929: Add override keyword for all relevant methods
* Feature 9930: Document Object#clone
* Feature 9931: Implement Object#clone and rename Array/Dictionary#clone to shallow_clone
* Feature 9933: Implement support for indexers in ConfigObject::RestoreAttribute
* Feature 9935: Implement support for restoring modified attributes
* Feature 9937: Add package attribute for ConfigObject and set its origin
* Feature 9940: Implement support for filter_vars
* Feature 9944: Add String::ToLower/ToUpper
* Feature 9946: Remove debug messages in HttpRequest class
* Feature 9953: Rename config/modules to config/packages
* Feature 9960: Implement ignore_on_error keyword
* Feature 10017: Use an AST node for the 'library' keyword
* Feature 10038: Add plural_name field to /v1/types
* Feature 10039: URL class improvements
* Feature 10042: Implement a demo API client: Icinga Studio
* Feature 10060: Implement joins for status queries
* Feature 10116: Add global status handler for the API
* Feature 10186: Make ConfigObject::{G,S}etField() method public
* Feature 10194: Sanitize error status codes and messages
* Feature 10202: Add documentation for api-users.conf and app.conf
* Feature 10209: Rename statusqueryhandler to objectqueryhandler
* Feature 10212: Move /v1/<type> to /v1/objects/<type>
* Feature 10243: Provide keywords to retrieve the current file name at parse time
* Feature 10257: Change object version to timestamps for diff updates on config sync
* Feature 10329: Pretty-print arrays and dictionaries when converting them to strings
* Feature 10368: Document that modified attributes require accept_config for cluster/clients
* Feature 10374: Add check command nginx_status
* Feature 10383: DB IDO should provide its connected state via /v1/status
* Feature 10385: Add 'support' tracker to changelog.py
* Feature 10387: Use the API for "icinga2 console"
* Feature 10388: Log a warning message on unauthorized http request
* Feature 10392: Original attributes list in IDO
* Feature 10393: Hide internal attributes
* Feature 10394: Add getter for endpoint 'connected' attribute
* Feature 10407: Remove api.cpp, api.hpp
* Feature 10409: Add documentation for apply+for in the language reference chapter
* Feature 10423: Ability to set port on SNMP Checks
* Feature 10431: Add the name for comments/downtimes next to legacy_id to DB IDO
* Feature 10441: Rewrite man page
* Feature 10479: Use ZoneName variable for parent_zone in node update-config
* Feature 10482: Documentation: Reorganize Livestatus and alternative frontends
* Feature 10503: Missing parameters for check jmx4perl
* Feature 10507: Add check command negate
* Feature 10509: Change GetLastStateUp/Down to host attributes
* Feature 10511: Add check command mysql
* Feature 10513: Add ipv4/ipv6 only to tcp and http CheckCommand
* Feature 10522: Change output format for 'icinga2 console'
* Feature 10547: Icinga 2 script debugger
* Feature 10548: Implement CSRF protection for the API
* Feature 10549: Change 'api setup' into a manual step while configuring the API
* Feature 10551: Change object query result set
* Feature 10566: Enhance programmatic examples for the API docs
* Feature 10574: Mention wxWidget (optional) requirement in INSTALL.md
* Feature 10575: Documentation for /v1/console
* Feature 10576: Explain variable names for joined objects in filter expressions
* Feature 10577: Documentation for the script debugger
* Feature 10591: Explain DELETE for config stages/packages
* Feature 10630: Update wxWidgets documentation for Icinga Studio

#### Bugfixes

* Bug 8822: Update OpenSSL for the Windows builds
* Bug 8823: Don't allow users to instantiate the StreamLogger class
* Bug 8830: Make default notifications include users from host.vars.notification.mail.users
* Bug 8865: Failed assertion in IdoMysqlConnection::FieldToEscapedString
* Bug 8907: Validation fails even though field is not required
* Bug 8924: Specify pidfile for status_of_proc in the init script
* Bug 8952: Crash in VMOps::FunctionCall
* Bug 8989: pgsql driver does not have latest mysql changes synced
* Bug 9015: Compiler warnings with latest HEAD 5ac5f98
* Bug 9027: PostgreSQL schema sets default timestamps w/o time zone
* Bug 9053: icinga demo module can not be built
* Bug 9188: Remove incorrect 'ignore where' expression from 'ssh' apply example
* Bug 9455: Fix incorrect datatype for the check_source column in icinga_statehistory table
* Bug 9547: Wrong vars changed handler in api events
* Bug 9576: Overflow in freshness_threshold column (smallint) w/ DB IDO MySQL
* Bug 9590: 'node wizard/setup' should always generate new CN certificates
* Bug 9703: Problem with child nodes in http url registry
* Bug 9735: Broken cluster config sync w/o include_zones
* Bug 9778: Accessing field ID 0 ("prototype") fails
* Bug 9793: Operator - should not work with "" and numbers
* Bug 9795: ScriptFrame's 'Self' attribute gets corrupted when an expression throws an exception
* Bug 9813: win32 build: S_ISDIR is undefined
* Bug 9843: console autocompletion should take into account parent classes' prototypes
* Bug 9868: Crash in ScriptFrame::~ScriptFrame
* Bug 9872: Color codes in console prompt break line editing
* Bug 9876: Crash during cluster log replay
* Bug 9879: Missing conf.d or zones.d cause parse failure
* Bug 9911: Do not let API users create objects with invalid names
* Bug 9966: Fix formatting in mkclass
* Bug 9968: Implement support for '.' when persisting modified attributes
* Bug 9987: Crash in ConfigCompiler::RegisterZoneDir
* Bug 10008: Don't parse config files for branches not taken
* Bug 10012: Unused variable 'dobj' in configobject.tcpp
* Bug 10024: HTTP keep-alive does not work with .NET WebClient
* Bug 10027: Filtering by name doesn't work
* Bug 10034: Unused variable console_type in consolecommand.cpp
* Bug 10041: build failure: demo module
* Bug 10048: Error handling in HttpClient/icinga-studio
* Bug 10110: Add object_id where clause for icinga_downtimehistory
* Bug 10180: API actions do not follow REST guidelines
* Bug 10198: Detect infinite recursion in user scripts
* Bug 10210: Move the Collection status handler to /v1/status
* Bug 10211: PerfdataValue is not properly serialised in status queries
* Bug 10224: URL parser is cutting off last character
* Bug 10234: ASCII NULs don't work in string values
* Bug 10238: Use a temporary file for modified-attributes.conf updates
* Bug 10241: Properly encode URLs in Icinga Studio
* Bug 10249: Config Sync shouldn't send updates for objects the client doesn't have access to
* Bug 10253: /v1/objects/<type> returns an HTTP error when there are no objects of that type
* Bug 10255: Config sync does not set endpoint syncing and plays disconnect-sync ping-pong
* Bug 10256: ConfigWriter::EmitValue should format floating point values properly
* Bug 10326: icinga2 repository host add does not work
* Bug 10350: Remove duplicated text in section "Apply Notifications to Hosts and Services"
* Bug 10355: Version updates are not working properly
* Bug 10360: Icinga2 API performance regression
* Bug 10371: Ensure that modified attributes work with clients with local config and no zone attribute
* Bug 10386: restore_attribute does not work in clusters
* Bug 10403: Escaping $ not documented
* Bug 10406: Misleading wording in generated zones.conf
* Bug 10410: OpenBSD: hang during ConfigItem::ActivateItems() in daemon startup
* Bug 10417: 'which' isn't available in a minimal CentOS container
* Bug 10422: Changing a group's attributes causes duplicate rows in the icinga_*group_members table
* Bug 10433: 'dig_lookup' custom attribute for the 'dig' check command isn't optional
* Bug 10436: Custom variables aren't removed from the IDO database
* Bug 10439: "Command options" is empty when executing icinga2 without any argument.
* Bug 10440: Improve --help output for the --log-level option
* Bug 10455: Improve error handling during log replay
* Bug 10456: Incorrect attribute name in the documentation
* Bug 10457: Don't allow scripts to access FANoUserView attributes in sandbox mode
* Bug 10461: Line continuation is broken in 'icinga2 console'
* Bug 10466: Crash in IndexerExpression::GetReference when attempting to set an attribute on an object other than the current one
* Bug 10473: IDO tries to execute empty UPDATE queries
* Bug 10491: Unique constraint violation with multiple comment inserts in DB IDO
* Bug 10495: Incorrect JSON-RPC message causes Icinga 2 to crash
* Bug 10498: IcingaStudio: Accessing non-ConfigObjects causes ugly exception
* Bug 10501: Plural name rule not treating edge case correcly
* Bug 10504: Increase the default timeout for OS checks
* Bug 10508: Figure out whether we need the Checkable attributes state_raw, last_state_raw, hard_state_raw
* Bug 10510: CreatePipeOverlapped is not thread-safe
* Bug 10512: Mismatch on {comment,downtime}_id vs internal name in the API
* Bug 10517: Circular reference between *Connection and TlsStream objects
* Bug 10518: Crash in ConfigWriter::GetKeywords
* Bug 10527: Fix indentation for Dictionary::ToString
* Bug 10529: Change session_token to integer timestamp
* Bug 10535: Spaces do not work in command arguments
* Bug 10538: Crash in ConfigWriter::EmitIdentifier
* Bug 10539: Don't validate custom attributes that aren't strings
* Bug 10540: Async mysql queries aren't logged in the debug log
* Bug 10545: Broken build - unresolved external symbol "public: void __thiscall icinga::ApiClient::ExecuteScript...
* Bug 10555: Don't try to use --gc-sections on Solaris
* Bug 10556: Update OpenSSL for the Windows builds
* Bug 10558: There's a variable called 'string' in filter expressions
* Bug 10559: Autocompletion doesn't work in the debugger
* Bug 10560: 'api setup' should create a user even when api feature is already enabled
* Bug 10561: 'remove-comment' action does not support filters
* Bug 10562: Documentation should not reference real host names
* Bug 10563: /v1/console should only use a single permission
* Bug 10568: Improve location information for errors in API filters
* Bug 10569: Icinga 2 API Docs
* Bug 10578: API call doesn't fail when trying to use a template that doesn't exist
* Bug 10580: Detailed error message is missing when object creation via API fails
* Bug 10583: modify_attribute: object cannot be cloned
* Bug 10588: Documentation for /v1/types
* Bug 10596: Deadlock in MacroProcessor::EvaluateFunction
* Bug 10601: Don't allow users to set state attributes via PUT
* Bug 10602: API overwrites (and then deletes) config file when trying to create an object that already exists
* Bug 10604: Group memberships are not updated for runtime created objects
* Bug 10629: Download URL for NSClient++ is incorrect
* Bug 10637: Utility::FormatErrorNumber fails when error message uses arguments

### What's New in Version 2.3.11

#### Changes

* Function for performing CIDR matches: cidr_match()
* New methods: String#reverse and Array#reverse
* New ITL command definitions: nwc_health, hpasm, squid, pgsql
* Additional arguments for ITL command definitions: by_ssh, dig, pop, spop, imap, simap
* Documentation updates
* Various bugfixes

#### Features

* Feature 9183: Add timestamp support for OpenTsdbWriter
* Feature 9466: Add FreeBSD setup to getting started
* Feature 9812: add check command for check_nwc_health
* Feature 9854: check_command for plugin check_hpasm
* Feature 10004: escape_shell_arg() method
* Feature 10006: Implement a way for users to resolve commands+arguments in the same way Icinga does
* Feature 10057: Command Execution Bridge: Use of same endpoint names in examples for a better understanding
* Feature 10109: Add check command squid
* Feature 10112: Add check command pgsql
* Feature 10129: Add ipv4/ipv6 only to nrpe CheckCommand
* Feature 10139: expand check command dig
* Feature 10142: Update debug docs for core dumps and full backtraces
* Feature 10157: Update graphing section in the docs
* Feature 10158: Make check_disk.exe CheckCommand Config more verbose
* Feature 10161: Improve documentation for check_memory
* Feature 10197: Implement the Array#reverse and String#reverse methods
* Feature 10207: Find a better description for cluster communication requirements
* Feature 10216: Clarify on cluster/client naming convention and add troubleshooting section
* Feature 10219: Add timeout argument for pop, spop, imap, simap commands
* Feature 10352: Improve timeperiod documentation
* Feature 10354: New method: cidr_match()
* Feature 10379: Add a debug log message for updating the program status table in DB IDO

#### Bugfixes

* Bug 8805: check cluster-zone returns wrong log lag
* Bug 9322: sending multiple Livestatus commands rejects all except the first
* Bug 10002: Deadlock in WorkQueue::Enqueue
* Bug 10079: Improve error message for socket errors in Livestatus
* Bug 10093: Rather use unique SID when granting rights for folders in NSIS on Windows Client
* Bug 10177: Windows Check Update -> Access denied
* Bug 10191: String methods cannot be invoked on an empty string
* Bug 10192: null + null should not be ""
* Bug 10199: Remove unnecessary MakeLiteral calls in SetExpression::DoEvaluate
* Bug 10204: Config parser problem with parenthesis and newlines
* Bug 10205: config checker reports wrong error on apply for rules
* Bug 10235: Deadlock in TlsStream::Close
* Bug 10239: Don't throw an exception when replaying the current replay log file
* Bug 10245: Percent character whitespace on Windows
* Bug 10254: Performance Data Labels including '=' will not be displayed correct
* Bug 10262: Don't log messages we've already relayed to all relevant zones
* Bug 10266: "Not after" value overflows in X509 certificates on RHEL5
* Bug 10348: Checkresultreader is unable to process host checks
* Bug 10349: Missing Start call for base class in CheckResultReader
* Bug 10351: Broken table layout in chapter 20
* Bug 10365: ApiListener::SyncRelayMessage doesn't send message to all zone members
* Bug 10377: Wrong connection log message for global zones

### What's New in Version 2.3.10

#### Features

* Feature 9218: Use the command_endpoint name as check_source value if defined

#### Bugfixes

* Bug 9244: String escape problem with PostgreSQL >= 9.1 and standard_conforming_strings=on
* Bug 10003: Nested "outer" macro calls fails on (handled) missing "inner" values
* Bug 10051: Missing fix for reload on Windows in 2.3.9
* Bug 10058: Wrong calculation for host compat state "UNREACHABLE" in DB IDO
* Bug 10074: Missing zero padding for generated CA serial.txt

### What's New in Version 2.3.9

#### Changes

* Fix that the first SOFT state is recognized as second SOFT state
* Implemented reload functionality for Windows
* New ITL check commands
* Documentation updates
* Various other bugfixes

#### Features

* Feature 9527: CheckCommand for check_interfaces
* Feature 9671: Add check_yum to ITL
* Feature 9675: Add check_redis to ITL
* Feature 9686: Update gdb pretty printer docs w/ Python 3
* Feature 9699: Adding "-r" parameter to the check_load command for dividing the load averages by the number of CPUs.
* Feature 9747: check_command for plugin check_clamd
* Feature 9796: Implement Dictionary#get and Array#get
* Feature 9801: Add check_jmx4perl to ITL
* Feature 9811: add check command for check_mailq
* Feature 9827: snmpv3 CheckCommand section improved
* Feature 9882: Implement the Dictionary#keys method
* Feature 9883: Use an empty dictionary for the 'this' scope when executing commands with Livestatus
* Feature 9985: add check command nscp-local-counter
* Feature 9996: Add new arguments openvmtools for Open VM Tools

#### Bugfixes

* Bug 8979: Missing DEL_DOWNTIME_BY_HOST_NAME command required by Classic UI 1.x
* Bug 9262: cluster check w/ immediate parent and child zone endpoints
* Bug 9623: missing config warning on empty port in endpoints
* Bug 9769: Set correct X509 version for certificates
* Bug 9773: Add log for missing EventCommand for command_endpoints
* Bug 9779: Trying to set a field for a non-object instance fails
* Bug 9782: icinga2 node wizard don't take zone_name input
* Bug 9806: Operator + is inconsistent when used with empty and non-empty strings
* Bug 9814: Build fix for Boost 1.59
* Bug 9835: Dict initializer incorrectly re-initializes field that is set to an empty string
* Bug 9860: missing check_perfmon.exe
* Bug 9867: Agent freezes when the check returns massive output
* Bug 9884: Warning about invalid API function icinga::Hello
* Bug 9897: First SOFT state is recognized as second SOFT state
* Bug 9902: typo in docs
* Bug 9912: check_command interfaces option match_aliases has to be boolean
* Bug 9913: Default disk checks on Windows fail because check_disk doesn't support -K
* Bug 9928: Add missing category for IDO query
* Bug 9947: Serial number field is not properly initialized for CA certificates
* Bug 9961: Don't re-download NSCP for every build
* Bug 9962: Utility::Glob on Windows doesn't support wildcards in all but the last path component
* Bug 9972: Icinga2 - too many open files - Exception
* Bug 9984: fix check command nscp-local
* Bug 9992: Duplicate severity type in the documentation for SyslogLogger

### What's New in Version 2.3.8

#### Changes

* Bugfixes

#### Bugfixes

* Bug 9554: Don't allow "ignore where" for groups when there's no "assign where"
* Bug 9634: DB IDO: Do not update endpointstatus table on config updates
* Bug 9637: Wrong parameter for CheckCommand "ping-common-windows"
* Bug 9665: Escaping does not work for OpenTSDB perfdata plugin
* Bug 9666: checkcommand disk does not check free inode - check_disk

### What's New in Version 2.3.7

#### Changes

* Bugfixes

#### Features

* Feature 9610: Enhance troubleshooting ssl errors & cluster replay log

#### Bugfixes

* Bug 9406: Selective cluster reconnecting breaks client communication
* Bug 9535: Config parser ignores "ignore" in template definition
* Bug 9584: Incorrect return value for the macro() function
* Bug 9585: Wrong formatting in DB IDO extensions docs
* Bug 9586: DB IDO: endpoint* tables are cleared on reload causing constraint violations
* Bug 9621: Assertion failed in icinga::ScriptUtils::Intersection
* Bug 9622: Missing lock in ScriptUtils::Union

### What's New in Version 2.3.6

#### Changes

* Require openssl1 on sles11sp3 from Security Module repository
  * Bug in SLES 11's OpenSSL version 0.9.8j preventing verification of generated certificates.
  * Re-create these certificates with 2.3.6 linking against openssl1 (cli command or CSR auto-signing).
* ITL: Add ldap, ntp_peer, mongodb and elasticsearch CheckCommand definitions
* Bugfixes

#### Features

* Feature 6714: add pagerduty notification documentation
* Feature 9172: Add "ldap" CheckCommand for "check_ldap" plugin
* Feature 9191: Add "mongodb" CheckCommand definition
* Feature 9415: Add elasticsearch checkcommand to itl
* Feature 9416: snmpv3 CheckCommand: Add possibility to set securityLevel
* Feature 9451: Merge documentation fixes from GitHub
* Feature 9523: Add ntp_peer CheckCommand
* Feature 9562: Add new options for ntp_time CheckCommand
* Feature 9578: new options for smtp CheckCommand

#### Bugfixes

* Bug 9205: port empty when using icinga2 node wizard
* Bug 9253: Incorrect variable name in the ITL
* Bug 9303: Missing 'snmp_is_cisco' in Manubulon snmp-memory command definition
* Bug 9436: Functions can't be specified as command arguments
* Bug 9450: node setup: indent accept_config and accept_commands
* Bug 9452: Wrong file reference in README.md
* Bug 9456: Windows client w/ command_endpoint broken with $nscp_path$ and NscpPath detection
* Bug 9463: Incorrect check_ping.exe parameter in the ITL
* Bug 9476: Documentation for checks in an HA zone is wrong
* Bug 9481: Fix stability issues in the TlsStream/Stream classes
* Bug 9489: Add log message for discarded cluster events (e.g. from unauthenticated clients)
* Bug 9490: Missing openssl verify in cluster troubleshooting docs
* Bug 9513: itl/plugins-contrib.d/*.conf should point to PluginContribDir
* Bug 9522: wrong default port documentated for nrpe
* Bug 9549: Generated certificates cannot be verified w/ openssl 0.9.8j on SLES 11
* Bug 9558: mysql-devel is not available in sles11sp3
* Bug 9563: Update getting started for Debian Jessie

### What's New in Version 2.3.5

#### Changes

* NSClient++ is now bundled with the Windows setup wizard and can optionally be installed
* Windows Wizard: "include <nscp>" is set by default
* Windows Wizard: Add update mode
* Plugins: Add check_perfmon plugin for Windows
* ITL: Add CheckCommand objects for Windows plugins ("include <windows-plugins>")
* ITL: Add CheckCommand definitions for "mongodb", "iftraffic", "disk_smb"
* ITL: Add arguments to CheckCommands "dns", "ftp", "tcp", "nscp"

#### Features

* Feature 8116: Extend Windows installer with an update mode
* Feature 8180: Add documentation and CheckCommands for the windows plugins
* Feature 8809: Add check_perfmon plugin for Windows
* Feature 9115: Add SHOWALL to NSCP Checkcommand
* Feature 9130: Add 'check_drivesize' as nscp-local check command
* Feature 9145: Add arguments to "dns" CheckCommand
* Feature 9146: Add arguments to "ftp" CheckCommand
* Feature 9147: Add arguments to "tcp" CheckCommand
* Feature 9176: ITL Documentation: Add a link for passing custom attributes as command parameters
* Feature 9180: Include Windows support details in the documentation
* Feature 9185: Add timestamp support for PerfdataWriter
* Feature 9191: Add "mongodb" CheckCommand definition
* Feature 9238: Bundle NSClient++ in Windows Installer
* Feature 9254: Add 'disk_smb' Plugin CheckCommand definition
* Feature 9256: Determine NSClient++ installation path using MsiGetComponentPath
* Feature 9260: Include <nscp> by default on Windows
* Feature 9261: Add the --load-all and --log options for nscp-local
* Feature 9263: Add support for installing NSClient++ in the Icinga 2 Windows wizard
* Feature 9270: Update service apply for documentation
* Feature 9272: Add 'iftraffic' to plugins-contrib check command definitions
* Feature 9285: Best practices: cluster config sync
* Feature 9297: Add examples for function usage in "set_if" and "command" attributes
* Feature 9310: Add typeof in 'assign/ignore where' expression as example
* Feature 9311: Add local variable scope for *Command to documentation (host, service, etc)
* Feature 9313: Use a more simple example for passing command parameters
* Feature 9318: Explain string concatenation in objects by real-world example
* Feature 9363: Update documentation for escape sequences
* Feature 9419: Enhance cluster/client troubleshooting
* Feature 9420: Enhance cluster docs with HA command_endpoints
* Feature 9431: Documentation: Move configuration before advanced topics

#### Bugfixes

* Bug 8853: Syntax Highlighting: host.address vs host.add
* Bug 8888: Icinga2 --version: Error showing Distribution
* Bug 8891: Node wont connect properly to master if host is is not set for Endpoint on new installs
* Bug 9055: Wrong timestamps w/ historical data replay in DB IDO
* Bug 9109: WIN: syslog is not an enable-able feature in windows
* Bug 9116: node update-config reports critical and warning
* Bug 9121: Possible DB deadlock
* Bug 9131: Missing ")" in last Apply Rules example
* Bug 9142: Downtimes are always "fixed"
* Bug 9143: Incorrect type and state filter mapping for User objects in DB IDO
* Bug 9161: 'disk': wrong order of threshold command arguments
* Bug 9187: SPEC: Give group write permissions for perfdata dir
* Bug 9205: port empty when using icinga2 node wizard
* Bug 9222: Missing custom attributes in backends if name is equal to object attribute
* Bug 9253: Incorrect variable name in the ITL
* Bug 9255: --scm-installs fails when the service is already installed
* Bug 9258: Some checks in the default Windows configuration fail
* Bug 9259: Disk and 'icinga' services are missing in the default Windows config
* Bug 9268: Typo in Configuration Best Practice
* Bug 9269: Wrong permission etc on windows
* Bug 9324: Multi line output not correctly handled from compat channels
* Bug 9328: Multiline vars are broken in objects.cache output
* Bug 9372: plugins-contrib.d/databases.conf: wrong argument for mssql_health
* Bug 9389: Documentation: Typo
* Bug 9390: Wrong service table attributes in Livestatus documentation
* Bug 9393: Documentation: Extend Custom Attributes with the boolean type
* Bug 9394: Including <nscp> on Linux fails with unregistered function
* Bug 9399: Documentation: Typo
* Bug 9406: Selective cluster reconnecting breaks client communication
* Bug 9412: Documentation: Update the link to register a new Icinga account

### What's New in Version 2.3.4

#### Changes

* ITL: Check commands for various databases
* Improve validation messages for time periods
* Update max_check_attempts in generic-{host,service} templates
* Update logrotate configuration
* Bugfixes

#### Features

* Feature 8760: Add database plugins to ITL
* Feature 8803: Agent Wizard: add options for API defaults
* Feature 8893: Improve timeperiod validation error messages
* Feature 8895: Add explanatory note for Icinga2 client documentation

#### Bugfixes

* Bug 8808: logrotate doesn't work on Ubuntu
* Bug 8821: command_endpoint check_results are not replicated to other endpoints in the same zone
* Bug 8879: Reword documentation of check_address
* Bug 8881: Add arguments to the UPS check
* Bug 8889: Fix a minor markdown error
* Bug 8892: Validation errors for time ranges which span the DST transition
* Bug 8894: Default max_check_attempts should be lower for hosts than for services
* Bug 8913: Windows Build: Flex detection
* Bug 8917: Node wizard should only accept 'y', 'n', 'Y' and 'N' as answers for boolean questions
* Bug 8919: Fix complexity class for Dictionary::Get
* Bug 8987: Fix a typo
* Bug 9012: Typo in graphite feature enable documentation
* Bug 9014: Don't update scheduleddowntime table w/ trigger_time column when only adding a downtime
* Bug 9016: Downtimes which have been triggered are not properly recorded in the database
* Bug 9017: scheduled_downtime_depth column is not reset when a downtime ends or when a downtime is being removed
* Bug 9021: Multiple log messages w/ "Attempting to send notifications for notification object"
* Bug 9041: Acknowledging problems w/ expire time does not add the expiry information to the related comment for IDO and compat
* Bug 9045: Vim syntax: Match groups before host/service/user objects
* Bug 9049: check_disk order of command arguments
* Bug 9050: web.conf is not in the RPM package
* Bug 9064: troubleshoot truncates crash reports
* Bug 9069: Documentation: set_if usage with boolean values and functions
* Bug 9073: custom attributes with recursive macro function calls causing sigabrt

### What's New in Version 2.3.3

#### Changes

* New function: parse_performance_data
* Include more details in --version
* Improve documentation
* Bugfixes

#### Features

* Feature 8685: Show state/type filter names in notice/debug log
* Feature 8686: Update documentation for "apply for" rules
* Feature 8693: New function: parse_performance_data
* Feature 8740: Add "access objects at runtime" examples to advanced section
* Feature 8761: Include more details in --version
* Feature 8816: Add "random" CheckCommand for test and demo purposes
* Feature 8827: Move release info in INSTALL.md into a separate file

#### Bugfixes

* Bug 8660: Update syntax highlighting for 2.3 features
* Bug 8677: Re-order the object types in alphabetical order
* Bug 8724: Missing config validator for command arguments 'set_if'
* Bug 8734: startup.log broken when the DB schema needs an update
* Bug 8736: Don't update custom vars for each status update
* Bug 8748: Don't ignore extraneous arguments for functions
* Bug 8749: Build warnings with CMake 3.1.3
* Bug 8750: Flex version check does not reject unsupported versions
* Bug 8753: Fix a typo in the documentation of ICINGA2_WITH_MYSQL and ICINGA2_WITH_PGSQL
* Bug 8755: Fix VIM syntax highlighting for comments
* Bug 8757: Add missing keywords in the syntax highlighting files
* Bug 8762: Plugin "check_http" is missing in Windows environments
* Bug 8763: Typo in doc library-reference
* Bug 8764: Revamp migration documentation
* Bug 8765: Explain processing logic/order of apply rules with for loops
* Bug 8766: Remove prompt to create a TicketSalt from the wizard
* Bug 8767: Typo and invalid example in the runtime macro documentation
* Bug 8769: Improve error message for invalid field access
* Bug 8770: object Notification + apply Service fails with error "...refers to service which doesn't exist"
* Bug 8771: Correct HA documentation
* Bug 8829: Figure out why command validators are not triggered
* Bug 8834: Return doesn't work inside loops
* Bug 8844: Segmentation fault when executing "icinga2 pki new-cert"
* Bug 8862: wrong 'dns_lookup' custom attribute default in command-plugins.conf
* Bug 8866: Fix incorrect perfdata templates in the documentation
* Bug 8869: Array in command arguments doesn't work

### What's New in Version 2.3.2

#### Changes

* Bugfixes

#### Bugfixes

* Bug 8721: Log message for cli commands breaks the init script

### What's New in Version 2.3.1

#### Changes

* Bugfixes

Please note that this version fixes the default thresholds for the disk check which were inadvertently broken in 2.3.0; if you're using percent-based custom thresholds you will need to add the '%' sign to your custom attributes

#### Features

* Feature 8659: Implement String#contains

#### Bugfixes

* Bug 8540: Kill signal sent only to check process, not whole process group
* Bug 8657: Missing program name in 'icinga2 --version'
* Bug 8658: Fix check_disk thresholds: make sure partitions are the last arguments
* Bug 8672: Api heartbeat message response time problem
* Bug 8673: Fix check_disk default thresholds and document the change of unit
* Bug 8679: Config validation fail because of unexpected new-line
* Bug 8680: Update documentation for DB IDO HA Run-Once
* Bug 8683: Make sure that the /var/log/icinga2/crash directory exists
* Bug 8684: Fix formatting for the GDB stacktrace
* Bug 8687: Crash in Dependency::Stop
* Bug 8691: Debian packages do not create /var/log/icinga2/crash

### What's New in Version 2.3.0

#### Changes

* Improved configuration validation
    * Unnecessary escapes are no longer permitted (e.g. \')
    * Dashes are no longer permitted in identifier names (as their semantics are ambiguous)
    * Unused values are detected (e.g. { "-M" })
    * Validation for time ranges has been improved
    * Additional validation rules for some object types (Notification and User)
* New language features
    * Implement a separate type for boolean values
    * Support for user-defined functions
    * Support for conditional statements (if/else)
    * Support for 'for' and 'while' loops
    * Support for local variables using the 'var' keyword
    * New operators: % (modulo), ^ (xor), - (unary minus) and + (unary plus)
    * Implemented prototype-based methods for most built-in types (e.g. [ 3, 2 ].sort())
    * Explicit access to local and global variables using the 'locals' and 'globals' keywords
    * Changed the order in which filters are evaluated for apply rules with 'for'
    * Make type objects accessible as global variables
    * Support for using functions in custom attributes
    * Access objects and their runtime attributes in functions (e.g. get_host(NodeName).state)
* ITL improvements
    * Additional check commands were added to the ITL
    * Additional arguments for existing check commands
* CLI improvements
    * Add the 'icinga2 console' CLI command which can be used to test expressions
    * Add the 'icinga2 troubleshoot' CLI command for collecting troubleshooting information
    * Performance improvements for the 'icinga2 node update-config' CLI command
    * Implement argument auto-completion for short options (e.g. daemon -c)
    * 'node setup' and 'node wizard' create backups for existing certificate files
* Add ignore_soft_states option for Dependency object configuration
* Fewer threads are used for socket I/O
* Flapping detection for hosts and services is disabled by default
* Added support for OpenTSDB
* New Livestatus tables: hostsbygroup, servicesbygroup, servicesbyhostgroup
* Include GDB backtrace in crash reports
* Various documentation improvements
* Solved a number of issues where cluster instances would not reconnect after intermittent connection problems
* A lot of other, minor changes

* [DB IDO schema upgrade](17-upgrading-icinga-2.md#upgrading-icinga-2) to `1.13.0` required!

#### Features

* Feature 3446: Add troubleshooting collect cli command
* Feature 6109: Don't spawn threads for network connections
* Feature 6570: Disallow side-effect-free r-value expressions in expression lists
* Feature 6697: Plugin Check Commands: add check_vmware_esx
* Feature 6857: Run CheckCommands with C locale (workaround for comma vs dot and plugin api bug)
* Feature 6858: Add some more PNP details
* Feature 6868: Disable flapping detection by default
* Feature 6923: IDO should fill program_end_time on a clean shutdown
* Feature 7136: extended Manubulon SNMP Check Plugin Command
* Feature 7209: ITL: Interfacetable
* Feature 7256: Add OpenTSDB Writer
* Feature 7292: ITL: Check_Mem.pl
* Feature 7294: ITL: ESXi-Hardware
* Feature 7326: Add parent soft states option to Dependency object configuration
* Feature 7361: Livestatus: Add GroupBy tables: hostsbygroup, servicesbygroup, servicesbyhostgroup
* Feature 7545: Please add labels in SNMP checks
* Feature 7564: Access object runtime attributes in custom vars & command arguments
* Feature 7610: Variable from for loop not usable in assign statement
* Feature 7700: Evaluate apply/object rules when the parent objects are created
* Feature 7702: Add an option that hides CLI commands
* Feature 7704: ConfigCompiler::HandleInclude* should return an AST node
* Feature 7706: ConfigCompiler::Compile* should return an AST node
* Feature 7748: Redesign how stack frames work for scripts
* Feature 7767: Rename _DEBUG to I2_DEBUG
* Feature 7774: Implement an AST Expression for T_CONST
* Feature 7778: Missing check_disk output on Windows
* Feature 7784: Implement the DISABLE_HOST_SVC_NOTIFICATIONS and ENABLE_HOST_SVC_NOTIFICATIONS commands
* Feature 7793: Don't build db_ido when both MySQL and PostgreSQL aren't enabled
* Feature 7794: Implement an option to disable building the Livestatus module
* Feature 7795: Implement an option to disable building the Demo component
* Feature 7805: Implement unit tests for the config parser
* Feature 7807: Move the cast functions into libbase
* Feature 7813: Implement the % operator
* Feature 7816: Document operator precedence
* Feature 7822: Make the config parser thread-safe
* Feature 7823: Figure out whether Number + String should implicitly convert the Number argument to a string
* Feature 7824: Implement the "if" and "else" keywords
* Feature 7873: Plugin Check Commands: Add icmp
* Feature 7879: Windows agent is missing the standard plugin check_ping
* Feature 7883: Implement official support for user-defined functions and the "for" keyword
* Feature 7901: Implement socket_path attribute for the IdoMysqlConnection class
* Feature 7910: The lexer shouldn't accept escapes for characters which don't have to be escaped
* Feature 7925: Move the config file for the ido-*sql features into the icinga2-ido-* packages
* Feature 8016: Documentation enhancement for snmp traps and passive checks.
* Feature 8019: Register type objects as global variables
* Feature 8020: Improve output of ToString for type objects
* Feature 8030: Evaluate usage of function()
* Feature 8033: Allow name changed from inside the object
* Feature 8040: Disallow calling strings as functions
* Feature 8043: Implement a boolean sub-type for the Value class
* Feature 8047: ConfigCompiler::HandleInclude should return an inline dictionary
* Feature 8060: Windows plugins should behave like their Linux cousins
* Feature 8065: Implement a way to remove dictionary keys
* Feature 8071: Implement a way to call methods on objects
* Feature 8074: Figure out how variable scopes should work
* Feature 8078: Backport i2tcl's error reporting functionality into "icinga2 console"
* Feature 8096: Document the new language features in 2.3
* Feature 8121: feature enable should use relative symlinks
* Feature 8133: Implement line-continuation for the "console" command
* Feature 8169: Implement additional methods for strings
* Feature 8172: Assignments shouldn't have a "return" value
* Feature 8195: Host/Service runtime macro downtime_depth
* Feature 8226: Make invalid log-severity option output an error instead of a warning
* Feature 8244: Implement keywords to explicitly access globals/locals
* Feature 8259: The check "hostalive" is not working with ipv6
* Feature 8269: Implement the while keyword
* Feature 8277: Add macros $host.check_source$ and $service.check_source$
* Feature 8290: Make operators &&, || behave like in JavaScript
* Feature 8291: Implement validator support for function objects
* Feature 8293: The Zone::global attribute is not documented
* Feature 8316: Extend disk checkcommand
* Feature 8322: Implement Array#join
* Feature 8371: Add path information for objects in object list
* Feature 8374: Add timestamp support for Graphite
* Feature 8386: Add documentation for cli command 'console'
* Feature 8393: Implement support for Json.encode and Json.decode
* Feature 8394: Implement continue/break keywords
* Feature 8399: Backup certificate files in 'node setup'
* Feature 8410: udp check command is missing arguments.
* Feature 8414: Add ITL check command for check_ipmi_sensor
* Feature 8429: add webinject checkcommand
* Feature 8465: Add the ability to use a CA certificate as a way of verifying hosts for CSR autosigning
* Feature 8467: introduce time dependent variable values
* Feature 8498: Snmp CheckCommand misses various options
* Feature 8515: Show slave lag for the cluster-zone check
* Feature 8522: Update Remote Client/Distributed Monitoring Documentation
* Feature 8527: Change Livestatus query log level to 'notice'
* Feature 8548: Add support for else-if
* Feature 8575: Include GDB backtrace in crash reports
* Feature 8599: Remove macro argument for IMPL_TYPE_LOOKUP
* Feature 8600: Add validator for time ranges in ScheduledDowntime objects
* Feature 8610: Support the SNI TLS extension
* Feature 8621: Add check commands for NSClient++
* Feature 8648: Document closures ('use')

#### Bugfixes

* Bug 6171: Remove name and return value for stats functions
* Bug 6959: Scheduled start time will be ignored if the host or service is already in a problem state
* Bug 7311: Invalid macro results in exception
* Bug 7542: Update validators for CustomVarObject
* Bug 7576: validate configured legacy timeperiod ranges
* Bug 7582: Variable expansion is single quoted.
* Bug 7644: Unity build doesn't work with MSVC
* Bug 7647: Avoid rebuilding libbase when the version number changes
* Bug 7731: Reminder notifications not being sent but logged every 5 secs
* Bug 7765: DB IDO: Duplicate entry icinga_{host,service}dependencies
* Bug 7800: Fix the shift/reduce conflicts in the parser
* Bug 7802: Change parameter type for include and include_recursive to T_STRING
* Bug 7808: Unterminated string literals should cause parser to return an error
* Bug 7809: Scoping rules for "for" are broken
* Bug 7810: Return values for functions are broken
* Bug 7811: The __return keyword is broken
* Bug 7812: Validate array subscripts
* Bug 7814: Set expression should check whether LHS is a null pointer
* Bug 7815: - operator doesn't work in expressions
* Bug 7826: Compiler warnings
* Bug 7830: - shouldn't be allowed in identifiers
* Bug 7871: Missing persistent_comment, notify_contact columns for acknowledgement table
* Bug 7894: Fix warnings when using CMake 3.1.0
* Bug 7895: Serialize() fails to serialize objects which don't have a registered type
* Bug 7995: Windows Agent: Missing directory "zones" in setup
* Bug 8018: Value("").IsEmpty() should return true
* Bug 8029: operator precedence for % and > is incorrect
* Bug 8041: len() overflows
* Bug 8061: Confusing error message for import
* Bug 8067: Require at least one user for notification objects (user or as member of user_groups)
* Bug 8076: icinga 2 Config Error needs to be more verbose
* Bug 8081: Location info for strings is incorrect
* Bug 8100: POSTGRES IDO: invalid syntax for integer: "true" while trying to update table icinga_hoststatus
* Bug 8111: User::ValidateFilters isn't being used
* Bug 8117: Agent checks fail when there's already a host with the same name
* Bug 8122: Config file passing validation causes segfault
* Bug 8132: Debug info for indexer is incorrect
* Bug 8136: Icinga crashes when config file name is invalid
* Bug 8164: escaped backslash in string literals
* Bug 8166: parsing include_recursive
* Bug 8173: Segfault on icinga::String::operator= when compiling configuration
* Bug 8175: Compiler warnings
* Bug 8179: Exception on missing config files
* Bug 8184: group assign fails with bad lexical cast when evaluating rules
* Bug 8185: Argument auto-completion doesn't work for short options
* Bug 8211: icinga2 node update should not write config for blacklisted zones/host
* Bug 8230: Lexer term for T_ANGLE_STRING is too aggressive
* Bug 8249: Problems using command_endpoint inside HA zone
* Bug 8257: Report missing command objects on remote agent
* Bug 8260: icinga2 node wizard: Create backups of certificates
* Bug 8289: Livestatus operator =~ is not case-insensitive
* Bug 8294: Running icinga2 command as non privilged user raises error
* Bug 8298: notify flag is ignored in ACKNOWLEDGE_*_PROBLEM commands
* Bug 8300: ApiListener::ReplayLog shouldn't hold mutex lock during call to Socket::Poll
* Bug 8307: PidPath, VarsPath, ObjectsPath and StatePath no longer read from init.conf
* Bug 8309: Crash in ScheduledDowntime::CreateNextDowntime
* Bug 8313: Incorrectly formatted timestamp in .timestamp file
* Bug 8318: Remote Clients: Add manual setup cli commands
* Bug 8323: Apply rule '' for host does not match anywhere!
* Bug 8333: Icinga2 master doesn't change check-status when "accept_commands = true" is not set at client node
* Bug 8372: Stacktrace on Endpoint not belonging to a zone or multiple zones
* Bug 8383: last_hard_state missing in StatusDataWriter
* Bug 8387: StatusDataWriter: Wrong host notification filters (broken fix in #8192)
* Bug 8388: Config sync authoritative file never created
* Bug 8389: Added downtimes must be triggered immediately if checkable is Not-OK
* Bug 8390: Agent writes CR CR LF in synchronized config files
* Bug 8397: Icinga2 config reset after package update (centos6.6)
* Bug 8425: DB IDO: Duplicate entry icinga_scheduleddowntime
* Bug 8433: Make the arguments for the stats functions const-ref
* Bug 8434: Build fails on OpenBSD
* Bug 8436: Indicate that Icinga2 is shutting down in case of a fatal error
* Bug 8438: DB IDO {host,service}checks command_line value is "Object of type 'icinga::Array'"
* Bug 8444: Don't attempt to restore program state from non-existing state file
* Bug 8452: Livestatus query on commands table with custom vars fails
* Bug 8461: Don't request heartbeat messages until after we've synced the log
* Bug 8473: Exception in WorkQueue::StatusTimerHandler
* Bug 8488: Figure out why 'node update-config' becomes slow over time
* Bug 8493: Misleading ApiListener connection log messages on a master (Endpoint vs Zone)
* Bug 8496: Icinga doesn't update long_output in DB
* Bug 8511: Deadlock with DB IDO dump and forcing a scheduled check
* Bug 8517: Config parser fails non-deterministic on Notification missing Checkable
* Bug 8519: apply-for incorrectly converts loop var to string
* Bug 8529: livestatus limit header not working
* Bug 8535: Crash in ApiEvents::RepositoryTimerHandler
* Bug 8536: Valgrind warning for ExternalCommandListener::CommandPipeThread
* Bug 8537: Crash in DbObject::SendStatusUpdate
* Bug 8544: Hosts: process_performance_data = 0 in database even though enable_perfdata = 1 in config
* Bug 8555: Don't accept config updates for zones for which we have an authoritative copy of the config
* Bug 8559: check_memory tool shows incorrect memory size on windows
* Bug 8593: Memory leak in Expression::GetReference
* Bug 8594: Improve Livestatus query performance
* Bug 8596: Dependency: Validate *_{host,service}_name objects on their existance
* Bug 8604: Attribute hints don't work for nested attributes
* Bug 8627: Icinga2 shuts down when service is reloaded
* Bug 8638: Fix a typo in documentation

### What's New in Version 2.2.4

#### Changes

* Bugfixes

#### Bugfixes

* Bug #6943: Configured recurring downtimes not applied on saturdays
* Bug #7660: livestatus / nsca / etc submits are ignored during reload
* Bug #7685: kUn-Bashify mail-{host,service}-notification.sh
* Bug #8128: Icinga 2.2.2 build fails on SLES11SP3 because of changed boost dependency
* Bug #8131: vfork() hangs on OS X
* Bug #8162: Satellite doesn't use manually supplied 'local zone name'
* Bug #8192: Feature statusdata shows wrong host notification options
* Bug #8201: Update Icinga Web 2 uri to /icingaweb2
* Bug #8214: Fix YAJL detection on Debian squeeze
* Bug #8222: inconsistent URL http(s)://www.icinga.com
* Bug #8223: Typos in readme file for windows plugins
* Bug #8245: check_ssmtp command does NOT support mail_from
* Bug #8256: Restart fails after deleting a Host
* Bug #8288: Crash in DbConnection::ProgramStatusHandler
* Bug #8295: Restart of Icinga hangs
* Bug #8299: Scheduling downtime for host and all services only schedules services
* Bug #8311: Segfault in Checkable::AddNotification
* Bug #8321: enable_event_handlers attribute is missing in status.dat
* Bug #8368: Output in "node wizard" is confusing

### What's New in Version 2.2.3

#### Changes

* Bugfixes

#### Bugfixes

* Bug #8063: Volatile checks trigger invalid notifications on OK->OK state changes
* Bug #8125: Incorrect ticket shouldn't cause "node wizard" to terminate
* Bug #8126: Icinga 2.2.2 doesn't build on i586 SUSE distributions
* Bug #8143: Windows plugin check_service.exe can't find service NTDS
* Bug #8144: Arguments without values are not used on plugin exec
* Bug #8147: check_interval must be greater than 0 error on update-config
* Bug #8152: DB IDO query queue limit reached on reload
* Bug #8171: Typo in example of StatusDataWriter
* Bug #8178: Icinga 2.2.2 segfaults on FreeBSD
* Bug #8181: icinga2 node update config shows hex instead of human readable names
* Bug #8182: Segfault on update-config old empty config

### What's New in Version 2.2.2

#### Changes

* Bugfixes

#### Bugfixes

* Bug #7045: icinga2 init-script doesn't validate configuration on reload action
* Bug #7064: Missing host downtimes/comments in Livestatus
* Bug #7301: Docs: Better explaination of dependency state filters
* Bug #7314: double macros in command arguments seems to lead to exception
* Bug #7511: Feature `compatlog' should flush output buffer on every new line
* Bug #7518: update-config fails to create hosts
* Bug #7591: CPU usage at 100% when check_interval = 0 in host object definition
* Bug #7618: Repository does not support services which have a slash in their name
* Bug #7683: If a parent host goes down, the child host isn't marked as unrechable in the db ido
* Bug #7707: "node wizard" shouldn't crash when SaveCert fails
* Bug #7745: Cluster heartbeats need to be more aggressive
* Bug #7769: The unit tests still crash sometimes
* Bug #7863: execute checks locally if command_endpoint == local endpoint
* Bug #7878: Segfault on issuing node update-config
* Bug #7882: Improve error reporting when libmysqlclient or libpq are missing
* Bug #7891: CLI `icinga2 node update-config` doesn't sync configs from remote clients as expected
* Bug #7913: /usr/lib/icinga2 is not owned by a package
* Bug #7914: SUSE packages %set_permissions post statement wasn't moved to common
* Bug #7917: update_config not updating configuration
* Bug #7920: Test Classic UI config file with Apache 2.4
* Bug #7929: Apache 2.2 fails with new apache conf
* Bug #8002: typeof() seems to return null for arrays and dictionaries
* Bug #8003: SIGABRT while evaluating apply rules
* Bug #8028: typeof does not work for numbers
* Bug #8039: Livestatus: Replace unixcat with nc -U
* Bug #8048: Wrong command in documentation for installing Icinga 2 pretty printers.
* Bug #8050: exception during config check
* Bug #8051: Update host examples in Dependencies for Network Reachability documentation
* Bug #8058: DB IDO: Missing last_hard_state column update in {host,service}status tables
* Bug #8059: Unit tests fail on FreeBSD
* Bug #8066: Setting a dictionary key to null does not cause the key/value to be removed
* Bug #8070: Documentation: Add note on default notification interval in getting started notifications.conf
* Bug #8075: No option to specify timeout to check_snmp and snmp manubulon commands

### What's New in Version 2.2.1

#### Changes

* Support arrays in [command argument macros](#command-passing-parameters) #6709
    * Allows to define multiple parameters for [nrpe -a](#plugin-check-command-nrpe), [nscp -l](#plugin-check-command-nscp), [disk -p](#plugin-check-command-disk), [dns -a](#plugin-check-command-dns).
* Bugfixes

#### Features

* Feature #6709: Support for arrays in macros
* Feature #7463: Update spec file to use yajl-devel
* Feature #7739: The classicui Apache conf doesn't support Apache 2.4
* Feature #7747: Increase default timeout for NRPE checks
* Feature #7867: Document how arrays in macros work

#### Bugfixes

* Bug #7173: service icinga2 status gives wrong information when run as unprivileged user
* Bug #7602: livestatus large amount of submitting unix socket command results in broken pipes
* Bug #7613: icinga2 checkconfig should fail if group given for command files does not exist
* Bug #7671: object and template with the same name generate duplicate object error
* Bug #7708: Built-in commands shouldn't be run on the master instance in remote command execution mode
* Bug #7725: Windows wizard uses incorrect CLI command
* Bug #7726: Windows wizard is missing --zone argument
* Bug #7730: Restart Icinga - Error Restoring program state from file '/var/lib/icinga2/icinga2.state'
* Bug #7735: 2.2.0 has out-of-date icinga2 man page
* Bug #7738: Systemd rpm scripts are run in wrong package
* Bug #7740: /usr/sbin/icinga-prepare-dirs conflicts in the bin and common package
* Bug #7741: Icinga 2.2 misses the build requirement libyajl-devel for SUSE distributions
* Bug #7743: Icinga2 node add failed with unhandled exception
* Bug #7754: Incorrect error message for localhost
* Bug #7770: Objects created with node update-config can't be seen in Classic UI
* Bug #7786: Move the icinga2-prepare-dirs script elsewhere
* Bug #7806: !in operator returns incorrect result
* Bug #7828: Verify if master radio box is disabled in the Windows wizard
* Bug #7847: Wrong information in section "Linux Client Setup Wizard for Remote Monitoring"
* Bug #7862: Segfault in CA handling
* Bug #7868: Documentation: Explain how unresolved macros are handled
* Bug #7890: Wrong permission in run directory after restart
* Bug #7896: Fix Apache config in the Debian package

### What's New in Version 2.2.0

#### Changes

* DB IDO schema update to version `1.12.0`
    * schema files in `lib/db_ido_{mysql,pgsql}/schema` (source)
    * Table `programstatus`: New column `program_version`
    * Table `customvariables` and `customvariablestatus`: New column `is_json` (required for custom attribute array/dictionary support)
* New features
    * [GelfWriter](#gelfwriter): Logging check results, state changes, notifications to GELF (graylog2, logstash) #7619
    * Agent/Client/Node framework #7249
    * Windows plugins for the client/agent parts #7242 #7243
* New CLI commands #7245
    * `icinga2 feature {enable,disable}` replaces `icinga2-{enable,disable}-feature` script  #7250
    * `icinga2 object list` replaces `icinga2-list-objects` script  #7251
    * `icinga2 pki` replaces` icinga2-build-{ca,key}` scripts  #7247
    * `icinga2 repository` manages `/etc/icinga2/repository.d` which must be included in `icinga2.conf` #7255
    * `icinga2 node` cli command provides node (master, satellite, agent) setup (wizard) and management functionality #7248
    * `icinga2 daemon` for existing daemon arguments (`-c`, `-C`). Removed `-u` and `-g` parameters in favor of [init.conf](#init-conf).
    * bash auto-completion & terminal colors #7396
* Configuration
    * Former `localhost` example host is now defined in [hosts.conf](#hosts-conf) #7594
    * All example services moved into advanced apply rules in [services.conf](#services-conf)
    * Updated downtimes configuration example in [downtimes.conf](#downtimes-conf) #7472
    * Updated notification apply example in [notifications.conf](#notifications-conf) #7594
    * Support for object attribute 'zone' #7400
    * Support setting [object variables in apply rules](#dependencies-apply-custom-attributes) #7479
    * Support arrays and dictionaries in [custom attributes](#custom-attributes-apply) #6544 #7560
    * Add [apply for rules](#using-apply-for) for advanced dynamic object generation #7561
    * New attribute `accept_commands` for [ApiListener](#objecttype-apilistener) #7559
    * New [init.conf](#init-conf) file included first containing new constants `RunAsUser` and `RunAsGroup`.
* Cluster
    * Add [CSR Auto-Signing support](#csr-autosigning-requirements) using generated ticket #7244
    * Allow to [execute remote commands](#icinga2-remote-monitoring-client-command-execution) on endpoint clients #7559
* Perfdata
    * [PerfdataWriter](#writing-performance-data-files): Don't change perfdata, pass through from plugins #7268
    * [GraphiteWriter](#graphite-carbon-cache-writer): Add warn/crit/min/max perfdata and downtime_depth stats values #7366 #6946
* Packages
    * `python-icinga2` package dropped in favor of integrated cli commands #7245
    * Windows Installer for the agent parts #7243

> **Note**
>
>  Please remove `conf.d/hosts/localhost*` after verifying your updated configuration!

#### Features

* Feature #6544: Support for array in custom variable.
* Feature #6946: Add downtime depth as statistic metric for GraphiteWriter
* Feature #7187: Document how to use multiple assign/ignore statements with logical "and" & "or"
* Feature #7199: Cli commands: add filter capability to 'object list'
* Feature #7241: Windows Wizard
* Feature #7242: Windows plugins
* Feature #7243: Windows installer
* Feature #7244: CSR auto-signing
* Feature #7245: Cli commands
* Feature #7246: Cli command framework
* Feature #7247: Cli command: pki
* Feature #7248: Cli command: Node
* Feature #7249: Node Repository
* Feature #7250: Cli command: Feature
* Feature #7251: Cli command: Object
* Feature #7252: Cli command: SCM
* Feature #7253: Cli Commands: Node Repository Blacklist & Whitelist
* Feature #7254: Documentation: Agent/Satellite Setup
* Feature #7255: Cli command: Repository
* Feature #7262: macro processor needs an array printer
* Feature #7319: Documentation: Add support for locally-scoped variables for host/service in applied Dependency
* Feature #7334: GraphiteWriter: Add support for customized metric prefix names
* Feature #7356: Documentation: Cli Commands
* Feature #7366: GraphiteWriter: Add warn/crit/min/max perfdata values if existing
* Feature #7370: CLI command: variable
* Feature #7391: Add program_version column to programstatus table
* Feature #7396: Implement generic color support for terminals
* Feature #7400: Remove zone keyword and allow to use object attribute 'zone'
* Feature #7415: CLI: List disabled features in feature list too
* Feature #7421: Add -h next to --help
* Feature #7423: Cli command: Node Setup
* Feature #7452: Replace cJSON with a better JSON parser
* Feature #7465: Cli command: Node Setup Wizard (for Satellites and Agents)
* Feature #7467: Remove virtual agent name feature for localhost
* Feature #7472: Update downtimes.conf example config
* Feature #7478: Documentation: Mention 'icinga2 object list' in config validation
* Feature #7479: Set host/service variable in apply rules
* Feature #7480: Documentation: Add host/services variables in apply rules
* Feature #7504: Documentation: Revamp getting started with 1 host and multiple (service) applies
* Feature #7514: Documentation: Move troubleshooting after the getting started chapter
* Feature #7524: Documentation: Explain how to manage agent config in central repository
* Feature #7543: Documentation for arrays & dictionaries in custom attributes and their usage in apply rules for
* Feature #7559: Execute remote commands on the agent w/o local objects by passing custom attributes
* Feature #7560: Support dictionaries in custom attributes
* Feature #7561: Generate objects using apply with foreach in arrays or dictionaries (key => value)
* Feature #7566: Implement support for arbitrarily complex indexers
* Feature #7594: Revamp sample configuration: add NodeName host, move services into apply rules schema
* Feature #7596: Plugin Check Commands: disk is missing '-p', 'x' parameter
* Feature #7619: Add GelfWriter for writing log events to graylog2/logstash
* Feature #7620: Documentation: Update Icinga Web 2 installation
* Feature #7622: Icinga 2 should use less RAM
* Feature #7680: Conditionally enable MySQL and PostgresSQL, add support for FreeBSD and DragonFlyBSD

#### Bugfixes

* Bug #6547: delaying notifications with times.begin should postpone first notification into that window
* Bug #7257: default value for "disable_notifications" in service dependencies is set to "false"
* Bug #7268: Icinga2 changes perfdata order and removes maximum
* Bug #7272: icinga2 returns exponential perfdata format with check_nt
* Bug #7275: snmp-load checkcommand has wrong threshold syntax
* Bug #7276: SLES (Suse Linux Enterprise Server) 11 SP3 package dependency failure
* Bug #7302: ITL: check_procs and check_http are missing arguments
* Bug #7324: config parser crashes on unknown attribute in assign
* Bug #7327: Icinga2 docs: link supported operators from sections about apply rules
* Bug #7331: Error messages for invalid imports missing
* Bug #7338: Docs: Default command timeout is 60s not 5m
* Bug #7339: Importing a CheckCommand in a NotificationCommand results in an exception without stacktrace.
* Bug #7349: Documentation: Wrong check command for snmp-int(erface)
* Bug #7351: snmp-load checkcommand has a wrong "-T" param value
* Bug #7359: Setting snmp_v2 can cause snmp-manubulon-command derived checks to fail
* Bug #7365: Typo for "HTTP Checks" match in groups.conf
* Bug #7369: Fix reading perfdata in compat/checkresultreader
* Bug #7372: custom attribute name 'type' causes empty vars dictionary
* Bug #7373: Wrong usermod command for external command pipe setup
* Bug #7378: Commands are auto-completed when they shouldn't be
* Bug #7379: failed en/disable feature should return error
* Bug #7380: Debian package root permissions interfere with icinga2 cli commands as icinga user
* Bug #7392: Schema upgrade files are missing in /usr/share/icinga2-ido-{mysql,pgsql}
* Bug #7417: CMake warnings on OS X
* Bug #7428: Documentation: 1-about contribute links to non-existing report a bug howto
* Bug #7433: Unity build fails on RHEL 5
* Bug #7446: When replaying logs the secobj attribute is ignored
* Bug #7473: Performance data via API is broken
* Bug #7475: can't assign Service to Host in nested HostGroup
* Bug #7477: Fix typos and other small corrections in documentation
* Bug #7482: OnStateLoaded isn't called for objects which don't have any state
* Bug #7483: Hosts/services should not have themselves as parents
* Bug #7495: Utility::GetFQDN doesn't work on OS X
* Bug #7503: Icinga2 fails to start due to configuration errors
* Bug #7520: Use ScriptVariable::Get for RunAsUser/RunAsGroup
* Bug #7536: Object list dump erraneously evaluates template definitions
* Bug #7537: Nesting an object in a template causes the template to become non-abstract
* Bug #7538: There is no __name available to nested objects
* Bug #7573: link missing in documentation about livestatus
* Bug #7577: Invalid checkresult object causes Icinga 2 to crash
* Bug #7579: only notify users on recovery which have been notified before (not-ok state)
* Bug #7585: Nested templates do not work (anymore)
* Bug #7586: Exception when executing check
* Bug #7597: Compilation Error with boost 1.56 under Windows
* Bug #7599: Plugin execution on Windows does not work
* Bug #7617: mkclass crashes when called without arguments
* Bug #7623: Missing state filter 'OK' must not prevent recovery notifications being sent
* Bug #7624: Installation on Windows fails
* Bug #7625: IDO module crashes on Windows
* Bug #7646: Get rid of static boost::mutex variables
* Bug #7648: Unit tests fail to run
* Bug #7650: Wrong set of dependency state when a host depends on a service
* Bug #7681: CreateProcess fails on Windows 7
* Bug #7688: DebugInfo is missing for nested dictionaries


### What's New in Version 2.1.1

#### Features

* Feature #6719: Change log message for checking/sending notifications
* Feature #7028: Document how to use @ to escape keywords
* Feature #7033: Add include guards for mkclass files
* Feature #7034: Ensure that namespaces for INITIALIZE_ONCE and REGISTER_TYPE are truly unique
* Feature #7035: Implement support for unity builds
* Feature #7039: Figure out a better way to set the version for snapshot builds
* Feature #7040: Unity builds: Detect whether __COUNTER__ is available
* Feature #7041: Enable unity build for RPM/Debian packages
* Feature #7070: Explain event commands and their integration by a real life example (httpd restart via ssh)
* Feature #7158: Extend documentation for icinga-web on Debian systems

#### Bugfixes

* Bug #6147: Link libcJSON against libm
* Bug #6696: make test fails on openbsd
* Bug #6841: Too many queued messages
* Bug #6862: SSL_read errors during restart
* Bug #6981: SSL errors with interleaved SSL_read/write
* Bug #7029: icinga2.spec: files-attr-not-set for python-icinga2 package
* Bug #7032: "Error parsing performance data" in spite of "enable_perfdata = false"
* Bug #7036: Remove validator for the Script type
* Bug #7037: icinga2-list-objects doesn't work with Python 3
* Bug #7038: Fix rpmlint errors
* Bug #7042: icinga2-list-objects complains about Umlauts and stops output
* Bug #7044: icinga2 init-script terminates with exit code 0 if $DAEMON is not in place or not executable
* Bug #7047: service icinga2 status - prints cat error if the service is stopped
* Bug #7058: Exit code is not initialized for some failed checks
* Bug #7065: pipe2 returns ENOSYS on GNU Hurd and Debian kfreebsd
* Bug #7072: GraphiteWriter should ignore empty perfdata value
* Bug #7080: Missing differentiation between service and systemctl
* Bug #7096: new SSL Errors with too many queued messages
* Bug #7115: Build fails on Haiku
* Bug #7123: Manubulon-Plugin conf Filename wrong
* Bug #7139: GNUInstallDirs.cmake outdated
* Bug #7167: Segfault using cluster in TlsStream::IsEof
* Bug #7168: fping4 doesn't work correctly with the shipped command-plugins.conf
* Bug #7186: Livestatus hangs from time to time
* Bug #7195: fix memory leak ido_pgsql
* Bug #7210: clarify on db ido upgrades

### What's New in Version 2.1.0

#### Changes

* DB IDO schema upgrade ([MySQL](#upgrading-mysql-db),[PostgreSQL](#upgrading-postgresql-db) required!
    * new schema version: **1.11.7**
    * RPMs install the schema files into `/usr/share/icinga2-ido*` instead of `/usr/share/doc/icinga2-ido*` #6881
* [Information for config objects](#list-configuration-objects) using `icinga2-list-objects` script #6702
* Add Python 2.4 as requirement #6702
* Add search path: If `-c /etc/icinga2/icinga2.conf` is omitted, use `SysconfDir + "/icinga2/icinga2.conf"` #6874
* Change log level for failed commands #6751
* Notifications are load-balanced in a [High Availability cluster setup](#high-availability-notifications) #6203
    * New config attribute: `enable_ha`
* DB IDO "run once" or "run everywhere" mode in a [High Availability cluster setup](#high-availability-db-ido) #6203 #6827
    * New config attributes: `enable_ha` and `failover_timeout`
* RPMs use the `icingacmd` group for /var/{cache,log,run}/icinga2 #6948

#### Features

* Feature #5219: Cluster support for modified attributes
* Feature #6066: Better log messages for cluster changes
* Feature #6203: Better cluster support for notifications / IDO
* Feature #6205: Log replay sends messages to instances which shouldn't get those messages
* Feature #6702: Information for config objects
* Feature #6704: Release 2.1
* Feature #6751: Change log level for failed commands
* Feature #6874: add search path for icinga2.conf
* Feature #6898: Enhance logging for perfdata/graphitewriter
* Feature #6919: Clean up spec file
* Feature #6920: Recommend related packages on SUSE distributions
* API - Bug #6998: ApiListener ignores bind_host attribute
* DB IDO - Feature #6827: delay ido connect in ha cluster
* Documentation - Bug #6870: Wrong object attribute 'enable_flap_detection'
* Documentation - Bug #6878: Wrong parent in Load Distribution
* Documentation - Bug #6909: clarify on which config tools are available
* Documentation - Bug #6968: Update command arguments 'set_if' and beautify error message
* Documentation - Bug #6995: Keyword "required" used inconsistently for host and service "icon_image*" attributes
* Documentation - Feature #6651: Migration: note on check command timeouts
* Documentation - Feature #6703: Documentation for zones and cluster permissions
* Documentation - Feature #6743: Better explanation for HA config cluster
* Documentation - Feature #6839: Explain how the order attribute works in commands
* Documentation - Feature #6864: Add section for reserved keywords
* Documentation - Feature #6867: add section about disabling re-notifications
* Documentation - Feature #6869: Add systemd options: enable, journal
* Documentation - Feature #6922: Enhance Graphite Writer description
* Documentation - Feature #6949: Add documentation for icinga2-list-objects
* Documentation - Feature #6997: how to add a new cluster node
* Documentation - Feature #7018: add example selinux policy for external command pipe
* Plugins - Feature #6650: Plugin Check Commands: add manubulon snmp plugins

#### Bugfixes

* Bug #6881: make install does not install the db-schema
* Bug #6915: use _rundir macro for configuring the run directory
* Bug #6916: External command pipe: Too many open files
* Bug #6917: enforce /usr/lib as base for the cgi path on SUSE distributions
* Bug #6942: ExternalCommandListener fails open pipe: Too many open files
* Bug #6948: check file permissions in /var/cache/icinga2
* Bug #6962: Commands are processed multiple times
* Bug #6964: Host and service checks stuck in "pending" when hostname = localhost a parent/satellite setup
* Bug #7001: Build fails with Boost 1.56
* Bug #7016: 64-bit RPMs are not installable

### What's New in Version 2.0.2

#### Changes

* DB IDO schema upgrade required (new schema version: 1.11.6)

#### Features

* Feature #5818: SUSE packages
* Feature #6655: Build packages for el7
* Feature #6688: Rename README to README.md
* Feature #6698: Require command to be an array when the arguments attribute is used
* Feature #6700: Release 2.0.2
* Feature #6783: Print application paths for --version
* DB IDO - Bug #6414: objects and their ids are inserted twice
* DB IDO - Bug #6608: Two Custom Variables with same name, but Upper/Lowercase creating IDO duplicate entry
* DB IDO - Bug #6646: NULL vs empty string
* DB IDO - Bug #6850: exit application if ido schema version does not match
* Documentation - Bug #6652: clarify on which features are required for classic ui/web/web2
* Documentation - Bug #6708: update installation with systemd usage
* Documentation - Bug #6711: icinga Web: wrong path to command pipe
* Documentation - Bug #6725: Missing documentation about implicit dependency
* Documentation - Bug #6728: wrong path for the file 'localhost.conf'
* Migration - Bug #6558: group names quoted twice in arrays
* Migration - Bug #6560: Service dependencies aren't getting converted properly
* Migration - Bug #6561: $TOTALHOSTSERVICESWARNING$ and $TOTALHOSTSERVICESCRITICAL$ aren't getting converted
* Migration - Bug #6563: Check and retry intervals are incorrect
* Migration - Bug #6786: Fix notification definition if no host_name / service_description given
* Plugins - Feature #6695: Plugin Check Commands: Add expect option to check_http
* Plugins - Feature #6791: Plugin Check Commands: Add timeout option to check_ssh

#### Bugfixes

* Bug #6450: ipmi-sensors segfault due to stack size
* Bug #6479: Notifications not always triggered
* Bug #6501: Classic UI Debian/Ubuntu: apache 2.4 requires 'a2enmod cgi' & apacheutils installed
* Bug #6548: Add cmake constant for PluginDir
* Bug #6549: GraphiteWriter regularly sends empty lines
* Bug #6550: add log message for invalid performance data
* Bug #6589: Command pipe blocks when trying to open it more than once in parallel
* Bug #6621: Infinite loop in TlsStream::Close
* Bug #6627: Location of the run directory is hard coded and bound to "local_state_dir"
* Bug #6659: RPMLint security warning - missing-call-to-setgroups-before-setuid /usr/sbin/icinga2
* Bug #6682: Missing detailed error messages on ApiListener SSL Errors
* Bug #6686: Event Commands are triggered in OK HARD state everytime
* Bug #6687: Remove superfluous quotes and commas in dictionaries
* Bug #6713: sample config: add check commands location hint (itl/plugin check commands)
* Bug #6718: "order" attribute doesn't seem to work as expected
* Bug #6724: TLS Connections still unstable in 2.0.1
* Bug #6756: GraphiteWriter: Malformatted integer values
* Bug #6765: Config validation without filename argument fails with unhandled exception
* Bug #6768: Repo Error on RHEL 6.5
* Bug #6773: Order doesn't work in check ssh command
* Bug #6782: The "ssl" check command always sets -D
* Bug #6790: Service icinga2 reload command does not cause effect
* Bug #6809: additional group rights missing when Icinga started with -u and -g
* Bug #6810: High Availablity does not synchronise the data like expected
* Bug #6820: Icinga 2 crashes during startup
* Bug #6821: [Patch] Fix build issue and crash found on Solaris, potentially other Unix OSes
* Bug #6825: incorrect sysconfig path on sles11
* Bug #6832: Remove if(NOT DEFINED ICINGA2_SYSCONFIGFILE) in etc/initsystem/CMakeLists.txt
* Bug #6840: Missing space in error message
* Bug #6849: Error handler for getaddrinfo must use gai_strerror
* Bug #6852: Startup logfile is not flushed to disk
* Bug #6856: event command execution does not call finish handler
* Bug #6861: write startup error messages to error.log

### What's New in Version 2.0.1

#### Features

* Feature #6531: Add port option to check imap/pop/smtp and a new dig
* Feature #6581: Add more options to snmp check
* DB IDO - Bug #5577: PostgreSQL string escaping
* DB IDO - Bug #6577: icinga2-ido-pgsql snapshot package missing dependecy dbconfig-common
* Documentation - Bug #6506: Array section confusing
* Documentation - Bug #6592: Documentation for || and && is missing
* Documentation - Feature #6658: change docs.icinga.com/icinga2/latest to git master
* Livestatus - Bug #6494: Thruk Panorama View cannot query Host Status
* Livestatus - Feature #5312: OutputFormat python
* Migration - Bug #6559: $SERVICEDESC$ isn't getting converted correctly

#### Bugfixes

* Bug #6316: application fails to start on wrong log file permissions but does not tell about it
* Bug #6368: Deadlock in ApiListener::RelayMessage
* Bug #6373: base64 on CentOS 5 fails to read certificate bundles
* Bug #6388: Debian package icinga2-classicui needs versioned dependency of icinga-cgi*
* Bug #6488: build warnings
* Bug #6492: icinga2.state could not be opened
* Bug #6493: Copyright problems
* Bug #6498: icinga2 cannot be built with both systemd and init.d files
* Bug #6510: Reminder notifications are sent on disabled services
* Bug #6526: htpasswd should be installed with icinga2-classicui on Ubuntu
* Bug #6529: parsing of double defined command can generate unexpected errors
* Bug #6537: Icinga doesn't send SetLogPosition messages when one of the endpoints fails to connect
* Bug #6551: Improve systemd service definition
* Bug #6565: Dependencies should cache their parent and child object
* Bug #6574: Check command result doesn't match
* Bug #6576: Remove line number information from stack traces
* Bug #6588: Notifications causing segfault from exim
* Bug #6605: Please add --sni option to http check command
* Bug #6612: Icinga stops updating IDO after a while
* Bug #6617: TLS connections are still unstable
* Bug #6620: icinga2-build-ca shouldn't prompt for DN
* Bug #6622: icinga2-sign-key creates ".crt" and ".key" files when the CA passphrase is invalid
* Bug #6657: ICINGA2_SYSCONFIGFILE should use full path using CMAKE_INSTALL_FULL_SYSCONFDIR
* Bug #6662: Increase icinga.cmd Limit
* Bug #6665: Build fails when MySQL is not installed
* Bug #6671: enabled_notification doesn't work as expected
* Bug #6672: Icinga crashes after "Too many queued messages"
* Bug #6673: enable_notifications = false for users has no effect

