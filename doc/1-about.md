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

## <a id="development"></a> Icinga 2 Development

You can follow Icinga 2's development closely by checking
out these resources:

* [Development Bug Tracker](https://dev.icinga.org/projects/i2): [How to report a bug?](http://www.icinga.org/faq/how-to-report-a-bug/)
* Git Repositories: [main mirror on icinga.org](https://git.icinga.org/?p=icinga2.git;a=summary) [release mirror at github.com](https://github.com/Icinga/icinga2)
* [Git Checkins Mailinglist](https://lists.icinga.org/mailman/listinfo/icinga-checkins)
* [Development](https://lists.icinga.org/mailman/listinfo/icinga-devel) and [Users](https://lists.icinga.org/mailman/listinfo/icinga-users) Mailinglists
* [#icinga-devel on irc.freenode.net](http://webchat.freenode.net/?channels=icinga-devel) including a Git Commit Bot

For general support questions, please refer to the [community support channels](https://support.icinga.org).

## <a id="demo-vm"></a> Demo VM

Icinga 2 is available as [Vagrant Demo VM](#vagrant).

## <a id="whats-new"></a> What's new

### What's New in Version 2.0.2

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

#### Changes

* DB IDO schema upgrade required (new schema version: 1.11.6)

#### Changes

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

* High Performance

Multithreaded and scalable for small embedded systems as well as large scale environments.
Running checks every second is no longer a problem and enables real-time monitoring capabilities.
Checks, notifications and event handlers [do not block Icinga 2](#differences-1x-2-async-event-execution)
in its operation. Same goes for performance data writers and the external command pipe, or any
file writers on disk (`statusdata`).
Unlike Icinga 1.x the [daemon reload](#differences-1x-2-real-reload) happens asynchronously.
A child daemon validates the new configuration, the parent process is still doing checks, replicating cluster events, triggering alert notifications, etc. If the configuration validation is ok, all remaining events are synchronized and the child process continues as normal.
The DB IDO configuration dump and status/historical event updates also runs asynchronously in a queue not blocking the core anymore. The configuration validation itself runs in paralell allowing fast verification checks.
That way you are not blind (anymore) during a configuration reload and benefit from a real scalable architecture.


* Modular & flexible [features](#features)

Enable only the features you require. Want to use Icinga Web 2 with DB IDO but no status data?
No problem! Just enable ido-mysql and disable statusdata. Another example: Graphite should be enabled
on a dedicated cluster node. Enable it over there and point it to the carbon cache socket.

* Native support for the [Livestatus protocol](#setting-up-livestatus)

In Icinga2, the 'Livestatus' protocol is available for use as either a UNIX, or TCP socket.

* Native support for [Graphite](#graphite-carbon-cache-writer)

Icinga 2 still supports writing performance data files for graphing addons, but also adds the
capability of writing performance data directly into a Graphite TCP socket simplifying realtime
monitoring graphs.

* Dynamic configuration language

Simple [apply](#using-apply) and [assign](#group-assign) rules for creating configuration object
relationships based on patterns. Supported with [duration literals](#duration-literals) for interval
attributes, [expression operators](#expression-operators), [function calls](#function-calls) for
pattern and regex matching and (global) [constants](#constants).
Sample configuration for common plugins is shipped with Icinga 2 as part of the [Icinga Template Library](#itl).

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

Detailed [migration hints](#manual-config-migration-hints) explain migration the Icinga 1.x
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
