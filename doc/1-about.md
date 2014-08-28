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

#### Issues

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
