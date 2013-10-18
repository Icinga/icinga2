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

