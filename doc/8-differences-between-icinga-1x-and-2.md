# <a id="differences-1x-2"></a> Differences between Icinga 1.x and 2

## Configuration Format

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

    set IcingaEnableNotifications = 1,

    object Service "test" {
        enable_notifications = 0,
    }

### Sample Configuration and ITL

While Icinga 1.x ships sample configuration and templates spread in various
object files Icinga 2 moves all templates into the Icinga Template Library (ITL)
and includes that in the sample configuration.

The ITL will be updated on every release and should not be edited by the user.

> **Note**
>
> Sample configuration files are located in the `conf.d/` directory which is
> included in `icinga2.conf` by default.

### Include Files and Directories

In Icinga 1.x the `icinga.cfg` file contains `cfg_file` and `cfg_dir`
directives. The `cfg_dir` directive recursively includes all files with a `.cfg`
suffix in the given directory. Only absolute paths may be used. The `cfg_file`
and `cfg_dir` directives can include the same file twice which leads to
configuration errors in Icinga 1.x.

    cfg_file=/etc/icinga/objects/commands.cfg
    cfg_dir=/etc/icinga/objects

Icinga 2 supports wildcard includes and relative paths, e.g. for including
`conf.d/*.conf` in the same directory. A global search path for includes is
available for advanced features like the Icinga Template Library (ITL). The file
suffix does not matter as long as it matches the (wildcard) include expression.

    include "conf.d/*.conf"
    include <itl/itl.conf>

> **Best Practice**
>
> By convention the `.conf` suffix is used for Icinga 2 configuration files.

## Resource File and Global Macros

Global macros such as for the plugin directory, usernames and passwords can be
set in the `resource.cfg` configuration file in Icinga 1.x. By convention the
`USER1` macro is used to define the directory for the plugins.

Icinga 2 uses a global `IcingaMacros` variable which is set in the
`conf.d/macros.conf` file:

    /**
     * Global macros
    */
    set IcingaMacros = {
      plugindir = "/usr/lib/nagios/plugins"
    }


## Comments

In Icinga 1.x comments are made using a leading hash (`#`) or a semi-colon (`;`)
for inline comments.

In Icinga 2 comments can either be encapsulated by `/*` and `*/` (allowing for
multi-line comments) or starting with two slashes (`//`).

## Object names

Object names must not contain a colon (`:`). Use the `display_name` attribute
to specify user-friendly names which should be shown in UIs (supported by
Icinga 1.x Classic UI and Web).

Object names are not specified using attributes (e.g. `service_description` for
services) like in Icinga 1.x but directly after their type definition.

    define service {
        host_name  localhost
        service_description  ping4
    }

    object Service "localhost-ping4" { }

## Templates

In Icinga 1.x templates are identified using the `register 0` setting. Icinga 2
uses the `template` identifier:

    template Service "ping4-template" { }

Icinga 1.x objects inherit from templates using the `use` attribute.
Icinga 2 uses the keyword `inherits` after the object name and requires a
comma-separated list with template names in double quotes.

    define service {
        service_description testservice
        use                 tmpl1,tmpl2,tmpl3
    }

    object Service "testservice" inherits "tmpl1", "tmpl2", "tmpl3" {
    }

## Object attributes

Icinga 1.x separates attribute and value with whitespaces/tabs. Icinga 2
requires an equal sign (=) between them.

    define service {
        check_interval  5
    }

    object Service "test" {
        check_interval = 5m,
    }

> **Note**
>
> Please note that the default time value is seconds, if no duration literal
> is given. check_interval = 5 behaves the same as check_interval = 5s.

All strings require double quotes in Icinga 2. Therefore a double-quote
must be escaped with a backslash (e.g. in command line).
If an attribute identifier starts with a number, it must be encapsulated
with double quotes as well.

Unlike in Icinga 1.x all attributes within the current object must be
terminated with a comma (,).

### Alias vs. Display Name

In Icinga 1.x a host can have an `alias` and a `display_name` attribute used
for a more descriptive name. A service only can have a `display_name` attribute.
The `alias` is used for group, timeperiod, etc. objects too.
Icinga 2 only supports the `display_name` attribute which is also taken into
account by Icinga 1.x Classic UI and Web.

## Custom Attributes

### Action Url, Notes Url, Notes

Icinga 1.x objects support configuration attributes not required as runtime
values but for external ressources such as Icinga 1.x Classic UI or Web.
The `notes`, `notes_url`, `action_url`, `icon_image`, `icon_image_alt`
attributes for host and service objects, additionally `statusmap_image` and
`2d_coords` for the host's representation in status maps.

These attributes can be set using the `custom` dictionary in Icinga 2 `Host`
or `Service` objects:

    custom = {
        notes = "Icinga 2 is the best!",
        notes_url = "http://docs.icinga.org",
        action_url = "http://dev.icinga.org",
        icon_image = "../../images/logos/Stats2.png",
        icon_image_alt = "icinga2 alt icon text",
        "2d_coords" = "1,2",
        statusmap_image = "../../images/logos/icinga.gif",
    }

External interfaces will recognize and display these attributes accordingly.

### Custom Variables

Icinga 1.x custom variable attributes must be prefixed using an underscore (`_`).
In Icinga 2 these attributes must be added to the `custom`dictionary.

    custom = {
        DN = "cn=icinga2-dev-host,ou=icinga,ou=main,ou=IcingaConfig,ou=LConf,dc=icinga,dc=org",
        CV = "my custom cmdb description",
    }

> **Note**
>
> If you are planning to access custom variables as runtime macros you should
> add them to the macros dictionary instead!


## Host Service Relation

In Icinga 1.x a service object is associated with a host by defining the
`host_name` attribute in the service definition. Alternate methods refer
to `hostgroup_name` or behavior changing regular expression. It's not possible
to define a service definition within a host definition.

The preferred way of associating hosts with services in Icinga 2 are services
defined inline to the host object (or template) definition. Icinga 2 will
implicitely create a new service object on configuration activation. These
inline service definitions can reference service templates.
Linking a service to a host is still possible with the 'host' attribute in
a service object in Icinga 2.

## Users

Contacts have been renamed to Users (same for groups). A user does not
only provide attributes and macros used for notifications, but is also
used for authorization checks.

In Icinga 2 notification commands are not directly associated with users.
Instead the notification command is specified using `Notification` objects.

The `StatusDataWriter`, `IdoMySqlConnection` and `LivestatusListener` types will
provide the contact and contactgroups attributes for services for compatibility
reasons. These values are calculated from all services, their notifications,
and their users.

## Macros

Various object attributes and runtime variables can be accessed as macros in
commands in Icinga 1.x - Icinga 2 supports all required [macros](#macros).

> **Note**
>
> Due to the `contact`objects renamed to `user` objects the associated macros
> have changed.
> Furthermore an `alias` is now reflected as `display_name`. The Icinga 1.x
> notation is still supported for compatibility reasons.

  Icinga 1.x Name        | Icinga 2 Name
  -----------------------|--------------
  CONTACTNAME            | USERNAME
  CONTACTALIAS           | USERDISPLAYNAME
  CONTACTEMAIL           | USEREMAIL
  CONTACTPAGER           | USERPAGER


### Command Macros

If you have previously used Icinga 1.x you may already be familiar with
user and argument macros (e.g., `USER1` or `ARG1`). Unlike in Icinga 1.x macros
may have arbitrary names and arguments are no longer specified in the
`check_command` setting.

In Icinga 1.x argument macros are specified in the `check_command` attribute and
are separated from the command name using an exclamation mark (`!`).

    define command {
        command_name  ping4
        command_line  $USER1$/check_ping -H $HOSTADDRESS$ -w $ARG1$ -c $ARG2$ -p 5
    }

    define service {
        use                     local-service
        host_name               localhost
        service_description     PING
        check_command           ping4!100.0,20%!500.0,60%
    }

With the freely definable macros in Icinga 2 it looks like this:

    object CheckCommand "ping4" {
        command = "$plugindir$/check_ping -H $HOSTADDRESS$ -w $wrta$,$wpl%$ -c $crta$,$cpl%$",
    }

    object Service "PING" {
        check_command = "ping4",
        macros = {
            wrta = 100,
            wpl = 20,
            crta = 500,
            cpl = 60
        }
    }

> **Note**
>
> Tip: The above example uses the global $plugindir$ macro instead of the Icinga 1.x
> $USER1$ macro. It also replaces the Icinga 1.x notation with $ARGn$ with freely
> definable macros.

### Environment Macros

The global configuration setting `enable_environment_macros` does not exist in
Icinga 2.

Macros exported into the environment must be set using the `export_macros`
attribute in command objects.

## Checks

### Host Check

Unlike in Icinga 1.x hosts are not checkable objects in Icinga 2. Instead hosts
inherit their state from the service that is specified using the `check`
attribute.

### Check Output

Icinga 2 does not make a difference between `output` (first line) and
`long_output` (remaining lines) like in Icinga 1.x. Performance Data is
provided separately.

The `StatusDataWriter`, `IdoMysqlConnection` and `LivestatusListener` types
split the raw output into `output` (first line) and `long_output` (remaining
lines) for compatibility reasons.

### Initial State

Icinga 1.x uses the `max_service_check_spread` setting to specify a timerange
where the initial state checks must have happened. Icinga 2 will use the
`retry_interval` setting instead and `check_interval` divided by 5 if
`retry_interval` is not defined.

### Performance Data

There is no host performance data generated in Icinga 2 because there are no
real host checks. Therefore the PerfDataWriter will only write service
performance data files.

## Commands

Unlike in Icinga 1.x there are 3 different command types in Icinga 2:
`CheckCommand`, `NotificationCommand` and EventCommand`.

For example in Icinga 1.x it is possible to accidently use a notification
command as an event handler which might cause problems depending on which
macros are used in the notification command.

In Icinga 2 these command types are separated and will generate an error on
configuration validation if used in the wrong context.

While Icinga 2 still supports the complete command line in command objects, it's
also possible to encapsulate all arguments into double quotes and passing them
as array to the `command_line` attribute i.e. for better readability.

It's also possible to define default macros for the command itself which can be
overridden by a service macro.

## Groups

In Icinga 2 hosts, services and users are added to groups using the `groups`
attribute in the object. The old way of listing all group members in the group's
`members` attribute is not supported.

The preferred way of assigning objects to groups is by using a template:

    template Host "dev-host" {
      groups += [ "dev-hosts" ],

      services["http"] = {
        check_command = [ "http-ip" ]
      }
    }

    object Host "web-dev" inherits "dev-host" { }

Host groups in Icinga 2 cannot be used to associate services with all members
of that group. The example above shows how to use templates to accomplish
the same effect.

## Notifications

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
* Create notification A-SMS, set notification_command for sms, add user X,
  assign notification A-SMS to service A
* Create notification B-Mail, set notification_command for mail, add user X,
  assign notification Mail to service B
* Create notification C-SMS, set notification_command for sms, add user Y,
  assign notification C-SMS to service C
* Create notification C-Mail, set notification_command for mail, add user Y,
  assign notification C-Mail to service C

> **Note**
>
> Notification objects are not required to be service-agnostic. They may use
> global notification templates and can be added to a service wherever needed.

Previously in Icinga 1.x it looked like this:

    service -> (contact, contactgroup) -> notification command

In Icinga 2 it will look like this:

    Service -> Notification -> NotificationCommand
                            -> User, UserGroup

### Escalations

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

### Notification Options

Unlike Icinga 1.x with the 'notification_options' attribute with comma-separated
state and type filters, Icinga 2 uses two configuration attributes for that.
All state and type filter use long names or'd with a pipe together

    notification_options w,u,c,r,f,s

    notification_state_filter = (StateFilterWarning | StateFilterUnknown | StateFilterCritical),
    notification_type_filter = (NotificationProblem | NotificationRecovery | NotificationFlappingStart | NotificationFlappingEnd | NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)

> **Note**
>
> Please note that `NotificationProblem` as type is required for all problem
> notifications.

Icinga 2 adds more fine-grained type filters for acknowledgements, downtime
and flapping type (start, end, ...).

> **Note**
>
> Notification state and type filters are only valid configuration attributes for
> `Notification` and `User` objects.

## Dependencies and Parents

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

## Flapping

The Icinga 1.x flapping detection uses the last 21 states of a service. This
value is hardcoded and cannot be changed. The algorithm on determining a flapping state
is as follows:

    flapping value = (number of actual state changes / number of possible state changes)

The flapping value is then compared to the low and high flapping thresholds.

The algorithm used in Icinga 2 does not store the past states but calculcates the flapping
threshold from a single value based on counters and half-life values. Icinga 2 compares
the value with a single flapping threshold configuration attribute.

## Check Result Freshness

Freshness of check results must be explicitely enabled in Icinga 1.x. The attribute
`freshness_treshold` defines the threshold in seconds. Once the threshold is triggered, an
active freshness check is executed defined by the `check_command` attribute. Both check
methods (active and passive) use the same freshness check method.

In Icinga 2 active check freshness is determined by the `check_interval` attribute and no
incoming check results in that period of time (last check + check interval). Passive check
freshness is calculated from the `check_interval` attribute if set. There is no extra
`freshness_threshold` attribute in Icinga 2. If the freshness checks are invalid, a new
service check is forced.

## State Retention

Icinga 1.x uses the `retention.dat` file to save its state in order to be able
to reload it after a restart. In Icinga 2 this file is called `icinga2.state`.

The format objects are stored in is not compatible with Icinga 1.x.

## Logging

Icinga 1.x supports syslog facilities and writes its own `icinga.log` log file
and archives. These logs are used in Icinga 1.x Classic UI to generate
historical reports.

Icinga 2 compat library provides the CompatLogger object which writes the icinga.log and archive
in Icinga 1.x format in order to stay compatible with Classic UI and other addons.
The native Icinga 2 logging facilities are split into three configuration objects: SyslogLogger,
FileLogger, StreamLogger. Each of them got their own severity and target configuration.


## Broker Modules and Features

Icinga 1.x broker modules are incompatible with Icinga 2.

In order to provide compatibility with Icinga 1.x the functionality of several
popular broker modules was implemented for Icinga 2:

* IDOUtils
* Livestatus
* Cluster (allows for high availability and load balancing)

In Icinga 1.x broker modules may only be loaded once which means it is not easily possible
to have one Icinga instance write to multiple IDO databases. Due to the way
objects work in Icinga 2 it is possible to set up multiple IDO database instances.


## Distributed Monitoring

Icinga 1.x uses the native "obsess over host/service" method which requires the NSCA addon
passing the slave's checkresults passively onto the master's external command pipe.
While this method may be used for check load distribution, it does not provide any configuration
distribution out-of-the-box. Furthermore comments, downtimes and other stateful runtime data is
not synced between the master and slave nodes. There are addons available solving the check
and configuration distribution problems Icinga 1.x distributed monitoring currently suffers from.

Icinga 2 implements a new built-in distributed monitoring architecture, including config and check
distribution, IPv4/IPv6 support, SSL certificates and domain support for DMZ. High Availability
and load balancing are also part of the Icinga 2 Cluster setup.
