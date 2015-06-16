# <a id="advanced-topics"></a> Advanced Topics

This chapter covers a number of advanced topics. If you're new to Icinga you
can safely skip over things you're not interested in.

## <a id="downtimes"></a> Downtimes

Downtimes can be scheduled for planned server maintenance or
any other targetted service outage you are aware of in advance.

Downtimes will suppress any notifications, and may trigger other
downtimes too. If the downtime was set by accident, or the duration
exceeds the maintenance, you can manually cancel the downtime.
Planned downtimes will also be taken into account for SLA reporting
tools calculating the SLAs based on the state and downtime history.

Multiple downtimes for a single object may overlap. This is useful
when you want to extend your maintenance window taking longer than expected.
If there are multiple downtimes triggered for one object, the overall downtime depth
will be greater than `1`.


If the downtime was scheduled after the problem changed to a critical hard
state triggering a problem notification, and the service recovers during
the downtime window, the recovery notification won't be suppressed.

### <a id="fixed-flexible-downtimes"></a> Fixed and Flexible Downtimes

A `fixed` downtime will be activated at the defined start time, and
removed at the end time. During this time window the service state
will change to `NOT-OK` and then actually trigger the downtime.
Notifications are suppressed and the downtime depth is incremented.

Common scenarios are a planned distribution upgrade on your linux
servers, or database updates in your warehouse. The customer knows
about a fixed downtime window between 23:00 and 24:00. After 24:00
all problems should be alerted again. Solution is simple -
schedule a `fixed` downtime starting at 23:00 and ending at 24:00.

Unlike a `fixed` downtime, a `flexible` downtime will be triggered
by the state change in the time span defined by start and end time,
and then last for the specified duration in minutes.

Imagine the following scenario: Your service is frequently polled
by users trying to grab free deleted domains for immediate registration.
Between 07:30 and 08:00 the impact will hit for 15 minutes and generate
a network outage visible to the monitoring. The service is still alive,
but answering too slow to Icinga 2 service checks.
For that reason, you may want to schedule a downtime between 07:30 and
08:00 with a duration of 15 minutes. The downtime will then last from
its trigger time until the duration is over. After that, the downtime
is removed (may happen before or after the actual end time!).

### <a id="scheduling-downtime"></a> Scheduling a downtime

This can either happen through a web interface or by sending an [external command](5-advanced-topics.md#external-commands)
to the external command pipe provided by the `ExternalCommandListener` configuration.

Fixed downtimes require a start and end time (a duration will be ignored).
Flexible downtimes need a start and end time for the time span, and a duration
independent from that time span.

### <a id="triggered-downtimes"></a> Triggered Downtimes

This is optional when scheduling a downtime. If there is already a downtime
scheduled for a future maintenance, the current downtime can be triggered by
that downtime. This renders useful if you have scheduled a host downtime and
are now scheduling a child host's downtime getting triggered by the parent
downtime on NOT-OK state change.

### <a id="recurring-downtimes"></a> Recurring Downtimes

[ScheduledDowntime objects](6-object-types.md#objecttype-scheduleddowntime) can be used to set up
recurring downtimes for services.

Example:

    apply ScheduledDowntime "backup-downtime" to Service {
      author = "icingaadmin"
      comment = "Scheduled downtime for backup"

      ranges = {
        monday = "02:00-03:00"
        tuesday = "02:00-03:00"
        wednesday = "02:00-03:00"
        thursday = "02:00-03:00"
        friday = "02:00-03:00"
        saturday = "02:00-03:00"
        sunday = "02:00-03:00"
      }

      assign where "backup" in service.groups
    }


## <a id="comments-intro"></a> Comments

Comments can be added at runtime and are persistent over restarts. You can
add useful information for others on repeating incidents (for example
"last time syslog at 100% cpu on 17.10.2013 due to stale nfs mount") which
is primarly accessible using web interfaces.

Adding and deleting comment actions are possible through the external command pipe
provided with the `ExternalCommandListener` configuration. The caller must
pass the comment id in case of manipulating an existing comment.


## <a id="acknowledgements"></a> Acknowledgements

If a problem is alerted and notified you may signal the other notification
recipients that you are aware of the problem and will handle it.

By sending an acknowledgement to Icinga 2 (using the external command pipe
provided with `ExternalCommandListener` configuration) all future notifications
are suppressed, a new comment is added with the provided description and
a notification with the type `NotificationFilterAcknowledgement` is sent
to all notified users.

### <a id="expiring-acknowledgements"></a> Expiring Acknowledgements

Once a problem is acknowledged it may disappear from your `handled problems`
dashboard and no-one ever looks at it again since it will suppress
notifications too.

This `fire-and-forget` action is quite common. If you're sure that a
current problem should be resolved in the future at a defined time,
you can define an expiration time when acknowledging the problem.

Icinga 2 will clear the acknowledgement when expired and start to
re-notify if the problem persists.


## <a id="timeperiods"></a> Time Periods

Time Periods define time ranges in Icinga where event actions are
triggered, for example whether a service check is executed or not within
the `check_period` attribute. Or a notification should be sent to
users or not, filtered by the `period` and `notification_period`
configuration attributes for `Notification` and `User` objects.

> **Note**
>
> If you are familar with Icinga 1.x - these time period definitions
> are called `legacy timeperiods` in Icinga 2.
>
> An Icinga 2 legacy timeperiod requires the `ITL` provided template
>`legacy-timeperiod`.

The `TimePeriod` attribute `ranges` may contain multiple directives,
including weekdays, days of the month, and calendar dates.
These types may overlap/override other types in your ranges dictionary.

The descending order of precedence is as follows:

* Calendar date (2008-01-01)
* Specific month date (January 1st)
* Generic month date (Day 15)
* Offset weekday of specific month (2nd Tuesday in December)
* Offset weekday (3rd Monday)
* Normal weekday (Tuesday)

If you don't set any `check_period` or `notification_period` attribute
on your configuration objects Icinga 2 assumes `24x7` as time period
as shown below.

    object TimePeriod "24x7" {
      import "legacy-timeperiod"

      display_name = "Icinga 2 24x7 TimePeriod"
      ranges = {
        "monday"    = "00:00-24:00"
        "tuesday"   = "00:00-24:00"
        "wednesday" = "00:00-24:00"
        "thursday"  = "00:00-24:00"
        "friday"    = "00:00-24:00"
        "saturday"  = "00:00-24:00"
        "sunday"    = "00:00-24:00"
      }
    }

If your operation staff should only be notified during workhours
create a new timeperiod named `workhours` defining a work day from
09:00 to 17:00.

    object TimePeriod "workhours" {
      import "legacy-timeperiod"

      display_name = "Icinga 2 8x5 TimePeriod"
      ranges = {
        "monday"    = "09:00-17:00"
        "tuesday"   = "09:00-17:00"
        "wednesday" = "09:00-17:00"
        "thursday"  = "09:00-17:00"
        "friday"    = "09:00-17:00"
      }
    }

Use the `period` attribute to assign time periods to
`Notification` and `Dependency` objects:

    object Notification "mail" {
      import "generic-notification"

      host_name = "localhost"

      command = "mail-notification"
      users = [ "icingaadmin" ]
      period = "workhours"
    }

## <a id="use-functions-object-config"></a> Use Functions in Object Configuration

There is a limited scope where functions can be used as object attributes such as:

* As value for [Custom Attributes](3-monitoring-basics.md#custom-attributes-functions)
* Returning boolean expressions for [set_if](5-advanced-topics.md#use-functions-command-arguments-setif) inside command arguments
* Returning a [command](5-advanced-topics.md#use-functions-command-attribute) array inside command objects

The other way around you can create objects dynamically using your own global functions.

> **Note**
>
> Functions called inside command objects share the same global scope as runtime macros.
> Therefore you can access host custom attributes like `host.vars.os`, or any other
> object attribute from inside the function definition used for [set_if](5-advanced-topics.md#use-functions-command-arguments-setif) or [command](5-advanced-topics.md#use-functions-command-attribute).

Tips when implementing functions:

* Use [log()](20-library-reference.md#global-functions) to dump variables. You can see the output
inside the `icinga2.log` file depending in your log severity
* Use the `icinga2 console` to test basic functionality (e.g. iterating over a dictionary)
* Build them step-by-step. You can always refactor your code later on.

### <a id="use-functions-command-arguments-setif"></a> Use Functions in Command Arguments set_if

The `set_if` attribute inside the command arguments definition in the
[CheckCommand object definition](6-object-types.md#objecttype-checkcommand) is primarly used to
evaluate whether the command parameter should be set or not.

By default you can evaluate runtime macros for their existance, and if the result is not an empty
string the command parameter is passed. This becomes fairly complicated when want to evaluate
multiple conditions and attributes.

The following example was found on the community support channels. The user had defined a host
dictionary named `compellent` with the key `disks`. This was then used inside service apply for rules.

    object Host "dict-host" {
      check_command = "check_compellent"
      vars.compellent["disks"] = {
        file = "/var/lib/check_compellent/san_disks.0.json",
        checks = ["disks"]
      }
    }

The more significant problem was to only add the command parameter `--disk` to the plugin call
when the dictionary `compellent` contains the key `disks`, and omit it if not found.

By defining `set_if` as [abbreviated lambda function](19-language-reference.md#nullary-lambdas)
and evaluating the host custom attribute `compellent` containing the `disks` this problem was
solved like this:

    object CheckCommand "check_compellent" {
      import "plugin-check-command"
      command   = [ "/usr/bin/check_compellent" ]
      arguments   = {
        "--disks"  = {
          set_if = {{
            var host_vars = host.vars
            log(host_vars)
            var compel = host_vars.compellent
            log(compel)
            compel.contains("disks")
          }}
        }
      }
    }

This implementation uses the dictionary type method [contains](20-library-reference.md#dictionary-contains)
and will fail if `host.vars.compellent` is not of the type `Dictionary`.
Therefore you can extend the checks using the [typeof](19-language-reference.md#types) function.

You can test the types using the `icinga2 console`:

    # icinga2 console
    Icinga (version: v2.3.0-193-g3eb55ad)
    <1> => srv_vars.compellent["check_a"] = { file="outfile_a.json", checks = [ "disks", "fans" ] }
    null
    <2> => srv_vars.compellent["check_b"] = { file="outfile_b.json", checks = [ "power", "voltages" ] }
    null
    <3> => typeof(srv_vars.compellent)
    type 'Dictionary'
    <4> =>

The more programmatic approach for `set_if` could look like this:

        "--disks" = {
          set_if = {{
            var srv_vars = service.vars
            if(len(srv_vars) > 0) {
              if (typeof(srv_vars.compellent) == Dictionary) {
                return srv_vars.compellent.contains("disks")
              } else {
                log(LogInformationen, "checkcommand set_if", "custom attribute compellent_checks is not a dictionary, ignoring it.")
                return false
              }
            } else {
              log(LogWarning, "checkcommand set_if", "empty custom attributes")
              return false
            }
          }}
        }


### <a id="use-functions-command-attribute"></a> Use Functions as Command Attribute

This comes in handy for [NotificationCommands](6-object-types.md#objecttype-notificationcommand)
or [EventCommands](6-object-types.md#objecttype-eventcommand) which does not require
a returned checkresult including state/output.

The following example was taken from the community support channels. The requirement was to
specify a custom attribute inside the notification apply rule and decide which notification
script to call based on that.

    object User "short-dummy" {
    }
    
    object UserGroup "short-dummy-group" {
      assign where user.name == "short-dummy"
    }
    
    apply Notification "mail-admins-short" to Host {
       import "mail-host-notification"
       command = "mail-host-notification-test"
       user_groups = [ "short-dummy-group" ]
       vars.short = true
       assign where host.vars.notification.mail
    }

The solution is fairly simple: The `command` attribute is implemented as function returning
an array required by the caller Icinga 2.
The local variable `mailscript` sets the default value for the notification scrip location.
If the notification custom attribute `short` is set, it will override the local variable `mailscript`
with a new value.
The `mailscript` variable is then used to compute the final notification command array being
returned.

You can omit the `log()` calls, they only help debugging.

    object NotificationCommand "mail-host-notification-test" {
      import "plugin-notification-command"
      command = {{
        log("command as function")
        var mailscript = "mail-host-notification-long.sh"
        if (notification.vars.short) {
           mailscript = "mail-host-notification-short.sh"
        }
        log("Running command")
        log(mailscript)
    
        var cmd = [ SysconfDir + "/icinga2/scripts/" + mailscript ]
        log(LogCritical, "me", cmd)
        return cmd
      }}
    
      env = {
      }
    }



## <a id="access-object-attributes-at-runtime"></a> Access Object Attributes at Runtime

The [Object Accessor Functions](20-library-reference.md#object-accessor-functions)
can be used to retrieve references to other objects by name.

This allows you to access configuration and runtime object attributes. A detailed
list can be found [here](6-object-types.md#object-types).

Simple cluster example for accessing two host object states and calculating a virtual
cluster state and output:

    object Host "cluster-host-01" {
      check_command = "dummy"
      vars.dummy_state = 2
      vars.dummy_text = "This host is down."
    }

    object Host "cluster-host-02" {
      check_command = "dummy"
      vars.dummy_state = 0
      vars.dummy_text = "This host is up."
    }

    object Host "cluster" {
      check_command = "dummy"
      vars.cluster_nodes = [ "cluster-host-01", "cluster-host-02" ]

      vars.dummy_state = {{
        var up_count = 0
        var down_count = 0
        var cluster_nodes = macro("$cluster_nodes$")

        for (node in cluster_nodes) {
          if (get_host(node).state > 0) {
            down_count += 1
          } else {
            up_count += 1
          }
        }

        if (up_count >= down_count) {
          return 0 //same up as down -> UP
        } else {
          return 1 //something is broken
        }
      }}

      vars.dummy_text = {{
        var output = "Cluster hosts:\n"
        var cluster_nodes = macro("$cluster_nodes$")

        for (node in cluster_nodes) {
          output += node + ": " + get_host(node).last_check_result.output + "\n"
        }

        return output
      }}
    }


The following example sets time dependent thresholds for the load check based on the current
time of the day compared to the defined time period.

    object TimePeriod "backup" {
      import "legacy-timeperiod"

      ranges = {
        monday = "02:00-03:00"
        tuesday = "02:00-03:00"
        wednesday = "02:00-03:00"
        thursday = "02:00-03:00"
        friday = "02:00-03:00"
        saturday = "02:00-03:00"
        sunday = "02:00-03:00"
      }
    }

    object Host "webserver-with-backup" {
      check_command = "hostalive"
      address = "127.0.0.1"
    }

    object Service "webserver-backup-load" {
      check_command = "load"
      host_name = "webserver-with-backup"

      vars.load_wload1 = {{
        if (get_time_period("backup").is_inside) {
          return 20
        } else {
          return 5
        }
      }}
      vars.load_cload1 = {{
        if (get_time_period("backup").is_inside) {
          return 40
        } else {
          return 10
        }
      }}
    }


## <a id="check-result-freshness"></a> Check Result Freshness

In Icinga 2 active check freshness is enabled by default. It is determined by the
`check_interval` attribute and no incoming check results in that period of time.

    threshold = last check execution time + check interval

Passive check freshness is calculated from the `check_interval` attribute if set.

    threshold = last check result time + check interval

If the freshness checks are invalid, a new check is executed defined by the
`check_command` attribute.


## <a id="check-flapping"></a> Check Flapping

The flapping algorithm used in Icinga 2 does not store the past states but
calculcates the flapping threshold from a single value based on counters and
half-life values. Icinga 2 compares the value with a single flapping threshold
configuration attribute named `flapping_threshold`.

Flapping detection can be enabled or disabled using the `enable_flapping` attribute.


## <a id="volatile-services"></a> Volatile Services

By default all services remain in a non-volatile state. When a problem
occurs, the `SOFT` state applies and once `max_check_attempts` attribute
is reached with the check counter, a `HARD` state transition happens.
Notifications are only triggered by `HARD` state changes and are then
re-sent defined by the `interval` attribute.

It may be reasonable to have a volatile service which stays in a `HARD`
state type if the service stays in a `NOT-OK` state. That way each
service recheck will automatically trigger a notification unless the
service is acknowledged or in a scheduled downtime.


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

A list of currently supported external commands can be found [here](22-appendix.md#external-commands-list-detail).

Detailed information on the commands and their required parameters can be found
on the [Icinga 1.x documentation](http://docs.icinga.org/latest/en/extcommands2.html).

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

## <a id="performance-data"></a> Performance Data

When a host or service check is executed plugins should provide so-called
`performance data`. Next to that additional check performance data
can be fetched using Icinga 2 runtime macros such as the check latency
or the current service state (or additional custom attributes).

The performance data can be passed to external applications which aggregate and
store them in their backends. These tools usually generate graphs for historical
reporting and trending.

Well-known addons processing Icinga performance data are PNP4Nagios,
inGraph and Graphite.

### <a id="writing-performance-data-files"></a> Writing Performance Data Files

PNP4Nagios, inGraph and Graphios use performance data collector daemons to fetch
the current performance files for their backend updates.

Therefore the Icinga 2 `PerfdataWriter` object allows you to define
the output template format for host and services backed with Icinga 2
runtime vars.

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

While there are some Graphite collector scripts and daemons like Graphios available for
Icinga 1.x it's more reasonable to directly process the check and plugin performance
in memory in Icinga 2. Once there are new metrics available, Icinga 2 will directly
write them to the defined Graphite Carbon daemon tcp socket.

You can enable the feature using

    # icinga2 feature enable graphite

By default the `GraphiteWriter` object expects the Graphite Carbon Cache to listen at
`127.0.0.1` on TCP port `2003`.

The current naming schema is

    icinga.<hostname>.<metricname>
    icinga.<hostname>.<servicename>.<metricname>

You can customize the metric prefix name by using the `host_name_template` and
`service_name_template` configuration attributes.

The example below uses [runtime macros](3-monitoring-basics.md#runtime-macros) and a
[global constant](19-language-reference.md#constants) named `GraphiteEnv`. The constant name
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


## <a id="status-data"></a> Status Data

Icinga 1.x writes object configuration data and status data in a cyclic
interval to its `objects.cache` and `status.dat` files. Icinga 2 provides
the `StatusDataWriter` object which dumps all configuration objects and
status updates in a regular interval.

    # icinga2 feature enable statusdata

Icinga 1.x Classic UI requires this data set as part of its backend.

> **Note**
>
> If you are not using any web interface or addon which uses these files
> you can safely disable this feature.


## <a id="compat-logging"></a> Compat Logging

The Icinga 1.x log format is considered being the `Compat Log`
in Icinga 2 provided with the `CompatLogger` object.

These logs are not only used for informational representation in
external web interfaces parsing the logs, but also to generate
SLA reports and trends in Icinga 1.x Classic UI. Furthermore the
[Livestatus](15-livestatus.md#setting-up-livestatus) feature uses these logs for answering queries to
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




## <a id="db-ido"></a> DB IDO

The IDO (Icinga Data Output) modules for Icinga 2 take care of exporting all
configuration and status information into a database. The IDO database is used
by a number of projects including Icinga Web 1.x and 2.

Details on the installation can be found in the [Configuring DB IDO](2-getting-started.md#configuring-db-ido-mysql)
chapter. Details on the configuration can be found in the
[IdoMysqlConnection](6-object-types.md#objecttype-idomysqlconnection) and
[IdoPgsqlConnection](6-object-types.md#objecttype-idopgsqlconnection)
object configuration documentation.
The DB IDO feature supports [High Availability](12-distributed-monitoring-ha.md#high-availability-db-ido) in
the Icinga 2 cluster.

The following example query checks the health of the current Icinga 2 instance
writing its current status to the DB IDO backend table `icinga_programstatus`
every 10 seconds. By default it checks 60 seconds into the past which is a reasonable
amount of time - adjust it for your requirements. If the condition is not met,
the query returns an empty result.

> **Tip**
>
> Use [check plugins](13-addons-plugins.md#plugins) to monitor the backend.

Replace the `default` string with your instance name, if different.

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


A detailed list on the available table attributes can be found in the [DB IDO Schema documentation](22-appendix.md#schema-db-ido).


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
