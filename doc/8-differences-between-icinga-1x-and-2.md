# <a id="differences-1x-2"></a> Differences between Icinga 1.x and 2

## <a id="differences-1x-2-configuration-format"></a> Configuration Format

Icinga 1.x supports two configuration formats: key-value-based settings in the
`icinga.cfg` configuration file and object-based in included files (`cfg_dir`,
`cfg_file`). The path to the `icinga.cfg` configuration file must be passed to
the Icinga daemon at startup.

    enable_notifications=1

    define service {
       notifications_enabled    0
    }

Icinga 2 supports objects and (global) variables, but does not make a difference
if it's the main configuration file, or any included file.

    const EnableNotifications = true

    object Service "test" {
        enable_notifications = 0
    }

### <a id="differences-1x-2-sample-configuration-itl"></a> Sample Configuration and ITL

While Icinga 1.x ships sample configuration and templates spread in various
object files Icinga 2 moves all templates into the Icinga Template Library (ITL)
and includes that in the sample configuration.

The ITL will be updated on every release and should not be edited by the user.

There are still generic templates available for your convenience which may or may
not be re-used in your configuration. For instance, `generic-service` includes
all required attributes except `check_command` for an inline service.

Sample configuration files are located in the `conf.d/` directory which is
included in `icinga2.conf` by default.

### <a id="differences-1x-2-include-files-dirs"></a> Include Files and Directories

In Icinga 1.x the `icinga.cfg` file contains `cfg_file` and `cfg_dir`
directives. The `cfg_dir` directive recursively includes all files with a `.cfg`
suffix in the given directory. Only absolute paths may be used. The `cfg_file`
and `cfg_dir` directives can include the same file twice which leads to
configuration errors in Icinga 1.x.

    cfg_file=/etc/icinga/objects/commands.cfg
    cfg_dir=/etc/icinga/objects

Icinga 2 supports wildcard includes and relative paths, e.g. for including
`conf.d/*.conf` in the same directory.

    include "conf.d/*.conf"

If you want to include files and directories recursively, you need to define
a separate option and add the directory and an option pattern.

    include_recursive "conf.d" "*.conf"

A global search path for includes is available for advanced features like
the Icinga Template Library (ITL). The file suffix does not matter as long
as it matches the (wildcard) include expression.

    include <itl/itl.conf>

By convention the `.conf` suffix is used for Icinga 2 configuration files.

## <a id="differences-1x-2-resource-file-global-macros"></a> Resource File and Global Macros

Global macros such as for the plugin directory, usernames and passwords can be
set in the `resource.cfg` configuration file in Icinga 1.x. By convention the
`USER1` macro is used to define the directory for the plugins.

Icinga 2 uses global constants instead. In the default config these are
set in the `constants.conf` configuration file:

    /**
     * This file defines global constants which can be used in
     * the other configuration files. At a minimum the
     * PluginDir constant should be defined.
     */

    const PluginDir = "/usr/lib/nagios/plugins"

[Global macros](#global-constants) can only be defined once. Trying to modify a
global constant will result in an error.

## <a id="differences-1x-2-configuration-comments"></a> Configuration Comments

In Icinga 1.x comments are made using a leading hash (`#`) or a semi-colon (`;`)
for inline comments.

In Icinga 2 comments can either be encapsulated by `/*` and `*/` (allowing for
multi-line comments) or starting with two slashes (`//`).

## <a id="differences-1x-2-object-names"></a> Object names

Object names must not contain a colon (`!`). Use the `display_name` attribute
to specify user-friendly names which should be shown in UIs (supported by
Icinga 1.x Classic UI and Web).

Object names are not specified using attributes (e.g. `service_description` for
services) like in Icinga 1.x but directly after their type definition.

    define service {
        host_name  localhost
        service_description  ping4
    }

    object Service "ping4" {
      host_name = "localhost"
    }

## <a id="differences-1x-2-templates"></a> Templates

In Icinga 1.x templates are identified using the `register 0` setting. Icinga 2
uses the `template` identifier:

    template Service "ping4-template" { }

Icinga 1.x objects inherit from templates using the `use` attribute.
Icinga 2 uses the keyword `import` with template names in double quotes.

    define service {
        service_description testservice
        use                 tmpl1,tmpl2,tmpl3
    }

    object Service "testservice" {
      import "tmpl1"
      import "tmpl2"
      import "tmpl3"
    }

## <a id="differences-1x-2-object-attributes"></a> Object attributes

Icinga 1.x separates attribute and value with whitespaces/tabs. Icinga 2
requires an equal sign (=) between them.

    define service {
        check_interval  5
    }

    object Service "test" {
        check_interval = 5m
    }

Please note that the default time value is seconds, if no duration literal
is given. `check_interval = 5` behaves the same as `check_interval = 5s`.

All strings require double quotes in Icinga 2. Therefore a double-quote
must be escaped with a backslash (e.g. in command line).
If an attribute identifier starts with a number, it must be encapsulated
with double quotes as well.

### <a id="differences-1x-2-alias-display-name"></a> Alias vs. Display Name

In Icinga 1.x a host can have an `alias` and a `display_name` attribute used
for a more descriptive name. A service only can have a `display_name` attribute.
The `alias` is used for group, timeperiod, etc. objects too.
Icinga 2 only supports the `display_name` attribute which is also taken into
account by Icinga 1.x Classic UI and Web.

## <a id="differences-1x-2-custom-attributes"></a> Custom Attributes

Icinga 2 allows you to define custom attributes in the `vars` dictionary.
The `notes`, `notes_url`, `action_url`, `icon_image`, `icon_image_alt`
attributes for host and service objects are still available in Icinga 2.

`2d_coords` and `statusmap_image` are not supported in Icinga 2.

### <a id="differences-1x-2-custom-variables"></a> Custom Variables

Icinga 1.x custom variable attributes must be prefixed using an underscore (`_`).
In Icinga 2 these attributes must be added to the `vars` dictionary as custom attributes.

    vars.dn = "cn=icinga2-dev-host,ou=icinga,ou=main,ou=IcingaConfig,ou=LConf,dc=icinga,dc=org"
    vars.cv = "my custom cmdb description"

## <a id="differences-1x-2-host-service-relation"></a> Host Service Relation

In Icinga 1.x a service object is associated with a host by defining the
`host_name` attribute in the service definition. Alternate methods refer
to `hostgroup_name` or behavior changing regular expression. It's not possible
to define a service definition within a host definition.

The preferred way of associating hosts with services in Icinga 2 is by
using the `apply` keyword.

## <a id="differences-1x-2-users"></a> Users

Contacts have been renamed to Users (same for groups). A user does not
only provide attributes and custom attributes used for notifications, but is also
used for authorization checks.

In Icinga 2 notification commands are not directly associated with users.
Instead the notification command is specified using `Notification` objects.

The `StatusDataWriter`, `IdoMySqlConnection` and `LivestatusListener` types will
provide the contact and contactgroups attributes for services for compatibility
reasons. These values are calculated from all services, their notifications,
and their users.

## <a id="differences-1x-2-macros"></a> Macros

Various object attributes and runtime variables can be accessed as macros in
commands in Icinga 1.x - Icinga 2 supports all required [custom attributes](#custom-attributes).

### <a id="differences-1x-2-command-arguments"></a> Command Arguments

If you have previously used Icinga 1.x you may already be familiar with
user and argument definitions (e.g., `USER1` or `ARG1`). Unlike in Icinga 1.x
the Icinga 2 custom attributes may have arbitrary names and arguments are no
longer specified in the `check_command` setting.

In Icinga 1.x arguments are specified in the `check_command` attribute and
are separated from the command name using an exclamation mark (`!`).

    define command {
        command_name  ping4
        command_line  $USER1$/check_ping -H $address$ -w $ARG1$ -c $ARG2$ -p 5
    }

    define service {
        use                     local-service
        host_name               localhost
        service_description     PING
        check_command           ping4!100.0,20%!500.0,60%
    }

With the freely definable custom attributes in Icinga 2 it looks like this:

    object CheckCommand "ping4" {
        command = PluginDir + "/check_ping -H $address$ -w $wrta$,$wpl%$ -c $crta$,$cpl%$"
    }

    object Service "PING" {
        check_command = "ping4"
        vars.wrta = 100
        vars.wpl = 20
        vars.crta = 500
        vars.cpl = 60
    }

### <a id="differences-1x-2-environment-macros"></a> Environment Macros

The global configuration setting `enable_environment_macros` does not exist in
Icinga 2.

Macros exported into the environment must be set using the `env`
attribute in command objects.

### <a id="differences-1x-2-runtime-macros"></a> Runtime Macros

Icinga 2 requires an object specific namespace when accessing configuration
and stateful runtime macros. Custom attributes can be access directly.

Changes to host runtime macros

   Icinga 1.x             | Icinga 2
   -----------------------|----------------------
   USERNAME               | user.name
   USERDISPLAYNAME        | user.displayname
   USEREMAIL              | email if set as `email` custom attribute.
   USERPAGER              | pager if set as `pager` custom attribute.


Changes to service runtime macros

   Icinga 1.x             | Icinga 2
   -----------------------|----------------------
   SERVICEDESC            | service.description
   SERVICEDISPLAYNAME     | service.displayname
   SERVICECHECKCOMMAND    | service.checkcommand
   SERVICESTATE           | service.state
   SERVICESTATEID         | service.stateid
   SERVICESTATETYPE       | service.statetype
   SERVICEATTEMPT         | service.attempt
   MAXSERVICEATTEMPT      | service.maxattempt
   LASTSERVICESTATE       | service.laststate
   LASTSERVICESTATEID     | service.laststateid
   LASTSERVICESTATETYPE   | service.laststatetype
   LASTSERVICESTATECHANGE | service.laststatechange
   SERVICEDURATIONSEC     | service.durationsec
   SERVICELATENCY         | service.latency
   SERVICEEXECUTIONTIME   | service.executiontime
   SERVICEOUTPUT          | service.output
   SERVICEPERFDATA        | service.perfdata
   LASTSERVICECHECK       | service.lastcheck


Changes to user (contact) runtime macros

   Icinga 1.x             | Icinga 2
   -----------------------|----------------------
   HOSTNAME               | host.name
   HOSTDISPLAYNAME        | host.displayname
   HOSTALIAS              | ..
   HOSTSTATE              | host.state
   HOSTSTATEID            | host.stateid
   HOSTSTATETYPE          | host.statetype
   HOSTATTEMPT            | host.attempt
   MAXHOSTATTEMPT         | host.maxattempt
   LASTHOSTSTATE          | host.laststate
   LASTHOSTSTATEID        | host.laststateid
   LASTHOSTSTATETYPE      | host.laststatetype
   LASTHOSTSTATECHANGE    | host.laststatechange
   HOSTDURATIONSEC        | host.durationsec
   HOSTLATENCY            | host.latency
   HOSTEXECUTIONTIME      | host.executiontime
   HOSTOUTPUT             | host.output
   HOSTPERFDATA           | host.perfdata
   LASTHOSTCHECK          | host.lastcheck
   HOSTADDRESS            | --
   HOSTADDRESS6           | --

Changes to global runtime macros:

   Icinga 1.x             | Icinga 2
   -----------------------|----------------------
   TIMET                  | icinga.timet
   LONGDATETIME           | icinga.longdatetime
   SHORTDATETIME          | icinga.shortdatetime
   DATE                   | icinga.date
   TIME                   | icinga.time


## <a id="differences-1x-2-external-commands"></a> External Commands

`CHANGE_CUSTOM_CONTACT_VAR` was renamed to `CHANGE_CUSTOM_USER_VAR`.
`CHANGE_CONTACT_MODATTR` was renamed to `CHANGE_USER_MODATTR`.

The following external commands are not supported:

    CHANGE_CONTACT_HOST_NOTIFICATION_TIMEPERIOD
    CHANGE_HOST_NOTIFICATION_TIMEPERIOD
    CHANGE_SVC_NOTIFICATION_TIMEPERIOD
    DEL_DOWNTIME_BY_HOSTGROUP_NAME
    DEL_DOWNTIME_BY_START_TIME_COMMENT
    DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST
    DISABLE_CONTACT_HOST_NOTIFICATIONS
    DISABLE_CONTACT_SVC_NOTIFICATIONS
    DISABLE_CONTACTGROUP_HOST_NOTIFICATIONS
    DISABLE_CONTACTGROUP_SVC_NOTIFICATIONS
    DISABLE_FAILURE_PREDICTION
    DISABLE_HOST_AND_CHILD_NOTIFICATIONS
    DISABLE_HOST_FRESHNESS_CHECKS
    DISABLE_HOST_SVC_NOTIFICATIONS
    DISABLE_NOTIFICATIONS_EXPIRE_TIME
    DISABLE_SERVICE_FRESHNESS_CHECKS
    ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST
    ENABLE_CONTACT_HOST_NOTIFICATIONS
    ENABLE_CONTACT_SVC_NOTIFICATIONS
    ENABLE_CONTACTGROUP_HOST_NOTIFICATIONS
    ENABLE_CONTACTGROUP_SVC_NOTIFICATIONS
    ENABLE_FAILURE_PREDICTION
    ENABLE_HOST_AND_CHILD_NOTIFICATIONS
    ENABLE_HOST_FRESHNESS_CHECKS
    ENABLE_HOST_SVC_NOTIFICATIONS
    ENABLE_SERVICE_FRESHNESS_CHECKS
    READ_STATE_INFORMATION
    SAVE_STATE_INFORMATION
    SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME
    SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME
    SET_HOST_NOTIFICATION_NUMBER
    SET_SVC_NOTIFICATION_NUMBER
    START_ACCEPTING_PASSIVE_HOST_CHECKS
    START_ACCEPTING_PASSIVE_SVC_CHECKS
    START_OBSESSING_OVER_HOST
    START_OBSESSING_OVER_HOST_CHECKS
    START_OBSESSING_OVER_SVC
    START_OBSESSING_OVER_SVC_CHECKS
    STOP_ACCEPTING_PASSIVE_HOST_CHECKS
    STOP_ACCEPTING_PASSIVE_SVC_CHECKS
    STOP_OBSESSING_OVER_HOST
    STOP_OBSESSING_OVER_HOST_CHECKS
    STOP_OBSESSING_OVER_SVC
    STOP_OBSESSING_OVER_SVC_CHECKS

## <a id="differences-1x-2-checks"></a> Checks

### <a id="differences-1x-2-check-output"></a> Check Output

Icinga 2 does not make a difference between `output` (first line) and
`long_output` (remaining lines) like in Icinga 1.x. Performance Data is
provided separately.

The `StatusDataWriter`, `IdoMysqlConnection` and `LivestatusListener` types
split the raw output into `output` (first line) and `long_output` (remaining
lines) for compatibility reasons.

### <a id="differences-1x-2-initial-state"></a> Initial State

Icinga 1.x uses the `max_service_check_spread` setting to specify a timerange
where the initial state checks must have happened. Icinga 2 will use the
`retry_interval` setting instead and `check_interval` divided by 5 if
`retry_interval` is not defined.

## <a id="differences-1x-2-comments"></a> Comments

Icinga 2 doesn't support non-persistent comments.

## <a id="differences-1x-2-commands"></a> Commands

Unlike in Icinga 1.x there are 3 different command types in Icinga 2:
`CheckCommand`, `NotificationCommand` and EventCommand`.

For example in Icinga 1.x it is possible to accidently use a notification
command as an event handler which might cause problems depending on which
runtime macros are used in the notification command.

In Icinga 2 these command types are separated and will generate an error on
configuration validation if used in the wrong context.

While Icinga 2 still supports the complete command line in command objects, it's
also possible to encapsulate all arguments into double quotes and passing them
as array to the `command_line` attribute i.e. for better readability.

It's also possible to define default custom attributes for the command itself which can be
overridden by a service macro.

## <a id="differences-1x-2-groups"></a> Groups

In Icinga 2 hosts, services and users are added to groups using the `groups`
attribute in the object. The old way of listing all group members in the group's
`members` attribute is not supported.

The preferred way of assigning objects to groups is by using a template:

    template Host "dev-host" {
      groups += [ "dev-hosts" ]
    }

    object Host "web-dev" {
      import "dev-host"
    }

In order to associate a service with all hosts in a host group the `apply`
keyword can be used:

    apply Service "ping" {
      import "generic-service"

      check_command = "ping4"

      assign where "group" in host.groups
    }

## <a id="differences-1x-2-notifications"></a> Notifications

Notifications are a new object type in Icinga 2. Imagine the following
notification configuration problem in Icinga 1.x:

* Service A should notify contact X via SMS
* Service B should notify contact X via Mail
* Service C should notify contact Y via Mail and SMS
* Contact X and Y should also be used for authorization (e.g. in Classic UI)

The only way achieving a semi-clean solution is to

* Create contact X-sms, set service_notification_command for sms, assign contact
  to service A
* Create contact X-mail, set service_notification_command for mail, assign
  contact to service B
* Create contact Y, set service_notification_command for sms and mail, assign
  contact to service C
* Create contact X without notification commands, assign to service A and B

Basically you are required to create duplicated contacts for either each
notification method or used for authorization only.

Icinga 2 attempts to solve that problem in this way

* Create user X, set SMS and Mail attributes, used for authorization
* Create user Y, set SMS and Mail attributes, used for authorization
* Create notification A-SMS, set command for sms, add user X,
  assign notification A-SMS to service A
* Create notification B-Mail, set command for mail, add user X,
  assign notification Mail to service B
* Create notification C-SMS, set command for sms, add user Y,
  assign notification C-SMS to service C
* Create notification C-Mail, set command for mail, add user Y,
  assign notification C-Mail to service C

Previously in Icinga 1.x it looked like this:

    service -> (contact, contactgroup) -> notification command

In Icinga 2 it will look like this:

    Service -> Notification -> NotificationCommand
                            -> User, UserGroup

### <a id="differences-1x-2-escalations"></a> Escalations

Escalations in Icinga 1.x require a separated object matching on existing
objects. Escalations happen between a defined start and end time which is
calculated from the notification_interval:

    start = notification start + (notification_interval * first_notification)
    end = notification start + (notification_interval * last_notification)

In theory first_notification and last_notification can be set to readable
numbers. In practice users are manipulating those attributes in combination
with notification_interval in order to get a start and end time.

In Icinga 2 the notification object can be used as notification escalation
if the start and end times are defined within the 'times' attribute using
duration literals (e.g. 30m).

The Icinga 2 escalation does not replace the current running notification.
In Icinga 1.x it's required to copy the contacts from the service notification
to the escalation to garantuee the normal notifications once an escalation
happens.
That's not necessary with Icinga 2 only requiring an additional notification
object for the escalation itself.

### <a id="differences-1x-2-notification-options"></a> Notification Options

TODO

Unlike Icinga 1.x with the 'notification_options' attribute with comma-separated
state and type filters, Icinga 2 uses two configuration attributes for that.
All state and type filter use long names or'd with a pipe together

    notification_options w,u,c,r,f,s

    states = [ Warning, Unknown, Critical ]
    filters = [ Problem, Recovery, FlappingStart, FlappingEnd, DowntimeStart, DowntimeEnd, DowntimeRemoved ]

Icinga 2 adds more fine-grained type filters for acknowledgements, downtime
and flapping type (start, end, ...).

## <a id="differences-1x-2-dependencies-parents"></a> Dependencies and Parents

In Icinga 1.x it's possible to define host parents to determine network reachability
and keep a host's state unreachable rather than down.
Furthermore there are host and service dependencies preventing unnecessary checks and
notifications. A host must not depend on a service, and vice versa. All dependencies
are configured as separate objects and cannot be set directly on the host or service
object.

Icinga 2 adds host and service dependencies as attribute directly onto the host or
service object or template. A service can now depend on a host, and vice versa. A
service has an implicit dependeny (parent) to its host. A host to host dependency acts
implicit as host parent relation.

The `StatusDataWriter`, `IdoMysqlConnection` and `LivestatusListener` types
support the Icinga 1.x schema with dependencies and parent attributes for
compatibility reasons.

## <a id="differences-1x-2-flapping"></a> Flapping

The Icinga 1.x flapping detection uses the last 21 states of a service. This
value is hardcoded and cannot be changed. The algorithm on determining a flapping state
is as follows:

    flapping value = (number of actual state changes / number of possible state changes)

The flapping value is then compared to the low and high flapping thresholds.

The algorithm used in Icinga 2 does not store the past states but calculcates the flapping
threshold from a single value based on counters and half-life values. Icinga 2 compares
the value with a single flapping threshold configuration attribute.

## <a id="differences-1x-2-check-result-freshness"></a> Check Result Freshness

Freshness of check results must be explicitely enabled in Icinga 1.x. The attribute
`freshness_treshold` defines the threshold in seconds. Once the threshold is triggered, an
active freshness check is executed defined by the `check_command` attribute. Both check
methods (active and passive) use the same freshness check method.

In Icinga 2 active check freshness is determined by the `check_interval` attribute and no
incoming check results in that period of time (last check + check interval). Passive check
freshness is calculated from the `check_interval` attribute if set. There is no extra
`freshness_threshold` attribute in Icinga 2. If the freshness checks are invalid, a new
service check is forced.

## <a id="differences-1x-2-state-retention"></a> State Retention

Icinga 1.x uses the `retention.dat` file to save its state in order to be able
to reload it after a restart. In Icinga 2 this file is called `icinga2.state`.

The format objects are stored in is not compatible with Icinga 1.x.

## <a id="differences-1x-2-logging"></a> Logging

Icinga 1.x supports syslog facilities and writes its own `icinga.log` log file
and archives. These logs are used in Icinga 1.x Classic UI to generate
historical reports.

Icinga 2 compat library provides the CompatLogger object which writes the icinga.log and archive
in Icinga 1.x format in order to stay compatible with Classic UI and other addons.
The native Icinga 2 logging facilities are split into three configuration objects: SyslogLogger,
FileLogger, StreamLogger. Each of them got their own severity and target configuration.


## <a id="differences-1x-2-broker-modules-features"></a> Broker Modules and Features

Icinga 1.x broker modules are incompatible with Icinga 2.

In order to provide compatibility with Icinga 1.x the functionality of several
popular broker modules was implemented for Icinga 2:

* IDOUtils
* Livestatus
* Cluster (allows for high availability and load balancing)

In Icinga 1.x broker modules may only be loaded once which means it is not easily possible
to have one Icinga instance write to multiple IDO databases. Due to the way
objects work in Icinga 2 it is possible to set up multiple IDO database instances.


## <a id="differences-1x-2-distributed-monitoring"></a> Distributed Monitoring

Icinga 1.x uses the native "obsess over host/service" method which requires the NSCA addon
passing the slave's checkresults passively onto the master's external command pipe.
While this method may be used for check load distribution, it does not provide any configuration
distribution out-of-the-box. Furthermore comments, downtimes and other stateful runtime data is
not synced between the master and slave nodes. There are addons available solving the check
and configuration distribution problems Icinga 1.x distributed monitoring currently suffers from.

Icinga 2 implements a new built-in distributed monitoring architecture, including config and check
distribution, IPv4/IPv6 support, SSL certificates and domain support for DMZ. High Availability
and load balancing are also part of the Icinga 2 [Cluster](#cluster) setup.

