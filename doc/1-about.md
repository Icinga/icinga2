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

### What's New in Version 2.2.0

#### Changes

* DB IDO schema update to version `1.12.0`
    * schema files in `lib/db_ido_{mysql,pgsql}/schema` (source)
    * Table `programstatus`: New column `program_version`
    * Table `customvariables` and `customvariablestatus`: New column `is_json` (required for custom attribute array/dictionary support)
* New features
    * [GelfWriter](#gelfwriter): Logging check results, state changes, notifications to GELF (graylog2, logstash) #7619
* New CLI commands #7245
    * `icinga2 feature {enable,disable}` replaces `icinga2-{enable,disable}-feature` script  #7250
    * `icinga2 object list` replaces `icinga2-list-objects` script  #7251
    * `icinga2 pki` replaces` icinga2-build-{ca,key}` scripts  #7247
    * `icinga2 repository` manages `/etc/icinga2/repository.d` which must be included in `icinga2.conf` #7255
    * `icinga2 node` cli command provides node (master, satellite, agent) setup (wizard) and management functionality #7248
    * bash auto-completion & terminal colors #7396
* Configuration
    * Former `localhost` example host is now defined in [hosts.conf](#hosts-conf) #7594
    * All example services moved into advanced apply rules in [services.conf](#services-conf)
    * Updated downtimes configuration example in [downtimes.conf](#downtimes-conf) #7472
    * Updated notification apply example in [notifications.conf](#notifications-conf) #7594
    * Support for object attribute 'zone' #7400
    * Support setting object variables in apply rules #7479
    * Support arrays and dictionaries in custom attributes #6544 #7560
    * Add [apply for rules](#using-apply-for) for advanced dynamic object generation #7561
* Cluster
    * Add CSR Auto-Signing support using generated ticket #7244
* Perfdata
    * PerfdataWriter: Don't change perfdata, pass through from plugins #7268
    * GraphiteWriter: Add warn/crit/min/max perfdata and downtime_depth stats values #7366 #6946
* Packages
    * `python-icinga2` package dropped in favor of integrated cli commands #7245
    * Windows Installer for the agent parts #7243

> **Note**
>
>  Please remove `conf.d/hosts/localhost*` after verifying your updated configuration!

#### Issues



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

Built on proven [cluster](#distributed-monitoring-high-availability) stack, [Icinga 2 clients](#icinga2-remote-client-monitoring)
can be installed acting as remote satellite or agent. Secured communication by SSL x509 certificates,
install them with [cli commands](#cli-commands), and configure them either locally with
discovery on the master, or use them for executing checks and event handlers remotely.


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
