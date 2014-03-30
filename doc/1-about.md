# <a id="about-icinga2"></a> About Icinga 2

## <a id="what-is-icinga2"></a> What is Icinga 2?

Icinga 2 is an open source monitoring system which checks the availability of your
network resources, notifies users of outages and generates performance data for reporting.

Scalable and extensible, Icinga 2 can monitor complex, large environments across
multiple locations.

## <a id="licensing"></a> Licensing

Icinga 2 and the Icinga 2 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
LICENSE file included in the package.

## <a id="support"></a> Support

Support for Icinga 2 is available in a number of ways. Please have a look at
the support overview page at [https://support.icinga.org].

## <a id="contribute"></a> Contribute

There are many possiblities to contribute to Icinga - be it sending patches, testing and
reporting bugs, reviewing and updating the documentation, ... every contribution
is appreciated! :-)

Please get in touch with the Icinga team at [https://www.icinga.org/ecosystem/]

## <a id="whats-new"></a> What's new

### What's New in Version 0.0.8

* Add [Dependency](#objecttype-dependency) object for advanced host/service dependency definition
* Add optional [IcingaNodeName](#global-constants) for cluster feature
* Populate check_source attribute with the checker's node name
* [Cluster](#objecttype-endpoint) supports recursive config includes
* Add [Cluster health check](#cluster-health-check)
* Add more performance data to the [Icinga health check](#itl-icinga)
* Add [IcingaStatusWriter](#objecttype-icingastatuswriter) feature writing a status json file
* Smoother pending service checking during startup
* Reduce virtual memory usage
* Stack traces include file names and line numbers
* Treat script variables as constants preventing override
* Fix pending services are being checked with the retry interval
* DB IDO: Fix deleted objects are not marked as is_active=0
* DB IDO: additional fields for cluster/checker nodes

#### Changes
* {host,service}_dependencies attributes have been changed to [Dependency](#objecttype-dependency)
objects supporting new attributes: `disable_checks`, `disable_notifications`, `state_filter`,
`period`. For better readability, there is `parent_service` and `child_service` for example.

> **Note**
>
> Update your existing configuration!

* DB IDO: Schema updates for 0.0.8: [MySQL](#upgrading-mysql-db) [PostgreSQL](#upgrading-postgresql-db)


### What's New in Version 0.0.7

* DB IDO performance improvements on startup
* Fix notification_id handling in DB IDO
* More automated tests (based on the Vagrant VM)
* New documentation chapters

### What's New in Version 0.0.6

* Scheduled Downtimes as configuration object (also known as "Recurring Downtimes").
* Log command arguments
* Performance improvements for the config compiler
* Config validation provides stats at the end
* icinga2-enable-feature lists already enabled features
* Add support for latency statistics to IcingaCheckTask
* Implement support for using custom attributes as macros
* StatusDataWriter update interval as config attribute
* Improve performance with fetching data for status.dat/objects.cache, DB IDO and Livestatus
* Livestatus History Table performance improvements

#### Changes
* Generated object names (host with services array) use an exclamation mark instead of a colon
as separator. State file objects with downtimes, comments, etc are invalid (unknown) for that
reason.
* Script variables are set using 'var' and 'const' instead of the previous 'set' identifier
* ITL constants are now embedded in libicinga
* Removed the ConsoleLogger object and keep the default console log enabled until we daemonize

### What's New in Version 0.0.5

* Cluster: Implement support for CRLs
* Implement modified attributes
* Log messages providing more context
* Default log is a file (rather than syslog)
* Improve latency after start-up
* NSCA-ng support for the Vagrant demo VM
* Configuration: Recursively include configuration files matching a certain pattern
* IDO: Improve performance
* Migration: Add fallback for objects.cache instead of cfg_{dir,file}
* Lots of bugfixes and performance improvements
* Package fixes (Note: GPG key of packages.icinga.org has been updated)

### What's New in Version 0.0.4

* IDO: PostgreSQL support
* IDO: implemented options to filter which kind of events are written to the database
* Livestatus: implemented support for the log and statehist tables
* Livestatus: implemented regex filters (~ and ~~)
* Replaced autotools-based build system with cmake
* Lots of bug fixes and performance improvements

### What's New in Version 0.0.3

* `StatusDataWriter` and `ExternalCommandListener` (former `Compat`) and `CompatLogger`
(former CompatLog) for status.dat/objects.cache/icinga2.cmd/icinga.log for Icinga 1.x Classic UI support
* `IdoMysqlConnection` and `ExternalCommandListener` for Icinga 1.x Web
* `IdoMysqlConnection` for Icinga 1.x Reporting, NagVis
* `LivestatusListener` for addons using the livestatus interface (history tables tbd)
* `PerfDataWriter` for graphing addons such as PNP/inGraph/graphite (can be loaded multiple times!)
* `GraphiteWriter` for sending metrics directly to graphite carbon sockets
* `CheckResultReader` to collect Icinga 1.x slave checkresults (migrate your distributed setup step-by-step)
* `ClusterListener` for real distributed architecture including config and runtime data (checks, comments, downtimes) sync and replay
* `SyslogLogger`, `FileLogger` and `ConsoleLogger` for different types of logging
* Domain support for distributed cluster environments
* Config migration script supporting easier migration from Icinga 1.x configuration
* Reviewed configuration options, additional attributes added
* Enhanced ITL, added sample configuration
* Enable/Disable Icinga 2 features on CLI
* Documentation using Markdown (`Getting Started`, `Monitoring Basics`, `Object Types`, `Icinga Template Library`,
`Advanced Topics`, `Migration from Icinga 1.x`, `Differences between Icinga 1.x and 2`, `Vagrant Demo VM`)
* Vagrant Demo VM supported by Puppet modules installing RPM snapshots for Icinga 2, Icinga 1.x Classic UI and Web
* Package snapshots available on [packages.icinga.org]

