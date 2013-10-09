# Migrating from Icinga 1.x

## Configuration Migration

There are plenty of differences and behavior changes introduced with the new Icinga 2.x
configuration format. In order to quickly surpass the issue, Icinga 2.x ships its own
config conversion script.

### Configuration Conversion Script

Since Icinga 1.x configuration provides multiple ways of creating a valid configuration
it may not work for all the use cases out there. 

> **Note**
>
> While automated conversion will quickly create a working Icinga 2.x configuration
> it does not keep the original organisation structure or any special kind of
> group or template logic. Please keep that mind when using the script.

The config conversion script provides support for basic Icinga 1.x
configuration format conversion to native Icinga 2.x configuration syntax.

It won't just compile all objects and drop them at once, but keep your
existing 1.x template structure, only adding a new host->service link relation.

The script will also detect the "attach service to hostgroup and put
hosts as members" trick from 1.x and convert that into Icinga2's template
system.

Furthermore the old "service with contacts and notification commands" logic
will be converted into Icinga2's logic with new notification objects,
allowing to define notifications directly on the service definition then.

Commands will be split by type (Check, Event, Notification) and relinked where
possible. The host's check_command is dropped, and a possible host check service
looked up, if possible. Otherwise a new service object will be added and linked.

Notifications will be built based on the service->contact relations, and
escalations will also be merged into notifications, having times begin/end as
additional attributes. Notification options will be converted into the new Icinga2
logic.

Dependencies and Parents are resolved into host and service dependencies with
many objects tricks as well.

Timeperiods will be converted as is, because Icinga2's ITL provides the legacy-timeperiod
template which supports that format for compatibility reasons.

Custom attributes like custom variables, *_urls, etc will be collected into the
custom dictionary, while possible macros are automagically converted into the macro
dictionary (freely definable macros in Icinga 2.x).

All required templates will be inherited from Icinga2's Template Library (ITL).

Regular expressions are not supported, also for the reason that this is optional in Icinga 1.x

> **Note**
>
> Please check the provided README for additional NOTES and possible caveats.

    # cd tools/configconvert
    # ./icinga2_convert_v1_v2.pl -c /etc/icinga/icinga.cfg -o conf/


### Manual Config Conversion

For a long term migration of your configuration you should consider recreating your
configuration based on the Icinga 2.x proposed way of doing configuration right.

> **Note**
>
> Configuration addons in the Icinga ecosystem should be able to adopt the new
> Icinga 2.x configuration format soon but please ask them kindly while waiting :)

Please read the next chapter to get an idea about the differences between 1.x and 2.x




# Differences between Icinga 1.x and 2.x

## Configuration Format

Icinga 1.x supports two configuration formats: key-value-based in icinga.cfg and
object based in included files (cfg_dir, cfg_file). The icinga.cfg must be passed
onto the Icinga daemon at startup.

    enable_notifications=1
    
    define service {
       notifications_enabled=0
    }

Icinga 2.x supports objects and (global) variables, but does not make a difference if it's
the "main" configuration file, or any included file.

    set IcingaEnableNotifications = 1,
    
    object Service "test" {
        enable_notifications = 0,
    }
    

### Sample Configuration and ITL

While Icinga 1.x ships sample configuration and templates spread in various object files
Icinga 2.x moves all templates into the Icinga Template Library (ITL) and includes that
in the sample configuration.

The ITL will be updated on every releases and must not be edited by the user.

> **Note**
>
> Sample configuration is located in conf.d/ which is included in icinga2.conf by default.

### Include Files and Directories

In Icinga 1.x icinga.cfg contains cfg_file and cfg_dir keys. cfg_dir recursively includes all
files with a .cfg suffix in the given directory. All paths must be configured absolute,
relative paths are not possible. cfg_file and cfg_dir can include the same file twice which
leads into configuration errors in Icinga 1.x.

    cfg_file=/etc/icinga/objects/commands.cfg
    cfg_dir=/etc/icinga/objects

Icinga 2.x supports wildcard includes and relative paths for e.g. including conf.d/*.conf
in the same directory. A global search path for includes is available for advanced features
like the Icinga Template Library (ITL). The file suffix does not matter as long as it matches
the (wildcard) include expression.

    include "conf.d/*.conf"
    include <itl/itl.conf>


## Resource File and Global Macros

Adding global macros such as plugin directory, or hidden passwords/community strings
happens with user macros in resource.cfg in Icinga 1.x. By default the USER1 macro
is used to define the directory for the plugins.

Icinga 2.x uses a global IcingaMacros dictionary and ships that by default in
conf.d/macros.conf

    /**
     * Global macros
    */
    set IcingaMacros = {
      plugindir = "/usr/lib/nagios/plugins"
    }


## Comments

In Icinga 1.x comments are made using a leading hash (#) or a semi-colon (;)
for inline comments.
In Icinga 2.x comments can either be encapsulated by /* ... */ (multiple lines
possible) or starting with two slashes (//). If you're familiar with C/C++
you'll get the idea.

## Object names

Object names must not contain a colon (:). Use the 'display_name' attribute
for detailed identifiers and gui representation (supported by Icinga 1.x Classic
UI and Web).

Object names are not defined as attribute like in Icinga 1.x but directly after
their type definition.

    define service {
        host_name  localhost
        service_description  ping4
    }    

    object Service "localhost-ping4" { }

    
## Templates

In Icinga 1.x templates are identified by 'register 0' not registering the
final object. Icinga 2.x uses the "template" identifier.

    template Service "ping4-template" { }
    
Icinga 1.x objects literally "use" template objects in a comma seperated list.
Icinga 2.x uses the keyword "inherits" after the object name and requires a
comma seperated list with template names in double quotes.

    define service {
        service_description testservice
        use                 tmpl1,tmpl2,tmpl3
    }
    
    object Service "testservice" inherits "tmpl1", "tmpl2", "tmpl3" {
    }

## Object attributes

### Attribute values

Icinga 1.x seperates attribute and value with whitespaces/tabs. Icinga 2.x
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

### Attribute strings

All strings require double quotes in Icinga 2.x. Therefore a double-quote
must be escaped with a backslash (e.g. in command line).
If an attribute identifier starts with a number, it must be encapsulated
with double quotes as well.

### Attribute lines

Unlike in Icinga 1.x all attributes within the current object must be
terminated with a comma (,).


## Host Service Relation

In Icinga 1.x a service object is linked to a host by defining the 'host_name'
attribute in the service definition. Alternate object tricks refer to
'hostgroup_name' or behavior changing regular expression. It's not possible
to define a service definition within a host definition, or even link from
a host definition to services.

The preferred way of linking hosts with services in Icinga 2.x are services
defined inline to the host object (or template) definition. Icinga 2.x will
implicitely create a new service object on configuration activation. These
inline service definitions can reference service templates.
Linking a service to a host is still possible with the 'host' attribute in
a service object in Icinga 2.x

### Service Hostgroup to Hosts Trick

A common pattern in Icinga 1.x is to add services to hostgroups. When a host
is added as hostgroup member, it will automatically collect all services linked
to that hostgroup. Inheriting services from a parent hostgroup to a member
hostgroup does not work.

    define hostgroup {
        hostgroup_name      testhg
    }
    define service {
        service_description testservice
        hostgroup_name      testhg
    }
    define host {
        host_name           testhost
        hostgroups          testhg
    }
    
Since it's possible to define services inline in a host template object and
inherit them to actual host objects in Icinga 2.x the preferred method works
like: Create a host template (acting as "hostgroup" relation) and define all
services (either inline, or reference a service template). Then let all hosts
inherit from that host template, collecting all service relations.

    template Host "testhg" {
        services["testservice"] = {
            ... 
        }
    }
    
    object Host "testhost" inherits "testhg" {
        ...
    }
    
Hostgroups in Icinga 2.x are only used for grouping the views but must not
be used for host service relation building.
    
> **Note**
>
> It's also possible to modify attributes in the host's service array inherited
> from the host template. E.g. macros.




## Users

Contacts have been renamed to Users (same for groups). A user does not
only provide attributes and macros used for notifications, but is also
used for authorization checks.
The revamped notification logic removes the notification commands from
the contacts (users) too.

StatusDataWriter, IdoMySqlConnection and LivestatusListener will provide
the contact and contactgroups attributes for services for compatibility
reasons. These values are calculated from all services, their notifications,
and their users.


## Macros

### Command Macros

If you have previously used Icinga 1.x you may already be familiar with
user and argument macros (e.g., USER1 or ARG1). Unlike in Icinga 1.x macros
may have arbitrary names and arguments are no longer specified in the
check_command setting.

In Icinga 1.x the 2 argument macros will be passed onto the 'check_command'
attribute seperated by an exclamation mark (!).

    define command {
        command_name  ping4
        command_line  $USER1$/check_ping -H $HOSTADDRESS$ -w $ARG1$ -c $ARG2$ -p 5
    }
    
    define service{
        use                     local-service
        host_name               localhost
        service_description     PING
        check_command           check_ping!100.0,20%!500.0,60%
    }

In Icinga 2.x 

### Environment Macros

The global configuration setting 'enable_environment_macros' does not exist in
Icinga 2.x.
Macros exported into the environment must be set using the 'export_macros' attribute
in command objects.



## Checks

### Host Check

Unlike in Icinga 1.x hosts are not checkable objects in Icinga 2. Instead hosts
inherit their state from the service that is specified using the `check` attribute.

### Check Output

Icinga 2.x does not make a difference between output (first line) and long_output
(remaining lines) like in Icinga 1.x. Performance Data is provided seperately.

StatusDataWriter, IdoMysqlConnection, LivestatusListener split the raw output into
output (first line) and long_output (remaining lines) for compatibility reasons.

### Initial State

Icinga 1.x uses max_service_check_spread to define a timerange where the initial
state checks must have happened. Icinga 2.x will use the retry_interval instead
and check_interval / 5 if not defined.

### Performance Data

There is no host performance data generated in Icinga 2.x because there are no
real host checks anymore. Therefore the PerfDataWriter will only write service
performance data files.


## Commands

Unlike in Icinga 1.x there are 3 different command types in Icinga 2: CheckCommand,
NotificationCommand, EventCommand.
Previously it was possible to accidently use e.g. a notification command as event
handler, generating problems with macro resolution and so on.
In Icinga 2.x those types are seperated and will generate an error on configuration
validation if used in the wrong context.

While Icinga 2.x still supports the complete command line in command objects, it's
also possible to encapsulate all arguments into double quotes and passing them as
array to the 'command_line' attribute i.e. for better readability.

It's also possible to define default (argument) macros for the command itsself which
can be overridden by a service (argument) macro.


## Groups

### Group Membership

Assigning members to hostgroups, servicegroups, usergroups is done only at the
host/service/user object using the 'groups' attribute. The old method defining
that directly as group attribute is not supported. Better use templates inheriting
the 'groups' attribute to all your objects.

### Hostgroup with Services

Hostgroups are used for grouping only, and cannot be used for object tricks like in
Icinga 1.x. 


## Notifications

Notifications are a new object type in Icinga 2.x. Imagine the following
notification configuration problem in Icinga 1.x:

Service A should notify contact X via SMS
Service B should notify contact X via Mail
Service C should notify contact Y via Mail and SMS
Contact X and Y should also be used for authorization (e.g. in Classic UI)

The only way achieving a semi-clean solution is to

Create contact X-sms, set service_notification_command for sms, assign
contact to service A
Create contact X-mail, set service_notification_command for mail, assign
contact to service B
Create contact Y, set service_notification_command for sms and mail, assign
contact to service C
Create contact X without notification commands, assign to service A and B

Basically you are required to create duplicated contacts for either each
notification method or used for authorization only.

Icinga 2.x attempts to solve that problem in this way

Create user X, set SMS and Mail attributes, used for authorization
Create user Y, set SMS and Mail attributes, used for authorization
Create notification A-SMS, set notification_command for sms, add user X, assign
notification A-SMS to service A
Create notification B-Mail, set notification_command for mail, add user X, assign
notification Mail to service B
Create notification C-SMS, set notification_command for sms, add user Y, assign
notification C-SMS to service C
Create notification C-Mail, set notification_command for mail, add user Y, assign
notification C-Mail to service C

> **Note**
>
> Notification objects are not required to be service agnostic. They may use
> global notification templates and can be added to a service wherever needed.

Previously in Icinga 1.x it looked like

service -> (contact, contactgroup) -> notification command

In Icinga 2.x it will look like

Service -> Notification -> NotificationCommand
                        -> User, UserGroup


### Escalations

Escalations in Icinga 1.x require a seperated object matching on existing
objects. Escalations happen between a defined start and end time which is
calculated from the notification_interval:

start = notification start + (notification_interval * first_notification)
end = notification start + (notification_interval * last_notification)

In theory first_notification and last_notification can be set to readable
numbers. In practice users are manipulating those attributes in combination
with notification_interval in order to get a start and end time.

In Icinga 2.x the notification object can be used as notification escalation
if the start and end times are defined within the 'times' attribute using
duration literals (e.g. 30m).

The Icinga 2.x escalation does not replace the current running notification.
In Icinga 1.x it's required to copy the contacts from the service notification
to the escalation to garantuee the normal notifications once an escalation
happens.
That's not necessary with Icinga 2.x only requiring an additional notification
object for the escalation itsself.

### Notification Options

Unlike Icinga 1.x with the 'notification_options' attribute with comma seperated
state and type filters, Icinga 2.x uses two configuration attributes for that.
All state and type filter use long names or'd with a pipe together

    notification_options w,u,c,r,f,s
   
    notification_state_filter = (StateFilterWarning | StateFilterUnknown | StateFilterCritical),
    notification_type_filter = (NotificationProblem | NotificationRecovery | NotificationFlappingStart | NotificationFlappingEnd | NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)


> **Note**
>
> Please note that 'NotificationProblem' as type is required for all problem
> notifications.

Icinga 2.x adds more fine granular type filters for acknowledgements, downtime
and flapping type (start, end, ...). 

> **Note**
>
> Notification state and type filters are only valid configuration attributes for
> Notification and User objects.


## Dependencies and Parents

In Icinga 1.x it's possible to define host parents to determine network reachability
and keep a host's state unreachable rather than down.
Furthermore there are host and service dependencies preventing unnecessary checks and
notifications. A host must not depend on a service, and vice versa. All dependencies
are configured as seperate objects and cannot be set directly on the host or service
object.

Icinga 2.x adds host and service dependencies as attribute directly onto the host or
service object or template. A service can now depend on a host, and vice versa. A
service has an implicit dependeny (parent) to its host. A host to host dependency acts
implicit as host parent relation.

StatusDataWriter, IdoMysqlConnection and LivestatusListener support the Icinga 1.x
schema with dependencies and parent attributes for compatibility reasons.


## Flapping

The Icinga 1.x logic on flapping detection uses the last 21 states of a service. This
value is hardcoded and cannot be changed. The algorithm on determining a flapping state
is

   flap threshold = (number of actual state changes / number of possible state changes) * 100%

comparing that to low and high flapping thresholds.

The algorithm uses in Icinga 2.x does not store the past states but calculcates the flapping
threshold from a single value based on counters and half-life values. Icinga 2.x compares
the value with a single flapping threshold configuration attribute.


## State Retention

Icinga 1.x uses retention.dat to save historical and modified-at-runtime data over restarts.
Icinga 2.x uses its own icinga2.state file with a json-like serialized format.


## Logging

Icinga 1.x supports syslog facilities and writes to its own icinga.log and archives. These logs
are used in Icinga 1.x Classic UI to generate historical reports.

Icinga 2.x compat library provides the CompatLogger object which writes the icinga.log and archive
in Icinga 1.x format in order to stay compatible with Classic UI and other addons.
The native Icinga 2.x logging facilities are split into three configuration objects: SyslogLogger,
FileLogger, StreamLogger. Each of them got their own severity and target configuration.


## Broker Modules and Features

Icinga 1.x broker modules are binary incompatible with the Icinga 2.x component loader.
Therefore the module configuration cannot be copied 1:1

Icinga 1.x IDOUtils was implemented from scratch as Icinga 2.x feature which can be loaded
and enabled on-demand. The Icinga 1.x Livestatus addon is implemented as Icinga 2.x
LivestatusListener. Icinga 1.x broker modules used for check distributions are replaced
by the Icinga 2.x cluster and distributed capabilities using the same protocol and security
mechanisms as the Icinga 2.x instance itsself.

Each feature can be created multiple times, i.e. having 3 IDO Mysql databases, 5 Performance
Data Writers and 2 Livestatus Listeners (one listening on tcp, and one on unix sockets).

### IDOUtils Database Backend

Icinga 2.x uses Ido<DBType>Connection configuration objects re-using some options known from
Icinga 1.x IDOUtils such as the database credentials, instance_name or the cleanup attributes
for max age of table entries.

### Enable Features

Icinga 2 features require a library to be loaded, and object configuration. In order to simplify
the process of enabling/disabling these features Icinga 2.x ships with two scripts inspired by
Apache: i2enfeature and i2disfeature.



