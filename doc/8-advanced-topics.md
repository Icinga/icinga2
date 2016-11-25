# <a id="advanced-topics"></a> Advanced Topics

This chapter covers a number of advanced topics. If you're new to Icinga, you
can safely skip over things you're not interested in.

## <a id="downtimes"></a> Downtimes

Downtimes can be scheduled for planned server maintenance or
any other targeted service outage you are aware of in advance.

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

This can either happen through a web interface or by sending an [external command](14-features.md#external-commands)
to the external command pipe provided by the `ExternalCommandListener` configuration.

Fixed downtimes require a start and end time (a duration will be ignored).
Flexible downtimes need a start and end time for the time span, and a duration
independent from that time span.

### <a id="triggered-downtimes"></a> Triggered Downtimes

This is optional when scheduling a downtime. If there is already a downtime
scheduled for a future maintenance, the current downtime can be triggered by
that downtime. This renders useful if you have scheduled a host downtime and
are now scheduling a child host's downtime getting triggered by the parent
downtime on `NOT-OK` state change.

### <a id="recurring-downtimes"></a> Recurring Downtimes

[ScheduledDowntime objects](9-object-types.md#objecttype-scheduleddowntime) can be used to set up
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
is primarily accessible using web interfaces.

Adding and deleting comment actions are possible through the external command pipe
provided with the `ExternalCommandListener` configuration. The caller must
pass the comment id in case of manipulating an existing comment.


## <a id="acknowledgements"></a> Acknowledgements

If a problem is alerted and notified, you may signal the other notification
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
re-notify, if the problem persists.


## <a id="timeperiods"></a> Time Periods

[Time Periods](9-object-types.md#objecttype-timeperiod) define
time ranges in Icinga where event actions are triggered, for
example whether a service check is executed or not within
the `check_period` attribute. Or a notification should be sent to
users or not, filtered by the `period` and `notification_period`
configuration attributes for `Notification` and `User` objects.

> **Note**
>
> If you are familiar with Icinga 1.x, these time period definitions
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
on your configuration objects, Icinga 2 assumes `24x7` as time period
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

If your operation staff should only be notified during workhours,
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

Furthermore if you wish to specify a notification period across midnight,
you can define it the following way:

    object Timeperiod "across-midnight" {
      import "legacy-timeperiod"

      display_name = "Nightly Notification"
      ranges = {
        "saturday" = "22:00-24:00"
        "sunday" = "00:00-03:00"
      }
    }

Below you can see another example for configuring timeperiods across several
days, weeks or months. This can be useful when taking components offline
for a distinct period of time.

    object Timeperiod "standby" {
      import "legacy-timeperiod"

      display_name = "Standby"
      ranges = {
        "2016-09-30 - 2016-10-30" = "00:00-24:00"
      }
    }

Please note that the spaces before and after the dash are mandatory.

Once your time period is configured you can Use the `period` attribute
to assign time periods to `Notification` and `Dependency` objects:

    object Notification "mail" {
      import "generic-notification"

      host_name = "localhost"

      command = "mail-notification"
      users = [ "icingaadmin" ]
      period = "workhours"
    }

### <a id="timeperiods-includes-excludes"></a> Time Periods Inclusion and Exclusion

Sometimes it is necessary to exclude certain time ranges from
your default time period definitions, for example, if you don't
want to send out any notification during the holiday season,
or if you only want to allow small time windows for executed checks.

The [TimePeriod object](9-object-types.md#objecttype-timeperiod)
provides the `includes` and `excludes` attributes to solve this issue.
`prefer_includes` defines whether included or excluded time periods are
preferred.

The following example defines a time period called `holidays` where
notifications should be supressed:

    object TimePeriod "holidays" {
      import "legacy-timeperiod"
    
      ranges = {
        "january 1" = "00:00-24:00"                 //new year's day
        "july 4" = "00:00-24:00"                    //independence day
        "december 25" = "00:00-24:00"               //christmas
        "december 31" = "18:00-24:00"               //new year's eve (6pm+)
        "2017-04-16" = "00:00-24:00"                //easter 2017
        "monday -1 may" = "00:00-24:00"             //memorial day (last monday in may)
        "monday 1 september" = "00:00-24:00"        //labor day (1st monday in september)
        "thursday 4 november" = "00:00-24:00"       //thanksgiving (4th thursday in november)
      }
    }

In addition to that the time period `weekends` defines an additional
time window which should be excluded from notifications:

    object TimePeriod "weekends-excluded" {
      import "legacy-timeperiod"
    
      ranges = {
        "saturday"  = "00:00-09:00,18:00-24:00"
        "sunday"    = "00:00-09:00,18:00-24:00"
      }
    }

The time period `prod-notification` defines the default time ranges
and adds the excluded time period names as an array.

    object TimePeriod "prod-notification" {
      import "legacy-timeperiod"
    
      excludes = [ "holidays", "weekends-excluded" ]
    
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


## <a id="use-functions-object-config"></a> Use Functions in Object Configuration

There is a limited scope where functions can be used as object attributes such as:

* As value for [Custom Attributes](3-monitoring-basics.md#custom-attributes-functions)
* Returning boolean expressions for [set_if](8-advanced-topics.md#use-functions-command-arguments-setif) inside command arguments
* Returning a [command](8-advanced-topics.md#use-functions-command-attribute) array inside command objects

The other way around you can create objects dynamically using your own global functions.

> **Note**
>
> Functions called inside command objects share the same global scope as runtime macros.
> Therefore you can access host custom attributes like `host.vars.os`, or any other
> object attribute from inside the function definition used for [set_if](8-advanced-topics.md#use-functions-command-arguments-setif) or [command](8-advanced-topics.md#use-functions-command-attribute).

Tips when implementing functions:

* Use [log()](18-library-reference.md#global-functions-log) to dump variables. You can see the output
inside the `icinga2.log` file depending in your log severity
* Use the `icinga2 console` to test basic functionality (e.g. iterating over a dictionary)
* Build them step-by-step. You can always refactor your code later on.

### <a id="use-functions-command-arguments-setif"></a> Use Functions in Command Arguments set_if

The `set_if` attribute inside the command arguments definition in the
[CheckCommand object definition](9-object-types.md#objecttype-checkcommand) is primarily used to
evaluate whether the command parameter should be set or not.

By default you can evaluate runtime macros for their existence. If the result is not an empty
string, the command parameter is passed. This becomes fairly complicated when want to evaluate
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

By defining `set_if` as [abbreviated lambda function](17-language-reference.md#nullary-lambdas)
and evaluating the host custom attribute `compellent` containing the `disks` this problem was
solved like this:

    object CheckCommand "check_compellent" {
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

This implementation uses the dictionary type method [contains](18-library-reference.md#dictionary-contains)
and will fail if `host.vars.compellent` is not of the type `Dictionary`.
Therefore you can extend the checks using the [typeof](17-language-reference.md#types) function.

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

This comes in handy for [NotificationCommands](9-object-types.md#objecttype-notificationcommand)
or [EventCommands](9-object-types.md#objecttype-eventcommand) which does not require
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

### <a id="custom-functions-as-attribute"></a> Use Custom Functions as Attribute

To use custom functions as attributes, the function must be defined in a
slightly unexpected way. The following example shows how to assign values
depending on group membership. All hosts in the `slow-lan` host group use 300
as value for `ping_wrta`, all other hosts use 100.

    globals.group_specific_value = function(group, group_value, non_group_value) {
        return function() use (group, group_value, non_group_value) {
            if (group in host.groups) {
                return group_value
            } else {
                return non_group_value
            }
        }
    }
    
    apply Service "ping4" {
        import "generic-service"
        check_command = "ping4"
    
        vars.ping_wrta = group_specific_value("slow-lan", 300, 100)
        vars.ping_crta = group_specific_value("slow-lan", 500, 200)
    
        assign where true
    }

### <a id="use-functions-assign-where"></a> Use Functions in Assign Where Expressions

If a simple expression for matching a name or checking if an item
exists in an array or dictionary does not fit, you should consider
writing your own global [functions](17-language-reference.md#functions).
You can call them inside `assign where` and `ignore where` expressions
for [apply rules](3-monitoring-basics.md#using-apply-expressions) or
[group assignments](3-monitoring-basics.md#group-assign-intro) just like
any other global functions for example [match](18-library-reference.md#global-functions-match).

The following example requires the host `myprinter` being added
to the host group `printers-lexmark` but only if the host uses
a template matching the name `lexmark*`.

    template Host "lexmark-printer-host" {
      vars.printer_type = "Lexmark"
    }

    object Host "myprinter" {
      import "generic-host"
      import "lexmark-printer-host"

      address = "192.168.1.1"
    }

    /* register a global function for the assign where call */
    globals.check_host_templates = function(host, search) {
      /* iterate over all host templates and check if the search matches */
      for (tmpl in host.templates) {
        if (match(search, tmpl)) {
          return true
        }
      }

      /* nothing matched */
      return false
    }

    object HostGroup "printers-lexmark" {
      display_name = "Lexmark Printers"
      /* call the global function and pass the arguments */
      assign where check_host_templates(host, "lexmark*")
    }


Take a different more complex example: All hosts with the
custom attribute `vars_app` as nested dictionary should be
added to the host group `ABAP-app-server`. But only if the
`app_type` for all entries is set to `ABAP`.

It could read as wildcard match for nested dictionaries:

    where host.vars.vars_app["*"].app_type == "ABAP"

The solution for this problem is to register a global
function which checks the `app_type` for all hosts
with the `vars_app` dictionary.

    object Host "appserver01" {
      check_command = "dummy"
      vars.vars_app["ABC"] = { app_type = "ABAP" }
    }
    object Host "appserver02" {
      check_command = "dummy"
      vars.vars_app["DEF"] = { app_type = "ABAP" }
    }

    globals.check_app_type = function(host, type) {
      /* ensure that other hosts without the custom attribute do not match */
      if (typeof(host.vars.vars_app) != Dictionary) {
        return false
      }

      /* iterate over the vars_app dictionary */
      for (key => val in host.vars.vars_app) {
        /* if the value is a dictionary and if contains the app_type being the requested type */
        if (typeof(val) == Dictionary && val.app_type == type) {
          return true
        }
      }

      /* nothing matched */
      return false
    }

    object HostGroup "ABAP-app-server" {
      assign where check_app_type(host, "ABAP")
    }

## <a id="access-object-attributes-at-runtime"></a> Access Object Attributes at Runtime

The [Object Accessor Functions](18-library-reference.md#object-accessor-functions)
can be used to retrieve references to other objects by name.

This allows you to access configuration and runtime object attributes. A detailed
list can be found [here](9-object-types.md#object-types).

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
          return 2 //something is broken
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
calculates the flapping threshold from a single value based on counters and
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
