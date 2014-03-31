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

There are many ways to contribute to Icinga - be it by sending patches, testing and
reporting bugs, reviewing and updating the documentation, ... every contribution
is appreciated!

Please get in touch with the Icinga team at [https://www.icinga.org/ecosystem/].

## <a id="whats-new"></a> What's new

### What's New in Version 0.0.9

* new [apply](#apply) rules for assigning objects based on attribute conditions
* inline object definitions removed in favor of [apply](#apply) rules
* [import](#template-imports) keyword instead of `inherits` keyword for all objects
* new [constants.conf](#constants-conf) providing `PluginDir` constant instead of `$plugindir$` macro
* unknown attributes and duplicate objects generate a configuration error
* improved configuration error output
* create endpoint tables for legacy interfaces (status data, DB IDO, Livestatus)
* export host `check` attribute in legacy interfaces (status data, DB IDO, Livestatus)
* add documentation about [cluster scenarios](#cluster-scenarios)
* Livestatus: add `check_source` attribute to services table
* Compat: Fix host service order for Classic UI
* Remove comments when clearing acknowledgements
* Recovery [Notifications](#objecttype-notification) require StateFilterOK

#### Changes

> **Note**
>
> Configuration updates required!

* removed deprecated `var`/`set` identifier, use [const](#const) instead
* [constants.conf](#constants-conf) needs to be included in [icinga2.conf](#icinga2-conf) before [ITL](#itl) inclusion
* [import](#template-imports) instead of `inherits` (examples in [localhost.conf](#localhost-conf))
* [apply](#apply) rules instead of inline definitions for [Service](#objecttype-service),
[Dependency](#objecttype-dependency), [Notification](#objecttype-notitifcation),
[ScheduledDowntime](#objecttype-scheduleddowntime) objects (examples in [localhost.conf](#localhost-conf)).
* unknown attributes and duplicate objects generate a configuration error
* DB IDO: schema update for 0.0.9 ([MySQL](#upgrading-mysql-db), [PostgreSQL](#upgrading-postgresql-db))

### Archive

Please check the `ChangeLog` file.








