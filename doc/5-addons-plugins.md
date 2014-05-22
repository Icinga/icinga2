# <a id="addons-plugins"></a> Icinga Addons and Plugins

## <a id="addons"></a> Addons

### <a id="addons-graphing-reporting"></a> Graphing Addons

#### <a id="addons-graphing-pnp"></a> PNP

PNP (http://www.pnp4nagios.org) must be configured using the "bulk mode with npcd and npcdmod"
hence Icinga 2's [PerfdataWriter](#performance-data) acts as npcdmod. NPCD will collect
the rotated performance data files.

#### <a id="addons-graphing-pnp"></a> inGraph

inGraph (https://www.netways.org/projects/ingraph/wiki) requires only the ingraph-collector
configured pointed to the perfdata files Icinga 2's [PerfdataWriter](#performance-data) will
write to the performance data spool directory.

#### <a id="addons-graphing-pnp"></a> Graphite

There are Graphite addons available collecting the performance data files as well. But
natively you just use the [GraphiteWriter](#graphite-carbon-cache-writer) feature.

#### <a id="addons-reporting"></a> Icinga Reporting

By enabling the DB IDO feature you can use the Icinga Reporting package.


### <a id="addons-visualization"></a> Visualization

#### <a id="addons-visualization-nagvis"></a> NagVis

Either using Livestatus or DB IDO as backend you can create your own network maps
based on your monitoring configuration and status data using NagVis (http://www.nagvis.org).

### <a id="addons-web-interfaces"></a> Web Interfaces

Next to the Icinga supported web interfaces (Classic UI 1.x, Web 1.x, Web 2) there are a
couple of community provided web interfaces too:

* Thruk (http://www.thruk.org) based on the Livestatus feature



## <a id="plugins"></a> Plugins

Additional plugins next to the [Monitoring Plugins](https://www.monitoring-plugins.org)
are available at the [Monitoring Exchange](#https://www.monitoringexchange.org) platform.

More details on the plugins can also be found on the Icinga Wiki at https://www.icinga.org

## <a id="plugin-api"></a> Plugin API

Currently Icinga 2 supports the native plugin API specification from the `Monitoring Plugins`
project.

The `Monitoring Plugin API` is defined in the [Monitoring Plugins Development Guidelines](https://www.monitoring-plugins.org/doc/guidelines.html).
