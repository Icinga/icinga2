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
the [support overview page](https://support.icinga.org).

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
* When reporting a bug, please include the details described in the [Troubleshooting](7-troubleshooting.md#troubleshooting-information-required) chapter (version, configs, logs, etc).

## <a id="whats-new"></a> What's new

### What's New in Version 2.3

#### Changes

TODO

#### Issues

TODO

## <a id="icinga2-in-a-nutshell"></a> Icinga 2 in a Nutshell

* Use [Packages](2-getting-started.md#getting-started)

Look for available packages on http://packages.icinga.org or ask your distribution's maintainer.
Compiling from source is not recommended.

* Real Distributed Architecture

[Cluster](4-monitoring-remote-systems.md#distributed-monitoring-high-availability) model for distributed setups, load balancing
and High-Availability installations (or a combination of them). On-demand configuration
synchronisation between zones is available, but not mandatory (for example when config management
tools such as Puppet are used). Secured by TLS with certificates, supporting IPv4 and IPv6.
High Availability for DB IDO: Only active on the current zone master, failover happens automatically.

* Monitoring Remote Clients

Built on proven [cluster](4-monitoring-remote-systems.md#distributed-monitoring-high-availability) stack,
[Icinga 2 clients](4-monitoring-remote-systems.md#icinga2-remote-client-monitoring) can be installed acting as remote satellite or
agent. Secured communication by TLS with certificates, install them with [CLI commands](5-cli-commands.md#cli-commands),
and configure them either locally with discovery on the master, or use them for executing checks and
event handlers remotely.


* High Performance

Multithreaded and scalable for small embedded systems as well as large scale environments.
Running checks every second is no longer a problem and enables real-time monitoring capabilities.
Checks, notifications and event handlers [do not block Icinga 2](8-migrating-from-icinga-1x.md#differences-1x-2-async-event-execution)
in its operation. Same goes for performance data writers and the external command pipe, or any
file writers on disk (`statusdata`).
Unlike Icinga 1.x the [daemon reload](8-migrating-from-icinga-1x.md#differences-1x-2-real-reload) happens asynchronously.
A child daemon validates the new configuration, the parent process is still doing checks, replicating cluster events, triggering alert notifications, etc. If the configuration validation is ok, all remaining events are synchronized and the child process continues as normal.
The DB IDO configuration dump and status/historical event updates also runs asynchronously in a queue not blocking the core anymore. The configuration validation itself runs in parallel allowing fast verification checks.
That way you are not blind (anymore) during a configuration reload and benefit from a real scalable architecture.

* Integrated CLI with Bash Auto-Completion

Enable only the [features](5-cli-commands.md#cli-command-feature) which are currently disabled,
[list objects](5-cli-commands.md#cli-command-object) generated from [apply rules](3-monitoring-basics.md#using-apply) or
[generate X.509 certificates](5-cli-commands.md#cli-command-pki) for remote clients or cluster setup.
Start/stop the Icinga 2 [daemon](5-cli-commands.md#cli-command-daemon) or validate your configuration,
[manage and install](5-cli-commands.md#cli-command-node) remote clients and service discovery helped
with black- and whitelists.

* Modular & flexible [features](5-cli-commands.md#features)

Enable only the features you require. Want to use Icinga Web 2 with DB IDO but no status data?
No problem! Just enable ido-mysql and disable statusdata. Another example: Graphite should be enabled
on a dedicated cluster node. Enable it over there and point it to the carbon cache socket.

Combine Icinga 2 Core with web user interfaces: Use [Icinga Web 2](2-getting-started.md#setting-up-icingaweb2), but also
Web 1.x or Classic UI or your own preferred addon.

* Native support for the [Livestatus protocol](2-getting-started.md#setting-up-livestatus)

In Icinga2, the 'Livestatus' protocol is available for use as either a UNIX, or TCP socket.

* Native support for [Graphite](3-monitoring-basics.md#graphite-carbon-cache-writer)

Icinga 2 still supports writing performance data files for graphing addons, but also adds the
capability of writing performance data directly into a Graphite TCP socket simplifying realtime
monitoring graphs.

* Native support for writing log events to [GELF](#gelf-writer) receivers (graylog2, Logstash)

Icinga 2 will write all check result, state change and notification event logs into a defined
[GELF](3-monitoring-basics.md#gelfwriter) input receiver. Natively provided by [graylog2](http://www.graylog2.org),
and as additional input type provided by [Logstash](http://logstash.net).

* Dynamic configuration language

Simple [apply](3-monitoring-basics.md#using-apply) and [assign](9-language-reference.md#group-assign) rules for creating configuration object
relationships based on patterns. More advanced features for dynamic object generation using
[apply for rules](3-monitoring-basics.md#using-apply-for) helped with arrays and dictionaries for
[custom attributes](3-monitoring-basics.md#custom-attributes-apply).
Supported with [duration literals](9-language-reference.md#duration-literals) for interval
attributes, [expression operators](9-language-reference.md#expression-operators), [function calls](9-language-reference.md#function-calls) for
pattern and regex matching and (global) [constants](9-language-reference.md#constants).
[Check command configuration](#plugin-check-commands) for common plugins is shipped with Icinga 2 as part of the [Icinga Template Library](#itl).

* Revamped Commands

One command to rule them all - supporting optional and conditional [command arguments](3-monitoring-basics.md#command-arguments).
[Environment variables](3-monitoring-basics.md#command-environment-variables) exported on-demand populated with
runtime evaluated macros.
Three types of commands used for different actions: checks, notifications and events.
Check timeout for commands instead of a global option. Commands also have custom attributes allowing
you to specify default values.
There is no plugin output or performance data length restriction anymore compared to Icinga 1.x.

* Custom Runtime Macros

Access [custom attributes](3-monitoring-basics.md#custom-attributes) with their short name, for example $mysql_user$,
or any object attribute, for example $host.notes$. Additional macros with runtime and statistic
information are available as well. Use these [runtime macros](3-monitoring-basics.md#runtime-custom-attributes) in
the command line, environment variables and custom attribute assignments.

* Notifications simplified

Multiple [notifications](3-monitoring-basics.md#notifications) for one host or service with existing users
and notification commands. No more duplicated contacts for different notification types.
Telling notification filters by state and type, even more fine-grained than Icinga 1.x.
[Escalation notifications](#notification-escalations) and [delayed notifications](#first-notification-delay)
are just notifications with an additional begin and/or end time attribute.

* Dependencies between Hosts and Services

Classic [dependencies](3-monitoring-basics.md#dependencies) between host and parent hosts, and services and parent services work the
same way as "mixed" dependencies from a service to a parent host and vice versa. Host checks
depending on an upstream link port (as service) are not a problem anymore.
No more additional parents settings - host dependencies already define the host parent relationship
required for network reachability calculations.
Set parent host/services based on [host/service custom attributes](3-monitoring-basics.md#dependencies-apply-custom-attributes)
generated from your cloud inventory or CMDB and make your dependency rules simple and short.

* [Recurring Downtimes](3-monitoring-basics.md#recurring-downtimes)

Forget using cronjobs to set up recurring downtime - you can configure them as Icinga 2 configuration
objects and specify their active time window.

* Embedded Health Checks

No more external statistic tool but an [instance](#itl-icinga) and [cluster](#itl-cluster) health
check providing direct statistics as performance data for your graphing addon, for example Graphite.

* Compatibility with Icinga 1.x

All known interfaces are optionally available: [status files](3-monitoring-basics.md#status-data), [logs](3-monitoring-basics.md#compat-logging),
[DB IDO](#configuring-ido) MySQL/PostgreSQL, [performance data](3-monitoring-basics.md#performance-data),
[external command pipe](3-monitoring-basics.md#external-commands) and for migration reasons a
[checkresult file reader](3-monitoring-basics.md#check-result-files) too.
All [Monitoring Plugins](2-getting-started.md#setting-up-check-plugins) can be integrated into Icinga 2 with
newly created check command configuration if not already provided.
[Configuration migration](8-migrating-from-icinga-1x.md#configuration-migration) is possible through an external migration tool.

Detailed [migration hints](8-migrating-from-icinga-1x.md#manual-config-migration-hints) explain migration of the Icinga 1.x
configuration objects into the native Icinga 2 configuration schema.
Additional information on the differences is documented in the [migration](8-migrating-from-icinga-1x.md#differences-1x-2) chapter.

* Configuration Syntax Highlighting

Icinga 2 ships [syntax highlighting](2-getting-started.md#configuration-syntax-highlighting) for `vim` and `nano` to help
edit your configuration.

* Puppet modules, Chef Cookbooks, Ansible Playbooks, Salt Formulas, etc

This is a constant work-in-progress. For details checkout https://dev.icinga.org/projects/icinga-tools
If you want to contribute to these projects, do not hesitate to contact us at https://support.icinga.org

