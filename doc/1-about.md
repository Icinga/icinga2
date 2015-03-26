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

* [Register](https://exchange.icinga.org/authentication/register) an Icinga account.
* Create a new issue at the [Icinga 2 Development Tracker](https://dev.icinga.org/projects/i2).
* When reporting a bug, please include the details described in the [Troubleshooting](16-troubleshooting.md#troubleshooting-information-required) chapter (version, configs, logs, etc).

## <a id="whats-new"></a> What's New

### What's New in Version 2.3.3

#### Changes

* New function: parse_performance_data
* Include more details in --version
* Improve documentation
* Bugfixes

#### Issues

* Feature 8685: Show state/type filter names in notice/debug log
* Feature 8686: Update documentation for "apply for" rules
* Feature 8693: New function: parse_performance_data
* Feature 8740: Add "access objects at runtime" examples to advanced section
* Feature 8761: Include more details in --version
* Feature 8816: Add "random" CheckCommand for test and demo purposes
* Feature 8827: Move release info in INSTALL.md into a separate file

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

#### Issues

* Bug 8721: Log message for cli commands breaks the init script

### What's New in Version 2.3.1

#### Changes

* Bugfixes

Please note that this version fixes the default thresholds for the disk check which were inadvertently broken in 2.3.0; if you're using percent-based custom thresholds you will need to add the '%' sign to your custom attributes

#### Issues

* Feature 8659: Implement String#contains

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

#### Issues

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
