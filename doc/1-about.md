# About Icinga 2

## What is Icinga 2?

Icinga 2 is an enterprise-grade open source monitoring system which keeps watch over networks
and any conceivable network resource, notifies the user of errors and recoveries and generates
performance data for reporting. Scalable and extensible, Icinga 2 can monitor complex, large
environments across dispersed locations.

## Licensing

Icinga 2 and the Icinga 2 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
LICENSE file included in the package.

## Support

Support for Icinga 2 is available in a number of ways. Please have a look at
the support overview page at [https://support.icinga.org].


## What's New in Version 0.0.6

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

### Changes
* Generated object names (host with services array) use an exclamation mark instead of a colon
as seperator. State file objects with downtimes, comments, etc are invalid (unknown) for that
reason.
* Script variables are set using 'var' and 'const' instead of the previous 'set' identifier
* ITL constants are now embedded in libicinga
* Removed the ConsoleLogger object and keep the default console log enabled until we daemonize

## What's New in Version 0.0.5

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

## What's New in Version 0.0.4

* IDO: PostgreSQL support
* IDO: implemented options to filter which kind of events are written to the database
* Livestatus: implemented support for the log and statehist tables
* Livestatus: implemented regex filters (~ and ~~)
* Replaced autotools-based build system with cmake
* Lots of bug fixes and performance improvements

## What's New in Version 0.0.3

* `StatusDataWriter` and `ExternalCommandListener` (former `Compat`) and `CompatLogger`
(former CompatLog) for status.dat/objects.cache/icinga2.cmd/icinga.log for Icinga 1.x Classic UI support
* `IdoMysqlConnection` and `ExternalCommandListener` for Icinga 1.x Web
* `IdoMysqlConnection` for Icinga 1.x Reporting, NagVis
* `LivestatusListener` for addons using the livestatus interface (history tables tbd)
* `PerfDataWriter` for graphing addons such as PNP/inGraph/graphite (can be loaded multiple times!)
* `GraphiteWriter` for sending metrics to directly to graphite carbon sockets
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

