## <a id="setting-up-livestatus"></a> Livestatus

The [MK Livestatus](http://mathias-kettner.de/checkmk_livestatus.html) project
implements a query protocol that lets users query their Icinga instance for
status information. It can also be used to send commands.

> **Tip**
>
> Only install the Livestatus feature if your web interface or addon requires
> you to do so (for example, [Icinga Web 2](2-getting-started.md#setting-up-icingaweb2)).
> [Icinga Classic UI](11-alternative-frontends.md#setting-up-icinga-classic-ui) and [Icinga Web](11-alternative-frontends.md#setting-up-icinga-web)
> do not use Livestatus as backend.

The Livestatus component that is distributed as part of Icinga 2 is a
re-implementation of the Livestatus protocol which is compatible with MK
Livestatus.

Details on the available tables and attributes with Icinga 2 can be found
in the [Livestatus Schema](18-appendix.md#schema-livestatus) section.

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

Details on the configuration can be found in the [LivestatusListener](6-object-types.md#objecttype-livestatuslistener)
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

A list of available external commands and their parameters can be found [here](18-appendix.md#external-commands-list-detail)

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
  log           | services, hosts, contacts, commands | parses [compatlog](6-object-types.md#objecttype-compatlogger) and shows log attributes
  statehist     | hosts, services | parses [compatlog](6-object-types.md#objecttype-compatlogger) and aggregates state change attributes

The `commands` table is populated with `CheckCommand`, `EventCommand` and `NotificationCommand` objects.

A detailed list on the available table attributes can be found in the [Livestatus Schema documentation](18-appendix.md#schema-livestatus).

