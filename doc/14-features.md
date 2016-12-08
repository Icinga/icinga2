# <a id="icinga2-features"></a> Icinga 2 Features

## <a id="logging"></a> Logging

Icinga 2 supports three different types of logging:

* File logging
* Syslog (on *NIX-based operating systems)
* Console logging (`STDOUT` on tty)

You can enable additional loggers using the `icinga2 feature enable`
and `icinga2 feature disable` commands to configure loggers:

Feature  | Description
---------|------------
debuglog | Debug log (path: `/var/log/icinga2/debug.log`, severity: `debug` or higher)
mainlog  | Main log (path: `/var/log/icinga2/icinga2.log`, severity: `information` or higher)
syslog   | Syslog (severity: `warning` or higher)

By default file the `mainlog` feature is enabled. When running Icinga 2
on a terminal log messages with severity `information` or higher are
written to the console.

Packages will install a configuration file for logrotate on supported
platforms. This configuration ensures that the `icinga2.log`, `error.log` and
`debug.log` files are rotated on a daily basis.

## <a id="db-ido"></a> DB IDO

The IDO (Icinga Data Output) modules for Icinga 2 take care of exporting all
configuration and status information into a database. The IDO database is used
by a number of projects including Icinga Web 1.x and 2.

Details on the installation can be found in the [Configuring DB IDO](2-getting-started.md#configuring-db-ido-mysql)
chapter. Details on the configuration can be found in the
[IdoMysqlConnection](9-object-types.md#objecttype-idomysqlconnection) and
[IdoPgsqlConnection](9-object-types.md#objecttype-idopgsqlconnection)
object configuration documentation.
The DB IDO feature supports [High Availability](6-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido) in
the Icinga 2 cluster.

The following example query checks the health of the current Icinga 2 instance
writing its current status to the DB IDO backend table `icinga_programstatus`
every 10 seconds. By default it checks 60 seconds into the past which is a reasonable
amount of time -- adjust it for your requirements. If the condition is not met,
the query returns an empty result.

> **Tip**
>
> Use [check plugins](5-service-monitoring.md#service-monitoring-plugins) to monitor the backend.

Replace the `default` string with your instance name if different.

Example for MySQL:

    # mysql -u root -p icinga -e "SELECT status_update_time FROM icinga_programstatus ps
      JOIN icinga_instances i ON ps.instance_id=i.instance_id
      WHERE (UNIX_TIMESTAMP(ps.status_update_time) > UNIX_TIMESTAMP(NOW())-60)
      AND i.instance_name='default';"

    +---------------------+
    | status_update_time  |
    +---------------------+
    | 2014-05-29 14:29:56 |
    +---------------------+


Example for PostgreSQL:

    # export PGPASSWORD=icinga; psql -U icinga -d icinga -c "SELECT ps.status_update_time FROM icinga_programstatus AS ps
      JOIN icinga_instances AS i ON ps.instance_id=i.instance_id
      WHERE ((SELECT extract(epoch from status_update_time) FROM icinga_programstatus) > (SELECT extract(epoch from now())-60))
      AND i.instance_name='default'";

    status_update_time
    ------------------------
     2014-05-29 15:11:38+02
    (1 Zeile)


A detailed list on the available table attributes can be found in the [DB IDO Schema documentation](23-appendix.md#schema-db-ido).


## <a id="external-commands"></a> External Commands

Icinga 2 provides an external command pipe for processing commands
triggering specific actions (for example rescheduling a service check
through the web interface).

In order to enable the `ExternalCommandListener` configuration use the
following command and restart Icinga 2 afterwards:

    # icinga2 feature enable command

Icinga 2 creates the command pipe file as `/var/run/icinga2/cmd/icinga2.cmd`
using the default configuration.

Web interfaces and other Icinga addons are able to send commands to
Icinga 2 through the external command pipe, for example for rescheduling
a forced service check:

    # /bin/echo "[`date +%s`] SCHEDULE_FORCED_SVC_CHECK;localhost;ping4;`date +%s`" >> /var/run/icinga2/cmd/icinga2.cmd

    # tail -f /var/log/messages

    Oct 17 15:01:25 icinga-server icinga2: Executing external command: [1382014885] SCHEDULE_FORCED_SVC_CHECK;localhost;ping4;1382014885
    Oct 17 15:01:25 icinga-server icinga2: Rescheduling next check for service 'ping4'

A list of currently supported external commands can be found [here](23-appendix.md#external-commands-list-detail).

Detailed information on the commands and their required parameters can be found
on the [Icinga 1.x documentation](http://docs.icinga.com/latest/en/extcommands2.html).

## <a id="performance-data"></a> Performance Data

When a host or service check is executed plugins should provide so-called
`performance data`. Next to that additional check performance data
can be fetched using Icinga 2 runtime macros such as the check latency
or the current service state (or additional custom attributes).

The performance data can be passed to external applications which aggregate and
store them in their backends. These tools usually generate graphs for historical
reporting and trending.

Well-known addons processing Icinga performance data are [PNP4Nagios](13-addons.md#addons-graphing-pnp),
[Graphite](13-addons.md#addons-graphing-graphite) or [OpenTSDB](14-features.md#opentsdb-writer).

### <a id="writing-performance-data-files"></a> Writing Performance Data Files

PNP4Nagios and Graphios use performance data collector daemons to fetch
the current performance files for their backend updates.

Therefore the Icinga 2 [PerfdataWriter](9-object-types.md#objecttype-perfdatawriter)
feature allows you to define the output template format for host and services helped
with Icinga 2 runtime vars.

    host_format_template = "DATATYPE::HOSTPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tHOSTPERFDATA::$host.perfdata$\tHOSTCHECKCOMMAND::$host.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$"
    service_format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tSERVICEDESC::$service.name$\tSERVICEPERFDATA::$service.perfdata$\tSERVICECHECKCOMMAND::$service.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$\tSERVICESTATE::$service.state$\tSERVICESTATETYPE::$service.state_type$"

The default templates are already provided with the Icinga 2 feature configuration
which can be enabled using

    # icinga2 feature enable perfdata

By default all performance data files are rotated in a 15 seconds interval into
the `/var/spool/icinga2/perfdata/` directory as `host-perfdata.<timestamp>` and
`service-perfdata.<timestamp>`.
External collectors need to parse the rotated performance data files and then
remove the processed files.

### <a id="graphite-carbon-cache-writer"></a> Graphite Carbon Cache Writer

While there are some [Graphite](13-addons.md#addons-graphing-graphite)
collector scripts and daemons like Graphios available for Icinga 1.x it's more
reasonable to directly process the check and plugin performance
in memory in Icinga 2. Once there are new metrics available, Icinga 2 will directly
write them to the defined Graphite Carbon daemon tcp socket.

You can enable the feature using

    # icinga2 feature enable graphite

By default the [GraphiteWriter](9-object-types.md#objecttype-graphitewriter) feature
expects the Graphite Carbon Cache to listen at `127.0.0.1` on TCP port `2003`.

#### <a id="graphite-carbon-cache-writer-schema"></a> Current Graphite Schema

The current naming schema is defined as follows. The official Icinga Web 2 Graphite
module will use that schema too.

The default prefix for hosts and services is configured using
[runtime macros](3-monitoring-basics.md#runtime-macros)like this:

    icinga2.$host.name$.host.$host.check_command$
    icinga2.$host.name$.services.$service.name$.$service.check_command$

You can customize the prefix name by using the `host_name_template` and
`service_name_template` configuration attributes.

The additional levels will allow fine granular filters and also template
capabilities, e.g. by using the check command `disk` for specific
graph templates in web applications rendering the Graphite data.

The following characters are escaped in prefix labels:

  Character	| Escaped character
  --------------|--------------------------
  whitespace	| _
  .		| _
  \		| _
  /		| _

Metric values are stored like this:

    <prefix>.perfdata.<perfdata-label>.value

The following characters are escaped in perfdata labels:

  Character	| Escaped character
  --------------|--------------------------
  whitespace	| _
  \		| _
  /		| _
  ::		| .

Note that perfdata labels may contain dots (`.`) allowing to
add more subsequent levels inside the Graphite tree.
`::` adds support for [multi performance labels](http://my-plugin.de/wiki/projects/check_multi/configuration/performance)
and is therefore replaced by `.`.

By enabling `enable_send_thresholds` Icinga 2 automatically adds the following threshold metrics:

    <prefix>.perfdata.<perfdata-label>.min
    <prefix>.perfdata.<perfdata-label>.max
    <prefix>.perfdata.<perfdata-label>.warn
    <prefix>.perfdata.<perfdata-label>.crit

By enabling `enable_send_metadata` Icinga 2 automatically adds the following metadata metrics:

    <prefix>.metadata.current_attempt
    <prefix>.metadata.downtime_depth
    <prefix>.metadata.acknowledgement
    <prefix>.metadata.execution_time
    <prefix>.metadata.latency
    <prefix>.metadata.max_check_attempts
    <prefix>.metadata.reachable
    <prefix>.metadata.state
    <prefix>.metadata.state_type

Metadata metric overview:

  metric             | description
  -------------------|------------------------------------------
  current_attempt    | current check attempt
  max_check_attempts | maximum check attempts until the hard state is reached
  reachable          | checked object is reachable
  downtime_depth     | number of downtimes this object is in
  acknowledgement    | whether the object is acknowledged or not
  execution_time     | check execution time
  latency            | check latency
  state              | current state of the checked object
  state_type         | 0=SOFT, 1=HARD state

The following example illustrates how to configure the storage schemas for Graphite Carbon
Cache.

    [icinga2_default]
    # intervals like PNP4Nagios uses them per default
    pattern = ^icinga2\.
    retentions = 1m:2d,5m:10d,30m:90d,360m:4y

#### <a id="graphite-carbon-cache-writer-schema-legacy"></a> Graphite Schema < 2.4

In order to restore the old legacy schema, you'll need to adopt the `GraphiteWriter`
configuration:

    object GraphiteWriter "graphite" {

      enable_legacy_mode = true

      host_name_template = "icinga.$host.name$"
      service_name_template = "icinga.$host.name$.$service.name$"
    }

The old legacy naming schema is

    icinga.<hostname>.<metricname>
    icinga.<hostname>.<servicename>.<metricname>

You can customize the metric prefix name by using the `host_name_template` and
`service_name_template` configuration attributes.

The example below uses [runtime macros](3-monitoring-basics.md#runtime-macros) and a
[global constant](17-language-reference.md#constants) named `GraphiteEnv`. The constant name
is freely definable and should be put in the [constants.conf](4-configuring-icinga-2.md#constants-conf) file.

    const GraphiteEnv = "icinga.env1"

    object GraphiteWriter "graphite" {
      host_name_template = GraphiteEnv + ".$host.name$"
      service_name_template = GraphiteEnv + ".$host.name$.$service.name$"
    }

To make sure Icinga 2 writes a valid label into Graphite some characters are replaced
with `_` in the target name:

    \/.-  (and space)

The resulting name in Graphite might look like:

    www-01 / http-cert / response time
    icinga.www_01.http_cert.response_time

In addition to the performance data retrieved from the check plugin, Icinga 2 sends
internal check statistic data to Graphite:

  metric             | description
  -------------------|------------------------------------------
  current_attempt    | current check attempt
  max_check_attempts | maximum check attempts until the hard state is reached
  reachable          | checked object is reachable
  downtime_depth     | number of downtimes this object is in
  acknowledgement    | whether the object is acknowledged or not
  execution_time     | check execution time
  latency            | check latency
  state              | current state of the checked object
  state_type         | 0=SOFT, 1=HARD state

The following example illustrates how to configure the storage-schemas for Graphite Carbon
Cache. Please make sure that the order is correct because the first match wins.

    [icinga_internals]
    pattern = ^icinga\..*\.(max_check_attempts|reachable|current_attempt|execution_time|latency|state|state_type)
    retentions = 5m:7d

    [icinga_default]
    # intervals like PNP4Nagios uses them per default
    pattern = ^icinga\.
    retentions = 1m:2d,5m:10d,30m:90d,360m:4y

### <a id="influxdb-writer"></a> InfluxDB Writer

Once there are new metrics available, Icinga 2 will directly write them to the
defined InfluxDB HTTP API.

You can enable the feature using

    # icinga2 feature enable influxdb

By default the [InfluxdbWriter](9-object-types.md#objecttype-influxdbwriter) feature
expects the InfluxDB daemon to listen at `127.0.0.1` on port `8086`.

More configuration details can be found [here](9-object-types.md#objecttype-influxdbwriter).

### <a id="gelfwriter"></a> GELF Writer

The `Graylog Extended Log Format` (short: [GELF](http://www.graylog2.org/resources/gelf))
can be used to send application logs directly to a TCP socket.

While it has been specified by the [graylog2](http://www.graylog2.org/) project as their
[input resource standard](http://www.graylog2.org/resources/gelf), other tools such as
[Logstash](http://www.logstash.net) also support `GELF` as
[input type](http://logstash.net/docs/latest/inputs/gelf).

You can enable the feature using

    # icinga2 feature enable gelf

By default the `GelfWriter` object expects the GELF receiver to listen at `127.0.0.1` on TCP port `12201`.
The default `source`  attribute is set to `icinga2`. You can customize that for your needs if required.

Currently these events are processed:
* Check results
* State changes
* Notifications

### <a id="opentsdb-writer"></a> OpenTSDB Writer

While there are some OpenTSDB collector scripts and daemons like tcollector available for
Icinga 1.x it's more reasonable to directly process the check and plugin performance
in memory in Icinga 2. Once there are new metrics available, Icinga 2 will directly
write them to the defined TSDB TCP socket.

You can enable the feature using

    # icinga2 feature enable opentsdb

By default the `OpenTsdbWriter` object expects the TSD to listen at
`127.0.0.1` on port `4242`.

The current naming schema is

    icinga.host.<metricname>
    icinga.service.<servicename>.<metricname>

for host and service checks. The tag host is always applied.

To make sure Icinga 2 writes a valid metric into OpenTSDB some characters are replaced
with `_` in the target name:

    \  (and space)

The resulting name in OpenTSDB might look like:

    www-01 / http-cert / response time
    icinga.http_cert.response_time

In addition to the performance data retrieved from the check plugin, Icinga 2 sends
internal check statistic data to OpenTSDB:

  metric             | description
  -------------------|------------------------------------------
  current_attempt    | current check attempt
  max_check_attempts | maximum check attempts until the hard state is reached
  reachable          | checked object is reachable
  downtime_depth     | number of downtimes this object is in
  acknowledgement    | whether the object is acknowledged or not
  execution_time     | check execution time
  latency            | check latency
  state              | current state of the checked object
  state_type         | 0=SOFT, 1=HARD state

While reachable, state and state_type are metrics for the host or service the
other metrics follow the current naming schema

    icinga.check.<metricname>

with the following tags

  tag     | description
  --------|------------------------------------------
  type    | the check type, one of [host, service]
  host    | hostname, the check ran on
  service | the service name (if type=service)

> **Note**
>
> You might want to set the tsd.core.auto_create_metrics setting to `true`
> in your opentsdb.conf configuration file.


## <a id="setting-up-livestatus"></a> Livestatus

The [MK Livestatus](http://mathias-kettner.de/checkmk_livestatus.html) project
implements a query protocol that lets users query their Icinga instance for
status information. It can also be used to send commands.

> **Tip**
>
> Only install the Livestatus feature if your web interface or addon requires
> you to do so (for example, [Icinga Web 2](2-getting-started.md#setting-up-icingaweb2)).
> Icinga Classic UI 1.x and Icinga Web 1.x do not use Livestatus as backend.

The Livestatus component that is distributed as part of Icinga 2 is a
re-implementation of the Livestatus protocol which is compatible with MK
Livestatus.

Details on the available tables and attributes with Icinga 2 can be found
in the [Livestatus Schema](23-appendix.md#schema-livestatus) section.

You can enable Livestatus using icinga2 feature enable:

    # icinga2 feature enable livestatus

After that you will have to restart Icinga 2:

Debian/Ubuntu, RHEL/CentOS 6 and SUSE:

    # service icinga2 restart

RHEL/CentOS 7 and Fedora:

    # systemctl restart icinga2

By default the Livestatus socket is available in `/var/run/icinga2/cmd/livestatus`.

In order for queries and commands to work you will need to add your query user
(e.g. your web server) to the `icingacmd` group:

    # usermod -a -G icingacmd www-data

The Debian packages use `nagios` as the user and group name. Make sure to change `icingacmd` to
`nagios` if you're using Debian.

Change `www-data` to the user you're using to run queries.

In order to use the historical tables provided by the livestatus feature (for example, the
`log` table) you need to have the `CompatLogger` feature enabled. By default these logs
are expected to be in `/var/log/icinga2/compat`. A different path can be set using the
`compat_log_path` configuration attribute.

    # icinga2 feature enable compatlog


### <a id="livestatus-sockets"></a> Livestatus Sockets

Other to the Icinga 1.x Addon, Icinga 2 supports two socket types

* Unix socket (default)
* TCP socket

Details on the configuration can be found in the [LivestatusListener](9-object-types.md#objecttype-livestatuslistener)
object configuration.

### <a id="livestatus-get-queries"></a> Livestatus GET Queries

> **Note**
>
> All Livestatus queries require an additional empty line as query end identifier.
> The `nc` tool (`netcat`) provides the `-U` parameter to communicate using
> a unix socket.

There also is a Perl module available in CPAN for accessing the Livestatus socket
programmatically: [Monitoring::Livestatus](http://search.cpan.org/~nierlein/Monitoring-Livestatus-0.74/)


Example using the unix socket:

    # echo -e "GET services\n" | /usr/bin/nc -U /var/run/icinga2/cmd/livestatus

Example using the tcp socket listening on port `6558`:

    # echo -e 'GET services\n' | netcat 127.0.0.1 6558

    # cat servicegroups <<EOF
    GET servicegroups

    EOF

    (cat servicegroups; sleep 1) | netcat 127.0.0.1 6558


### <a id="livestatus-command-queries"></a> Livestatus COMMAND Queries

A list of available external commands and their parameters can be found [here](23-appendix.md#external-commands-list-detail)

    $ echo -e 'COMMAND <externalcommandstring>' | netcat 127.0.0.1 6558


### <a id="livestatus-filters"></a> Livestatus Filters

and, or, negate

  Operator  | Negate   | Description
  ----------|------------------------
   =        | !=       | Equality
   ~        | !~       | Regex match
   =~       | !=~      | Equality ignoring case
   ~~       | !~~      | Regex ignoring case
   <        |          | Less than
   >        |          | Greater than
   <=       |          | Less than or equal
   >=       |          | Greater than or equal


### <a id="livestatus-stats"></a> Livestatus Stats

Schema: "Stats: aggregatefunction aggregateattribute"

  Aggregate Function | Description
  -------------------|--------------
  sum                | &nbsp;
  min                | &nbsp;
  max                | &nbsp;
  avg                | sum / count
  std                | standard deviation
  suminv             | sum (1 / value)
  avginv             | suminv / count
  count              | ordinary default for any stats query if not aggregate function defined

Example:

    GET hosts
    Filter: has_been_checked = 1
    Filter: check_type = 0
    Stats: sum execution_time
    Stats: sum latency
    Stats: sum percent_state_change
    Stats: min execution_time
    Stats: min latency
    Stats: min percent_state_change
    Stats: max execution_time
    Stats: max latency
    Stats: max percent_state_change
    OutputFormat: json
    ResponseHeader: fixed16

### <a id="livestatus-output"></a> Livestatus Output

* CSV

CSV output uses two levels of array separators: The members array separator
is a comma (1st level) while extra info and host|service relation separator
is a pipe (2nd level).

Separators can be set using ASCII codes like:

    Separators: 10 59 44 124

* JSON

Default separators.

### <a id="livestatus-error-codes"></a> Livestatus Error Codes

  Code      | Description
  ----------|--------------
  200       | OK
  404       | Table does not exist
  452       | Exception on query

### <a id="livestatus-tables"></a> Livestatus Tables

  Table         | Join      |Description
  --------------|-----------|----------------------------
  hosts         | &nbsp;    | host config and status attributes, services counter
  hostgroups    | &nbsp;    | hostgroup config, status attributes and host/service counters
  services      | hosts     | service config and status attributes
  servicegroups | &nbsp;    | servicegroup config, status attributes and service counters
  contacts      | &nbsp;    | contact config and status attributes
  contactgroups | &nbsp;    | contact config, members
  commands      | &nbsp;    | command name and line
  status        | &nbsp;    | programstatus, config and stats
  comments      | services  | status attributes
  downtimes     | services  | status attributes
  timeperiods   | &nbsp;    | name and is inside flag
  endpoints     | &nbsp;    | config and status attributes
  log           | services, hosts, contacts, commands | parses [compatlog](9-object-types.md#objecttype-compatlogger) and shows log attributes
  statehist     | hosts, services | parses [compatlog](9-object-types.md#objecttype-compatlogger) and aggregates state change attributes
  hostsbygroup  | hostgroups | host attributes grouped by hostgroup and its attributes
  servicesbygroup | servicegroups | service attributes grouped by servicegroup and its attributes
  servicesbyhostgroup  | hostgroups | service attributes grouped by hostgroup and its attributes

The `commands` table is populated with `CheckCommand`, `EventCommand` and `NotificationCommand` objects.

A detailed list on the available table attributes can be found in the [Livestatus Schema documentation](23-appendix.md#schema-livestatus).


## <a id="status-data"></a> Status Data Files

Icinga 1.x writes object configuration data and status data in a cyclic
interval to its `objects.cache` and `status.dat` files. Icinga 2 provides
the `StatusDataWriter` object which dumps all configuration objects and
status updates in a regular interval.

    # icinga2 feature enable statusdata

Icinga 1.x Classic UI requires this data set as part of its backend.

> **Note**
>
> If you are not using any web interface or addon which uses these files,
> you can safely disable this feature.


## <a id="compat-logging"></a> Compat Log Files

The Icinga 1.x log format is considered being the `Compat Log`
in Icinga 2 provided with the `CompatLogger` object.

These logs are not only used for informational representation in
external web interfaces parsing the logs, but also to generate
SLA reports and trends in Icinga 1.x Classic UI. Furthermore the
[Livestatus](14-features.md#setting-up-livestatus) feature uses these logs for answering queries to
historical tables.

The `CompatLogger` object can be enabled with

    # icinga2 feature enable compatlog

By default, the Icinga 1.x log file called `icinga.log` is located
in `/var/log/icinga2/compat`. Rotated log files are moved into
`var/log/icinga2/compat/archives`.

The format cannot be changed without breaking compatibility to
existing log parsers.

    # tail -f /var/log/icinga2/compat/icinga.log

    [1382115688] LOG ROTATION: HOURLY
    [1382115688] LOG VERSION: 2.0
    [1382115688] HOST STATE: CURRENT;localhost;UP;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;disk;WARNING;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;http;OK;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;load;OK;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;ping4;OK;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;ping6;OK;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;processes;WARNING;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;ssh;OK;HARD;1;
    [1382115688] SERVICE STATE: CURRENT;localhost;users;OK;HARD;1;
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;disk;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;http;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;load;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;ping4;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;ping6;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;processes;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;ssh;1382115705
    [1382115706] EXTERNAL COMMAND: SCHEDULE_FORCED_SVC_CHECK;localhost;users;1382115705
    [1382115731] EXTERNAL COMMAND: PROCESS_SERVICE_CHECK_RESULT;localhost;ping6;2;critical test|
    [1382115731] SERVICE ALERT: localhost;ping6;CRITICAL;SOFT;2;critical test


## <a id="check-result-files"></a> Check Result Files

Icinga 1.x writes its check result files to a temporary spool directory
where they are processed in a regular interval.
While this is extremely inefficient in performance regards it has been
rendered useful for passing passive check results directly into Icinga 1.x
skipping the external command pipe.

Several clustered/distributed environments and check-aggregation addons
use that method. In order to support step-by-step migration of these
environments, Icinga 2 supports the `CheckResultReader` object.

There is no feature configuration available, but it must be defined
on-demand in your Icinga 2 objects configuration.

    object CheckResultReader "reader" {
      spool_dir = "/data/check-results"
    }

