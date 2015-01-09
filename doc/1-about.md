# <a id="about-icinga2"></a> About Icinga 2

## <a id="what-is-icinga2"></a> What is Icinga 2?

Icinga 2 is an open source monitoring system which checks the availability of your
network resources, notifies users of outages, and generates performance data for reporting.

Scalable and extensible, Icinga 2 can monitor large, complex environments across
multiple locations.

## <a id="licensing"></a> Licensing

Icinga 2 and the Icinga 2 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
LICENSE file included in the source package.

## <a id="support"></a> Support

Support for Icinga 2 is available in a number of ways. Please have a look at
the support overview page at https://support.icinga.org.

## <a id="contribute"></a> Contribute

There are many ways to contribute to Icinga - whether it be sending patches, testing,
reporting bugs, or reviewing and updating the documentation. Every contribution
is appreciated!

Please get in touch with the Icinga team at https://www.icinga.org/community/.

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
* When reporting a bug, please include the details described in the [Troubleshooting](#troubleshooting-information-required) chapter (version, configs, logs, etc).

## <a id="demo-vm"></a> Demo VM

Icinga 2 is available as [Vagrant Demo VM](#vagrant).

## <a id="whats-new"></a> What's new

### What's New in Version 2.2.3

#### Changes

* Bugfixes

#### Issues

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

#### Issues

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

#### Issues

* Feature #6709: Support for arrays in macros
* Feature #7463: Update spec file to use yajl-devel
* Feature #7739: The classicui Apache conf doesn't support Apache 2.4
* Feature #7747: Increase default timeout for NRPE checks
* Feature #7867: Document how arrays in macros work

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

#### Issues

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

### Archive

Please check the `ChangeLog` file.

## <a id="icinga2-in-a-nutshell"></a> Icinga 2 in a Nutshell

* Use [Packages](#getting-started)

Look for available packages on http://packages.icinga.org or ask your distribution's maintainer.
Compiling from source is not recommended.

* Real Distributed Architecture

[Cluster](#distributed-monitoring-high-availability) model for distributed setups, load balancing
and High-Availability installations (or a combination of them). On-demand configuration
synchronisation between zones is available, but not mandatory (for example when config management
tools such as Puppet are used). Secured by SSL x509 certificates, supporting IPv4 and IPv6.
High Availability for DB IDO: Only active on the current zone master, failover happens automatically.

* Monitoring Remote Clients

Built on proven [cluster](#distributed-monitoring-high-availability) stack,
[Icinga 2 clients](#icinga2-remote-client-monitoring) can be installed acting as remote satellite or
agent. Secured communication by SSL x509 certificates, install them with [cli commands](#cli-commands),
and configure them either locally with discovery on the master, or use them for executing checks and
event handlers remotely.


* High Performance

Multithreaded and scalable for small embedded systems as well as large scale environments.
Running checks every second is no longer a problem and enables real-time monitoring capabilities.
Checks, notifications and event handlers [do not block Icinga 2](#differences-1x-2-async-event-execution)
in its operation. Same goes for performance data writers and the external command pipe, or any
file writers on disk (`statusdata`).
Unlike Icinga 1.x the [daemon reload](#differences-1x-2-real-reload) happens asynchronously.
A child daemon validates the new configuration, the parent process is still doing checks, replicating cluster events, triggering alert notifications, etc. If the configuration validation is ok, all remaining events are synchronized and the child process continues as normal.
The DB IDO configuration dump and status/historical event updates also runs asynchronously in a queue not blocking the core anymore. The configuration validation itself runs in parallel allowing fast verification checks.
That way you are not blind (anymore) during a configuration reload and benefit from a real scalable architecture.

* Integrated CLI with Bash Auto-Completion

Enable only the [features](#cli-command-feature) which are currently disabled,
[list objects](#cli-command-object) generated from [apply rules](#using-apply) or
[generate SSL x509 certificates](#cli-command-pki) for remote clients or cluster setup.
Start/stop the Icinga 2 [daemon](#cli-command-daemon) or validate your configuration,
[manage and install](#cli-command-node) remote clients and service discovery helped
with black- and whitelists.

* Modular & flexible [features](#features)

Enable only the features you require. Want to use Icinga Web 2 with DB IDO but no status data?
No problem! Just enable ido-mysql and disable statusdata. Another example: Graphite should be enabled
on a dedicated cluster node. Enable it over there and point it to the carbon cache socket.

Combine Icinga 2 Core with web user interfaces: Use [Icinga Web 2](#setting-up-icingaweb2), but also
Web 1.x or Classic UI or your own preferred addon.

* Native support for the [Livestatus protocol](#setting-up-livestatus)

In Icinga2, the 'Livestatus' protocol is available for use as either a UNIX, or TCP socket.

* Native support for [Graphite](#graphite-carbon-cache-writer)

Icinga 2 still supports writing performance data files for graphing addons, but also adds the
capability of writing performance data directly into a Graphite TCP socket simplifying realtime
monitoring graphs.

* Native support for writing log events to [GELF](#gelf-writer) receivers (graylog2, Logstash)

Icinga 2 will write all check result, state change and notification event logs into a defined
[GELF](#gelfwriter) input receiver. Natively provided by [graylog2](http://www.graylog2.org),
and as additional input type provided by [Logstash](http://logstash.net).

* Dynamic configuration language

Simple [apply](#using-apply) and [assign](#group-assign) rules for creating configuration object
relationships based on patterns. More advanced features for dynamic object generation using
[apply for rules](#using-apply-for) helped with arrays and dictionaries for
[custom attributes](#custom-attributes-apply).
Supported with [duration literals](#duration-literals) for interval
attributes, [expression operators](#expression-operators), [function calls](#function-calls) for
pattern and regex matching and (global) [constants](#constants).
[Check command configuration](#plugin-check-commands) for common plugins is shipped with Icinga 2 as part of the [Icinga Template Library](#itl).

* Revamped Commands

One command to rule them all - supporting optional and conditional [command arguments](#command-arguments).
[Environment variables](#command-environment-variables) exported on-demand populated with
runtime evaluated macros.
Three types of commands used for different actions: checks, notifications and events.
Check timeout for commands instead of a global option. Commands also have custom attributes allowing
you to specify default values.
There is no plugin output or performance data length restriction anymore compared to Icinga 1.x.

* Custom Runtime Macros

Access [custom attributes](#custom-attributes) with their short name, for example $mysql_user$,
or any object attribute, for example $host.notes$. Additional macros with runtime and statistic
information are available as well. Use these [runtime macros](#runtime-custom-attributes) in
the command line, environment variables and custom attribute assignments.

* Notifications simplified

Multiple [notifications](#notifications) for one host or service with existing users
and notification commands. No more duplicated contacts for different notification types.
Telling notification filters by state and type, even more fine-grained than Icinga 1.x.
[Escalation notifications](#notification-escalations) and [delayed notifications](#first-notification-delay)
are just notifications with an additional begin and/or end time attribute.

* Dependencies between Hosts and Services

Classic [dependencies](#dependencies) between host and parent hosts, and services and parent services work the
same way as "mixed" dependencies from a service to a parent host and vice versa. Host checks
depending on an upstream link port (as service) are not a problem anymore.
No more additional parents settings - host dependencies already define the host parent relationship
required for network reachability calculations.
Set parent host/services based on [host/service custom attributes](#dependencies-apply-custom-attributes)
generated from your cloud inventory or CMDB and make your dependency rules simple and short.

* [Recurring Downtimes](#recurring-downtimes)

Forget using cronjobs to set up recurring downtime - you can configure them as Icinga 2 configuration
objects and specify their active time window.

* Embedded Health Checks

No more external statistic tool but an [instance](#itl-icinga) and [cluster](#itl-cluster) health
check providing direct statistics as performance data for your graphing addon, for example Graphite.

* Compatibility with Icinga 1.x

All known interfaces are optionally available: [status files](#status-data), [logs](#compat-logging),
[DB IDO](#configuring-ido) MySQL/PostgreSQL, [performance data](#performance-data),
[external command pipe](#external-commands) and for migration reasons a
[checkresult file reader](#check-result-files) too.
All [Monitoring Plugins](#setting-up-check-plugins) can be integrated into Icinga 2 with
newly created check command configuration if not already provided.
[Configuration migration](#configuration-migration) is possible through an external migration tool.

Detailed [migration hints](#manual-config-migration-hints) explain migration of the Icinga 1.x
configuration objects into the native Icinga 2 configuration schema.
Additional information on the differences is documented in the [migration](#differences-1x-2) chapter.

* Configuration Syntax Highlighting

Icinga 2 ships [syntax highlighting](#configuration-syntax-highlighting) for `vim` and `nano` to help
edit your configuration.

* Puppet modules, Chef Cookbooks, Ansible Playbooks, Salt Formulas, etc

This is a constant work-in-progress. For details checkout https://dev.icinga.org/projects/icinga-tools
If you want to contribute to these projects, do not hesitate to contact us at https://support.icinga.org

* [Vagrant Demo VM](#vagrant)

Used for demo cases and development tests. Get Icinga 2 running within minutes and spread the #monitoringlove
to your friends and colleagues.
