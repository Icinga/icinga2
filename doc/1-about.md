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

### <a id="development"></a> Icinga 2 Development

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
* When reporting a bug, please include the details described in the [Troubleshooting](17-troubleshooting.md#troubleshooting-information-required) chapter (version, configs, logs, etc).

## <a id="whats-new"></a> What's New

### What's New in Version 2.3.9

#### Changes

* Fix that the first SOFT state is recognized as second SOFT state
* Implemented reload functionality for Windows
* New ITL check commands
* Documentation updates
* Various other bugfixes

#### Features

* Feature [9527](https://dev.icinga.org/issues/9527 "Feature 9527"): CheckCommand for check_interfaces
* Feature [9671](https://dev.icinga.org/issues/9671 "Feature 9671"): Add check_yum to ITL
* Feature [9675](https://dev.icinga.org/issues/9675 "Feature 9675"): Add check_redis to ITL
* Feature [9686](https://dev.icinga.org/issues/9686 "Feature 9686"): Update gdb pretty printer docs w/ Python 3
* Feature [9699](https://dev.icinga.org/issues/9699 "Feature 9699"): Adding "-r" parameter to the check_load command for dividing the load averages by the number of CPUs.
* Feature [9747](https://dev.icinga.org/issues/9747 "Feature 9747"): check_command for plugin check_clamd
* Feature [9796](https://dev.icinga.org/issues/9796 "Feature 9796"): Implement Dictionary#get and Array#get
* Feature [9801](https://dev.icinga.org/issues/9801 "Feature 9801"): Add check_jmx4perl to ITL
* Feature [9811](https://dev.icinga.org/issues/9811 "Feature 9811"): add check command for check_mailq
* Feature [9827](https://dev.icinga.org/issues/9827 "Feature 9827"): snmpv3 CheckCommand section improved
* Feature [9882](https://dev.icinga.org/issues/9882 "Feature 9882"): Implement the Dictionary#keys method
* Feature [9883](https://dev.icinga.org/issues/9883 "Feature 9883"): Use an empty dictionary for the 'this' scope when executing commands with Livestatus
* Feature [9985](https://dev.icinga.org/issues/9985 "Feature 9985"): add check command nscp-local-counter
* Feature [9996](https://dev.icinga.org/issues/9996 "Feature 9996"): Add new arguments openvmtools for Open VM Tools

#### Bugfixes

* Bug [8979](https://dev.icinga.org/issues/8979 "Bug 8979"): Missing DEL_DOWNTIME_BY_HOST_NAME command required by Classic UI 1.x
* Bug [9262](https://dev.icinga.org/issues/9262 "Bug 9262"): cluster check w/ immediate parent and child zone endpoints
* Bug [9623](https://dev.icinga.org/issues/9623 "Bug 9623"): missing config warning on empty port in endpoints
* Bug [9769](https://dev.icinga.org/issues/9769 "Bug 9769"): Set correct X509 version for certificates
* Bug [9773](https://dev.icinga.org/issues/9773 "Bug 9773"): Add log for missing EventCommand for command_endpoints
* Bug [9779](https://dev.icinga.org/issues/9779 "Bug 9779"): Trying to set a field for a non-object instance fails
* Bug [9782](https://dev.icinga.org/issues/9782 "Bug 9782"): icinga2 node wizard don't take zone_name input
* Bug [9806](https://dev.icinga.org/issues/9806 "Bug 9806"): Operator + is inconsistent when used with empty and non-empty strings
* Bug [9814](https://dev.icinga.org/issues/9814 "Bug 9814"): Build fix for Boost 1.59
* Bug [9835](https://dev.icinga.org/issues/9835 "Bug 9835"): Dict initializer incorrectly re-initializes field that is set to an empty string
* Bug [9860](https://dev.icinga.org/issues/9860 "Bug 9860"): missing check_perfmon.exe
* Bug [9867](https://dev.icinga.org/issues/9867 "Bug 9867"): Agent freezes when the check returns massive output
* Bug [9884](https://dev.icinga.org/issues/9884 "Bug 9884"): Warning about invalid API function icinga::Hello
* Bug [9897](https://dev.icinga.org/issues/9897 "Bug 9897"): First SOFT state is recognized as second SOFT state
* Bug [9902](https://dev.icinga.org/issues/9902 "Bug 9902"): typo in docs
* Bug [9912](https://dev.icinga.org/issues/9912 "Bug 9912"): check_command interfaces option match_aliases has to be boolean
* Bug [9913](https://dev.icinga.org/issues/9913 "Bug 9913"): Default disk checks on Windows fail because check_disk doesn't support -K
* Bug [9928](https://dev.icinga.org/issues/9928 "Bug 9928"): Add missing category for IDO query
* Bug [9947](https://dev.icinga.org/issues/9947 "Bug 9947"): Serial number field is not properly initialized for CA certificates
* Bug [9961](https://dev.icinga.org/issues/9961 "Bug 9961"): Don't re-download NSCP for every build
* Bug [9962](https://dev.icinga.org/issues/9962 "Bug 9962"): Utility::Glob on Windows doesn't support wildcards in all but the last path component
* Bug [9972](https://dev.icinga.org/issues/9972 "Bug 9972"): Icinga2 - too many open files - Exception
* Bug [9984](https://dev.icinga.org/issues/9984 "Bug 9984"): fix check command nscp-local
* Bug [9992](https://dev.icinga.org/issues/9992 "Bug 9992"): Duplicate severity type in the documentation for SyslogLogger

### What's New in Version 2.3.8

#### Changes

* Bugfixes

#### Bugfixes

* Bug [9554](https://dev.icinga.org/issues/9554 "Bug 9554"): Don't allow "ignore where" for groups when there's no "assign where"
* Bug [9634](https://dev.icinga.org/issues/9634 "Bug 9634"): DB IDO: Do not update endpointstatus table on config updates
* Bug [9637](https://dev.icinga.org/issues/9637 "Bug 9637"): Wrong parameter for CheckCommand "ping-common-windows"
* Bug [9665](https://dev.icinga.org/issues/9665 "Bug 9665"): Escaping does not work for OpenTSDB perfdata plugin
* Bug [9666](https://dev.icinga.org/issues/9666 "Bug 9666"): checkcommand disk does not check free inode - check_disk

### What's New in Version 2.3.7

#### Changes

* Bugfixes

#### Features

* Feature [9610](https://dev.icinga.org/issues/9610 "Feature 9610"): Enhance troubleshooting ssl errors & cluster replay log

#### Bugfixes

* Bug [9406](https://dev.icinga.org/issues/9406 "Bug 9406"): Selective cluster reconnecting breaks client communication
* Bug [9535](https://dev.icinga.org/issues/9535 "Bug 9535"): Config parser ignores "ignore" in template definition
* Bug [9584](https://dev.icinga.org/issues/9584 "Bug 9584"): Incorrect return value for the macro() function
* Bug [9585](https://dev.icinga.org/issues/9585 "Bug 9585"): Wrong formatting in DB IDO extensions docs
* Bug [9586](https://dev.icinga.org/issues/9586 "Bug 9586"): DB IDO: endpoint* tables are cleared on reload causing constraint violations
* Bug [9621](https://dev.icinga.org/issues/9621 "Bug 9621"): Assertion failed in icinga::ScriptUtils::Intersection
* Bug [9622](https://dev.icinga.org/issues/9622 "Bug 9622"): Missing lock in ScriptUtils::Union

### What's New in Version 2.3.6

#### Changes

* Require openssl1 on sles11sp3 from Security Module repository
  * Bug in SLES 11's OpenSSL version 0.9.8j preventing verification of generated certificates.
  * Re-create these certificates with 2.3.6 linking against openssl1 (cli command or CSR auto-signing).
* ITL: Add ldap, ntp_peer, mongodb and elasticsearch CheckCommand definitions
* Bugfixes

#### Features

* Feature [6714](https://dev.icinga.org/issues/6714 "Feature 6714"): add pagerduty notification documentation
* Feature [9172](https://dev.icinga.org/issues/9172 "Feature 9172"): Add "ldap" CheckCommand for "check_ldap" plugin
* Feature [9191](https://dev.icinga.org/issues/9191 "Feature 9191"): Add "mongodb" CheckCommand definition
* Feature [9415](https://dev.icinga.org/issues/9415 "Feature 9415"): Add elasticsearch checkcommand to itl
* Feature [9416](https://dev.icinga.org/issues/9416 "Feature 9416"): snmpv3 CheckCommand: Add possibility to set securityLevel
* Feature [9451](https://dev.icinga.org/issues/9451 "Feature 9451"): Merge documentation fixes from GitHub
* Feature [9523](https://dev.icinga.org/issues/9523 "Feature 9523"): Add ntp_peer CheckCommand
* Feature [9562](https://dev.icinga.org/issues/9562 "Feature 9562"): Add new options for ntp_time CheckCommand
* Feature [9578](https://dev.icinga.org/issues/9578 "Feature 9578"): new options for smtp CheckCommand

#### Bugfixes

* Bug [9205](https://dev.icinga.org/issues/9205 "Bug 9205"): port empty when using icinga2 node wizard
* Bug [9253](https://dev.icinga.org/issues/9253 "Bug 9253"): Incorrect variable name in the ITL
* Bug [9303](https://dev.icinga.org/issues/9303 "Bug 9303"): Missing 'snmp_is_cisco' in Manubulon snmp-memory command definition
* Bug [9436](https://dev.icinga.org/issues/9436 "Bug 9436"): Functions can't be specified as command arguments
* Bug [9450](https://dev.icinga.org/issues/9450 "Bug 9450"): node setup: indent accept_config and accept_commands
* Bug [9452](https://dev.icinga.org/issues/9452 "Bug 9452"): Wrong file reference in README.md
* Bug [9456](https://dev.icinga.org/issues/9456 "Bug 9456"): Windows client w/ command_endpoint broken with $nscp_path$ and NscpPath detection
* Bug [9463](https://dev.icinga.org/issues/9463 "Bug 9463"): Incorrect check_ping.exe parameter in the ITL
* Bug [9476](https://dev.icinga.org/issues/9476 "Bug 9476"): Documentation for checks in an HA zone is wrong
* Bug [9481](https://dev.icinga.org/issues/9481 "Bug 9481"): Fix stability issues in the TlsStream/Stream classes
* Bug [9489](https://dev.icinga.org/issues/9489 "Bug 9489"): Add log message for discarded cluster events (e.g. from unauthenticated clients)
* Bug [9490](https://dev.icinga.org/issues/9490 "Bug 9490"): Missing openssl verify in cluster troubleshooting docs
* Bug [9513](https://dev.icinga.org/issues/9513 "Bug 9513"): itl/plugins-contrib.d/*.conf should point to PluginContribDir
* Bug [9522](https://dev.icinga.org/issues/9522 "Bug 9522"): wrong default port documentated for nrpe
* Bug [9549](https://dev.icinga.org/issues/9549 "Bug 9549"): Generated certificates cannot be verified w/ openssl 0.9.8j on SLES 11
* Bug [9558](https://dev.icinga.org/issues/9558 "Bug 9558"): mysql-devel is not available in sles11sp3
* Bug [9563](https://dev.icinga.org/issues/9563 "Bug 9563"): Update getting started for Debian Jessie

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

* Feature [8116](https://dev.icinga.org/issues/8116 "Feature 8116"): Extend Windows installer with an update mode
* Feature [8180](https://dev.icinga.org/issues/8180 "Feature 8180"): Add documentation and CheckCommands for the windows plugins
* Feature [8809](https://dev.icinga.org/issues/8809 "Feature 8809"): Add check_perfmon plugin for Windows
* Feature [9115](https://dev.icinga.org/issues/9115 "Feature 9115"): Add SHOWALL to NSCP Checkcommand
* Feature [9130](https://dev.icinga.org/issues/9130 "Feature 9130"): Add 'check_drivesize' as nscp-local check command
* Feature [9145](https://dev.icinga.org/issues/9145 "Feature 9145"): Add arguments to "dns" CheckCommand
* Feature [9146](https://dev.icinga.org/issues/9146 "Feature 9146"): Add arguments to "ftp" CheckCommand
* Feature [9147](https://dev.icinga.org/issues/9147 "Feature 9147"): Add arguments to "tcp" CheckCommand
* Feature [9176](https://dev.icinga.org/issues/9176 "Feature 9176"): ITL Documentation: Add a link for passing custom attributes as command parameters
* Feature [9180](https://dev.icinga.org/issues/9180 "Feature 9180"): Include Windows support details in the documentation
* Feature [9185](https://dev.icinga.org/issues/9185 "Feature 9185"): Add timestamp support for PerfdataWriter
* Feature [9191](https://dev.icinga.org/issues/9191 "Feature 9191"): Add "mongodb" CheckCommand definition
* Feature [9238](https://dev.icinga.org/issues/9238 "Feature 9238"): Bundle NSClient++ in Windows Installer
* Feature [9254](https://dev.icinga.org/issues/9254 "Feature 9254"): Add 'disk_smb' Plugin CheckCommand definition
* Feature [9256](https://dev.icinga.org/issues/9256 "Feature 9256"): Determine NSClient++ installation path using MsiGetComponentPath
* Feature [9260](https://dev.icinga.org/issues/9260 "Feature 9260"): Include <nscp> by default on Windows
* Feature [9261](https://dev.icinga.org/issues/9261 "Feature 9261"): Add the --load-all and --log options for nscp-local
* Feature [9263](https://dev.icinga.org/issues/9263 "Feature 9263"): Add support for installing NSClient++ in the Icinga 2 Windows wizard
* Feature [9270](https://dev.icinga.org/issues/9270 "Feature 9270"): Update service apply for documentation
* Feature [9272](https://dev.icinga.org/issues/9272 "Feature 9272"): Add 'iftraffic' to plugins-contrib check command definitions
* Feature [9285](https://dev.icinga.org/issues/9285 "Feature 9285"): Best practices: cluster config sync
* Feature [9297](https://dev.icinga.org/issues/9297 "Feature 9297"): Add examples for function usage in "set_if" and "command" attributes
* Feature [9310](https://dev.icinga.org/issues/9310 "Feature 9310"): Add typeof in 'assign/ignore where' expression as example
* Feature [9311](https://dev.icinga.org/issues/9311 "Feature 9311"): Add local variable scope for *Command to documentation (host, service, etc)
* Feature [9313](https://dev.icinga.org/issues/9313 "Feature 9313"): Use a more simple example for passing command parameters
* Feature [9318](https://dev.icinga.org/issues/9318 "Feature 9318"): Explain string concatenation in objects by real-world example
* Feature [9363](https://dev.icinga.org/issues/9363 "Feature 9363"): Update documentation for escape sequences
* Feature [9419](https://dev.icinga.org/issues/9419 "Feature 9419"): Enhance cluster/client troubleshooting
* Feature [9420](https://dev.icinga.org/issues/9420 "Feature 9420"): Enhance cluster docs with HA command_endpoints
* Feature [9431](https://dev.icinga.org/issues/9431 "Feature 9431"): Documentation: Move configuration before advanced topics

#### Bugfixes

* Bug [8853](https://dev.icinga.org/issues/8853 "Bug 8853"): Syntax Highlighting: host.address vs host.add
* Bug [8888](https://dev.icinga.org/issues/8888 "Bug 8888"): Icinga2 --version: Error showing Distribution
* Bug [8891](https://dev.icinga.org/issues/8891 "Bug 8891"): Node wont connect properly to master if host is is not set for Endpoint on new installs
* Bug [9055](https://dev.icinga.org/issues/9055 "Bug 9055"): Wrong timestamps w/ historical data replay in DB IDO
* Bug [9109](https://dev.icinga.org/issues/9109 "Bug 9109"): WIN: syslog is not an enable-able feature in windows
* Bug [9116](https://dev.icinga.org/issues/9116 "Bug 9116"): node update-config reports critical and warning
* Bug [9121](https://dev.icinga.org/issues/9121 "Bug 9121"): Possible DB deadlock
* Bug [9131](https://dev.icinga.org/issues/9131 "Bug 9131"): Missing ")" in last Apply Rules example
* Bug [9142](https://dev.icinga.org/issues/9142 "Bug 9142"): Downtimes are always "fixed"
* Bug [9143](https://dev.icinga.org/issues/9143 "Bug 9143"): Incorrect type and state filter mapping for User objects in DB IDO
* Bug [9161](https://dev.icinga.org/issues/9161 "Bug 9161"): 'disk': wrong order of threshold command arguments
* Bug [9187](https://dev.icinga.org/issues/9187 "Bug 9187"): SPEC: Give group write permissions for perfdata dir
* Bug [9205](https://dev.icinga.org/issues/9205 "Bug 9205"): port empty when using icinga2 node wizard
* Bug [9222](https://dev.icinga.org/issues/9222 "Bug 9222"): Missing custom attributes in backends if name is equal to object attribute
* Bug [9253](https://dev.icinga.org/issues/9253 "Bug 9253"): Incorrect variable name in the ITL
* Bug [9255](https://dev.icinga.org/issues/9255 "Bug 9255"): --scm-installs fails when the service is already installed
* Bug [9258](https://dev.icinga.org/issues/9258 "Bug 9258"): Some checks in the default Windows configuration fail
* Bug [9259](https://dev.icinga.org/issues/9259 "Bug 9259"): Disk and 'icinga' services are missing in the default Windows config
* Bug [9268](https://dev.icinga.org/issues/9268 "Bug 9268"): Typo in Configuration Best Practice
* Bug [9269](https://dev.icinga.org/issues/9269 "Bug 9269"): Wrong permission etc on windows
* Bug [9324](https://dev.icinga.org/issues/9324 "Bug 9324"): Multi line output not correctly handled from compat channels
* Bug [9328](https://dev.icinga.org/issues/9328 "Bug 9328"): Multiline vars are broken in objects.cache output
* Bug [9372](https://dev.icinga.org/issues/9372 "Bug 9372"): plugins-contrib.d/databases.conf: wrong argument for mssql_health
* Bug [9389](https://dev.icinga.org/issues/9389 "Bug 9389"): Documentation: Typo
* Bug [9390](https://dev.icinga.org/issues/9390 "Bug 9390"): Wrong service table attributes in Livestatus documentation
* Bug [9393](https://dev.icinga.org/issues/9393 "Bug 9393"): Documentation: Extend Custom Attributes with the boolean type
* Bug [9394](https://dev.icinga.org/issues/9394 "Bug 9394"): Including <nscp> on Linux fails with unregistered function
* Bug [9399](https://dev.icinga.org/issues/9399 "Bug 9399"): Documentation: Typo
* Bug [9406](https://dev.icinga.org/issues/9406 "Bug 9406"): Selective cluster reconnecting breaks client communication
* Bug [9412](https://dev.icinga.org/issues/9412 "Bug 9412"): Documentation: Update the link to register a new Icinga account

### What's New in Version 2.3.4

#### Changes

* ITL: Check commands for various databases
* Improve validation messages for time periods
* Update max_check_attempts in generic-{host,service} templates
* Update logrotate configuration
* Bugfixes

#### Features

* Feature [8760](https://dev.icinga.org/issues/8760 "Feature 8760"): Add database plugins to ITL
* Feature [8803](https://dev.icinga.org/issues/8803 "Feature 8803"): Agent Wizard: add options for API defaults
* Feature [8893](https://dev.icinga.org/issues/8893 "Feature 8893"): Improve timeperiod validation error messages
* Feature [8895](https://dev.icinga.org/issues/8895 "Feature 8895"): Add explanatory note for Icinga2 client documentation

#### Bugfixes

* Bug [8808](https://dev.icinga.org/issues/8808 "Bug 8808"): logrotate doesn't work on Ubuntu
* Bug [8821](https://dev.icinga.org/issues/8821 "Bug 8821"): command_endpoint check_results are not replicated to other endpoints in the same zone
* Bug [8879](https://dev.icinga.org/issues/8879 "Bug 8879"): Reword documentation of check_address
* Bug [8881](https://dev.icinga.org/issues/8881 "Bug 8881"): Add arguments to the UPS check
* Bug [8889](https://dev.icinga.org/issues/8889 "Bug 8889"): Fix a minor markdown error
* Bug [8892](https://dev.icinga.org/issues/8892 "Bug 8892"): Validation errors for time ranges which span the DST transition
* Bug [8894](https://dev.icinga.org/issues/8894 "Bug 8894"): Default max_check_attempts should be lower for hosts than for services
* Bug [8913](https://dev.icinga.org/issues/8913 "Bug 8913"): Windows Build: Flex detection
* Bug [8917](https://dev.icinga.org/issues/8917 "Bug 8917"): Node wizard should only accept 'y', 'n', 'Y' and 'N' as answers for boolean questions
* Bug [8919](https://dev.icinga.org/issues/8919 "Bug 8919"): Fix complexity class for Dictionary::Get
* Bug [8987](https://dev.icinga.org/issues/8987 "Bug 8987"): Fix a typo
* Bug [9012](https://dev.icinga.org/issues/9012 "Bug 9012"): Typo in graphite feature enable documentation
* Bug [9014](https://dev.icinga.org/issues/9014 "Bug 9014"): Don't update scheduleddowntime table w/ trigger_time column when only adding a downtime
* Bug [9016](https://dev.icinga.org/issues/9016 "Bug 9016"): Downtimes which have been triggered are not properly recorded in the database
* Bug [9017](https://dev.icinga.org/issues/9017 "Bug 9017"): scheduled_downtime_depth column is not reset when a downtime ends or when a downtime is being removed
* Bug [9021](https://dev.icinga.org/issues/9021 "Bug 9021"): Multiple log messages w/ "Attempting to send notifications for notification object"
* Bug [9041](https://dev.icinga.org/issues/9041 "Bug 9041"): Acknowledging problems w/ expire time does not add the expiry information to the related comment for IDO and compat
* Bug [9045](https://dev.icinga.org/issues/9045 "Bug 9045"): Vim syntax: Match groups before host/service/user objects
* Bug [9049](https://dev.icinga.org/issues/9049 "Bug 9049"): check_disk order of command arguments
* Bug [9050](https://dev.icinga.org/issues/9050 "Bug 9050"): web.conf is not in the RPM package
* Bug [9064](https://dev.icinga.org/issues/9064 "Bug 9064"): troubleshoot truncates crash reports
* Bug [9069](https://dev.icinga.org/issues/9069 "Bug 9069"): Documentation: set_if usage with boolean values and functions
* Bug [9073](https://dev.icinga.org/issues/9073 "Bug 9073"): custom attributes with recursive macro function calls causing sigabrt

### What's New in Version 2.3.3

#### Changes

* New function: parse_performance_data
* Include more details in --version
* Improve documentation
* Bugfixes

#### Features

* Feature [8685](https://dev.icinga.org/issues/8685 "Feature 8685"): Show state/type filter names in notice/debug log
* Feature [8686](https://dev.icinga.org/issues/8686 "Feature 8686"): Update documentation for "apply for" rules
* Feature [8693](https://dev.icinga.org/issues/8693 "Feature 8693"): New function: parse_performance_data
* Feature [8740](https://dev.icinga.org/issues/8740 "Feature 8740"): Add "access objects at runtime" examples to advanced section
* Feature [8761](https://dev.icinga.org/issues/8761 "Feature 8761"): Include more details in --version
* Feature [8816](https://dev.icinga.org/issues/8816 "Feature 8816"): Add "random" CheckCommand for test and demo purposes
* Feature [8827](https://dev.icinga.org/issues/8827 "Feature 8827"): Move release info in INSTALL.md into a separate file

#### Bugfixes

* Bug [8660](https://dev.icinga.org/issues/8660 "Bug 8660"): Update syntax highlighting for 2.3 features
* Bug [8677](https://dev.icinga.org/issues/8677 "Bug 8677"): Re-order the object types in alphabetical order
* Bug [8724](https://dev.icinga.org/issues/8724 "Bug 8724"): Missing config validator for command arguments 'set_if'
* Bug [8734](https://dev.icinga.org/issues/8734 "Bug 8734"): startup.log broken when the DB schema needs an update
* Bug [8736](https://dev.icinga.org/issues/8736 "Bug 8736"): Don't update custom vars for each status update
* Bug [8748](https://dev.icinga.org/issues/8748 "Bug 8748"): Don't ignore extraneous arguments for functions
* Bug [8749](https://dev.icinga.org/issues/8749 "Bug 8749"): Build warnings with CMake 3.1.3
* Bug [8750](https://dev.icinga.org/issues/8750 "Bug 8750"): Flex version check does not reject unsupported versions
* Bug [8753](https://dev.icinga.org/issues/8753 "Bug 8753"): Fix a typo in the documentation of ICINGA2_WITH_MYSQL and ICINGA2_WITH_PGSQL
* Bug [8755](https://dev.icinga.org/issues/8755 "Bug 8755"): Fix VIM syntax highlighting for comments
* Bug [8757](https://dev.icinga.org/issues/8757 "Bug 8757"): Add missing keywords in the syntax highlighting files
* Bug [8762](https://dev.icinga.org/issues/8762 "Bug 8762"): Plugin "check_http" is missing in Windows environments
* Bug [8763](https://dev.icinga.org/issues/8763 "Bug 8763"): Typo in doc library-reference
* Bug [8764](https://dev.icinga.org/issues/8764 "Bug 8764"): Revamp migration documentation
* Bug [8765](https://dev.icinga.org/issues/8765 "Bug 8765"): Explain processing logic/order of apply rules with for loops
* Bug [8766](https://dev.icinga.org/issues/8766 "Bug 8766"): Remove prompt to create a TicketSalt from the wizard
* Bug [8767](https://dev.icinga.org/issues/8767 "Bug 8767"): Typo and invalid example in the runtime macro documentation
* Bug [8769](https://dev.icinga.org/issues/8769 "Bug 8769"): Improve error message for invalid field access
* Bug [8770](https://dev.icinga.org/issues/8770 "Bug 8770"): object Notification + apply Service fails with error "...refers to service which doesn't exist"
* Bug [8771](https://dev.icinga.org/issues/8771 "Bug 8771"): Correct HA documentation
* Bug [8829](https://dev.icinga.org/issues/8829 "Bug 8829"): Figure out why command validators are not triggered
* Bug [8834](https://dev.icinga.org/issues/8834 "Bug 8834"): Return doesn't work inside loops
* Bug [8844](https://dev.icinga.org/issues/8844 "Bug 8844"): Segmentation fault when executing "icinga2 pki new-cert"
* Bug [8862](https://dev.icinga.org/issues/8862 "Bug 8862"): wrong 'dns_lookup' custom attribute default in command-plugins.conf
* Bug [8866](https://dev.icinga.org/issues/8866 "Bug 8866"): Fix incorrect perfdata templates in the documentation
* Bug [8869](https://dev.icinga.org/issues/8869 "Bug 8869"): Array in command arguments doesn't work

### What's New in Version 2.3.2

#### Changes

* Bugfixes

#### Bugfixes

* Bug [8721](https://dev.icinga.org/issues/8721 "Bug 8721"): Log message for cli commands breaks the init script

### What's New in Version 2.3.1

#### Changes

* Bugfixes

Please note that this version fixes the default thresholds for the disk check which were inadvertently broken in 2.3.0; if you're using percent-based custom thresholds you will need to add the '%' sign to your custom attributes

#### Features

* Feature [8659](https://dev.icinga.org/issues/8659 "Feature 8659"): Implement String#contains

#### Bugfixes

* Bug [8540](https://dev.icinga.org/issues/8540 "Bug 8540"): Kill signal sent only to check process, not whole process group
* Bug [8657](https://dev.icinga.org/issues/8657 "Bug 8657"): Missing program name in 'icinga2 --version'
* Bug [8658](https://dev.icinga.org/issues/8658 "Bug 8658"): Fix check_disk thresholds: make sure partitions are the last arguments
* Bug [8672](https://dev.icinga.org/issues/8672 "Bug 8672"): Api heartbeat message response time problem
* Bug [8673](https://dev.icinga.org/issues/8673 "Bug 8673"): Fix check_disk default thresholds and document the change of unit
* Bug [8679](https://dev.icinga.org/issues/8679 "Bug 8679"): Config validation fail because of unexpected new-line
* Bug [8680](https://dev.icinga.org/issues/8680 "Bug 8680"): Update documentation for DB IDO HA Run-Once
* Bug [8683](https://dev.icinga.org/issues/8683 "Bug 8683"): Make sure that the /var/log/icinga2/crash directory exists
* Bug [8684](https://dev.icinga.org/issues/8684 "Bug 8684"): Fix formatting for the GDB stacktrace
* Bug [8687](https://dev.icinga.org/issues/8687 "Bug 8687"): Crash in Dependency::Stop
* Bug [8691](https://dev.icinga.org/issues/8691 "Bug 8691"): Debian packages do not create /var/log/icinga2/crash

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

* [DB IDO schema upgrade](18-upgrading-icinga-2.md#upgrading-icinga-2) to `1.13.0` required!

#### Features

* Feature [3446](https://dev.icinga.org/issues/3446 "Feature 3446"): Add troubleshooting collect cli command
* Feature [6109](https://dev.icinga.org/issues/6109 "Feature 6109"): Don't spawn threads for network connections
* Feature [6570](https://dev.icinga.org/issues/6570 "Feature 6570"): Disallow side-effect-free r-value expressions in expression lists
* Feature [6697](https://dev.icinga.org/issues/6697 "Feature 6697"): Plugin Check Commands: add check_vmware_esx
* Feature [6857](https://dev.icinga.org/issues/6857 "Feature 6857"): Run CheckCommands with C locale (workaround for comma vs dot and plugin api bug)
* Feature [6858](https://dev.icinga.org/issues/6858 "Feature 6858"): Add some more PNP details
* Feature [6868](https://dev.icinga.org/issues/6868 "Feature 6868"): Disable flapping detection by default
* Feature [6923](https://dev.icinga.org/issues/6923 "Feature 6923"): IDO should fill program_end_time on a clean shutdown
* Feature [7136](https://dev.icinga.org/issues/7136 "Feature 7136"): extended Manubulon SNMP Check Plugin Command
* Feature [7209](https://dev.icinga.org/issues/7209 "Feature 7209"): ITL: Interfacetable
* Feature [7256](https://dev.icinga.org/issues/7256 "Feature 7256"): Add OpenTSDB Writer
* Feature [7292](https://dev.icinga.org/issues/7292 "Feature 7292"): ITL: Check_Mem.pl
* Feature [7294](https://dev.icinga.org/issues/7294 "Feature 7294"): ITL: ESXi-Hardware
* Feature [7326](https://dev.icinga.org/issues/7326 "Feature 7326"): Add parent soft states option to Dependency object configuration
* Feature [7361](https://dev.icinga.org/issues/7361 "Feature 7361"): Livestatus: Add GroupBy tables: hostsbygroup, servicesbygroup, servicesbyhostgroup
* Feature [7545](https://dev.icinga.org/issues/7545 "Feature 7545"): Please add labels in SNMP checks
* Feature [7564](https://dev.icinga.org/issues/7564 "Feature 7564"): Access object runtime attributes in custom vars & command arguments
* Feature [7610](https://dev.icinga.org/issues/7610 "Feature 7610"): Variable from for loop not usable in assign statement
* Feature [7700](https://dev.icinga.org/issues/7700 "Feature 7700"): Evaluate apply/object rules when the parent objects are created
* Feature [7702](https://dev.icinga.org/issues/7702 "Feature 7702"): Add an option that hides CLI commands
* Feature [7704](https://dev.icinga.org/issues/7704 "Feature 7704"): ConfigCompiler::HandleInclude* should return an AST node
* Feature [7706](https://dev.icinga.org/issues/7706 "Feature 7706"): ConfigCompiler::Compile* should return an AST node
* Feature [7748](https://dev.icinga.org/issues/7748 "Feature 7748"): Redesign how stack frames work for scripts
* Feature [7767](https://dev.icinga.org/issues/7767 "Feature 7767"): Rename _DEBUG to I2_DEBUG
* Feature [7774](https://dev.icinga.org/issues/7774 "Feature 7774"): Implement an AST Expression for T_CONST
* Feature [7778](https://dev.icinga.org/issues/7778 "Feature 7778"): Missing check_disk output on Windows
* Feature [7784](https://dev.icinga.org/issues/7784 "Feature 7784"): Implement the DISABLE_HOST_SVC_NOTIFICATIONS and ENABLE_HOST_SVC_NOTIFICATIONS commands
* Feature [7793](https://dev.icinga.org/issues/7793 "Feature 7793"): Don't build db_ido when both MySQL and PostgreSQL aren't enabled
* Feature [7794](https://dev.icinga.org/issues/7794 "Feature 7794"): Implement an option to disable building the Livestatus module
* Feature [7795](https://dev.icinga.org/issues/7795 "Feature 7795"): Implement an option to disable building the Demo component
* Feature [7805](https://dev.icinga.org/issues/7805 "Feature 7805"): Implement unit tests for the config parser
* Feature [7807](https://dev.icinga.org/issues/7807 "Feature 7807"): Move the cast functions into libbase
* Feature [7813](https://dev.icinga.org/issues/7813 "Feature 7813"): Implement the % operator
* Feature [7816](https://dev.icinga.org/issues/7816 "Feature 7816"): Document operator precedence
* Feature [7822](https://dev.icinga.org/issues/7822 "Feature 7822"): Make the config parser thread-safe
* Feature [7823](https://dev.icinga.org/issues/7823 "Feature 7823"): Figure out whether Number + String should implicitly convert the Number argument to a string
* Feature [7824](https://dev.icinga.org/issues/7824 "Feature 7824"): Implement the "if" and "else" keywords
* Feature [7873](https://dev.icinga.org/issues/7873 "Feature 7873"): Plugin Check Commands: Add icmp
* Feature [7879](https://dev.icinga.org/issues/7879 "Feature 7879"): Windows agent is missing the standard plugin check_ping
* Feature [7883](https://dev.icinga.org/issues/7883 "Feature 7883"): Implement official support for user-defined functions and the "for" keyword
* Feature [7901](https://dev.icinga.org/issues/7901 "Feature 7901"): Implement socket_path attribute for the IdoMysqlConnection class
* Feature [7910](https://dev.icinga.org/issues/7910 "Feature 7910"): The lexer shouldn't accept escapes for characters which don't have to be escaped
* Feature [7925](https://dev.icinga.org/issues/7925 "Feature 7925"): Move the config file for the ido-*sql features into the icinga2-ido-* packages
* Feature [8016](https://dev.icinga.org/issues/8016 "Feature 8016"): Documentation enhancement for snmp traps and passive checks.
* Feature [8019](https://dev.icinga.org/issues/8019 "Feature 8019"): Register type objects as global variables
* Feature [8020](https://dev.icinga.org/issues/8020 "Feature 8020"): Improve output of ToString for type objects
* Feature [8030](https://dev.icinga.org/issues/8030 "Feature 8030"): Evaluate usage of function()
* Feature [8033](https://dev.icinga.org/issues/8033 "Feature 8033"): Allow name changed from inside the object
* Feature [8040](https://dev.icinga.org/issues/8040 "Feature 8040"): Disallow calling strings as functions
* Feature [8043](https://dev.icinga.org/issues/8043 "Feature 8043"): Implement a boolean sub-type for the Value class
* Feature [8047](https://dev.icinga.org/issues/8047 "Feature 8047"): ConfigCompiler::HandleInclude should return an inline dictionary
* Feature [8060](https://dev.icinga.org/issues/8060 "Feature 8060"): Windows plugins should behave like their Linux cousins
* Feature [8065](https://dev.icinga.org/issues/8065 "Feature 8065"): Implement a way to remove dictionary keys
* Feature [8071](https://dev.icinga.org/issues/8071 "Feature 8071"): Implement a way to call methods on objects
* Feature [8074](https://dev.icinga.org/issues/8074 "Feature 8074"): Figure out how variable scopes should work
* Feature [8078](https://dev.icinga.org/issues/8078 "Feature 8078"): Backport i2tcl's error reporting functionality into "icinga2 console"
* Feature [8096](https://dev.icinga.org/issues/8096 "Feature 8096"): Document the new language features in 2.3
* Feature [8121](https://dev.icinga.org/issues/8121 "Feature 8121"): feature enable should use relative symlinks
* Feature [8133](https://dev.icinga.org/issues/8133 "Feature 8133"): Implement line-continuation for the "console" command
* Feature [8169](https://dev.icinga.org/issues/8169 "Feature 8169"): Implement additional methods for strings
* Feature [8172](https://dev.icinga.org/issues/8172 "Feature 8172"): Assignments shouldn't have a "return" value
* Feature [8195](https://dev.icinga.org/issues/8195 "Feature 8195"): Host/Service runtime macro downtime_depth
* Feature [8226](https://dev.icinga.org/issues/8226 "Feature 8226"): Make invalid log-severity option output an error instead of a warning
* Feature [8244](https://dev.icinga.org/issues/8244 "Feature 8244"): Implement keywords to explicitly access globals/locals
* Feature [8259](https://dev.icinga.org/issues/8259 "Feature 8259"): The check "hostalive" is not working with ipv6
* Feature [8269](https://dev.icinga.org/issues/8269 "Feature 8269"): Implement the while keyword
* Feature [8277](https://dev.icinga.org/issues/8277 "Feature 8277"): Add macros $host.check_source$ and $service.check_source$
* Feature [8290](https://dev.icinga.org/issues/8290 "Feature 8290"): Make operators &&, || behave like in JavaScript
* Feature [8291](https://dev.icinga.org/issues/8291 "Feature 8291"): Implement validator support for function objects
* Feature [8293](https://dev.icinga.org/issues/8293 "Feature 8293"): The Zone::global attribute is not documented
* Feature [8316](https://dev.icinga.org/issues/8316 "Feature 8316"): Extend disk checkcommand
* Feature [8322](https://dev.icinga.org/issues/8322 "Feature 8322"): Implement Array#join
* Feature [8371](https://dev.icinga.org/issues/8371 "Feature 8371"): Add path information for objects in object list
* Feature [8374](https://dev.icinga.org/issues/8374 "Feature 8374"): Add timestamp support for Graphite
* Feature [8386](https://dev.icinga.org/issues/8386 "Feature 8386"): Add documentation for cli command 'console'
* Feature [8393](https://dev.icinga.org/issues/8393 "Feature 8393"): Implement support for Json.encode and Json.decode
* Feature [8394](https://dev.icinga.org/issues/8394 "Feature 8394"): Implement continue/break keywords
* Feature [8399](https://dev.icinga.org/issues/8399 "Feature 8399"): Backup certificate files in 'node setup'
* Feature [8410](https://dev.icinga.org/issues/8410 "Feature 8410"): udp check command is missing arguments.
* Feature [8414](https://dev.icinga.org/issues/8414 "Feature 8414"): Add ITL check command for check_ipmi_sensor
* Feature [8429](https://dev.icinga.org/issues/8429 "Feature 8429"): add webinject checkcommand
* Feature [8465](https://dev.icinga.org/issues/8465 "Feature 8465"): Add the ability to use a CA certificate as a way of verifying hosts for CSR autosigning
* Feature [8467](https://dev.icinga.org/issues/8467 "Feature 8467"): introduce time dependent variable values
* Feature [8498](https://dev.icinga.org/issues/8498 "Feature 8498"): Snmp CheckCommand misses various options
* Feature [8515](https://dev.icinga.org/issues/8515 "Feature 8515"): Show slave lag for the cluster-zone check
* Feature [8522](https://dev.icinga.org/issues/8522 "Feature 8522"): Update Remote Client/Distributed Monitoring Documentation
* Feature [8527](https://dev.icinga.org/issues/8527 "Feature 8527"): Change Livestatus query log level to 'notice'
* Feature [8548](https://dev.icinga.org/issues/8548 "Feature 8548"): Add support for else-if
* Feature [8575](https://dev.icinga.org/issues/8575 "Feature 8575"): Include GDB backtrace in crash reports
* Feature [8599](https://dev.icinga.org/issues/8599 "Feature 8599"): Remove macro argument for IMPL_TYPE_LOOKUP
* Feature [8600](https://dev.icinga.org/issues/8600 "Feature 8600"): Add validator for time ranges in ScheduledDowntime objects
* Feature [8610](https://dev.icinga.org/issues/8610 "Feature 8610"): Support the SNI TLS extension
* Feature [8621](https://dev.icinga.org/issues/8621 "Feature 8621"): Add check commands for NSClient++
* Feature [8648](https://dev.icinga.org/issues/8648 "Feature 8648"): Document closures ('use')

#### Bugfixes

* Bug [6171](https://dev.icinga.org/issues/6171 "Bug 6171"): Remove name and return value for stats functions
* Bug [6959](https://dev.icinga.org/issues/6959 "Bug 6959"): Scheduled start time will be ignored if the host or service is already in a problem state
* Bug [7311](https://dev.icinga.org/issues/7311 "Bug 7311"): Invalid macro results in exception
* Bug [7542](https://dev.icinga.org/issues/7542 "Bug 7542"): Update validators for CustomVarObject
* Bug [7576](https://dev.icinga.org/issues/7576 "Bug 7576"): validate configured legacy timeperiod ranges
* Bug [7582](https://dev.icinga.org/issues/7582 "Bug 7582"): Variable expansion is single quoted.
* Bug [7644](https://dev.icinga.org/issues/7644 "Bug 7644"): Unity build doesn't work with MSVC
* Bug [7647](https://dev.icinga.org/issues/7647 "Bug 7647"): Avoid rebuilding libbase when the version number changes
* Bug [7731](https://dev.icinga.org/issues/7731 "Bug 7731"): Reminder notifications not being sent but logged every 5 secs
* Bug [7765](https://dev.icinga.org/issues/7765 "Bug 7765"): DB IDO: Duplicate entry icinga_{host,service}dependencies
* Bug [7800](https://dev.icinga.org/issues/7800 "Bug 7800"): Fix the shift/reduce conflicts in the parser
* Bug [7802](https://dev.icinga.org/issues/7802 "Bug 7802"): Change parameter type for include and include_recursive to T_STRING
* Bug [7808](https://dev.icinga.org/issues/7808 "Bug 7808"): Unterminated string literals should cause parser to return an error
* Bug [7809](https://dev.icinga.org/issues/7809 "Bug 7809"): Scoping rules for "for" are broken
* Bug [7810](https://dev.icinga.org/issues/7810 "Bug 7810"): Return values for functions are broken
* Bug [7811](https://dev.icinga.org/issues/7811 "Bug 7811"): The __return keyword is broken
* Bug [7812](https://dev.icinga.org/issues/7812 "Bug 7812"): Validate array subscripts
* Bug [7814](https://dev.icinga.org/issues/7814 "Bug 7814"): Set expression should check whether LHS is a null pointer
* Bug [7815](https://dev.icinga.org/issues/7815 "Bug 7815"): - operator doesn't work in expressions
* Bug [7826](https://dev.icinga.org/issues/7826 "Bug 7826"): Compiler warnings
* Bug [7830](https://dev.icinga.org/issues/7830 "Bug 7830"): - shouldn't be allowed in identifiers
* Bug [7871](https://dev.icinga.org/issues/7871 "Bug 7871"): Missing persistent_comment, notify_contact columns for acknowledgement table
* Bug [7894](https://dev.icinga.org/issues/7894 "Bug 7894"): Fix warnings when using CMake 3.1.0
* Bug [7895](https://dev.icinga.org/issues/7895 "Bug 7895"): Serialize() fails to serialize objects which don't have a registered type
* Bug [7995](https://dev.icinga.org/issues/7995 "Bug 7995"): Windows Agent: Missing directory "zones" in setup
* Bug [8018](https://dev.icinga.org/issues/8018 "Bug 8018"): Value("").IsEmpty() should return true
* Bug [8029](https://dev.icinga.org/issues/8029 "Bug 8029"): operator precedence for % and > is incorrect
* Bug [8041](https://dev.icinga.org/issues/8041 "Bug 8041"): len() overflows
* Bug [8061](https://dev.icinga.org/issues/8061 "Bug 8061"): Confusing error message for import
* Bug [8067](https://dev.icinga.org/issues/8067 "Bug 8067"): Require at least one user for notification objects (user or as member of user_groups)
* Bug [8076](https://dev.icinga.org/issues/8076 "Bug 8076"): icinga 2 Config Error needs to be more verbose
* Bug [8081](https://dev.icinga.org/issues/8081 "Bug 8081"): Location info for strings is incorrect
* Bug [8100](https://dev.icinga.org/issues/8100 "Bug 8100"): POSTGRES IDO: invalid syntax for integer: "true" while trying to update table icinga_hoststatus
* Bug [8111](https://dev.icinga.org/issues/8111 "Bug 8111"): User::ValidateFilters isn't being used
* Bug [8117](https://dev.icinga.org/issues/8117 "Bug 8117"): Agent checks fail when there's already a host with the same name
* Bug [8122](https://dev.icinga.org/issues/8122 "Bug 8122"): Config file passing validation causes segfault
* Bug [8132](https://dev.icinga.org/issues/8132 "Bug 8132"): Debug info for indexer is incorrect
* Bug [8136](https://dev.icinga.org/issues/8136 "Bug 8136"): Icinga crashes when config file name is invalid
* Bug [8164](https://dev.icinga.org/issues/8164 "Bug 8164"): escaped backslash in string literals
* Bug [8166](https://dev.icinga.org/issues/8166 "Bug 8166"): parsing include_recursive
* Bug [8173](https://dev.icinga.org/issues/8173 "Bug 8173"): Segfault on icinga::String::operator= when compiling configuration
* Bug [8175](https://dev.icinga.org/issues/8175 "Bug 8175"): Compiler warnings
* Bug [8179](https://dev.icinga.org/issues/8179 "Bug 8179"): Exception on missing config files
* Bug [8184](https://dev.icinga.org/issues/8184 "Bug 8184"): group assign fails with bad lexical cast when evaluating rules
* Bug [8185](https://dev.icinga.org/issues/8185 "Bug 8185"): Argument auto-completion doesn't work for short options
* Bug [8211](https://dev.icinga.org/issues/8211 "Bug 8211"): icinga2 node update should not write config for blacklisted zones/host
* Bug [8230](https://dev.icinga.org/issues/8230 "Bug 8230"): Lexer term for T_ANGLE_STRING is too aggressive
* Bug [8249](https://dev.icinga.org/issues/8249 "Bug 8249"): Problems using command_endpoint inside HA zone
* Bug [8257](https://dev.icinga.org/issues/8257 "Bug 8257"): Report missing command objects on remote agent
* Bug [8260](https://dev.icinga.org/issues/8260 "Bug 8260"): icinga2 node wizard: Create backups of certificates
* Bug [8289](https://dev.icinga.org/issues/8289 "Bug 8289"): Livestatus operator =~ is not case-insensitive
* Bug [8294](https://dev.icinga.org/issues/8294 "Bug 8294"): Running icinga2 command as non privilged user raises error
* Bug [8298](https://dev.icinga.org/issues/8298 "Bug 8298"): notify flag is ignored in ACKNOWLEDGE_*_PROBLEM commands
* Bug [8300](https://dev.icinga.org/issues/8300 "Bug 8300"): ApiListener::ReplayLog shouldn't hold mutex lock during call to Socket::Poll
* Bug [8307](https://dev.icinga.org/issues/8307 "Bug 8307"): PidPath, VarsPath, ObjectsPath and StatePath no longer read from init.conf
* Bug [8309](https://dev.icinga.org/issues/8309 "Bug 8309"): Crash in ScheduledDowntime::CreateNextDowntime
* Bug [8313](https://dev.icinga.org/issues/8313 "Bug 8313"): Incorrectly formatted timestamp in .timestamp file
* Bug [8318](https://dev.icinga.org/issues/8318 "Bug 8318"): Remote Clients: Add manual setup cli commands
* Bug [8323](https://dev.icinga.org/issues/8323 "Bug 8323"): Apply rule '' for host does not match anywhere!
* Bug [8333](https://dev.icinga.org/issues/8333 "Bug 8333"): Icinga2 master doesn't change check-status when "accept_commands = true" is not set at client node
* Bug [8372](https://dev.icinga.org/issues/8372 "Bug 8372"): Stacktrace on Endpoint not belonging to a zone or multiple zones
* Bug [8383](https://dev.icinga.org/issues/8383 "Bug 8383"): last_hard_state missing in StatusDataWriter
* Bug [8387](https://dev.icinga.org/issues/8387 "Bug 8387"): StatusDataWriter: Wrong host notification filters (broken fix in #8192)
* Bug [8388](https://dev.icinga.org/issues/8388 "Bug 8388"): Config sync authoritative file never created
* Bug [8389](https://dev.icinga.org/issues/8389 "Bug 8389"): Added downtimes must be triggered immediately if checkable is Not-OK
* Bug [8390](https://dev.icinga.org/issues/8390 "Bug 8390"): Agent writes CR CR LF in synchronized config files
* Bug [8397](https://dev.icinga.org/issues/8397 "Bug 8397"): Icinga2 config reset after package update (centos6.6)
* Bug [8425](https://dev.icinga.org/issues/8425 "Bug 8425"): DB IDO: Duplicate entry icinga_scheduleddowntime
* Bug [8433](https://dev.icinga.org/issues/8433 "Bug 8433"): Make the arguments for the stats functions const-ref
* Bug [8434](https://dev.icinga.org/issues/8434 "Bug 8434"): Build fails on OpenBSD
* Bug [8436](https://dev.icinga.org/issues/8436 "Bug 8436"): Indicate that Icinga2 is shutting down in case of a fatal error
* Bug [8438](https://dev.icinga.org/issues/8438 "Bug 8438"): DB IDO {host,service}checks command_line value is "Object of type 'icinga::Array'"
* Bug [8444](https://dev.icinga.org/issues/8444 "Bug 8444"): Don't attempt to restore program state from non-existing state file
* Bug [8452](https://dev.icinga.org/issues/8452 "Bug 8452"): Livestatus query on commands table with custom vars fails
* Bug [8461](https://dev.icinga.org/issues/8461 "Bug 8461"): Don't request heartbeat messages until after we've synced the log
* Bug [8473](https://dev.icinga.org/issues/8473 "Bug 8473"): Exception in WorkQueue::StatusTimerHandler
* Bug [8488](https://dev.icinga.org/issues/8488 "Bug 8488"): Figure out why 'node update-config' becomes slow over time
* Bug [8493](https://dev.icinga.org/issues/8493 "Bug 8493"): Misleading ApiListener connection log messages on a master (Endpoint vs Zone)
* Bug [8496](https://dev.icinga.org/issues/8496 "Bug 8496"): Icinga doesn't update long_output in DB
* Bug [8511](https://dev.icinga.org/issues/8511 "Bug 8511"): Deadlock with DB IDO dump and forcing a scheduled check
* Bug [8517](https://dev.icinga.org/issues/8517 "Bug 8517"): Config parser fails non-deterministic on Notification missing Checkable
* Bug [8519](https://dev.icinga.org/issues/8519 "Bug 8519"): apply-for incorrectly converts loop var to string
* Bug [8529](https://dev.icinga.org/issues/8529 "Bug 8529"): livestatus limit header not working
* Bug [8535](https://dev.icinga.org/issues/8535 "Bug 8535"): Crash in ApiEvents::RepositoryTimerHandler
* Bug [8536](https://dev.icinga.org/issues/8536 "Bug 8536"): Valgrind warning for ExternalCommandListener::CommandPipeThread
* Bug [8537](https://dev.icinga.org/issues/8537 "Bug 8537"): Crash in DbObject::SendStatusUpdate
* Bug [8544](https://dev.icinga.org/issues/8544 "Bug 8544"): Hosts: process_performance_data = 0 in database even though enable_perfdata = 1 in config
* Bug [8555](https://dev.icinga.org/issues/8555 "Bug 8555"): Don't accept config updates for zones for which we have an authoritative copy of the config
* Bug [8559](https://dev.icinga.org/issues/8559 "Bug 8559"): check_memory tool shows incorrect memory size on windows
* Bug [8593](https://dev.icinga.org/issues/8593 "Bug 8593"): Memory leak in Expression::GetReference
* Bug [8594](https://dev.icinga.org/issues/8594 "Bug 8594"): Improve Livestatus query performance
* Bug [8596](https://dev.icinga.org/issues/8596 "Bug 8596"): Dependency: Validate *_{host,service}_name objects on their existance
* Bug [8604](https://dev.icinga.org/issues/8604 "Bug 8604"): Attribute hints don't work for nested attributes
* Bug [8627](https://dev.icinga.org/issues/8627 "Bug 8627"): Icinga2 shuts down when service is reloaded
* Bug [8638](https://dev.icinga.org/issues/8638 "Bug 8638"): Fix a typo in documentation

