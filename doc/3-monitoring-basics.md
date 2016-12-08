# <a id="monitoring-basics"></a> Monitoring Basics

This part of the Icinga 2 documentation provides an overview of all the basic
monitoring concepts you need to know to run Icinga 2.
Keep in mind these examples are made with a Linux server in mind. If you are
using Windows, you will need to change the services accordingly. See the [ITL reference](10-icinga-template-library.md#windows-plugins)
 for further information.

## <a id="hosts-services"></a> Hosts and Services

Icinga 2 can be used to monitor the availability of hosts and services. Hosts
and services can be virtually anything which can be checked in some way:

* Network services (HTTP, SMTP, SNMP, SSH, etc.)
* Printers
* Switches or routers
* Temperature sensors
* Other local or network-accessible services

Host objects provide a mechanism to group services that are running
on the same physical device.

Here is an example of a host object which defines two child services:

    object Host "my-server1" {
      address = "10.0.0.1"
      check_command = "hostalive"
    }

    object Service "ping4" {
      host_name = "my-server1"
      check_command = "ping4"
    }

    object Service "http" {
      host_name = "my-server1"
      check_command = "http"
    }

The example creates two services `ping4` and `http` which belong to the
host `my-server1`.

It also specifies that the host should perform its own check using the `hostalive`
check command.

The `address` attribute is used by check commands to determine which network
address is associated with the host object.

Details on troubleshooting check problems can be found [here](15-troubleshooting.md#troubleshooting).

### <a id="host-states"></a> Host States

Hosts can be in any of the following states:

  Name        | Description
  ------------|--------------
  UP          | The host is available.
  DOWN        | The host is unavailable.

### <a id="service-states"></a> Service States

Services can be in any of the following states:

  Name        | Description
  ------------|--------------
  OK          | The service is working properly.
  WARNING     | The service is experiencing some problems but is still considered to be in working condition.
  CRITICAL    | The service is in a critical state.
  UNKNOWN     | The check could not determine the service's state.

### <a id="hard-soft-states"></a> Hard and Soft States

When detecting a problem with a host/service Icinga re-checks the object a number of
times (based on the `max_check_attempts` and `retry_interval` settings) before sending
notifications. This ensures that no unnecessary notifications are sent for
transient failures. During this time the object is in a `SOFT` state.

After all re-checks have been executed and the object is still in a non-OK
state the host/service switches to a `HARD` state and notifications are sent.

  Name        | Description
  ------------|--------------
  HARD        | The host/service's state hasn't recently changed.
  SOFT        | The host/service has recently changed state and is being re-checked.

### <a id="host-service-checks"></a> Host and Service Checks

Hosts and services determine their state by running checks in a regular interval.

    object Host "router" {
      check_command = "hostalive"
      address = "10.0.0.1"
    }

The `hostalive` command is one of several built-in check commands. It sends ICMP
echo requests to the IP address specified in the `address` attribute to determine
whether a host is online.

A number of other [built-in check commands](10-icinga-template-library.md#plugin-check-commands) are also
available. In addition to these commands the next few chapters will explain in
detail how to set up your own check commands.


## <a id="object-inheritance-using-templates"></a> Templates

Templates may be used to apply a set of identical attributes to more than one
object:

    template Service "generic-service" {
      max_check_attempts = 3
      check_interval = 5m
      retry_interval = 1m
      enable_perfdata = true
    }

    apply Service "ping4" {
      import "generic-service"

      check_command = "ping4"

      assign where host.address
    }

    apply Service "ping6" {
      import "generic-service"

      check_command = "ping6"

      assign where host.address6
    }


In this example the `ping4` and `ping6` services inherit properties from the
template `generic-service`.

Objects as well as templates themselves can import an arbitrary number of
other templates. Attributes inherited from a template can be overridden in the
object if necessary.

You can also import existing non-template objects. Note that templates
and objects share the same namespace, i.e. you can't define a template
that has the same name like an object.


## <a id="custom-attributes"></a> Custom Attributes

In addition to built-in attributes you can define your own attributes:

    object Host "localhost" {
      vars.ssh_port = 2222
    }

Valid values for custom attributes include:

* [Strings](17-language-reference.md#string-literals), [numbers](17-language-reference.md#numeric-literals) and [booleans](17-language-reference.md#boolean-literals)
* [Arrays](17-language-reference.md#array) and [dictionaries](17-language-reference.md#dictionary)
* [Functions](3-monitoring-basics.md#custom-attributes-functions)

### <a id="custom-attributes-functions"></a> Functions as Custom Attributes

Icinga 2 lets you specify [functions](17-language-reference.md#functions) for custom attributes.
The special case here is that whenever Icinga 2 needs the value for such a custom attribute it runs
the function and uses whatever value the function returns:

    object CheckCommand "random-value" {
      command = [ PluginDir + "/check_dummy", "0", "$text$" ]

      vars.text = {{ Math.random() * 100 }}
    }

This example uses the [abbreviated lambda syntax](17-language-reference.md#nullary-lambdas).

These functions have access to a number of variables:

  Variable     | Description
  -------------|---------------
  user         | The User object (for notifications).
  service      | The Service object (for service checks/notifications/event handlers).
  host         | The Host object.
  command      | The command object (e.g. a CheckCommand object for checks).

Here's an example:

    vars.text = {{ host.check_interval }}

In addition to these variables the `macro` function can be used to retrieve the
value of arbitrary macro expressions:

    vars.text = {{
      if (macro("$address$") == "127.0.0.1") {
        log("Running a check for localhost!")
      }

      return "Some text"
    }}

The `resolve_arguments` can be used to resolve a command and its arguments much in
the same fashion Icinga does this for the `command` and `arguments` attributes for
commands. The `by_ssh` command uses this functionality to let users specify a
command and arguments that should be executed via SSH:

    arguments = {
      "-C" = {{
        var command = macro("$by_ssh_command$")
        var arguments = macro("$by_ssh_arguments$")

        if (typeof(command) == String && !arguments) {
          return command
        }

        var escaped_args = []
        for (arg in resolve_arguments(command, arguments)) {
          escaped_args.add(escape_shell_arg(arg))
        }
        return escaped_args.join(" ")
      }}
      ...
    }

Acessing object attributes at runtime inside these functions is described in the
[advanced topics](8-advanced-topics.md#access-object-attributes-at-runtime) chapter.

## <a id="runtime-macros"></a> Runtime Macros

Macros can be used to access other objects' attributes at runtime. For example they
are used in command definitions to figure out which IP address a check should be
run against:

    object CheckCommand "my-ping" {
      command = [ PluginDir + "/check_ping", "-H", "$ping_address$" ]

      arguments = {
        "-w" = "$ping_wrta$,$ping_wpl$%"
        "-c" = "$ping_crta$,$ping_cpl$%"
        "-p" = "$ping_packets$"
      }

      vars.ping_address = "$address$"

      vars.ping_wrta = 100
      vars.ping_wpl = 5

      vars.ping_crta = 250
      vars.ping_cpl = 10

      vars.ping_packets = 5
    }

    object Host "router" {
      check_command = "my-ping"
      address = "10.0.0.1"
    }

In this example we are using the `$address$` macro to refer to the host's `address`
attribute.

We can also directly refer to custom attributes, e.g. by using `$ping_wrta$`. Icinga
automatically tries to find the closest match for the attribute you specified. The
exact rules for this are explained in the next section.

> **Note**
>
> When using the `$` sign as single character you must escape it with an
> additional dollar character (`$$`).


### <a id="macro-evaluation-order"></a> Evaluation Order

When executing commands Icinga 2 checks the following objects in this order to look
up macros and their respective values:

1. User object (only for notifications)
2. Service object
3. Host object
4. Command object
5. Global custom attributes in the `Vars` constant

This execution order allows you to define default values for custom attributes
in your command objects.

Here's how you can override the custom attribute `ping_packets` from the previous
example:

    object Service "ping" {
      host_name = "localhost"
      check_command = "my-ping"

      vars.ping_packets = 10 // Overrides the default value of 5 given in the command
    }

If a custom attribute isn't defined anywhere, an empty value is used and a warning is
written to the Icinga 2 log.

You can also directly refer to a specific attribute -- thereby ignoring these evaluation
rules -- by specifying the full attribute name:

    $service.vars.ping_wrta$

This retrieves the value of the `ping_wrta` custom attribute for the service. This
returns an empty value if the service does not have such a custom attribute no matter
whether another object such as the host has this attribute.


### <a id="host-runtime-macros"></a> Host Runtime Macros

The following host custom attributes are available in all commands that are executed for
hosts or services:

  Name                         | Description
  -----------------------------|--------------
  host.name                    | The name of the host object.
  host.display_name            | The value of the `display_name` attribute.
  host.state                   | The host's current state. Can be one of `UNREACHABLE`, `UP` and `DOWN`.
  host.state_id                | The host's current state. Can be one of `0` (up), `1` (down) and `2` (unreachable).
  host.state_type              | The host's current state type. Can be one of `SOFT` and `HARD`.
  host.check_attempt           | The current check attempt number.
  host.max_check_attempts      | The maximum number of checks which are executed before changing to a hard state.
  host.last_state              | The host's previous state. Can be one of `UNREACHABLE`, `UP` and `DOWN`.
  host.last_state_id           | The host's previous state. Can be one of `0` (up), `1` (down) and `2` (unreachable).
  host.last_state_type         | The host's previous state type. Can be one of `SOFT` and `HARD`.
  host.last_state_change       | The last state change's timestamp.
  host.downtime_depth	       | The number of active downtimes.
  host.duration_sec            | The time since the last state change.
  host.latency                 | The host's check latency.
  host.execution_time          | The host's check execution time.
  host.output                  | The last check's output.
  host.perfdata                | The last check's performance data.
  host.last_check              | The timestamp when the last check was executed.
  host.check_source            | The monitoring instance that performed the last check.
  host.num_services            | Number of services associated with the host.
  host.num_services_ok         | Number of services associated with the host which are in an `OK` state.
  host.num_services_warning    | Number of services associated with the host which are in a `WARNING` state.
  host.num_services_unknown    | Number of services associated with the host which are in an `UNKNOWN` state.
  host.num_services_critical   | Number of services associated with the host which are in a `CRITICAL` state.

### <a id="service-runtime-macros"></a> Service Runtime Macros

The following service macros are available in all commands that are executed for
services:

  Name                       | Description
  ---------------------------|--------------
  service.name               | The short name of the service object.
  service.display_name       | The value of the `display_name` attribute.
  service.check_command      | The short name of the command along with any arguments to be used for the check.
  service.state              | The service's current state. Can be one of `OK`, `WARNING`, `CRITICAL` and `UNKNOWN`.
  service.state_id           | The service's current state. Can be one of `0` (ok), `1` (warning), `2` (critical) and `3` (unknown).
  service.state_type         | The service's current state type. Can be one of `SOFT` and `HARD`.
  service.check_attempt      | The current check attempt number.
  service.max_check_attempts | The maximum number of checks which are executed before changing to a hard state.
  service.last_state         | The service's previous state. Can be one of `OK`, `WARNING`, `CRITICAL` and `UNKNOWN`.
  service.last_state_id      | The service's previous state. Can be one of `0` (ok), `1` (warning), `2` (critical) and `3` (unknown).
  service.last_state_type    | The service's previous state type. Can be one of `SOFT` and `HARD`.
  service.last_state_change  | The last state change's timestamp.
  service.downtime_depth     | The number of active downtimes.
  service.duration_sec       | The time since the last state change.
  service.latency            | The service's check latency.
  service.execution_time     | The service's check execution time.
  service.output             | The last check's output.
  service.perfdata           | The last check's performance data.
  service.last_check         | The timestamp when the last check was executed.
  service.check_source       | The monitoring instance that performed the last check.

### <a id="command-runtime-macros"></a> Command Runtime Macros

The following custom attributes are available in all commands:

  Name                   | Description
  -----------------------|--------------
  command.name           | The name of the command object.

### <a id="user-runtime-macros"></a> User Runtime Macros

The following custom attributes are available in all commands that are executed for
users:

  Name                   | Description
  -----------------------|--------------
  user.name              | The name of the user object.
  user.display_name      | The value of the display_name attribute.

### <a id="notification-runtime-macros"></a> Notification Runtime Macros

  Name                   | Description
  -----------------------|--------------
  notification.type      | The type of the notification.
  notification.author    | The author of the notification comment if existing.
  notification.comment   | The comment of the notification if existing.

### <a id="global-runtime-macros"></a> Global Runtime Macros

The following macros are available in all executed commands:

  Name                   | Description
  -----------------------|--------------
  icinga.timet           | Current UNIX timestamp.
  icinga.long_date_time  | Current date and time including timezone information. Example: `2014-01-03 11:23:08 +0000`
  icinga.short_date_time | Current date and time. Example: `2014-01-03 11:23:08`
  icinga.date            | Current date. Example: `2014-01-03`
  icinga.time            | Current time including timezone information. Example: `11:23:08 +0000`
  icinga.uptime          | Current uptime of the Icinga 2 process.

The following macros provide global statistics:

  Name                              | Description
  ----------------------------------|--------------
  icinga.num_services_ok            | Current number of services in state 'OK'.
  icinga.num_services_warning       | Current number of services in state 'Warning'.
  icinga.num_services_critical      | Current number of services in state 'Critical'.
  icinga.num_services_unknown       | Current number of services in state 'Unknown'.
  icinga.num_services_pending       | Current number of pending services.
  icinga.num_services_unreachable   | Current number of unreachable services.
  icinga.num_services_flapping      | Current number of flapping services.
  icinga.num_services_in_downtime   | Current number of services in downtime.
  icinga.num_services_acknowledged  | Current number of acknowledged service problems.
  icinga.num_hosts_up               | Current number of hosts in state 'Up'.
  icinga.num_hosts_down             | Current number of hosts in state 'Down'.
  icinga.num_hosts_unreachable      | Current number of unreachable hosts.
  icinga.num_hosts_pending          | Current number of pending hosts.
  icinga.num_hosts_flapping         | Current number of flapping hosts.
  icinga.num_hosts_in_downtime      | Current number of hosts in downtime.
  icinga.num_hosts_acknowledged     | Current number of acknowledged host problems.


## <a id="using-apply"></a> Apply Rules

Instead of assigning each object ([Service](9-object-types.md#objecttype-service),
[Notification](9-object-types.md#objecttype-notification), [Dependency](9-object-types.md#objecttype-dependency),
[ScheduledDowntime](9-object-types.md#objecttype-scheduleddowntime))
based on attribute identifiers for example `host_name` objects can be [applied](17-language-reference.md#apply).

Before you start using the apply rules keep the following in mind:

* Define the best match.
    * A set of unique [custom attributes](3-monitoring-basics.md#custom-attributes) for these hosts/services?
    * Or [group](3-monitoring-basics.md#groups) memberships, e.g. a host being a member of a hostgroup, applying services to it?
    * A generic pattern [match](18-library-reference.md#global-functions-match) on the host/service name?
    * [Multiple expressions combined](3-monitoring-basics.md#using-apply-expressions) with `&&` or `||` [operators](17-language-reference.md#expression-operators)
* All expressions must return a boolean value (an empty string is equal to `false` e.g.)

> **Note**
>
> You can set/override object attributes in apply rules using the respectively available
> objects in that scope (host and/or service objects).

[Custom attributes](3-monitoring-basics.md#custom-attributes) can also store nested dictionaries and arrays. That way you can use them
for not only matching for their existance or values in apply expressions, but also assign
("inherit") their values into the generated objected from apply rules.

* [Apply services to hosts](3-monitoring-basics.md#using-apply-services)
* [Apply notifications to hosts and services](3-monitoring-basics.md#using-apply-notifications)
* [Apply dependencies to hosts and services](3-monitoring-basics.md#using-apply-dependencies)
* [Apply scheduled downtimes to hosts and services](3-monitoring-basics.md#using-apply-scheduledowntimes)

A more advanced example is using [apply with for loops on arrays or
dictionaries](3-monitoring-basics.md#using-apply-for) for example provided by
[custom atttributes](3-monitoring-basics.md#custom-attributes) or groups.

> **Tip**
>
> Building configuration in that dynamic way requires detailed information
> of the generated objects. Use the `object list` [CLI command](11-cli-commands.md#cli-command-object)
> after successful [configuration validation](11-cli-commands.md#config-validation).


### <a id="using-apply-expressions"></a> Apply Rules Expressions

You can use simple or advanced combinations of apply rule expressions. Each
expression must evaluate into the boolean `true` value. An empty string
will be for instance interpreted as `false`. In a similar fashion undefined
attributes will return `false`.

Returns `false`:

    assign where host.vars.attribute_does_not_exist

Multiple `assign where` condition rows are evaluated as `OR` condition.

You can combine multiple expressions for matching only a subset of objects. In some cases,
you want to be able to add more than one assign/ignore where expression which matches
a specific condition. To achieve this you can use the logical `and` and `or` operators.


[Match](18-library-reference.md#global-functions-match) all `*mysql*` patterns in the host name and (`&&`) custom attribute `prod_mysql_db`
matches the `db-*` pattern. All hosts with the custom attribute `test_server` set to `true`
should be ignored, or any host name ending with `*internal` pattern.

    object HostGroup "mysql-server" {
      display_name = "MySQL Server"

      assign where match("*mysql*", host.name) && match("db-*", host.vars.prod_mysql_db)
      ignore where host.vars.test_server == true
      ignore where match("*internal", host.name)
    }

Similar example for advanced notification apply rule filters: If the service
attribute `notes` [matches](18-library-reference.md#global-functions-match) the `has gold support 24x7` string `AND` one of the
two condition passes, either the `customer` host custom attribute is set to `customer-xy`
`OR` the host custom attribute `always_notify` is set to `true`.

The notification is ignored for services whose host name ends with `*internal`
`OR` the `priority` custom attribute is [less than](17-language-reference.md#expression-operators) `2`.

    template Notification "cust-xy-notification" {
      users = [ "noc-xy", "mgmt-xy" ]
      command = "mail-service-notification"
    }

    apply Notification "notify-cust-xy-mysql" to Service {
      import "cust-xy-notification"

      assign where match("*has gold support 24x7*", service.notes) && (host.vars.customer == "customer-xy" || host.vars.always_notify == true)
      ignore where match("*internal", host.name) || (service.vars.priority < 2 && host.vars.is_clustered == true)
    }

More advanced examples are covered [here](8-advanced-topics.md#use-functions-assign-where).

### <a id="using-apply-services"></a> Apply Services to Hosts

The sample configuration already includes a detailed example in [hosts.conf](4-configuring-icinga-2.md#hosts-conf)
and [services.conf](4-configuring-icinga-2.md#services-conf) for this use case.

The example for `ssh` applies a service object to all hosts with the `address`
attribute being defined and the custom attribute `os` set to the string `Linux` in `vars`.

    apply Service "ssh" {
      import "generic-service"

      check_command = "ssh"

      assign where host.address && host.vars.os == "Linux"
    }


Other detailed examples are used in their respective chapters, for example
[apply services with custom command arguments](3-monitoring-basics.md#command-passing-parameters).

### <a id="using-apply-notifications"></a> Apply Notifications to Hosts and Services

Notifications are applied to specific targets (`Host` or `Service`) and work in a similar
manner:


    apply Notification "mail-noc" to Service {
      import "mail-service-notification"

      user_groups = [ "noc" ]

      assign where host.vars.notification.mail
    }


In this example the `mail-noc` notification will be created as object for all services having the
`notification.mail` custom attribute defined. The notification command is set to `mail-service-notification`
and all members of the user group `noc` will get notified.

It is also possible to generally apply a notification template and dynamically overwrite values from
the template by checking for custom attributes. This can be achieved by using [conditional statements](17-language-reference.md#conditional-statements):

    apply Notification "host-mail-noc" to Host {
      import "mail-host-notification"

      // replace interval inherited from `mail-host-notification` template with new notfication interval set by a host custom attribute
      if (host.vars.notification_interval) {
        interval = host.vars.notification_interval
      }

      // same with notification period
      if (host.vars.notification_period) {
        period = host.vars.notification_period
      }

      // Send SMS instead of email if the host's custom attribute `notification_type` is set to `sms`
      if (host.vars.notification_type == "sms") {
        command = "sms-host-notification"
      } else {
        command = "mail-host-notification"
      }

      user_groups = [ "noc" ]

      assign where host.address
    }

In the example above, the notification template `mail-host-notification`, which contains all relevant
notification settings, is applied on all host objects where the `host.address` is defined.
Each host object is then checked for custom attributes (`host.vars.notification_interval`,
`host.vars.notification_period` and `host.vars.notification_type`). Depending if the custom
attibute is set or which value it has, the value from the notification template is dynamically
overwritten.

The corresponding host object could look like this:

    object Host "host1" {
      import "host-linux-prod"
      display_name = "host1"
      address = "192.168.1.50"
      vars.notification_interval = 1h
      vars.notification_period = "24x7"
      vars.notification_type = "sms"
    }

### <a id="using-apply-dependencies"></a> Apply Dependencies to Hosts and Services

Detailed examples can be found in the [dependencies](3-monitoring-basics.md#dependencies) chapter.

### <a id="using-apply-scheduledowntimes"></a> Apply Recurring Downtimes to Hosts and Services

The sample configuration includes an example in [downtimes.conf](4-configuring-icinga-2.md#downtimes-conf).

Detailed examples can be found in the [recurring downtimes](8-advanced-topics.md#recurring-downtimes) chapter.


### <a id="using-apply-for"></a> Using Apply For Rules

Next to the standard way of using [apply rules](3-monitoring-basics.md#using-apply)
there is the requirement of applying objects based on a set (array or
dictionary) using [apply for](17-language-reference.md#apply-for) expressions.

The sample configuration already includes a detailed example in [hosts.conf](4-configuring-icinga-2.md#hosts-conf)
and [services.conf](4-configuring-icinga-2.md#services-conf) for this use case.

Take the following example: A host provides the snmp oids for different service check
types. This could look like the following example:

    object Host "router-v6" {
      check_command = "hostalive"
      address6 = "::1"

      vars.oids["if01"] = "1.1.1.1.1"
      vars.oids["temp"] = "1.1.1.1.2"
      vars.oids["bgp"] = "1.1.1.1.5"
    }

Now we want to create service checks for `if01` and `temp`, but not `bgp`.
Furthermore we want to pass the snmp oid stored as dictionary value to the
custom attribute called `vars.snmp_oid` -- this is the command argument required
by the [snmp](10-icinga-template-library.md#plugin-check-command-snmp) check command.
The service's `display_name` should be set to the identifier inside the dictionary.

    apply Service for (identifier => oid in host.vars.oids) {
      check_command = "snmp"
      display_name = identifier
      vars.snmp_oid = oid

      ignore where identifier == "bgp" //don't generate service for bgp checks
    }

Icinga 2 evaluates the `apply for` rule for all objects with the custom attribute
`oids` set. It then iterates over all list items inside the `for` loop and evaluates the
`assign/ignore where` expressions. You can access the loop variable
in these expressions, e.g. for ignoring certain values.
In this example we'd ignore the `bgp` identifier and avoid generating an unwanted service.
We could extend the configuration by also matching the `oid` value on certain
[regex](18-library-reference.md#global-functions-regex)/[wildcard match](18-library-reference.md#global-functions-match) patterns for example.

> **Note**
>
> You don't need an `assign where` expression only checking for existance
> of the custom attribute.

That way you'll save duplicated apply rules by combining them into one
generic `apply for` rule generating the object name with or without a prefix.


#### <a id="using-apply-for-custom-attribute-override"></a> Apply For and Custom Attribute Override

Imagine a different more advanced example: You are monitoring your network device (host)
with many interfaces (services). The following requirements/problems apply:

* Each interface service check should be named with a prefix and a name defined in your host object (which could be generated from your CMDB, etc.)
* Each interface has its own vlan tag
* Some interfaces have QoS enabled
* Additional attributes such as `display_name` or `notes`, `notes_url` and `action_url` must be
dynamically generated


Tip: Define the snmp community as global constant in your [constants.conf](4-configuring-icinga-2.md#constants-conf) file.

    const IftrafficSnmpCommunity = "public"

By defining the `interfaces` dictionary with three example interfaces on the `cisco-catalyst-6509-34`
host object, you'll make sure to pass the [custom attribute](3-monitoring-basics.md#custom-attributes)
storage required by the for loop in the service apply rule.

    object Host "cisco-catalyst-6509-34" {
      import "generic-host"
      display_name = "Catalyst 6509 #34 VIE21"
      address = "127.0.1.4"

      /* "GigabitEthernet0/2" is the interface name,
       * and key name in service apply for later on
       */
      vars.interfaces["GigabitEthernet0/2"] = {
         /* define all custom attributes with the
          * same name required for command parameters/arguments
          * in service apply (look into your CheckCommand definition)
          */
         iftraffic_units = "g"
         iftraffic_community = IftrafficSnmpCommunity
	 iftraffic_bandwidth = 1
         vlan = "internal"
         qos = "disabled"
      }
      vars.interfaces["GigabitEthernet0/4"] = {
         iftraffic_units = "g"
         //iftraffic_community = IftrafficSnmpCommunity
	 iftraffic_bandwidth = 1
         vlan = "renote"
         qos = "enabled"
      }
      vars.interfaces["MgmtInterface1"] = {
         iftraffic_community = IftrafficSnmpCommunity
         vlan = "mgmt"
         interface_address = "127.99.0.100" #special management ip
      }
    }

You can also omit the `"if-"` string, then all generated service names are directly
taken from the `if_name` variable value.

The config dictionary contains all key-value pairs for the specific interface in one
loop cycle, like `iftraffic_units`, `vlan`, and `qos` for the specified interface.

You can either map the custom attributes from the `interface_config` dictionary to
local custom attributes stashed into `vars`. If the names match the required command
argument parameters already (for example `iftraffic_units`), you could also add the
`interface_config` dictionary to the `vars` dictionary using the `+=` operator.

After `vars` is fully populated, all object attributes can be set calculated from
provided host attributes. For strings, you can use string concatention with the `+` operator.

You can also specify the display_name, check command, interval, notes, notes_url, action_url, etc.
attributes that way. Attribute strings can be [concatenated](17-language-reference.md#expression-operators),
for example for adding a more detailed service `display_name`.

This example also uses [if conditions](17-language-reference.md#conditional-statements)
if specific values are not set, adding a local default value.
The other way around you can override specific custom attributes inherited from a service template if set.

    /* loop over the host.vars.interfaces dictionary
     * for (key => value in dict) means `interface_name` as key
     * and `interface_config` as value. Access config attributes
     * with the indexer (`.`) character.
     */
    apply Service "if-" for (interface_name => interface_config in host.vars.interfaces) {
      import "generic-service"
      check_command = "iftraffic"
      display_name = "IF-" + interface_name

      /* use the key as command argument (no duplication of values in host.vars.interfaces) */
      vars.iftraffic_interface = interface_name

      /* map the custom attributes as command arguments */
      vars.iftraffic_units = interface_config.iftraffic_units
      vars.iftraffic_community = interface_config.iftraffic_community

      /* the above can be achieved in a shorter fashion if the names inside host.vars.interfaces
       * are the _exact_ same as required as command parameter by the check command
       * definition.
       */
      vars += interface_config

      /* set a default value for units and bandwidth */
      if (interface_config.iftraffic_units == "") {
        vars.iftraffic_units = "m"
      }
      if (interface_config.iftraffic_bandwidth == "") {
        vars.iftraffic_bandwidth = 1
      }
      if (interface_config.vlan == "") {
        vars.vlan = "not set"
      }
      if (interface_config.qos == "") {
        vars.qos = "not set"
      }

      /* set the global constant if not explicitely
       * not provided by the `interfaces` dictionary on the host
       */
      if (len(interface_config.iftraffic_community) == 0 || len(vars.iftraffic_community) == 0) {
        vars.iftraffic_community = IftrafficSnmpCommunity
      }

      /* Calculate some additional object attributes after populating the `vars` dictionary */
      notes = "Interface check for " + interface_name + " (units: '" + interface_config.iftraffic_units + "') in VLAN '" + vars.vlan + "' with ' QoS '" + vars.qos + "'"
      notes_url = "http://foreman.company.com/hosts/" + host.name
      action_url = "http://snmp.checker.company.com/" + host.name + "/if-" + interface_name
    }



This example makes use of the [check_iftraffic](https://exchange.icinga.com/exchange/iftraffic) plugin.
The `CheckCommand` definition can be found in the
[contributed plugin check commands](10-icinga-template-library.md#plugin-contrib-command-iftraffic)
-- make sure to include them in your [icinga2 configuration file](4-configuring-icinga-2.md#icinga2-conf).


> **Tip**
>
> Building configuration in that dynamic way requires detailed information
> of the generated objects. Use the `object list` [CLI command](11-cli-commands.md#cli-command-object)
> after successful [configuration validation](11-cli-commands.md#config-validation).

Verify that the apply-for-rule successfully created the service objects with the
inherited custom attributes:

    # icinga2 daemon -C
    # icinga2 object list --type Service --name *catalyst*

    Object 'cisco-catalyst-6509-34!if-GigabitEthernet0/2' of type 'Service':
    ......
      * vars
        % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 59:3-59:26
        * iftraffic_bandwidth = 1
        * iftraffic_community = "public"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 53:3-53:65
        * iftraffic_interface = "GigabitEthernet0/2"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 49:3-49:43
        * iftraffic_units = "g"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 52:3-52:57
        * qos = "disabled"
        * vlan = "internal"


    Object 'cisco-catalyst-6509-34!if-GigabitEthernet0/4' of type 'Service':
    ...
      * vars
        % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 59:3-59:26
        * iftraffic_bandwidth = 1
        * iftraffic_community = "public"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 53:3-53:65
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 79:5-79:53
        * iftraffic_interface = "GigabitEthernet0/4"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 49:3-49:43
        * iftraffic_units = "g"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 52:3-52:57
        * qos = "enabled"
        * vlan = "renote"

    Object 'cisco-catalyst-6509-34!if-MgmtInterface1' of type 'Service':
    ...
      * vars
        % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 59:3-59:26
        * iftraffic_bandwidth = 1
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 66:5-66:32
        * iftraffic_community = "public"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 53:3-53:65
        * iftraffic_interface = "MgmtInterface1"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 49:3-49:43
        * iftraffic_units = "m"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 52:3-52:57
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 63:5-63:30
        * interface_address = "127.99.0.100"
        * qos = "not set"
          % = modified in '/etc/icinga2/conf.d/iftraffic.conf', lines 72:5-72:24
        * vlan = "mgmt"


### <a id="using-apply-object-attributes"></a> Use Object Attributes in Apply Rules

Since apply rules are evaluated after the generic objects, you
can reference existing host and/or service object attributes as
values for any object attribute specified in that apply rule.

    object Host "opennebula-host" {
      import "generic-host"
      address = "10.1.1.2"

      vars.hosting["xyz"] = {
        http_uri = "/shop"
        customer_name = "Customer xyz"
        customer_id = "7568"
        support_contract = "gold"
      }
      vars.hosting["abc"] = {
        http_uri = "/shop"
        customer_name = "Customer xyz"
        customer_id = "7568"
        support_contract = "silver"
      }
    }

    apply Service for (customer => config in host.vars.hosting) {
      import "generic-service"
      check_command = "ping4"

      vars.qos = "disabled"

      vars += config

      vars.http_uri = "/" + vars.customer + "/" + config.http_uri

      display_name = "Shop Check for " + vars.customer_name + "-" + vars.customer_id

      notes = "Support contract: " + vars.support_contract + " for Customer " + vars.customer_name + " (" + vars.customer_id + ")."

      notes_url = "http://foreman.company.com/hosts/" + host.name
      action_url = "http://snmp.checker.company.com/" + host.name + "/" + vars.customer_id
    }

## <a id="groups"></a> Groups

A group is a collection of similar objects. Groups are primarily used as a
visualization aid in web interfaces.

Group membership is defined at the respective object itself. If
you have a hostgroup name `windows` for example, and want to assign
specific hosts to this group for later viewing the group on your
alert dashboard, first create a HostGroup object:

    object HostGroup "windows" {
      display_name = "Windows Servers"
    }

Then add your hosts to this group:

    template Host "windows-server" {
      groups += [ "windows" ]
    }

    object Host "mssql-srv1" {
      import "windows-server"

      vars.mssql_port = 1433
    }

    object Host "mssql-srv2" {
      import "windows-server"

      vars.mssql_port = 1433
    }

This can be done for service and user groups the same way:

    object UserGroup "windows-mssql-admins" {
      display_name = "Windows MSSQL Admins"
    }

    template User "generic-windows-mssql-users" {
      groups += [ "windows-mssql-admins" ]
    }

    object User "win-mssql-noc" {
      import "generic-windows-mssql-users"

      email = "noc@example.com"
    }

    object User "win-mssql-ops" {
      import "generic-windows-mssql-users"

      email = "ops@example.com"
    }

### <a id="group-assign-intro"></a> Group Membership Assign

Instead of manually assigning each object to a group you can also assign objects
to a group based on their attributes:

    object HostGroup "prod-mssql" {
      display_name = "Production MSSQL Servers"

      assign where host.vars.mssql_port && host.vars.prod_mysql_db
      ignore where host.vars.test_server == true
      ignore where match("*internal", host.name)
    }

In this example all hosts with the `vars` attribute `mssql_port`
will be added as members to the host group `mssql`. However, all
hosts [matching](18-library-reference.md#global-functions-match) the string `\*internal`
or with the `test_server` attribute set to `true` are **not** added to this group.

Details on the `assign where` syntax can be found in the
[Language Reference](17-language-reference.md#apply).

## <a id="notifications"></a> Notifications

Notifications for service and host problems are an integral part of your
monitoring setup.

When a host or service is in a downtime, a problem has been acknowledged or
the dependency logic determined that the host/service is unreachable, no
notifications are sent. You can configure additional type and state filters
refining the notifications being actually sent.

There are many ways of sending notifications, e.g. by email, XMPP,
IRC, Twitter, etc. On its own Icinga 2 does not know how to send notifications.
Instead it relies on external mechanisms such as shell scripts to notify users.
More notification methods are listed in the [addons and plugins](13-addons.md#notification-scripts-interfaces)
chapter.

A notification specification requires one or more users (and/or user groups)
who will be notified in case of problems. These users must have all custom
attributes defined which will be used in the `NotificationCommand` on execution.

The user `icingaadmin` in the example below will get notified only on `WARNING` and
`CRITICAL` states and `problem` and `recovery` notification types.

    object User "icingaadmin" {
      display_name = "Icinga 2 Admin"
      enable_notifications = true
      states = [ OK, Warning, Critical ]
      types = [ Problem, Recovery ]
      email = "icinga@localhost"
    }

If you don't set the `states` and `types` configuration attributes for the `User`
object, notifications for all states and types will be sent.

Details on troubleshooting notification problems can be found [here](15-troubleshooting.md#troubleshooting).

> **Note**
>
> Make sure that the [notification](11-cli-commands.md#enable-features) feature is enabled
> in order to execute notification commands.

You should choose which information you (and your notified users) are interested in
case of emergency, and also which information does not provide any value to you and
your environment.

An example notification command is explained [here](3-monitoring-basics.md#notification-commands).

You can add all shared attributes to a `Notification` template which is inherited
to the defined notifications. That way you'll save duplicated attributes in each
`Notification` object. Attributes can be overridden locally.

    template Notification "generic-notification" {
      interval = 15m

      command = "mail-service-notification"

      states = [ Warning, Critical, Unknown ]
      types = [ Problem, Acknowledgement, Recovery, Custom, FlappingStart,
                FlappingEnd, DowntimeStart, DowntimeEnd, DowntimeRemoved ]

      period = "24x7"
    }

The time period `24x7` is included as example configuration with Icinga 2.

Use the `apply` keyword to create `Notification` objects for your services:

    apply Notification "notify-cust-xy-mysql" to Service {
      import "generic-notification"

      users = [ "noc-xy", "mgmt-xy" ]

      assign where match("*has gold support 24x7*", service.notes) && (host.vars.customer == "customer-xy" || host.vars.always_notify == true
      ignore where match("*internal", host.name) || (service.vars.priority < 2 && host.vars.is_clustered == true)
    }


Instead of assigning users to notifications, you can also add the `user_groups`
attribute with a list of user groups to the `Notification` object. Icinga 2 will
send notifications to all group members.

> **Note**
>
> Only users who have been notified of a problem before  (`Warning`, `Critical`, `Unknown`
> states for services, `Down` for hosts) will receive `Recovery` notifications.

### <a id="notification-escalations"></a> Notification Escalations

When a problem notification is sent and a problem still exists at the time of re-notification
you may want to escalate the problem to the next support level. A different approach
is to configure the default notification by email, and escalate the problem via SMS
if not already solved.

You can define notification start and end times as additional configuration
attributes making the `Notification` object a so-called `notification escalation`.
Using templates you can share the basic notification attributes such as users or the
`interval` (and override them for the escalation then).

Using the example from above, you can define additional users being escalated for SMS
notifications between start and end time.

    object User "icinga-oncall-2nd-level" {
      display_name = "Icinga 2nd Level"

      vars.mobile = "+1 555 424642"
    }

    object User "icinga-oncall-1st-level" {
      display_name = "Icinga 1st Level"

      vars.mobile = "+1 555 424642"
    }

Define an additional [NotificationCommand](3-monitoring-basics.md#notification-commands) for SMS notifications.

> **Note**
>
> The example is not complete as there are many different SMS providers.
> Please note that sending SMS notifications will require an SMS provider
> or local hardware with an active SIM card.

    object NotificationCommand "sms-notification" {
       command = [
         PluginDir + "/send_sms_notification",
         "$mobile$",
         "..."
    }

The two new notification escalations are added onto the local host
and its service `ping4` using the `generic-notification` template.
The user `icinga-oncall-2nd-level` will get notified by SMS (`sms-notification`
command) after `30m` until `1h`.

> **Note**
>
> The `interval` was set to 15m in the `generic-notification`
> template example. Lower that value in your escalations by using a secondary
> template or by overriding the attribute directly in the `notifications` array
> position for `escalation-sms-2nd-level`.

If the problem does not get resolved nor acknowledged preventing further notifications,
the `escalation-sms-1st-level` user will be escalated `1h` after the initial problem was
notified, but only for one hour (`2h` as `end` key for the `times` dictionary).

    apply Notification "mail" to Service {
      import "generic-notification"

      command = "mail-notification"
      users = [ "icingaadmin" ]

      assign where service.name == "ping4"
    }

    apply Notification "escalation-sms-2nd-level" to Service {
      import "generic-notification"

      command = "sms-notification"
      users = [ "icinga-oncall-2nd-level" ]

      times = {
        begin = 30m
        end = 1h
      }

      assign where service.name == "ping4"
    }

    apply Notification "escalation-sms-1st-level" to Service {
      import "generic-notification"

      command = "sms-notification"
      users = [ "icinga-oncall-1st-level" ]

      times = {
        begin = 1h
        end = 2h
      }

      assign where service.name == "ping4"
    }

### <a id="notification-delay"></a> Notification Delay

Sometimes the problem in question should not be announced when the notification is due
(the object reaching the `HARD` state), but after a certain period. In Icinga 2
you can use the `times` dictionary and set `begin = 15m` as key and value if you want to
postpone the notification window for 15 minutes. Leave out the `end` key -- if not set,
Icinga 2 will not check against any end time for this notification. Make sure to
specify a relatively low notification `interval` to get notified soon enough again.

    apply Notification "mail" to Service {
      import "generic-notification"

      command = "mail-notification"
      users = [ "icingaadmin" ]

      interval = 5m

      times.begin = 15m // delay notification window

      assign where service.name == "ping4"
    }

### <a id="disable-renotification"></a> Disable Re-notifications

If you prefer to be notified only once, you can disable re-notifications by setting the
`interval` attribute to `0`.

    apply Notification "notify-once" to Service {
      import "generic-notification"

      command = "mail-notification"
      users = [ "icingaadmin" ]

      interval = 0 // disable re-notification

      assign where service.name == "ping4"
    }

### <a id="notification-filters-state-type"></a> Notification Filters by State and Type

If there are no notification state and type filter attributes defined at the `Notification`
or `User` object, Icinga 2 assumes that all states and types are being notified.

Available state and type filters for notifications are:

    template Notification "generic-notification" {

      states = [ Warning, Critical, Unknown ]
      types = [ Problem, Acknowledgement, Recovery, Custom, FlappingStart,
                FlappingEnd, DowntimeStart, DowntimeEnd, DowntimeRemoved ]
    }

If you are familiar with Icinga 1.x `notification_options`, please note that they have been split
into type and state to allow more fine granular filtering for example on downtimes and flapping.
You can filter for acknowledgements and custom notifications too.


## <a id="commands"></a> Commands

Icinga 2 uses three different command object types to specify how
checks should be performed, notifications should be sent, and
events should be handled.

### <a id="check-commands"></a> Check Commands

[CheckCommand](9-object-types.md#objecttype-checkcommand) objects define the command line how
a check is called.

[CheckCommand](9-object-types.md#objecttype-checkcommand) objects are referenced by
[Host](9-object-types.md#objecttype-host) and [Service](9-object-types.md#objecttype-service) objects
using the `check_command` attribute.

> **Note**
>
> Make sure that the [checker](11-cli-commands.md#enable-features) feature is enabled in order to
> execute checks.

#### <a id="command-plugin-integration"></a> Integrate the Plugin with a CheckCommand Definition

Unless you have done so already, download your check plugin and put it
into the [PluginDir](4-configuring-icinga-2.md#constants-conf) directory. The following example uses the
`check_mysql` plugin contained in the Monitoring Plugins package.

The plugin path and all command arguments are made a list of
double-quoted string arguments for proper shell escaping.

Call the `check_disk` plugin with the `--help` parameter to see
all available options. Our example defines warning (`-w`) and
critical (`-c`) thresholds for the disk usage. Without any
partition defined (`-p`) it will check all local partitions.

    icinga@icinga2 $ /usr/lib64/nagios/plugins/check_mysql --help
    ...
    This program tests connections to a MySQL server
        
    Usage:
    check_mysql [-d database] [-H host] [-P port] [-s socket]
    [-u user] [-p password] [-S] [-l] [-a cert] [-k key]
    [-C ca-cert] [-D ca-dir] [-L ciphers] [-f optfile] [-g group]

Next step is to understand how [command parameters](3-monitoring-basics.md#command-passing-parameters)
are being passed from a host or service object, and add a [CheckCommand](9-object-types.md#objecttype-checkcommand)
definition based on these required parameters and/or default values.

Please continue reading in the [plugins section](5-service-monitoring.md#service-monitoring-plugins) for additional integration examples.

#### <a id="command-passing-parameters"></a> Passing Check Command Parameters from Host or Service

Check command parameters are defined as custom attributes which can be accessed as runtime macros
by the executed check command.

The check command parameters for ITL provided plugin check command definitions are documented
[here](10-icinga-template-library.md#plugin-check-commands), for example
[disk](10-icinga-template-library.md#plugin-check-command-disk).

In order to practice passing command parameters you should [integrate your own plugin](3-monitoring-basics.md#command-plugin-integration).

The following example will use `check_mysql` provided by the [Monitoring Plugins installation](2-getting-started.md#setting-up-check-plugins).

Define the default check command custom attributes, for example `mysql_user` and `mysql_password`
(freely definable naming schema) and optional their default threshold values. You can
then use these custom attributes as runtime macros for [command arguments](3-monitoring-basics.md#command-arguments)
on the command line.

> **Tip**
>
> Use a common command type as prefix for your command arguments to increase
> readability. `mysql_user` helps understanding the context better than just
> `user` as argument.

The default custom attributes can be overridden by the custom attributes
defined in the host or service using the check command `my-mysql`. The custom attributes
can also be inherited from a parent template using additive inheritance (`+=`).

    # vim /etc/icinga2/conf.d/commands.conf

    object CheckCommand "my-mysql" {
      command = [ PluginDir + "/check_mysql" ] //constants.conf -> const PluginDir

      arguments = {
        "-H" = "$mysql_host$"
        "-u" = {
          required = true
          value = "$mysql_user$"
        }
        "-p" = "$mysql_password$"
        "-P" = "$mysql_port$"
        "-s" = "$mysql_socket$"
        "-a" = "$mysql_cert$"
        "-d" = "$mysql_database$"
        "-k" = "$mysql_key$"
        "-C" = "$mysql_ca_cert$"
        "-D" = "$mysql_ca_dir$"
        "-L" = "$mysql_ciphers$"
        "-f" = "$mysql_optfile$"
        "-g" = "$mysql_group$"
        "-S" = {
          set_if = "$mysql_check_slave$"
          description = "Check if the slave thread is running properly."
        }
        "-l" = {
          set_if = "$mysql_ssl$"
          description = "Use ssl encryption"
        }
      }

      vars.mysql_check_slave = false
      vars.mysql_ssl = false
      vars.mysql_host = "$address$"
    }

The check command definition also sets `mysql_host` to the `$address$` default value. You can override
this command parameter if for example your MySQL host is not running on the same server's ip address.

Make sure pass all required command parameters, such as `mysql_user`, `mysql_password` and `mysql_database`.
`MysqlUsername` and `MysqlPassword` are specified as [global constants](4-configuring-icinga-2.md#constants-conf)
in this example.

    # vim /etc/icinga2/conf.d/services.conf

    apply Service "mysql-icinga-db-health" {
      import "generic-service"

      check_command = "my-mysql"

      vars.mysql_user = MysqlUsername
      vars.mysql_password = MysqlPassword

      vars.mysql_database = "icinga"
      vars.mysql_host = "192.168.33.11"

      assign where match("icinga2*", host.name)
      ignore where host.vars.no_health_check == true
    }


Take a different example: The example host configuration in [hosts.conf](4-configuring-icinga-2.md#hosts-conf)
also applies an `ssh` service check. Your host's ssh port is not the default `22`, but set to `2022`.
You can pass the command parameter as custom attribute `ssh_port` directly inside the service apply rule
inside [services.conf](4-configuring-icinga-2.md#services-conf):

    apply Service "ssh" {
      import "generic-service"

      check_command = "ssh"
      vars.ssh_port = 2022 //custom command parameter

      assign where (host.address || host.address6) && host.vars.os == "Linux"
    }

If you prefer this being configured at the host instead of the service, modify the host configuration
object instead. The runtime macro resolving order is described [here](3-monitoring-basics.md#macro-evaluation-order).

    object Host NodeName {
    ...
      vars.ssh_port = 2022
    }

#### <a id="command-passing-parameters-apply-for"></a> Passing Check Command Parameters Using Apply For

The host `localhost` with the generated services from the `basic-partitions` dictionary (see
[apply for](3-monitoring-basics.md#using-apply-for) for details) checks a basic set of disk partitions
with modified custom attributes (warning thresholds at `10%`, critical thresholds at `5%`
free disk space).

The custom attribute `disk_partition` can either hold a single string or an array of
string values for passing multiple partitions to the `check_disk` check plugin.

    object Host "my-server" {
      import "generic-host"
      address = "127.0.0.1"
      address6 = "::1"

      vars.local_disks["basic-partitions"] = {
        disk_partitions = [ "/", "/tmp", "/var", "/home" ]
      }
    }

    apply Service for (disk => config in host.vars.local_disks) {
      import "generic-service"
      check_command = "my-disk"

      vars += config

      vars.disk_wfree = "10%"
      vars.disk_cfree = "5%"
    }


More details on using arrays in custom attributes can be found in
[this chapter](3-monitoring-basics.md#custom-attributes).


#### <a id="command-arguments"></a> Command Arguments

By defining a check command line using the `command` attribute Icinga 2
will resolve all macros in the static string or array. Sometimes it is
required to extend the arguments list based on a met condition evaluated
at command execution. Or making arguments optional -- only set if the
macro value can be resolved by Icinga 2.

    object CheckCommand "check_http" {
      command = [ PluginDir + "/check_http" ]

      arguments = {
        "-H" = "$http_vhost$"
        "-I" = "$http_address$"
        "-u" = "$http_uri$"
        "-p" = "$http_port$"
        "-S" = {
          set_if = "$http_ssl$"
        }
        "--sni" = {
          set_if = "$http_sni$"
        }
        "-a" = {
          value = "$http_auth_pair$"
          description = "Username:password on sites with basic authentication"
        }
        "--no-body" = {
          set_if = "$http_ignore_body$"
        }
        "-r" = "$http_expect_body_regex$"
        "-w" = "$http_warn_time$"
        "-c" = "$http_critical_time$"
        "-e" = "$http_expect$"
      }

      vars.http_address = "$address$"
      vars.http_ssl = false
      vars.http_sni = false
    }

The example shows the `check_http` check command defining the most common
arguments. Each of them is optional by default and will be omitted if
the value is not set. For example, if the service calling the check command
does not have `vars.http_port` set, it won't get added to the command
line.

If the `vars.http_ssl` custom attribute is set in the service, host or command
object definition, Icinga 2 will add the `-S` argument based on the `set_if`
numeric value to the command line. String values are not supported.

If the macro value cannot be resolved, Icinga 2 will not add the defined argument
to the final command argument array. Empty strings for macro values won't omit
the argument.

That way you can use the `check_http` command definition for both, with and
without SSL enabled checks saving you duplicated command definitions.

Details on all available options can be found in the
[CheckCommand object definition](9-object-types.md#objecttype-checkcommand).


#### <a id="command-environment-variables"></a> Environment Variables

The `env` command object attribute specifies a list of environment variables with values calculated
from either runtime macros or custom attributes which should be exported as environment variables
prior to executing the command.

This is useful for example for hiding sensitive information on the command line output
when passing credentials to database checks:

    object CheckCommand "mysql-health" {
      command = [
        PluginDir + "/check_mysql"
      ]

      arguments = {
        "-H" = "$mysql_address$"
        "-d" = "$mysql_database$"
      }

      vars.mysql_address = "$address$"
      vars.mysql_database = "icinga"
      vars.mysql_user = "icinga_check"
      vars.mysql_pass = "password"

      env.MYSQLUSER = "$mysql_user$"
      env.MYSQLPASS = "$mysql_pass$"
    }



### <a id="notification-commands"></a> Notification Commands

[NotificationCommand](9-object-types.md#objecttype-notificationcommand) objects define how notifications are delivered to external
interfaces (email, XMPP, IRC, Twitter, etc.).

[NotificationCommand](9-object-types.md#objecttype-notificationcommand) objects are referenced by
[Notification](9-object-types.md#objecttype-notification) objects using the `command` attribute.

> **Note**
>
> Make sure that the [notification](11-cli-commands.md#enable-features) feature is enabled
> in order to execute notification commands.

Below is an example using runtime macros from Icinga 2 (such as `$service.output$` for
the current check output) sending an email to the user(s) associated with the
notification itself (`$user.email$`).

If you want to specify default values for some of the custom attribute definitions,
you can add a `vars` dictionary as shown for the `CheckCommand` object.

    object NotificationCommand "mail-service-notification" {
      command = [ SysconfDir + "/icinga2/scripts/mail-notification.sh" ]

      env = {
        NOTIFICATIONTYPE = "$notification.type$"
        SERVICEDESC = "$service.name$"
        HOSTALIAS = "$host.display_name$"
        HOSTADDRESS = "$address$"
        SERVICESTATE = "$service.state$"
        LONGDATETIME = "$icinga.long_date_time$"
        SERVICEOUTPUT = "$service.output$"
        NOTIFICATIONAUTHORNAME = "$notification.author$"
        NOTIFICATIONCOMMENT = "$notification.comment$"
    	HOSTDISPLAYNAME = "$host.display_name$"
        SERVICEDISPLAYNAME = "$service.display_name$"
        USEREMAIL = "$user.email$"
      }
    }

The command attribute in the `mail-service-notification` command refers to the following
shell script. The macros specified in the `env` array are exported
as environment variables and can be used in the notification script:

    #!/usr/bin/env bash
    template=$(cat <<TEMPLATE
    ***** Icinga  *****

    Notification Type: $NOTIFICATIONTYPE

    Service: $SERVICEDESC
    Host: $HOSTALIAS
    Address: $HOSTADDRESS
    State: $SERVICESTATE

    Date/Time: $LONGDATETIME

    Additional Info: $SERVICEOUTPUT

    Comment: [$NOTIFICATIONAUTHORNAME] $NOTIFICATIONCOMMENT
    TEMPLATE
    )

    /usr/bin/printf "%b" $template | mail -s "$NOTIFICATIONTYPE - $HOSTDISPLAYNAME - $SERVICEDISPLAYNAME is $SERVICESTATE" $USEREMAIL

> **Note**
>
> This example is for `exim` only. Requires changes for `sendmail` and
> other MTAs.

While it's possible to specify the entire notification command right
in the NotificationCommand object it is generally advisable to create a
shell script in the `/etc/icinga2/scripts` directory and have the
NotificationCommand object refer to that.

### <a id="event-commands"></a> Event Commands

Unlike notifications, event commands for hosts/services are called on every
check execution if one of these conditions matches:

* The host/service is in a [soft state](3-monitoring-basics.md#hard-soft-states)
* The host/service state changes into a [hard state](3-monitoring-basics.md#hard-soft-states)
* The host/service state recovers from a [soft or hard state](3-monitoring-basics.md#hard-soft-states) to [OK](3-monitoring-basics.md#service-states)/[Up](3-monitoring-basics.md#host-states)

[EventCommand](9-object-types.md#objecttype-eventcommand) objects are referenced by
[Host](9-object-types.md#objecttype-host) and [Service](9-object-types.md#objecttype-service) objects
using the `event_command` attribute.

Therefore the `EventCommand` object should define a command line
evaluating the current service state and other service runtime attributes
available through runtime vars. Runtime macros such as `$service.state_type$`
and `$service.state$` will be processed by Icinga 2 helping on fine-granular
events being triggered.

If you are using a client as [command endpoint](6-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint)
the event command will be executed on the client itself (similar to the check
command).

Common use case scenarios are a failing HTTP check requiring an immediate
restart via event command, or if an application is locked and requires
a restart upon detection.

#### <a id="event-command-restart-service-daemon"></a> Use Event Commands to Restart Service Daemon

The following example will trigger a restart of the `httpd` daemon
via ssh when the `http` service check fails. If the service state is
`OK`, it will not trigger any event action.

Requirements:

* ssh connection
* icinga user with public key authentication
* icinga user with sudo permissions for restarting the httpd daemon.

Example on Debian:

    # ls /home/icinga/.ssh/
    authorized_keys

    # visudo
    icinga  ALL=(ALL) NOPASSWD: /etc/init.d/apache2 restart


Define a generic [EventCommand](9-object-types.md#objecttype-eventcommand) object `event_by_ssh`
which can be used for all event commands triggered using ssh:

    /* pass event commands through ssh */
    object EventCommand "event_by_ssh" {
      command = [ PluginDir + "/check_by_ssh" ]

      arguments = {
        "-H" = "$event_by_ssh_address$"
        "-p" = "$event_by_ssh_port$"
        "-C" = "$event_by_ssh_command$"
        "-l" = "$event_by_ssh_logname$"
        "-i" = "$event_by_ssh_identity$"
        "-q" = {
          set_if = "$event_by_ssh_quiet$"
        }
        "-w" = "$event_by_ssh_warn$"
        "-c" = "$event_by_ssh_crit$"
        "-t" = "$event_by_ssh_timeout$"
      }

      vars.event_by_ssh_address = "$address$"
      vars.event_by_ssh_quiet = false
    }

The actual event command only passes the `event_by_ssh_command` attribute.
The `event_by_ssh_service` custom attribute takes care of passing the correct
daemon name, while `test $service.state_id$ -gt 0` makes sure that the daemon
is only restarted when the service is not in an `OK` state.


    object EventCommand "event_by_ssh_restart_service" {
      import "event_by_ssh"

      //only restart the daemon if state > 0 (not-ok)
      //requires sudo permissions for the icinga user
      vars.event_by_ssh_command = "test $service.state_id$ -gt 0 && sudo /etc/init.d/$event_by_ssh_service$ restart"
    }


Now set the `event_command` attribute to `event_by_ssh_restart_service` and tell it
which service should be restarted using the `event_by_ssh_service` attribute.

    object Service "http" {
      import "generic-service"
      host_name = "remote-http-host"
      check_command = "http"

      event_command = "event_by_ssh_restart_service"
      vars.event_by_ssh_service = "$host.vars.httpd_name$"

      //vars.event_by_ssh_logname = "icinga"
      //vars.event_by_ssh_identity = "/home/icinga/.ssh/id_rsa.pub"
    }


Each host with this service then must define the `httpd_name` custom attribute
(for example generated from your cmdb):

    object Host "remote-http-host" {
      import "generic-host"
      address = "192.168.1.100"

      vars.httpd_name = "apache2"
    }

You can testdrive this example by manually stopping the `httpd` daemon
on your `remote-http-host`. Enable the `debuglog` feature and tail the
`/var/log/icinga2/debug.log` file.

Remote Host Terminal:

    # date; service apache2 status
    Mon Sep 15 18:57:39 CEST 2014
    Apache2 is running (pid 23651).
    # date; service apache2 stop
    Mon Sep 15 18:57:47 CEST 2014
    [ ok ] Stopping web server: apache2 ... waiting .

Icinga 2 Host Terminal:

    [2014-09-15 18:58:32 +0200] notice/Process: Running command '/usr/lib64/nagios/plugins/check_http' '-I' '192.168.1.100': PID 32622
    [2014-09-15 18:58:32 +0200] notice/Process: PID 32622 ('/usr/lib64/nagios/plugins/check_http' '-I' '192.168.1.100') terminated with exit code 2
    [2014-09-15 18:58:32 +0200] notice/Checkable: State Change: Checkable remote-http-host!http soft state change from OK to CRITICAL detected.
    [2014-09-15 18:58:32 +0200] notice/Checkable: Executing event handler 'event_by_ssh_restart_service' for service 'remote-http-host!http'
    [2014-09-15 18:58:32 +0200] notice/Process: Running command '/usr/lib64/nagios/plugins/check_by_ssh' '-C' 'test 2 -gt 0 && sudo /etc/init.d/apache2 restart' '-H' '192.168.1.100': PID 32623
    [2014-09-15 18:58:33 +0200] notice/Process: PID 32623 ('/usr/lib64/nagios/plugins/check_by_ssh' '-C' 'test 2 -gt 0 && sudo /etc/init.d/apache2 restart' '-H' '192.168.1.100') terminated with exit code 0

Remote Host Terminal:

    # date; service apache2 status
    Mon Sep 15 18:58:44 CEST 2014
    Apache2 is running (pid 24908).


## <a id="dependencies"></a> Dependencies

Icinga 2 uses host and service [Dependency](9-object-types.md#objecttype-dependency) objects
for determing their network reachability.

A service can depend on a host, and vice versa. A service has an implicit
dependency (parent) to its host. A host to host dependency acts implicitly
as host parent relation.
When dependencies are calculated, not only the immediate parent is taken into
account but all parents are inherited.

The `parent_host_name` and `parent_service_name` attributes are mandatory for
service dependencies, `parent_host_name` is required for host dependencies.
[Apply rules](3-monitoring-basics.md#using-apply) will allow you to
[determine these attributes](3-monitoring-basics.md#dependencies-apply-custom-attributes) in a more
dynamic fashion if required.

    parent_host_name = "core-router"
    parent_service_name = "uplink-port"

Notifications are suppressed by default if a host or service becomes unreachable.
You can control that option by defining the `disable_notifications` attribute.

    disable_notifications = false

If the dependency should be triggered in the parent object's soft state, you
need to set `ignore_soft_states` to `false`.

The dependency state filter must be defined based on the parent object being
either a host (`Up`, `Down`) or a service (`OK`, `Warning`, `Critical`, `Unknown`).

The following example will make the dependency fail and trigger it if the parent
object is **not** in one of these states:

    states = [ OK, Critical, Unknown ]

Rephrased: If the parent service object changes into the `Warning` state, this
dependency will fail and render all child objects (hosts or services) unreachable.

You can determine the child's reachability by querying the `is_reachable` attribute
in for example [DB IDO](23-appendix.md#schema-db-ido-extensions).

### <a id="dependencies-implicit-host-service"></a> Implicit Dependencies for Services on Host

Icinga 2 automatically adds an implicit dependency for services on their host. That way
service notifications are suppressed when a host is `DOWN` or `UNREACHABLE`. This dependency
does not overwrite other dependencies and implicitely sets `disable_notifications = true` and
`states = [ Up ]` for all service objects.

Service checks are still executed. If you want to prevent them from happening, you can
apply the following dependency to all services setting their host as `parent_host_name`
and disabling the checks. `assign where true` matches on all `Service` objects.

    apply Dependency "disable-host-service-checks" to Service {
      disable_checks = true
      assign where true
    }

### <a id="dependencies-network-reachability"></a> Dependencies for Network Reachability

A common scenario is the Icinga 2 server behind a router. Checking internet
access by pinging the Google DNS server `google-dns` is a common method, but
will fail in case the `dsl-router` host is down. Therefore the example below
defines a host dependency which acts implicitly as parent relation too.

Furthermore the host may be reachable but ping probes are dropped by the
router's firewall. In case the `dsl-router`'s `ping4` service check fails, all
further checks for the `ping4` service on host `google-dns` service should
be suppressed. This is achieved by setting the `disable_checks` attribute to `true`.

    object Host "dsl-router" {
      import "generic-host"
      address = "192.168.1.1"
    }

    object Host "google-dns" {
      import "generic-host"
      address = "8.8.8.8"
    }

    apply Service "ping4" {
      import "generic-service"

      check_command = "ping4"

      assign where host.address
    }

    apply Dependency "internet" to Host {
      parent_host_name = "dsl-router"
      disable_checks = true
      disable_notifications = true

      assign where host.name != "dsl-router"
    }

    apply Dependency "internet" to Service {
      parent_host_name = "dsl-router"
      parent_service_name = "ping4"
      disable_checks = true

      assign where host.name != "dsl-router"
    }

### <a id="dependencies-apply-custom-attributes"></a> Apply Dependencies based on Custom Attributes

You can use [apply rules](3-monitoring-basics.md#using-apply) to set parent or
child attributes, e.g. `parent_host_name` to other objects'
attributes.

A common example are virtual machines hosted on a master. The object
name of that master is auto-generated from your CMDB or VMWare inventory
into the host's custom attributes (or a generic template for your
cloud).

Define your master host object:

    /* your master */
    object Host "master.example.com" {
      import "generic-host"
    }

Add a generic template defining all common host attributes:

    /* generic template for your virtual machines */
    template Host "generic-vm" {
      import "generic-host"
    }

Add a template for all hosts on your example.com cloud setting
custom attribute `vm_parent` to `master.example.com`:

    template Host "generic-vm-example.com" {
      import "generic-vm"
      vars.vm_parent = "master.example.com"
    }

Define your guest hosts:

    object Host "www.example1.com" {
      import "generic-vm-master.example.com"
    }

    object Host "www.example2.com" {
      import "generic-vm-master.example.com"
    }

Apply the host dependency to all child hosts importing the
`generic-vm` template and set the `parent_host_name`
to the previously defined custom attribute `host.vars.vm_parent`.

    apply Dependency "vm-host-to-parent-master" to Host {
      parent_host_name = host.vars.vm_parent
      assign where "generic-vm" in host.templates
    }

You can extend this example, and make your services depend on the
`master.example.com` host too. Their local scope allows you to use
`host.vars.vm_parent` similar to the example above.

    apply Dependency "vm-service-to-parent-master" to Service {
      parent_host_name = host.vars.vm_parent
      assign where "generic-vm" in host.templates
    }

That way you don't need to wait for your guest hosts becoming
unreachable when the master host goes down. Instead the services
will detect their reachability immediately when executing checks.

> **Note**
>
> This method with setting locally scoped variables only works in
> apply rules, but not in object definitions.


### <a id="dependencies-agent-checks"></a> Dependencies for Agent Checks

Another classic example are agent based checks. You would define a health check
for the agent daemon responding to your requests, and make all other services
querying that daemon depend on that health check.

The following configuration defines two nrpe based service checks `nrpe-load`
and `nrpe-disk` applied to the host `nrpe-server` [matched](18-library-reference.md#global-functions-match)
by its name. The health check is defined as `nrpe-health` service.

    apply Service "nrpe-health" {
      import "generic-service"
      check_command = "nrpe"
      assign where match("nrpe-*", host.name)
    }

    apply Service "nrpe-load" {
      import "generic-service"
      check_command = "nrpe"
      vars.nrpe_command = "check_load"
      assign where match("nrpe-*", host.name)
    }

    apply Service "nrpe-disk" {
      import "generic-service"
      check_command = "nrpe"
      vars.nrpe_command = "check_disk"
      assign where match("nrpe-*", host.name)
    }

    object Host "nrpe-server" {
      import "generic-host"
      address = "192.168.1.5"
    }

    apply Dependency "disable-nrpe-checks" to Service {
      parent_service_name = "nrpe-health"

      states = [ OK ]
      disable_checks = true
      disable_notifications = true
      assign where service.check_command == "nrpe"
      ignore where service.name == "nrpe-health"
    }

The `disable-nrpe-checks` dependency is applied to all services
on the `nrpe-service` host using the `nrpe` check_command attribute
but not the `nrpe-health` service itself.
