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

Check the project website at https://www.icinga.com for status updates. Join the
[community channels](https://www.icinga.com/community/get-involved/) for questions
or ask an Icinga partner for [professional support](https://www.icinga.com/services/support/).

## <a id="contribute"></a> Contribute

There are many ways to contribute to Icinga -- whether it be sending patches,
testing, reporting bugs, or reviewing and updating the documentation. Every
contribution is appreciated!

Read the [contributing section](https://www.icinga.com/community/get-involved/) and
get familiar with the code.

Pull requests on [GitHub](https://github.com/Icinga/icinga2) are preferred.

### <a id="development-info"></a> Icinga 2 Development

The Git repository is located on [GitHub](https://github.com/Icinga/icinga2).

Icinga 2 is written in C++ and can be built on Linux/Unix and Windows.
Read more about development builds in the [INSTALL.md](https://github.com/Icinga/icinga2/blob/master/INSTALL.md)
file.

## <a id="whats-new"></a> What's New

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

* Feature [12566](https://dev.icinga.com/issues/12566 "Feature 12566") (API): Provide location information for objects and templates in the API
* Feature [13255](https://dev.icinga.com/issues/13255 "Feature 13255") (Cluster): Deprecate cluster/client mode "bottom up" w/ repository.d and node update-config
* Feature [12844](https://dev.icinga.com/issues/12844 "Feature 12844") (Cluster): Check whether nodes are synchronizing the API log before putting them into UNKNOWN
* Feature [12623](https://dev.icinga.com/issues/12623 "Feature 12623") (Cluster): Improve log message for ignored config updates
* Feature [12635](https://dev.icinga.com/issues/12635 "Feature 12635") (Configuration): Suppress compiler warnings for auto-generated code
* Feature [12575](https://dev.icinga.com/issues/12575 "Feature 12575") (Configuration): Implement support for default templates
* Feature [12554](https://dev.icinga.com/issues/12554 "Feature 12554") (Configuration): Implement a command-line argument for "icinga2 console" to allow specifying a script file
* Feature [12544](https://dev.icinga.com/issues/12544 "Feature 12544") (Configuration): Remove unused method: ApplyRule::DiscardRules
* Feature [10675](https://dev.icinga.com/issues/10675 "Feature 10675") (Configuration): Command line option for config syntax validation
* Feature [13491](https://dev.icinga.com/issues/13491 "Feature 13491") (Documentation): Update README.md and correct project URLs
* Feature [13457](https://dev.icinga.com/issues/13457 "Feature 13457") (Documentation): Add a note for boolean values in the disk CheckCommand section
* Feature [13455](https://dev.icinga.com/issues/13455 "Feature 13455") (Documentation): Troubleshooting: Add examples for fetching the executed command line
* Feature [13443](https://dev.icinga.com/issues/13443 "Feature 13443") (Documentation): Update Windows screenshots in the client documentation
* Feature [13437](https://dev.icinga.com/issues/13437 "Feature 13437") (Documentation): Add example for concurrent_checks in CheckerComponent object type
* Feature [13395](https://dev.icinga.com/issues/13395 "Feature 13395") (Documentation): Add a note about removing "conf.d" on the client for "top down command endpoint" setups
* Feature [13327](https://dev.icinga.com/issues/13327 "Feature 13327") (Documentation): Update API and Library Reference chapters
* Feature [13319](https://dev.icinga.com/issues/13319 "Feature 13319") (Documentation): Add a note about pinning checks w/ command_endpoint
* Feature [13297](https://dev.icinga.com/issues/13297 "Feature 13297") (Documentation): Add a note about default template import to the CheckCommand object
* Feature [13199](https://dev.icinga.com/issues/13199 "Feature 13199") (Documentation): Doc: Swap packages.icinga.com w/ DebMon
* Feature [12834](https://dev.icinga.com/issues/12834 "Feature 12834") (Documentation): Add more Timeperiod examples in the documentation
* Feature [12832](https://dev.icinga.com/issues/12832 "Feature 12832") (Documentation): Add an example of multi-parents configuration for the Migration chapter
* Feature [12587](https://dev.icinga.com/issues/12587 "Feature 12587") (Documentation): Update service monitoring and distributed docs
* Feature [12449](https://dev.icinga.com/issues/12449 "Feature 12449") (Documentation): Add information about function 'range'
* Feature [13449](https://dev.icinga.com/issues/13449 "Feature 13449") (ITL): Add tempdir attribute to postgres CheckCommand
* Feature [13435](https://dev.icinga.com/issues/13435 "Feature 13435") (ITL): Add sudo option to mailq CheckCommand
* Feature [13433](https://dev.icinga.com/issues/13433 "Feature 13433") (ITL): Add verbose parameter to http CheckCommand
* Feature [13431](https://dev.icinga.com/issues/13431 "Feature 13431") (ITL): Add timeout option to mysql_health CheckCommand
* Feature [12762](https://dev.icinga.com/issues/12762 "Feature 12762") (ITL): Add a radius CheckCommand for the radius check provide by nagios-plugins
* Feature [12755](https://dev.icinga.com/issues/12755 "Feature 12755") (ITL): Add CheckCommand definition for check_logstash
* Feature [12739](https://dev.icinga.com/issues/12739 "Feature 12739") (ITL): Add timeout option to oracle_health CheckCommand
* Feature [12613](https://dev.icinga.com/issues/12613 "Feature 12613") (ITL): Add CheckCommand definition for check_iostats
* Feature [12516](https://dev.icinga.com/issues/12516 "Feature 12516") (ITL): ITL - check_vmware_esx - specify a datacenter/vsphere server for esx/host checks
* Feature [12040](https://dev.icinga.com/issues/12040 "Feature 12040") (ITL): Add CheckCommand definition for check_glusterfs
* Feature [12576](https://dev.icinga.com/issues/12576 "Feature 12576") (Installation): Use raw string literals in mkembedconfig
* Feature [12564](https://dev.icinga.com/issues/12564 "Feature 12564") (Installation): Improve detection for the -flto compiler flag
* Feature [12552](https://dev.icinga.com/issues/12552 "Feature 12552") (Installation): Set versions for all internal libraries
* Feature [12537](https://dev.icinga.com/issues/12537 "Feature 12537") (Installation): Update cmake config to require a compiler that supports C++11
* Feature [9119](https://dev.icinga.com/issues/9119 "Feature 9119") (Installation): Make the user account configurable for the Windows service
* Feature [12733](https://dev.icinga.com/issues/12733 "Feature 12733") (Packages): Windows Installer should include NSClient++ 0.5.0
* Feature [12679](https://dev.icinga.com/issues/12679 "Feature 12679") (Plugins): Review windows plugins performance output
* Feature [13225](https://dev.icinga.com/issues/13225 "Feature 13225") (Tests): Add unit test for notification state/type filter checks
* Feature [12530](https://dev.icinga.com/issues/12530 "Feature 12530") (Tests): Implement unit tests for state changes
* Feature [12562](https://dev.icinga.com/issues/12562 "Feature 12562") (libbase): Use lambda functions for INITIALIZE_ONCE
* Feature [12561](https://dev.icinga.com/issues/12561 "Feature 12561") (libbase): Use 'auto' for iterator declarations
* Feature [12555](https://dev.icinga.com/issues/12555 "Feature 12555") (libbase): Implement an rvalue constructor for the String and Value classes
* Feature [12538](https://dev.icinga.com/issues/12538 "Feature 12538") (libbase): Replace BOOST_FOREACH with range-based for loops
* Feature [12536](https://dev.icinga.com/issues/12536 "Feature 12536") (libbase): Add -fvisibility=hidden to the default compiler flags
* Feature [12510](https://dev.icinga.com/issues/12510 "Feature 12510") (libbase): Implement an environment variable to keep Icinga from closing FDs on startup
* Feature [12509](https://dev.icinga.com/issues/12509 "Feature 12509") (libbase): Avoid unnecessary string copies
* Feature [12507](https://dev.icinga.com/issues/12507 "Feature 12507") (libbase): Remove deprecated functions
* Feature [9182](https://dev.icinga.com/issues/9182 "Feature 9182") (libbase): Better message for apply errors
* Feature [12578](https://dev.icinga.com/issues/12578 "Feature 12578") (libicinga): Make sure that libmethods is automatically loaded even when not using the ITL

#### Bugfixes

* Bug [12860](https://dev.icinga.com/issues/12860 "Bug 12860") (API): Icinga crashes while deleting a config file which doesn't exist anymore
* Bug [12667](https://dev.icinga.com/issues/12667 "Bug 12667") (API): Crash in HttpRequest::Parse while processing HTTP request
* Bug [12621](https://dev.icinga.com/issues/12621 "Bug 12621") (API): Invalid API filter error messages
* Bug [11541](https://dev.icinga.com/issues/11541 "Bug 11541") (API): Objects created in a global zone are not synced to child endpoints
* Bug [11329](https://dev.icinga.com/issues/11329 "Bug 11329") (API): API requests from execute-script action are too verbose
* Bug [13419](https://dev.icinga.com/issues/13419 "Bug 13419") (CLI): Wrong help string for node setup cli command argument --master_host
* Bug [12741](https://dev.icinga.com/issues/12741 "Bug 12741") (CLI): Parse error: "premature EOF" when running "icinga2 node update-config"
* Bug [12596](https://dev.icinga.com/issues/12596 "Bug 12596") (CLI): Last option highlighted as the wrong one, even when it is not the culprit
* Bug [13151](https://dev.icinga.com/issues/13151 "Bug 13151") (Cluster): Crash w/ SendNotifications cluster handler and check result with empty perfdata
* Bug [11684](https://dev.icinga.com/issues/11684 "Bug 11684") (Cluster): Cluster resync problem with API created objects
* Bug [10897](https://dev.icinga.com/issues/10897 "Bug 10897") (Compat): SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME command missing
* Bug [10896](https://dev.icinga.com/issues/10896 "Bug 10896") (Compat): SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME command missing
* Bug [12749](https://dev.icinga.com/issues/12749 "Bug 12749") (Configuration): Configuration validation fails when setting tls_protocolmin to TLSv1.2
* Bug [12633](https://dev.icinga.com/issues/12633 "Bug 12633") (Configuration): Validation does not highlight the correct attribute
* Bug [12571](https://dev.icinga.com/issues/12571 "Bug 12571") (Configuration): Debug hints for dictionary expressions are nested incorrectly
* Bug [12556](https://dev.icinga.com/issues/12556 "Bug 12556") (Configuration): Config validation shouldnt allow 'endpoints = [ "" ]'
* Bug [13221](https://dev.icinga.com/issues/13221 "Bug 13221") (DB IDO): PostgreSQL: Don't use timestamp with timezone for UNIX timestamp columns
* Bug [12558](https://dev.icinga.com/issues/12558 "Bug 12558") (DB IDO): Getting error during schema update
* Bug [12514](https://dev.icinga.com/issues/12514 "Bug 12514") (DB IDO): Don't link against libmysqlclient_r
* Bug [10502](https://dev.icinga.com/issues/10502 "Bug 10502") (DB IDO): MySQL 5.7.9, Incorrect datetime value Error
* Bug [13519](https://dev.icinga.com/issues/13519 "Bug 13519") (Documentation): "2.1.4. Installation Paths" should contain systemd paths
* Bug [13517](https://dev.icinga.com/issues/13517 "Bug 13517") (Documentation): Update "2.1.3. Enabled Features during Installation" - outdated "feature list"
* Bug [13515](https://dev.icinga.com/issues/13515 "Bug 13515") (Documentation): Update package instructions for Fedora
* Bug [13411](https://dev.icinga.com/issues/13411 "Bug 13411") (Documentation): Missing API headers for X-HTTP-Method-Override
* Bug [13407](https://dev.icinga.com/issues/13407 "Bug 13407") (Documentation): Fix example in PNP template docs
* Bug [13267](https://dev.icinga.com/issues/13267 "Bug 13267") (Documentation): Docs: Typo in "CLI commands" chapter
* Bug [12933](https://dev.icinga.com/issues/12933 "Bug 12933") (Documentation): Docs: wrong heading level for commands.conf and groups.conf
* Bug [12831](https://dev.icinga.com/issues/12831 "Bug 12831") (Documentation): Typo in the documentation
* Bug [12822](https://dev.icinga.com/issues/12822 "Bug 12822") (Documentation): Fix some spelling mistakes
* Bug [12725](https://dev.icinga.com/issues/12725 "Bug 12725") (Documentation): Add documentation for logrotation for the mainlog feature
* Bug [12681](https://dev.icinga.com/issues/12681 "Bug 12681") (Documentation): Corrections for distributed monitoring chapter
* Bug [12664](https://dev.icinga.com/issues/12664 "Bug 12664") (Documentation): Docs: Migrating Notification example tells about filters instead of types
* Bug [12662](https://dev.icinga.com/issues/12662 "Bug 12662") (Documentation): GDB example in the documentation isn't working
* Bug [12594](https://dev.icinga.com/issues/12594 "Bug 12594") (Documentation): Typo in distributed monitoring docs
* Bug [12577](https://dev.icinga.com/issues/12577 "Bug 12577") (Documentation): Fix help output for update-links.py
* Bug [12995](https://dev.icinga.com/issues/12995 "Bug 12995") (Graphite): Performance data writer for Graphite : Values without fraction limited to 2147483647 (7FFFFFFF)
* Bug [12849](https://dev.icinga.com/issues/12849 "Bug 12849") (ITL): Default values for check_swap are incorrect
* Bug [12838](https://dev.icinga.com/issues/12838 "Bug 12838") (ITL): snmp_miblist variable to feed the -m option of check_snmp is missing in the snmpv3 CheckCommand object
* Bug [12747](https://dev.icinga.com/issues/12747 "Bug 12747") (ITL): Problem passing arguments to nscp-local CheckCommand objects
* Bug [12588](https://dev.icinga.com/issues/12588 "Bug 12588") (ITL): Default disk plugin check should not check inodes
* Bug [12586](https://dev.icinga.com/issues/12586 "Bug 12586") (ITL): Manubulon: Add missing procurve memory flag
* Bug [12573](https://dev.icinga.com/issues/12573 "Bug 12573") (ITL): Fix code style violations in the ITL
* Bug [12570](https://dev.icinga.com/issues/12570 "Bug 12570") (ITL): Incorrect help text for check_swap
* Bug [12535](https://dev.icinga.com/issues/12535 "Bug 12535") (Installation): logrotate file is not properly generated when the logrotate binary resides in /usr/bin
* Bug [13205](https://dev.icinga.com/issues/13205 "Bug 13205") (Notifications): Recovery notifications sent for Not-Problem notification type if notified before
* Bug [12892](https://dev.icinga.com/issues/12892 "Bug 12892") (Notifications): Flapping notifications sent for soft state changes
* Bug [12670](https://dev.icinga.com/issues/12670 "Bug 12670") (Notifications): Forced custom notification is setting "force_next_notification": true permanently
* Bug [12560](https://dev.icinga.com/issues/12560 "Bug 12560") (Notifications): Don't send Flapping* notifications when downtime is active
* Bug [12549](https://dev.icinga.com/issues/12549 "Bug 12549") (Notifications): Fixed downtimes scheduled for a future date trigger DOWNTIMESTART notifications
* Bug [12276](https://dev.icinga.com/issues/12276 "Bug 12276") (Perfdata): InfluxdbWriter does not write state other than 0
* Bug [12155](https://dev.icinga.com/issues/12155 "Bug 12155") (Plugins): check_network performance data in invalid format - ingraph
* Bug [10489](https://dev.icinga.com/issues/10489 "Bug 10489") (Plugins): Windows Agent: performance data of check_perfmon
* Bug [10487](https://dev.icinga.com/issues/10487 "Bug 10487") (Plugins): Windows Agent: Performance data values for check_perfmon.exe are invalid sometimes
* Bug [9831](https://dev.icinga.com/issues/9831 "Bug 9831") (Plugins): Implement support for resolving DNS hostnames in check_ping.exe
* Bug [12940](https://dev.icinga.com/issues/12940 "Bug 12940") (libbase): SIGALRM handling may be affected by recent commit
* Bug [12718](https://dev.icinga.com/issues/12718 "Bug 12718") (libbase): Crash in ClusterEvents::SendNotificationsAPIHandler
* Bug [12545](https://dev.icinga.com/issues/12545 "Bug 12545") (libbase): Add missing initializer for WorkQueue::m_NextTaskID
* Bug [12534](https://dev.icinga.com/issues/12534 "Bug 12534") (libbase): Fix compiler warnings
* Bug [8900](https://dev.icinga.com/issues/8900 "Bug 8900") (libbase): File descriptors are leaked to child processes which makes SELinux unhappy
* Bug [13275](https://dev.icinga.com/issues/13275 "Bug 13275") (libicinga): Icinga tries to delete Downtime objects that were statically configured
* Bug [13103](https://dev.icinga.com/issues/13103 "Bug 13103") (libicinga): Config validation crashes when using command_endpoint without also having an ApiListener object
* Bug [12602](https://dev.icinga.com/issues/12602 "Bug 12602") (libicinga): Remove unused last_in_downtime field
* Bug [12592](https://dev.icinga.com/issues/12592 "Bug 12592") (libicinga): Unexpected state changes with max_check_attempts = 2
* Bug [12511](https://dev.icinga.com/issues/12511 "Bug 12511") (libicinga): Don't update TimePeriod ranges for inactive objects
