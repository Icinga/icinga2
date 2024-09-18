# Monitoring Basics <a id="monitoring-basics"></a>

This part of the Icinga 2 documentation provides an overview of all the basic
monitoring concepts you need to know to run Icinga 2.
Keep in mind these examples are made with a Linux server. If you are
using Windows, you will need to change the services accordingly. See the [ITL reference](10-icinga-template-library.md#windows-plugins)
 for further information.

## Attribute Value Types <a id="attribute-value-types"></a>

The Icinga 2 configuration uses different value types for attributes.

  Type                                                   | Example
  -------------------------------------------------------|---------------------------------------------------------
  [Number](17-language-reference.md#numeric-literals)    | `5`
  [Duration](17-language-reference.md#duration-literals) | `1m`
  [String](17-language-reference.md#string-literals)     | `"These are notes"`
  [Boolean](17-language-reference.md#boolean-literals)   | `true`
  [Array](17-language-reference.md#array)                | `[ "value1", "value2" ]`
  [Dictionary](17-language-reference.md#dictionary)      | `{ "key1" = "value1", "key2" = false }`

It is important to use the correct value type for object attributes
as otherwise the [configuration validation](11-cli-commands.md#config-validation) will fail.

## Hosts and Services <a id="hosts-services"></a>

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

```
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
```

The example creates two services `ping4` and `http` which belong to the
host `my-server1`.

It also specifies that the host should perform its own check using the `hostalive`
check command.

The `address` attribute is used by check commands to determine which network
address is associated with the host object.

Details on troubleshooting check problems can be found [here](15-troubleshooting.md#troubleshooting).

### Host States <a id="host-states"></a>

Hosts can be in any one of the following states:

  Name        | Description
  ------------|--------------
  UP          | The host is available.
  DOWN        | The host is unavailable.

### Service States <a id="service-states"></a>

Services can be in any one of the following states:

  Name        | Description
  ------------|--------------
  OK          | The service is working properly.
  WARNING     | The service is experiencing some problems but is still considered to be in working condition.
  CRITICAL    | The check successfully determined that the service is in a critical state.
  UNKNOWN     | The check could not determine the service's state.

### Check Result State Mapping <a id="check-result-state-mapping"></a>

[Check plugins](05-service-monitoring.md#service-monitoring-plugins) return
with an exit code which is converted into a state number.
Services map the states directly while hosts will treat `0` or `1` as `UP`
for example.

  Value | Host State | Service State
  ------|------------|--------------
  0     | Up         | OK
  1     | Up         | Warning
  2     | Down       | Critical
  3     | Down       | Unknown

### Hard and Soft States <a id="hard-soft-states"></a>

When detecting a problem with a host/service, Icinga re-checks the object a number of
times (based on the `max_check_attempts` and `retry_interval` settings) before sending
notifications. This ensures that no unnecessary notifications are sent for
transient failures. During this time the object is in a `SOFT` state.

After all re-checks have been executed and the object is still in a non-OK
state, the host/service switches to a `HARD` state and notifications are sent.

  Name        | Description
  ------------|--------------
  HARD        | The host/service's state hasn't recently changed. `check_interval` applies here.
  SOFT        | The host/service has recently changed state and is being re-checked with `retry_interval`.

### Host and Service Checks <a id="host-service-checks"></a>

Hosts and services determine their state by running checks in a regular interval.

```
object Host "router" {
  check_command = "hostalive"
  address = "10.0.0.1"
}
```

The `hostalive` command is one of several built-in check commands. It sends ICMP
echo requests to the IP address specified in the `address` attribute to determine
whether a host is online.

> **Tip**
>
> `hostalive` is the same as `ping` but with different default thresholds.
> Both use the `ping` CLI command to execute sequential checks.
>
> If you need faster ICMP checks, look into the [icmp](10-icinga-template-library.md#plugin-check-command-icmp) CheckCommand.

A number of other [built-in check commands](10-icinga-template-library.md#icinga-template-library) are also
available. In addition to these commands the next few chapters will explain in
detail how to set up your own check commands.

#### Host Check Alternatives <a id="host-check-alternatives"></a>

If the host is not reachable with ICMP, HTTP, etc. you can
also use the [dummy](10-icinga-template-library.md#itl-dummy) CheckCommand to set a default state.

```
object Host "dummy-host" {
  check_command = "dummy"
  vars.dummy_state = 0 //Up
  vars.dummy_text = "Everything OK."
}
```

This method is also used when you send in [external check results](08-advanced-topics.md#external-check-results).

A more advanced technique is to calculate an overall state
based on all services. This is described  [here](08-advanced-topics.md#access-object-attributes-at-runtime-cluster-check).


## Templates <a id="object-inheritance-using-templates"></a>

Templates may be used to apply a set of identical attributes to more than one
object:

```
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
```


In this example the `ping4` and `ping6` services inherit properties from the
template `generic-service`.

Objects as well as templates themselves can import an arbitrary number of
other templates. Attributes inherited from a template can be overridden in the
object if necessary.

You can also import existing non-template objects.

> **Note**
>
> Templates and objects share the same namespace, i.e. you can't define a template
> that has the same name like an object.


### Multiple Templates <a id="object-inheritance-using-multiple-templates"></a>

The following example uses [custom variables](03-monitoring-basics.md#custom-variables) which
are provided in each template. The `web-server` template is used as the
base template for any host providing web services. In addition to that it
specifies the custom variable `webserver_type`, e.g. `apache`. Since this
template is also the base template, we import the `generic-host` template here.
This provides the `check_command` attribute by default and we don't need
to set it anywhere later on.

```
template Host "web-server" {
  import "generic-host"
  vars = {
    webserver_type = "apache"
  }
}
```

The `wp-server` host template specifies a Wordpress instance and sets
the `application_type` custom variable. Please note the `+=` [operator](17-language-reference.md#dictionary-operators)
which adds [dictionary](17-language-reference.md#dictionary) items,
but does not override any previous `vars` attribute.

```
template Host "wp-server" {
  vars += {
    application_type = "wordpress"
  }
}
```

The final host object imports both templates. The order is important here:
First the base template `web-server` is added to the object, then additional
attributes are imported from the `wp-server` object.

```
object Host "wp.example.com" {
  import "web-server"
  import "wp-server"

  address = "192.168.56.200"
}
```

If you want to override specific attributes inherited from templates, you can
specify them on the host object.

```
object Host "wp1.example.com" {
  import "web-server"
  import "wp-server"

  vars.webserver_type = "nginx" //overrides attribute from base template

  address = "192.168.56.201"
}
```

<!-- Keep this for compatibility -->
<a id="custom-attributes"></a>

## Custom Variables <a id="custom-variables"></a>

In addition to built-in object attributes you can define your own custom
attributes inside the `vars` attribute.

> **Tip**
>
> This is called `custom variables` throughout the documentation, backends and web interfaces.
>
> Older documentation versions referred to this as `custom attribute`.

The following example specifies the key `ssh_port` as custom
variable and assigns an integer value.

```
object Host "localhost" {
  check_command = "ssh"
  vars.ssh_port = 2222
}
```

`vars` is a [dictionary](17-language-reference.md#dictionary) where you
can set specific keys to values. The example above uses the shorter
[indexer](17-language-reference.md#indexer) syntax.

An alternative representation can be written like this:

```
  vars = {
    ssh_port = 2222
  }
```

or

```
  vars["ssh_port"] = 2222
```

### Custom Variable Values <a id="custom-variables-values"></a>

Valid values for custom variables include:

* [Strings](17-language-reference.md#string-literals), [numbers](17-language-reference.md#numeric-literals) and [booleans](17-language-reference.md#boolean-literals)
* [Arrays](17-language-reference.md#array) and [dictionaries](17-language-reference.md#dictionary)
* [Functions](03-monitoring-basics.md#custom-variables-functions)

You can also define nested values such as dictionaries in dictionaries.

This example defines the custom variable `disks` as dictionary.
The first key is set to `disk /` is itself set to a dictionary
with one key-value pair.

```
  vars.disks["disk /"] = {
    disk_partitions = "/"
  }
```

This can be written as resolved structure like this:

```
  vars = {
    disks = {
      "disk /" = {
        disk_partitions = "/"
      }
    }
  }
```

Keep this in mind when trying to access specific sub-keys
in apply rules or functions.

Another example which is shown in the example configuration:

```
  vars.notification["mail"] = {
    groups = [ "icingaadmins" ]
  }
```

This defines the `notification` custom variable as dictionary
with the key `mail`. Its value is a dictionary with the key `groups`
which itself has an array as value. Note: This array is the exact
same as the `user_groups` attribute for [notification apply rules](#03-monitoring-basics.md#using-apply-notifications)
expects.

```
  vars.notification = {
    mail = {
      groups = [
        "icingaadmins"
      ]
    }
  }
```

<!-- Keep this for compatibility -->
<a id="custom-attributes-functions"></a>

### Functions as Custom Variables <a id="custom-variables-functions"></a>

Icinga 2 lets you specify [functions](17-language-reference.md#functions) for custom variables.
The special case here is that whenever Icinga 2 needs the value for such a custom variable it runs
the function and uses whatever value the function returns:

```
object CheckCommand "random-value" {
  command = [ PluginDir + "/check_dummy", "0", "$text$" ]

  vars.text = {{ Math.random() * 100 }}
}
```

This example uses the [abbreviated lambda syntax](17-language-reference.md#nullary-lambdas).

These functions have access to a number of variables:

  Variable     | Description
  -------------|---------------
  user         | The User object (for notifications).
  service      | The Service object (for service checks/notifications/event handlers).
  host         | The Host object.
  command      | The command object (e.g. a CheckCommand object for checks).

Here's an example:

```
vars.text = {{ host.check_interval }}
```

In addition to these variables the [macro](18-library-reference.md#scoped-functions-macro) function can be used to retrieve the
value of arbitrary macro expressions:

```
vars.text = {{
  if (macro("$address$") == "127.0.0.1") {
    log("Running a check for localhost!")
  }

  return "Some text"
}}
```

The `resolve_arguments` function can be used to resolve a command and its arguments much in
the same fashion Icinga does this for the `command` and `arguments` attributes for
commands. The `by_ssh` command uses this functionality to let users specify a
command and arguments that should be executed via SSH:

```
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
```

Accessing object attributes at runtime inside these functions is described in the
[advanced topics](08-advanced-topics.md#access-object-attributes-at-runtime) chapter.


## Runtime Macros <a id="runtime-macros"></a>

Macros can be used to access other objects' attributes and [custom variables](03-monitoring-basics.md#custom-variables)
at runtime. For example they are used in command definitions to figure out
which IP address a check should be run against:

```
object CheckCommand "my-ping" {
  command = [ PluginDir + "/check_ping" ]

  arguments = {
    "-H" = "$ping_address$"
    "-w" = "$ping_wrta$,$ping_wpl$%"
    "-c" = "$ping_crta$,$ping_cpl$%"
    "-p" = "$ping_packets$"
  }

  // Resolve from a host attribute, or custom variable.
  vars.ping_address = "$address$"

  // Default values
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
```

In this example we are using the `$address$` macro to refer to the host's `address`
attribute.

We can also directly refer to custom variables, e.g. by using `$ping_wrta$`. Icinga
automatically tries to find the closest match for the attribute you specified. The
exact rules for this are explained in the next section.

> **Note**
>
> When using the `$` sign as single character you must escape it with an
> additional dollar character (`$$`).


### Evaluation Order <a id="macro-evaluation-order"></a>

When executing commands Icinga 2 checks the following objects in this order to look
up macros and their respective values:

1. User object (only for notifications)
2. Service object
3. Host object
4. Command object
5. Global custom variables in the `Vars` constant

This execution order allows you to define default values for custom variables
in your command objects.

Here's how you can override the custom variable `ping_packets` from the previous
example:

```
object Service "ping" {
  host_name = "localhost"
  check_command = "my-ping"

  vars.ping_packets = 10 // Overrides the default value of 5 given in the command
}
```

If a custom variable isn't defined anywhere, an empty value is used and a warning is
written to the Icinga 2 log.

You can also directly refer to a specific attribute -- thereby ignoring these evaluation
rules -- by specifying the full attribute name:

```
$service.vars.ping_wrta$
```

This retrieves the value of the `ping_wrta` custom variable for the service. This
returns an empty value if the service does not have such a custom variable no matter
whether another object such as the host has this attribute.


### Host Runtime Macros <a id="host-runtime-macros"></a>

The following host custom variables are available in all commands that are executed for
hosts or services:

  Name                         | Description
  -----------------------------|--------------
  host.name                    | The name of the host object.
  host.display\_name           | The value of the `display_name` attribute.
  host.state                   | The host's current state. Can be one of `UNREACHABLE`, `UP` and `DOWN`.
  host.state\_id               | The host's current state. Can be one of `0` (up), `1` (down) and `2` (unreachable).
  host.state\_type             | The host's current state type. Can be one of `SOFT` and `HARD`.
  host.check\_attempt          | The current check attempt number.
  host.max\_check\_attempts    | The maximum number of checks which are executed before changing to a hard state.
  host.last\_state             | The host's previous state. Can be one of `UNREACHABLE`, `UP` and `DOWN`.
  host.last\_state\_id         | The host's previous state. Can be one of `0` (up), `1` (down) and `2` (unreachable).
  host.last\_state\_type       | The host's previous state type. Can be one of `SOFT` and `HARD`.
  host.last\_state\_change     | The last state change's timestamp.
  host.downtime\_depth	       | The number of active downtimes.
  host.duration\_sec           | The time since the last state change.
  host.latency                 | The host's check latency.
  host.execution\_time         | The host's check execution time.
  host.output                  | The last check's output.
  host.perfdata                | The last check's performance data.
  host.last\_check             | The timestamp when the last check was executed.
  host.check\_source           | The monitoring instance that performed the last check.
  host.num\_services           | Number of services associated with the host.
  host.num\_services\_ok       | Number of services associated with the host which are in an `OK` state.
  host.num\_services\_warning  | Number of services associated with the host which are in a `WARNING` state.
  host.num\_services\_unknown  | Number of services associated with the host which are in an `UNKNOWN` state.
  host.num\_services\_critical | Number of services associated with the host which are in a `CRITICAL` state.

In addition to these specific runtime macros [host object](09-object-types.md#objecttype-host)
attributes can be accessed too.

### Service Runtime Macros <a id="service-runtime-macros"></a>

The following service macros are available in all commands that are executed for
services:

  Name                         | Description
  -----------------------------|--------------
  service.name                 | The short name of the service object.
  service.display\_name        | The value of the `display_name` attribute.
  service.check\_command       | The short name of the command along with any arguments to be used for the check.
  service.state                | The service's current state. Can be one of `OK`, `WARNING`, `CRITICAL` and `UNKNOWN`.
  service.state\_id            | The service's current state. Can be one of `0` (ok), `1` (warning), `2` (critical) and `3` (unknown).
  service.state\_type          | The service's current state type. Can be one of `SOFT` and `HARD`.
  service.check\_attempt       | The current check attempt number.
  service.max\_check\_attempts | The maximum number of checks which are executed before changing to a hard state.
  service.last\_state          | The service's previous state. Can be one of `OK`, `WARNING`, `CRITICAL` and `UNKNOWN`.
  service.last\_state\_id      | The service's previous state. Can be one of `0` (ok), `1` (warning), `2` (critical) and `3` (unknown).
  service.last\_state\_type    | The service's previous state type. Can be one of `SOFT` and `HARD`.
  service.last\_state\_change  | The last state change's timestamp.
  service.downtime\_depth      | The number of active downtimes.
  service.duration\_sec        | The time since the last state change.
  service.latency              | The service's check latency.
  service.execution\_time      | The service's check execution time.
  service.output               | The last check's output.
  service.perfdata             | The last check's performance data.
  service.last\_check          | The timestamp when the last check was executed.
  service.check\_source        | The monitoring instance that performed the last check.

In addition to these specific runtime macros [service object](09-object-types.md#objecttype-service)
attributes can be accessed too.

### Command Runtime Macros <a id="command-runtime-macros"></a>

The following custom variables are available in all commands:

  Name                   | Description
  -----------------------|--------------
  command.name           | The name of the command object.

### User Runtime Macros <a id="user-runtime-macros"></a>

The following custom variables are available in all commands that are executed for
users:

  Name                   | Description
  -----------------------|--------------
  user.name              | The name of the user object.
  user.display\_name     | The value of the `display_name` attribute.

In addition to these specific runtime macros [user object](09-object-types.md#objecttype-user)
attributes can be accessed too.

### Notification Runtime Macros <a id="notification-runtime-macros"></a>

  Name                   | Description
  -----------------------|--------------
  notification.type      | The type of the notification.
  notification.author    | The author of the notification comment if existing.
  notification.comment   | The comment of the notification if existing.

In addition to these specific runtime macros [notification object](09-object-types.md#objecttype-notification)
attributes can be accessed too.

### Global Runtime Macros <a id="global-runtime-macros"></a>

The following macros are available in all executed commands:

  Name                     | Description
  -------------------------|--------------
  icinga.timet             | Current UNIX timestamp.
  icinga.long\_date\_time  | Current date and time including timezone information. Example: `2014-01-03 11:23:08 +0000`
  icinga.short\_date\_time | Current date and time. Example: `2014-01-03 11:23:08`
  icinga.date              | Current date. Example: `2014-01-03`
  icinga.time              | Current time including timezone information. Example: `11:23:08 +0000`
  icinga.uptime            | Current uptime of the Icinga 2 process.

The following macros provide global statistics:

  Name                                | Description
  ------------------------------------|------------------------------------
  icinga.num\_services\_ok            | Current number of services in state 'OK'.
  icinga.num\_services\_warning       | Current number of services in state 'Warning'.
  icinga.num\_services\_critical      | Current number of services in state 'Critical'.
  icinga.num\_services\_unknown       | Current number of services in state 'Unknown'.
  icinga.num\_services\_pending       | Current number of pending services.
  icinga.num\_services\_unreachable   | Current number of unreachable services.
  icinga.num\_services\_flapping      | Current number of flapping services.
  icinga.num\_services\_in\_downtime  | Current number of services in downtime.
  icinga.num\_services\_acknowledged  | Current number of acknowledged service problems.
  icinga.num\_hosts\_up               | Current number of hosts in state 'Up'.
  icinga.num\_hosts\_down             | Current number of hosts in state 'Down'.
  icinga.num\_hosts\_unreachable      | Current number of unreachable hosts.
  icinga.num\_hosts\_pending          | Current number of pending hosts.
  icinga.num\_hosts\_flapping         | Current number of flapping hosts.
  icinga.num\_hosts\_in\_downtime     | Current number of hosts in downtime.
  icinga.num\_hosts\_acknowledged     | Current number of acknowledged host problems.

### Environment Variable Runtime Macros <a id="env-runtime-macros"></a>

All environment variables of the Icinga process are available as runtime macros
named `env.<env var name>`. E.g. `$env.ProgramFiles$` for ProgramFiles which is
especially useful on Windows. In contrast to the other runtime macros env vars
require the `env.` prefix.


## Apply Rules <a id="using-apply"></a>

Several object types require an object relation, e.g. [Service](09-object-types.md#objecttype-service),
[Notification](09-object-types.md#objecttype-notification), [Dependency](09-object-types.md#objecttype-dependency),
[ScheduledDowntime](09-object-types.md#objecttype-scheduleddowntime) objects. The
object relations are documented in the linked chapters.

If you for example create a service object you have to specify the [host_name](09-object-types.md#objecttype-service)
attribute and reference an existing host attribute.

```
object Service "ping4" {
  check_command = "ping4"
  host_name = "icinga2-agent1.localdomain"
}
```

This isn't comfortable when managing a huge set of configuration objects which could
[match](03-monitoring-basics.md#using-apply-expressions) on a common pattern.

Instead you want to use **[apply](17-language-reference.md#apply) rules**.

If you want basic monitoring for all your hosts, add a `ping4` service apply rule
for all hosts which have the `address` attribute specified. Just one rule for 1000 hosts
instead of 1000 service objects. Apply rules will automatically generate them for you.

```
apply Service "ping4" {
  check_command = "ping4"
  assign where host.address
}
```

More explanations on assign where expressions can be found [here](03-monitoring-basics.md#using-apply-expressions).

### Apply Rules: Prerequisites <a id="using-apply-prerquisites"></a>

Before you start with apply rules keep the following in mind:

* Define the best match.
    * A set of unique [custom variables](03-monitoring-basics.md#custom-variables) for these hosts/services?
    * Or [group](03-monitoring-basics.md#groups) memberships, e.g. a host being a member of a hostgroup which should have a service set?
    * A generic pattern [match](18-library-reference.md#global-functions-match) on the host/service name?
    * [Multiple expressions combined](03-monitoring-basics.md#using-apply-expressions) with `&&` or `||` [operators](17-language-reference.md#expression-operators)
* All expressions must return a boolean value (an empty string is equal to `false` e.g.)

More specific object type requirements are described in these chapters:

* [Apply services to hosts](03-monitoring-basics.md#using-apply-services)
* [Apply notifications to hosts and services](03-monitoring-basics.md#using-apply-notifications)
* [Apply dependencies to hosts and services](03-monitoring-basics.md#using-apply-dependencies)
* [Apply scheduled downtimes to hosts and services](03-monitoring-basics.md#using-apply-scheduledowntimes)

### Apply Rules: Usage Examples <a id="using-apply-usage-examples"></a>

You can set/override object attributes in apply rules using the respectively available
objects in that scope (host and/or service objects).

```
vars.application_type = host.vars.application_type
```

[Custom variables](03-monitoring-basics.md#custom-variables) can also store
nested dictionaries and arrays. That way you can use them for not only matching
for their existence or values in apply expressions, but also assign
("inherit") their values into the generated objected from apply rules.

Remember the examples shown for [custom variable values](03-monitoring-basics.md#custom-variables-values):

```
  vars.notification["mail"] = {
    groups = [ "icingaadmins" ]
  }
```

You can do two things here:

* Check for the existence of the `notification` custom variable and its nested dictionary key `mail`.
If this is boolean true, the notification object will be generated.
* Assign the value of the `groups` key to the `user_groups` attribute.

```
apply Notification "mail-icingaadmin" to Host {
  [...]

  user_groups = host.vars.notification.mail.groups

  assign where host.vars.notification.mail
}

```

A more advanced example is to use [apply rules with for loops on arrays or
dictionaries](03-monitoring-basics.md#using-apply-for) provided by
[custom atttributes](03-monitoring-basics.md#custom-variables) or groups.

Remember the examples shown for [custom variable values](03-monitoring-basics.md#custom-variables-values):

```
  vars.disks["disk /"] = {
    disk_partitions = "/"
  }
```

You can iterate over all dictionary keys defined in `disks`.
You can optionally use the value to specify additional object attributes.

```
apply Service for (disk => config in host.vars.disks) {
  [...]

  vars.disk_partitions = config.disk_partitions
}
```

Please read the [apply for chapter](03-monitoring-basics.md#using-apply-for)
for more specific insights.


> **Tip**
>
> Building configuration in that dynamic way requires detailed information
> of the generated objects. Use the `object list` [CLI command](11-cli-commands.md#cli-command-object)
> after successful [configuration validation](11-cli-commands.md#config-validation).


### Apply Rules Expressions <a id="using-apply-expressions"></a>

You can use simple or advanced combinations of apply rule expressions. Each
expression must evaluate into the boolean `true` value. An empty string
will be for instance interpreted as `false`. In a similar fashion undefined
attributes will return `false`.

Returns `false`:

```
assign where host.vars.attribute_does_not_exist
```

Multiple `assign where` condition rows are evaluated as `OR` condition.

You can combine multiple expressions for matching only a subset of objects. In some cases,
you want to be able to add more than one assign/ignore where expression which matches
a specific condition. To achieve this you can use the logical `and` and `or` operators.

#### Apply Rules Expressions Examples <a id="using-apply-expressions-examples"></a>

Assign a service to a specific host in a host group [array](18-library-reference.md#array-type) using the [in operator](17-language-reference.md#expression-operators):

```
assign where "hostgroup-dev" in host.groups
```

Assign an object when a custom variable is [equal](17-language-reference.md#expression-operators) to a value:

```
assign where host.vars.application_type == "database"

assign where service.vars.sms_notify == true
```

Assign an object if a dictionary [contains](18-library-reference.md#dictionary-contains) a given key:

```
assign where host.vars.app_dict.contains("app")
```

Match the host name by either using a [case insensitive match](18-library-reference.md#global-functions-match):

```
assign where match("webserver*", host.name)
```

Match the host name by using a [regular expression](18-library-reference.md#global-functions-regex). Please note the [escaped](17-language-reference.md#string-literals-escape-sequences) backslash character:

```
assign where regex("^webserver-[\\d+]", host.name)
```

[Match](18-library-reference.md#global-functions-match) all `*mysql*` patterns in the host name and (`&&`) custom variable `prod_mysql_db`
matches the `db-*` pattern. All hosts with the custom variable `test_server` set to `true`
should be ignored, or any host name ending with `*internal` pattern.

```
object HostGroup "mysql-server" {
  display_name = "MySQL Server"

  assign where match("*mysql*", host.name) && match("db-*", host.vars.prod_mysql_db)
  ignore where host.vars.test_server == true
  ignore where match("*internal", host.name)
}
```

Similar example for advanced notification apply rule filters: If the service
attribute `notes` [matches](18-library-reference.md#global-functions-match) the `has gold support 24x7` string `AND` one of the
two condition passes, either the `customer` host custom variable is set to `customer-xy`
`OR` the host custom variable `always_notify` is set to `true`.

The notification is ignored for services whose host name ends with `*internal`
`OR` the `priority` custom variable is [less than](17-language-reference.md#expression-operators) `2`.

```
template Notification "cust-xy-notification" {
  users = [ "noc-xy", "mgmt-xy" ]
  command = "mail-service-notification"
}

apply Notification "notify-cust-xy-mysql" to Service {
  import "cust-xy-notification"

  assign where match("*has gold support 24x7*", service.notes) && (host.vars.customer == "customer-xy" || host.vars.always_notify == true)
  ignore where match("*internal", host.name) || (service.vars.priority < 2 && host.vars.is_clustered == true)
}
```

More advanced examples are covered [here](08-advanced-topics.md#use-functions-assign-where).

### Apply Services to Hosts <a id="using-apply-services"></a>

The sample configuration already includes a detailed example in [hosts.conf](04-configuration.md#hosts-conf)
and [services.conf](04-configuration.md#services-conf) for this use case.

The example for `ssh` applies a service object to all hosts with the `address`
attribute being defined and the custom variable `os` set to the string `Linux` in `vars`.

```
apply Service "ssh" {
  import "generic-service"

  check_command = "ssh"

  assign where host.address && host.vars.os == "Linux"
}
```

Other detailed examples are used in their respective chapters, for example
[apply services with custom command arguments](03-monitoring-basics.md#command-passing-parameters).

### Apply Notifications to Hosts and Services <a id="using-apply-notifications"></a>

Notifications are applied to specific targets (`Host` or `Service`) and work in a similar
manner:

```
apply Notification "mail-noc" to Service {
  import "mail-service-notification"

  user_groups = [ "noc" ]

  assign where host.vars.notification.mail
}
```

In this example the `mail-noc` notification will be created as object for all services having the
`notification.mail` custom variable defined. The notification command is set to `mail-service-notification`
and all members of the user group `noc` will get notified.

It is also possible to generally apply a notification template and dynamically overwrite values from
the template by checking for custom variables. This can be achieved by using [conditional statements](17-language-reference.md#conditional-statements):

```
apply Notification "host-mail-noc" to Host {
  import "mail-host-notification"

  // replace interval inherited from `mail-host-notification` template with new notfication interval set by a host custom variable
  if (host.vars.notification_interval) {
    interval = host.vars.notification_interval
  }

  // same with notification period
  if (host.vars.notification_period) {
    period = host.vars.notification_period
  }

  // Send SMS instead of email if the host's custom variable `notification_type` is set to `sms`
  if (host.vars.notification_type == "sms") {
    command = "sms-host-notification"
  } else {
    command = "mail-host-notification"
  }

  user_groups = [ "noc" ]

  assign where host.address
}
```

In the example above the notification template `mail-host-notification`
contains all relevant notification settings.
The apply rule is applied on all host objects where the `host.address` is defined.

If the host object has a specific custom variable set, its value is inherited
into the local notification object scope, e.g. `host.vars.notification_interval`,
`host.vars.notification_period` and `host.vars.notification_type`.
This overwrites attributes already specified in the imported `mail-host-notification`
template.

The corresponding host object could look like this:

```
object Host "host1" {
  import "host-linux-prod"
  display_name = "host1"
  address = "192.168.1.50"
  vars.notification_interval = 1h
  vars.notification_period = "24x7"
  vars.notification_type = "sms"
}
```

### Apply Dependencies to Hosts and Services <a id="using-apply-dependencies"></a>

Detailed examples can be found in the [dependencies](03-monitoring-basics.md#dependencies) chapter.

### Apply Recurring Downtimes to Hosts and Services <a id="using-apply-scheduledowntimes"></a>

The sample configuration includes an example in [downtimes.conf](04-configuration.md#downtimes-conf).

Detailed examples can be found in the [recurring downtimes](08-advanced-topics.md#recurring-downtimes) chapter.


### Using Apply For Rules <a id="using-apply-for"></a>

Next to the standard way of using [apply rules](03-monitoring-basics.md#using-apply)
there is the requirement of applying objects based on a set (array or
dictionary) using [apply for](17-language-reference.md#apply-for) expressions.

The sample configuration already includes a detailed example in [hosts.conf](04-configuration.md#hosts-conf)
and [services.conf](04-configuration.md#services-conf) for this use case.

Take the following example: A host provides the snmp oids for different service check
types. This could look like the following example:

```
object Host "router-v6" {
  check_command = "hostalive"
  address6 = "2001:db8:1234::42"

  vars.oids["if01"] = "1.1.1.1.1"
  vars.oids["temp"] = "1.1.1.1.2"
  vars.oids["bgp"] = "1.1.1.1.5"
}
```

The idea is to create service objects for `if01` and `temp` but not `bgp`.
The oid value should also be used as service custom variable `snmp_oid`.
This is the command argument required by the [snmp](10-icinga-template-library.md#plugin-check-command-snmp)
check command.
The service's `display_name` should be set to the identifier inside the dictionary,
e.g. `if01`.

```
apply Service for (identifier => oid in host.vars.oids) {
  check_command = "snmp"
  display_name = identifier
  vars.snmp_oid = oid

  ignore where identifier == "bgp" //don't generate service for bgp checks
}
```

Icinga 2 evaluates the `apply for` rule for all objects with the custom variable
`oids` set.
It iterates over all dictionary items inside the `for` loop and evaluates the
`assign/ignore where` expressions. You can access the loop variable
in these expressions, e.g. to ignore specific values.

In this example the `bgp` identifier is ignored. This avoids to generate
unwanted services. A different approach would be to match the `oid` value with a
[regex](18-library-reference.md#global-functions-regex)/[wildcard match](18-library-reference.md#global-functions-match) pattern for example.

```
  ignore where regex("^\d.\d.\d.\d.5$", oid)
```

> **Note**
>
> You don't need an `assign where` expression which checks for the existence of the
> `oids` custom variable.

This method saves you from creating multiple apply rules. It also moves
the attribute specification logic from the service to the host.

<!-- Keep this for compatibility -->
<a id="using-apply-for-custom-attribute-override"></a>

#### Apply For and Custom Variable Override <a id="using-apply-for-custom-variable-override"></a>

Imagine a different more advanced example: You are monitoring your network device (host)
with many interfaces (services). The following requirements/problems apply:

* Each interface service should be named with a prefix and a name defined in your host object (which could be generated from your CMDB, etc.)
* Each interface has its own VLAN tag
* Some interfaces have QoS enabled
* Additional attributes such as `display_name` or `notes`, `notes_url` and `action_url` must be
dynamically generated.


> **Tip**
>
> Define the SNMP community as global constant in your [constants.conf](04-configuration.md#constants-conf) file.

```
const IftrafficSnmpCommunity = "public"
```

Define the `interfaces` [custom variable](03-monitoring-basics.md#custom-variables)
on the `cisco-catalyst-6509-34` host object and add three example interfaces as dictionary keys.

Specify additional attributes inside the nested dictionary
as learned with [custom variable values](03-monitoring-basics.md#custom-variables-values):

```
object Host "cisco-catalyst-6509-34" {
  import "generic-host"
  display_name = "Catalyst 6509 #34 VIE21"
  address = "127.0.1.4"

  /* "GigabitEthernet0/2" is the interface name,
   * and key name in service apply for later on
   */
  vars.interfaces["GigabitEthernet0/2"] = {
     /* define all custom variables with the
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
     vlan = "remote"
     qos = "enabled"
  }
  vars.interfaces["MgmtInterface1"] = {
     iftraffic_community = IftrafficSnmpCommunity
     vlan = "mgmt"
     interface_address = "127.99.0.100" #special management ip
  }
}
```

Start with the apply for definition and iterate over `host.vars.interfaces`.
This is a dictionary and should use the variables `interface_name` as key
and `interface_config` as value for each generated object scope.

`"if-"` specifies the object name prefix for each service which results
in `if-<interface_name>` for each iteration.

```
/* loop over the host.vars.interfaces dictionary
 * for (key => value in dict) means `interface_name` as key
 * and `interface_config` as value. Access config attributes
 * with the indexer (`.`) character.
 */
apply Service "if-" for (interface_name => interface_config in host.vars.interfaces) {
```

Import the `generic-service` template, assign the [iftraffic](10-icinga-template-library.md#plugin-contrib-command-iftraffic)
`check_command`. Use the dictionary key `interface_name` to set a proper `display_name`
string for external interfaces.

```
  import "generic-service"
  check_command = "iftraffic"
  display_name = "IF-" + interface_name
```

The `interface_name` key's value is the same string used as command parameter for
`iftraffic`:

```
  /* use the key as command argument (no duplication of values in host.vars.interfaces) */
  vars.iftraffic_interface = interface_name
```

Remember that `interface_config` is a nested dictionary. In the first iteration it looks
like this:

```
interface_config = {
  iftraffic_units = "g"
  iftraffic_community = IftrafficSnmpCommunity
  iftraffic_bandwidth = 1
  vlan = "internal"
  qos = "disabled"
}
```

Access the dictionary keys with the [indexer](17-language-reference.md#indexer) syntax
and assign them to custom variables used as command parameters for the `iftraffic`
check command.

```
  /* map the custom variables as command arguments */
  vars.iftraffic_units = interface_config.iftraffic_units
  vars.iftraffic_community = interface_config.iftraffic_community
```

If you just want to inherit all attributes specified inside the `interface_config`
dictionary, add it to the generated service custom variables like this:

```
  /* the above can be achieved in a shorter fashion if the names inside host.vars.interfaces
   * are the _exact_ same as required as command parameter by the check command
   * definition.
   */
  vars += interface_config
```

If the user did not specify default values for required service custom variables,
add them here. This also helps to avoid unwanted configuration validation errors or
runtime failures. Please read more about conditional statements [here](17-language-reference.md#conditional-statements).

```
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
```

If the host object did not specify a custom SNMP community,
set a default value specified by the [global constant](17-language-reference.md#constants) `IftrafficSnmpCommunity`.

```
  /* set the global constant if not explicitely
   * not provided by the `interfaces` dictionary on the host
   */
  if (len(interface_config.iftraffic_community) == 0 || len(vars.iftraffic_community) == 0) {
    vars.iftraffic_community = IftrafficSnmpCommunity
  }
```

Use the provided values to [calculate](17-language-reference.md#expression-operators)
more object attributes which can be e.g. seen in external interfaces.

```
  /* Calculate some additional object attributes after populating the `vars` dictionary */
  notes = "Interface check for " + interface_name + " (units: '" + interface_config.iftraffic_units + "') in VLAN '" + vars.vlan + "' with ' QoS '" + vars.qos + "'"
  notes_url = "https://foreman.company.com/hosts/" + host.name
  action_url = "https://snmp.checker.company.com/" + host.name + "/if-" + interface_name
}
```

> **Tip**
>
> Building configuration in that dynamic way requires detailed information
> of the generated objects. Use the `object list` [CLI command](11-cli-commands.md#cli-command-object)
> after successful [configuration validation](11-cli-commands.md#config-validation).

Verify that the apply-for-rule successfully created the service objects with the
inherited custom variables:

```
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
    * vlan = "remote"

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
```

### Use Object Attributes in Apply Rules <a id="using-apply-object-attributes"></a>

Since apply rules are evaluated after the generic objects, you
can reference existing host and/or service object attributes as
values for any object attribute specified in that apply rule.

```
object Host "opennebula-host" {
  import "generic-host"
  address = "10.1.1.2"

  vars.hosting["cust1"] = {
    http_uri = "/shop"
    customer_name = "Customer 1"
    customer_id = "7568"
    support_contract = "gold"
  }
  vars.hosting["cust2"] = {
    http_uri = "/"
    customer_name = "Customer 2"
    customer_id = "7569"
    support_contract = "silver"
  }
}
```

`hosting` is a custom variable with the Dictionary value type.
This is mandatory to iterate with the `key => value` notation
in the below apply for rule.

```
apply Service for (customer => config in host.vars.hosting) {
  import "generic-service"
  check_command = "ping4"

  vars.qos = "disabled"

  vars += config

  vars.http_uri = "/" + customer + "/" + config.http_uri

  display_name = "Shop Check for " + vars.customer_name + "-" + vars.customer_id

  notes = "Support contract: " + vars.support_contract + " for Customer " + vars.customer_name + " (" + vars.customer_id + ")."

  notes_url = "https://foreman.company.com/hosts/" + host.name
  action_url = "https://snmp.checker.company.com/" + host.name + "/" + vars.customer_id
}
```

Each loop iteration has different values for `customer` and config`
in the local scope.

1.

```
customer = "cust 1"
config = {
  http_uri = "/shop"
  customer_name = "Customer 1"
  customer_id = "7568"
  support_contract = "gold"
}
```

2.

```
customer = "cust2"
config = {
  http_uri = "/"
  customer_name = "Customer 2"
  customer_id = "7569"
  support_contract = "silver"
}
```

You can now add the `config` dictionary into `vars`.

```
vars += config
```

Now it looks like the following in the first iteration:

```
customer = "cust 1"
vars = {
  http_uri = "/shop"
  customer_name = "Customer 1"
  customer_id = "7568"
  support_contract = "gold"
}
```

Remember, you know this structure already. Custom
attributes can also be accessed by using the [indexer](17-language-reference.md#indexer)
syntax.

```
  vars.http_uri = ... + config.http_uri
```

can also be written as

```
  vars += config
  vars.http_uri = ... + vars.http_uri
```


## Groups <a id="groups"></a>

A group is a collection of similar objects. Groups are primarily used as a
visualization aid in web interfaces.

Group membership is defined at the respective object itself. If
you have a hostgroup name `windows` for example, and want to assign
specific hosts to this group for later viewing the group on your
alert dashboard, first create a HostGroup object:

```
object HostGroup "windows" {
  display_name = "Windows Servers"
}
```

Then add your hosts to this group:

```
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
```

This can be done for service and user groups the same way:

```
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
```

### Group Membership Assign <a id="group-assign-intro"></a>

Instead of manually assigning each object to a group you can also assign objects
to a group based on their attributes:

```
object HostGroup "prod-mssql" {
  display_name = "Production MSSQL Servers"

  assign where host.vars.mssql_port && host.vars.prod_mysql_db
  ignore where host.vars.test_server == true
  ignore where match("*internal", host.name)
}
```

In this example all hosts with the `vars` attribute `mssql_port`
will be added as members to the host group `mssql`. However, all
hosts [matching](18-library-reference.md#global-functions-match) the string `\*internal`
or with the `test_server` attribute set to `true` are **not** added to this group.

Details on the `assign where` syntax can be found in the
[Language Reference](17-language-reference.md#apply).

## Notifications <a id="alert-notifications"></a>

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

The user `icingaadmin` in the example below will get notified only on `Warning` and
`Critical` problems. In addition to that `Recovery` notifications are sent (they require
the `OK` state).

```
object User "icingaadmin" {
  display_name = "Icinga 2 Admin"
  enable_notifications = true
  states = [ OK, Warning, Critical ]
  types = [ Problem, Recovery ]
  email = "icinga@localhost"
}
```

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

An example notification command is explained [here](03-monitoring-basics.md#notification-commands).

You can add all shared attributes to a `Notification` template which is inherited
to the defined notifications. That way you'll save duplicated attributes in each
`Notification` object. Attributes can be overridden locally.

```
template Notification "generic-notification" {
  interval = 15m

  command = "mail-service-notification"

  states = [ Warning, Critical, Unknown ]
  types = [ Problem, Acknowledgement, Recovery, Custom, FlappingStart,
            FlappingEnd, DowntimeStart, DowntimeEnd, DowntimeRemoved ]

  period = "24x7"
}
```

The time period `24x7` is included as example configuration with Icinga 2.

Use the `apply` keyword to create `Notification` objects for your services:

```
apply Notification "notify-cust-xy-mysql" to Service {
  import "generic-notification"

  users = [ "noc-xy", "mgmt-xy" ]

  assign where match("*has gold support 24x7*", service.notes) && (host.vars.customer == "customer-xy" || host.vars.always_notify == true
  ignore where match("*internal", host.name) || (service.vars.priority < 2 && host.vars.is_clustered == true)
}
```


Instead of assigning users to notifications, you can also add the `user_groups`
attribute with a list of user groups to the `Notification` object. Icinga 2 will
send notifications to all group members.

> **Note**
>
> Only users who have been notified of a problem before  (`Warning`, `Critical`, `Unknown`
states for services, `Down` for hosts) will receive `Recovery` notifications.

Icinga 2 v2.10 allows you to configure a `User` object with `Acknowledgement` and/or `Recovery`
without a `Problem` notification. These notifications will be sent without
any problem notifications beforehand, and can be used for e.g. ticket systems.

```
object User "ticketadmin" {
  display_name = "Ticket Admin"
  enable_notifications = true
  states = [ OK, Warning, Critical ]
  types = [ Acknowledgement, Recovery ]
  email = "ticket@localhost"
}
```

### Notifications: Users from Host/Service <a id="alert-notifications-users-host-service"></a>

A common pattern is to store the users and user groups
on the host or service objects instead of the notification
object itself.

The sample configuration provided in [hosts.conf](04-configuration.md#hosts-conf) and [notifications.conf](04-configuration.md#notifications-conf)
already provides an example for this question.

> **Tip**
>
> Please make sure to read the [apply](03-monitoring-basics.md#using-apply) and
> [custom variable values](03-monitoring-basics.md#custom-variables-values) chapter to
> fully understand these examples.


Specify the user and groups as nested custom variable on the host object:

```
object Host "icinga2-agent1.localdomain" {
  [...]

  vars.notification["mail"] = {
    groups = [ "icingaadmins" ]
    users = [ "icingaadmin" ]
  }
  vars.notification["sms"] = {
    users = [ "icingaadmin" ]
  }
}
```

As you can see, there is the option to use two different notification
apply rules here: One for `mail` and one for `sms`.

This example assigns the `users` and `groups` nested keys from the `notification`
custom variable to the actual notification object attributes.

Since errors are hard to debug if host objects don't specify the required
configuration attributes, you can add a safety condition which logs which
host object is affected.

```
critical/config: Host 'icinga2-client3.localdomain' does not specify required user/user_groups configuration attributes for notification 'mail-icingaadmin'.
```

You can also use the [script debugger](20-script-debugger.md#script-debugger) for more advanced insights.

```
apply Notification "mail-host-notification" to Host {
  [...]

  /* Log which host does not specify required user/user_groups attributes. This will fail immediately during config validation and help a lot. */
  if (len(host.vars.notification.mail.users) == 0 && len(host.vars.notification.mail.user_groups) == 0) {
    log(LogCritical, "config", "Host '" + host.name + "' does not specify required user/user_groups configuration attributes for notification '" + name + "'.")
  }

  users = host.vars.notification.mail.users
  user_groups = host.vars.notification.mail.groups

  assign where host.vars.notification.mail && typeof(host.vars.notification.mail) == Dictionary
}

apply Notification "sms-host-notification" to Host {
  [...]

  /* Log which host does not specify required user/user_groups attributes. This will fail immediately during config validation and help a lot. */
  if (len(host.vars.notification.sms.users) == 0 && len(host.vars.notification.sms.user_groups) == 0) {
    log(LogCritical, "config", "Host '" + host.name + "' does not specify required user/user_groups configuration attributes for notification '" + name + "'.")
  }

  users = host.vars.notification.sms.users
  user_groups = host.vars.notification.sms.groups

  assign where host.vars.notification.sms && typeof(host.vars.notification.sms) == Dictionary
}
```

The example above uses [typeof](18-library-reference.md#global-functions-typeof) as safety function to ensure that
the `mail` key really provides a dictionary as value. Otherwise
the configuration validation could fail if an admin adds something
like this on another host:

```
  vars.notification.mail = "yes"
```


You can also do a more fine granular assignment on the service object:

```
apply Service "http" {
  [...]

  vars.notification["mail"] = {
    groups = [ "icingaadmins" ]
    users = [ "icingaadmin" ]
  }

  [...]
}
```

This notification apply rule is different to the one above. The service
notification users and groups are inherited from the service and if not set,
from the host object. A default user is set too.

```
apply Notification "mail-service-notification" to Service {
  [...]

  if (service.vars.notification.mail.users) {
    users = service.vars.notification.mail.users
  } else if (host.vars.notification.mail.users) {
    users = host.vars.notification.mail.users
  } else {
    /* Default user who receives everything. */
    users = [ "icingaadmin" ]
  }

  if (service.vars.notification.mail.groups) {
    user_groups = service.vars.notification.mail.groups
  } else if (host.vars.notification.mail.groups) {
    user_groups = host.vars.notification.mail.groups
  }

  assign where ( host.vars.notification.mail && typeof(host.vars.notification.mail) == Dictionary ) || ( service.vars.notification.mail && typeof(service.vars.notification.mail) == Dictionary )
}
```

### Notification Escalations <a id="notification-escalations"></a>

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

```
object User "icinga-oncall-2nd-level" {
  display_name = "Icinga 2nd Level"

  vars.mobile = "+1 555 424642"
}

object User "icinga-oncall-1st-level" {
  display_name = "Icinga 1st Level"

  vars.mobile = "+1 555 424642"
}
```

Define an additional [NotificationCommand](03-monitoring-basics.md#notification-commands) for SMS notifications.

> **Note**
>
> The example is not complete as there are many different SMS providers.
> Please note that sending SMS notifications will require an SMS provider
> or local hardware with an active SIM card.

```
object NotificationCommand "sms-notification" {
   command = [
     PluginDir + "/send_sms_notification",
     "$mobile$",
     "..."
}
```

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

```
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
```

### Notification Delay <a id="notification-delay"></a>

Sometimes the problem in question should not be announced when the notification is due
(the object reaching the `HARD` state), but after a certain period. In Icinga 2
you can use the `times` dictionary and set `begin = 15m` as key and value if you want to
postpone the notification window for 15 minutes. Leave out the `end` key -- if not set,
Icinga 2 will not check against any end time for this notification.

> **Note**
>
> Setting the `end` key to `0` will stop sending notifications immediately
> when a problem occurs, effectively disabling the notification.

Make sure to specify a relatively low notification `interval` to get notified soon enough again.

```
apply Notification "mail" to Service {
  import "generic-notification"

  command = "mail-notification"
  users = [ "icingaadmin" ]

  interval = 5m

  times.begin = 15m // delay notification window

  assign where service.name == "ping4"
}
```

Also note that this mechanism doesn't take downtimes etc. into account, only
the `HARD` state change time matters. E.g. for a problem which occurred in the
middle of a downtime from 2 PM to 4 PM `times.begin = 2h` means 5 PM, not 6 PM.

### Disable Re-notifications <a id="disable-renotification"></a>

If you prefer to be notified only once, you can disable re-notifications by setting the
`interval` attribute to `0`.

```
apply Notification "notify-once" to Service {
  import "generic-notification"

  command = "mail-notification"
  users = [ "icingaadmin" ]

  interval = 0 // disable re-notification

  assign where service.name == "ping4"
}
```

### Notification Filters by State and Type <a id="notification-filters-state-type"></a>

If there are no notification state and type filter attributes defined at the `Notification`
or `User` object, Icinga 2 assumes that all states and types are being notified.

Available state and type filters for notifications are:

```
template Notification "generic-notification" {

  states = [ OK, Warning, Critical, Unknown ]
  types = [ Problem, Acknowledgement, Recovery, Custom, FlappingStart,
            FlappingEnd, DowntimeStart, DowntimeEnd, DowntimeRemoved ]
}
```


## Commands <a id="commands"></a>

Icinga 2 uses three different command object types to specify how
checks should be performed, notifications should be sent, and
events should be handled.

### Check Commands <a id="check-commands"></a>

[CheckCommand](09-object-types.md#objecttype-checkcommand) objects define the command line how
a check is called.

[CheckCommand](09-object-types.md#objecttype-checkcommand) objects are referenced by
[Host](09-object-types.md#objecttype-host) and [Service](09-object-types.md#objecttype-service) objects
using the `check_command` attribute.

> **Note**
>
> Make sure that the [checker](11-cli-commands.md#enable-features) feature is enabled in order to
> execute checks.

#### Integrate the Plugin with a CheckCommand Definition <a id="command-plugin-integration"></a>

Unless you have done so already, download your check plugin and put it
into the [PluginDir](04-configuration.md#constants-conf) directory. The following example uses the
`check_mysql` plugin contained in the Monitoring Plugins package.

The plugin path and all command arguments are made a list of
double-quoted string arguments for proper shell escaping.

Call the `check_mysql` plugin with the `--help` parameter to see
all available options. Our example defines warning (`-w`) and
critical (`-c`) thresholds.

```
icinga@icinga2 $ /usr/lib64/nagios/plugins/check_mysql --help
...
This program tests connections to a MySQL server

Usage:
check_mysql [-d database] [-H host] [-P port] [-s socket]
[-u user] [-p password] [-S] [-l] [-a cert] [-k key]
[-C ca-cert] [-D ca-dir] [-L ciphers] [-f optfile] [-g group]
```

Next step is to understand how [command parameters](03-monitoring-basics.md#command-passing-parameters)
are being passed from a host or service object, and add a [CheckCommand](09-object-types.md#objecttype-checkcommand)
definition based on these required parameters and/or default values.

Please continue reading in the [plugins section](05-service-monitoring.md#service-monitoring-plugins) for additional integration examples.

#### Passing Check Command Parameters from Host or Service <a id="command-passing-parameters"></a>

Check command parameters are defined as custom variables which can be accessed as runtime macros
by the executed check command.

The check command parameters for ITL provided plugin check command definitions are documented
[here](10-icinga-template-library.md#icinga-template-library), for example
[disk](10-icinga-template-library.md#plugin-check-command-disk).

In order to practice passing command parameters you should [integrate your own plugin](03-monitoring-basics.md#command-plugin-integration).

The following example will use `check_mysql` provided by the [Monitoring Plugins](https://www.monitoring-plugins.org/).

Define the default check command custom variables, for example `mysql_user` and `mysql_password`
(freely definable naming schema) and optional their default threshold values. You can
then use these custom variables as runtime macros for [command arguments](03-monitoring-basics.md#command-arguments)
on the command line.

> **Tip**
>
> Use a common command type as prefix for your command arguments to increase
> readability. `mysql_user` helps understanding the context better than just
> `user` as argument.

The default custom variables can be overridden by the custom variables
defined in the host or service using the check command `my-mysql`. The custom variables
can also be inherited from a parent template using additive inheritance (`+=`).

```
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
```

The check command definition also sets `mysql_host` to the `$address$` default value. You can override
this command parameter if for example your MySQL host is not running on the same server's ip address.

Make sure pass all required command parameters, such as `mysql_user`, `mysql_password` and `mysql_database`.
`MysqlUsername` and `MysqlPassword` are specified as [global constants](04-configuration.md#constants-conf)
in this example.

```
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
```


Take a different example: The example host configuration in [hosts.conf](04-configuration.md#hosts-conf)
also applies an `ssh` service check. Your host's ssh port is not the default `22`, but set to `2022`.
You can pass the command parameter as custom variable `ssh_port` directly inside the service apply rule
inside [services.conf](04-configuration.md#services-conf):

```
apply Service "ssh" {
  import "generic-service"

  check_command = "ssh"
  vars.ssh_port = 2022 //custom command parameter

  assign where (host.address || host.address6) && host.vars.os == "Linux"
}
```

If you prefer this being configured at the host instead of the service, modify the host configuration
object instead. The runtime macro resolving order is described [here](03-monitoring-basics.md#macro-evaluation-order).

```
object Host "icinga2-agent1.localdomain {
...
  vars.ssh_port = 2022
}
```

#### Passing Check Command Parameters Using Apply For <a id="command-passing-parameters-apply-for"></a>

The host `localhost` with the generated services from the `basic-partitions` dictionary (see
[apply for](03-monitoring-basics.md#using-apply-for) for details) checks a basic set of disk partitions
with modified custom variables (warning thresholds at `10%`, critical thresholds at `5%`
free disk space).

The custom variable `disk_partition` can either hold a single string or an array of
string values for passing multiple partitions to the `check_disk` check plugin.

```
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
```


More details on using arrays in custom variables can be found in
[this chapter](03-monitoring-basics.md#custom-variables).


#### Command Arguments <a id="command-arguments"></a>

Next to the short `command` array specified in the command object,
it is advised to define plugin/script parameters in the `arguments`
dictionary attribute.

The value of the `--parameter` key itself is a dictionary with additional
keys. They allow to create generic command objects and are also for documentation
purposes, e.g. with the `description` field copying the plugin's help text in there.
The Icinga Director uses this field to show the argument's purpose when selecting it.

```
  arguments = {
    "--parameter" = {
      description = "..."
      value = "..."
    }
  }
```

Each argument is optional by default and is omitted if
the value is not set.

Learn more about integrating plugins with CheckCommand
objects in [this chapter](05-service-monitoring.md#service-monitoring-plugin-checkcommand).

There are additional possibilities for creating a command only once,
with different parameters and arguments, shown below.

##### Command Arguments: Value <a id="command-arguments-value"></a>

In order to find out about the command argument, call the plugin's help
or consult the README.

```
./check_systemd --help

...

  -u UNIT, --unit UNIT  Name of the systemd unit that is beeing tested.
```

Whenever the long parameter name is available, prefer this over the short one.

```
  arguments = {
    "--unit" = {

    }
  }
```

Define a unique `prefix` for the command's specific arguments. Best practice is to follow this schema:

```
<command name>_<parameter name>
```

Therefore use `systemd_` as prefix, and use the long plugin parameter name `unit` inside the [runtime macro](03-monitoring-basics.md#runtime-macros)
syntax.

```
  arguments = {
    "--unit" = {
      value = "$systemd_unit$"
    }
  }
```

In order to specify a default value, specify
a [custom variable](03-monitoring-basics.md#custom-variables) inside
the CheckCommand object.

```
  vars.systemd_unit = "icinga2"
```

This value can be overridden from the host/service
object as command parameters.


##### Command Arguments: Description <a id="command-arguments-description"></a>

Best practice, also inside the [ITL](10-icinga-template-library.md#icinga-template-library), is to always
copy the command parameter help output into the `description`
field of your check command.

Learn more about integrating plugins with CheckCommand
objects in [this chapter](05-service-monitoring.md#service-monitoring-plugin-checkcommand).

With the [example above](03-monitoring-basics.md#command-arguments-value),
inspect the parameter's help text.

```
./check_systemd --help

...

  -u UNIT, --unit UNIT  Name of the systemd unit that is beeing tested.
```

Copy this into the command arguments `description` entry.

```
  arguments = {
    "--unit" = {
      value = "$systemd_unit$"
      description = "Name of the systemd unit that is beeing tested."
    }
  }
```

##### Command Arguments: Required <a id="command-arguments-required"></a>

Specifies whether this command argument is required, or not. By
default all arguments are optional.

> **Tip**
>
> Good plugins provide optional parameters in square brackets, e.g. `[-w SECONDS]`.

The `required` field can be toggled with a [boolean](17-language-reference.md#boolean-literals) value.

```
  arguments = {
    "--host" = {
      value = "..."
      description = "..."
      required = true
    }
  }
```

Whenever the check is executed and the argument is missing, Icinga
logs an error. This allows to better debug configuration errors
instead of sometimes unreadable plugin errors when parameters are
missing.

##### Command Arguments: Skip Key <a id="command-arguments-skip-key"></a>

The `arguments` attribute requires a key, empty values are not allowed.
To overcome this for parameters which don't need the name in front of
the value, use the `skip_key` [boolean](17-language-reference.md#boolean-literals) toggle.

```
  command = [ PrefixDir + "/bin/icingacli", "businessprocess", "process", "check" ]

  arguments = {
    "--process" = {
      value = "$icingacli_businessprocess_process$"
      description = "Business process to monitor"
      skip_key = true
      required = true
      order = -1
    }
  }
```

The service specifies the [custom variable](03-monitoring-basics.md#custom-variables) `icingacli_businessprocess_process`.

```
  vars.icingacli_businessprocess_process = "bp-shop-web"
```

This results in this command line without the `--process` parameter:

```bash
'/bin/icingacli' 'businessprocess' 'process' 'check' 'bp-shop-web'
```

You can use this method to put everything into the `arguments` attribute
in a defined order and without keys. This avoids entries in the `command`
attributes too.


##### Command Arguments: Set If <a id="command-arguments-set-if"></a>

This can be used for the following scenarios:

**Parameters without value, e.g. `--sni`.**

```
  command = [ PluginDir + "/check_http"]

  arguments = {
    "--sni" = {
      set_if = "$http_sni$"
    }
  }
```

Whenever a host/service object sets the `http_sni` [custom variable](03-monitoring-basics.md#custom-variables)
to `true`, the parameter is added to the command line.

```bash
'/usr/lib64/nagios/plugins/check_http' '--sni'
```

[Numeric](17-language-reference.md#numeric-literals) values are allowed too.

**Parameters with value, but additionally controlled with an extra custom variable boolean flag.**

The following example is taken from the [postgres]() CheckCommand. The host
parameter should use a `value` but only whenever the `postgres_unixsocket`
[custom variable](03-monitoring-basics.md#custom-variables) is set to false.

Note: `set_if` is using a runtime lambda function because the value
is evaluated at runtime. This is explained in [this chapter](08-advanced-topics.md#use-functions-object-config).

```
  command = [ PluginContribDir + "/check_postgres.pl" ]

  arguments = {
    "-H" = {
      value = "$postgres_host$"
      set_if = {{ macro("$postgres_unixsocket$") == false }}
      description = "hostname(s) to connect to; defaults to none (Unix socket)"
  }
```

An executed check for this host and services ...

```
object Host "postgresql-cluster" {
  // ...

  vars.postgres_host = "192.168.56.200"
  vars.postgres_unixsocket = false
}
```

... use the following command line:

```bash
'/usr/lib64/nagios/plugins/check_postgres.pl' '-H' '192.168.56.200'
```

Host/service objects which set `postgres_unixsocket` to `false` don't add the `-H` parameter
and its value to the command line.

References: [abbreviated lambda syntax](17-language-reference.md#nullary-lambdas), [macro](18-library-reference.md#scoped-functions-macro).

##### Command Arguments: Order <a id="command-arguments-order"></a>

Plugin may require parameters in a special order. One after the other,
or e.g. one parameter always in the first position.

```
  arguments = {
    "--first" = {
      value = "..."
      description = "..."
      order = -5
    }
    "--second" = {
      value = "..."
      description = "..."
      order = -4
    }
    "--last" = {
      value = "..."
      description = "..."
      order = 99
    }
  }
```

Keep in mind that positional arguments need to be tested thoroughly.

##### Command Arguments: Repeat Key <a id="command-arguments-repeat-key"></a>

Parameters can use [Array](17-language-reference.md#array) as value type. Whenever Icinga encounters
an array, it repeats the parameter key and each value element by default.

```
  command = [ NscpPath + "\\nscp.exe", "client" ]

  arguments = {
    "-a" = {
      value = "$nscp_arguments$"
      description = "..."
      repeat_key = true
    }
  }
```

On a host/service object, specify the `nscp_arguments` [custom variable](03-monitoring-basics.md#custom-variables)
as an array.

```
  vars.nscp_arguments = [ "exclude=sppsvc", "exclude=ShellHWDetection" ]
```

This translates into the following command line:

```
nscp.exe 'client' '-a' 'exclude=sppsvc' '-a' 'exclude=ShellHWDetection'
```

If the plugin requires you to pass the list without repeating the key,
set `repeat_key = false` in the argument definition.

```
  command = [ NscpPath + "\\nscp.exe", "client" ]

  arguments = {
    "-a" = {
      value = "$nscp_arguments$"
      description = "..."
      repeat_key = false
    }
  }
```

This translates into the following command line:

```
nscp.exe 'client' '-a' 'exclude=sppsvc' 'exclude=ShellHWDetection'
```


##### Command Arguments: Key <a id="command-arguments-key"></a>

The `arguments` attribute requires unique keys. Sometimes, you'll
need to override this in the resulting command line with same key
names. Therefore you can specifically override the arguments key.

```
arguments = {
  "--key1" = {
    value = "..."
    key = "-specialkey"
  }
  "--key2" = {
    value = "..."
    key = "-specialkey"
  }
}
```

This results in the following command line:

```
  '-specialkey' '...' '-specialkey' '...'
```

#### Environment Variables <a id="command-environment-variables"></a>

The `env` command object attribute specifies a list of environment variables with values calculated
from custom variables which should be exported as environment variables prior to executing the command.

This is useful for example for hiding sensitive information on the command line output
when passing credentials to database checks:

```
object CheckCommand "mysql" {
  command = [ PluginDir + "/check_mysql" ]

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
```

The executed command line visible with `ps` or `top` looks like this and hides
the database credentials in the user's environment.

```bash
/usr/lib/nagios/plugins/check_mysql -H 192.168.56.101 -d icinga
```

> **Note**
>
> If the CheckCommand also supports setting the parameter in the command line,
> ensure to use a different name for the custom variable. Otherwise Icinga 2
> adds the command line parameter.

If a specific CheckCommand object provided with the [Icinga Template Library](10-icinga-template-library.md#icinga-template-library)
needs additional environment variables, you can import it into a new custom
CheckCommand object and add additional `env` keys. Example for the [mysql_health](10-icinga-template-library.md#plugin-contrib-command-mysql_health)
CheckCommand:

```
object CheckCommand "mysql_health_env" {
  import "mysql_health"

  // https://labs.consol.de/nagios/check_mysql_health/
  env.NAGIOS__SERVICEMYSQL_USER = "$mysql_health_env_username$"
  env.NAGIOS__SERVICEMYSQL_PASS = "$mysql_health_env_password$"
}
```

Specify the custom variables `mysql_health_env_username` and `mysql_health_env_password`
in the service object then.

> **Note**
>
> Keep in mind that the values are still visible with the [debug console](11-cli-commands.md#cli-command-console)
> and the inspect mode in the [Icinga Director](https://icinga.com/docs/director/latest/).

You can also set global environment variables in the application's
sysconfig configuration file, e.g. `HOME` or specific library paths
for Oracle. Beware that these environment variables can be used
by any CheckCommand object and executed plugin and can leak sensitive
information.

### Notification Commands <a id="notification-commands"></a>

[NotificationCommand](09-object-types.md#objecttype-notificationcommand)
objects define how notifications are delivered to external interfaces
(email, XMPP, IRC, Twitter, etc.).
[NotificationCommand](09-object-types.md#objecttype-notificationcommand)
objects are referenced by [Notification](09-object-types.md#objecttype-notification)
objects using the `command` attribute.

> **Note**
>
> Make sure that the [notification](11-cli-commands.md#enable-features) feature is enabled
> in order to execute notification commands.

While it's possible to specify an entire notification command right
in the NotificationCommand object it is generally advisable to create a
shell script in the `/etc/icinga2/scripts` directory and have the
NotificationCommand object refer to that.

A fresh Icinga 2 install comes with with two example scripts for host
and service notifications by email. Based on the Icinga 2 runtime macros
(such as `$service.output$` for the current check output) it's possible
to send email to the user(s) associated with the notification itself
(`$user.email$`). Feel free to take these scripts as a starting point
for your own individual notification solution - and keep in mind that
nearly everything is technically possible.

Information needed to generate notifications is passed to the scripts as
arguments. The NotificationCommand objects `mail-host-notification` and
`mail-service-notification` correspond to the shell scripts
`mail-host-notification.sh` and `mail-service-notification.sh` in
`/etc/icinga2/scripts` and define default values for arguments. These
defaults can always be overwritten locally.

> **Note**
>
> This example requires the `mail` binary installed on the Icinga 2
> master.
>
> Depending on the distribution, you need a local mail transfer
> agent (MTA) such as Postfix, Exim or Sendmail in order
> to send emails.
>
> These tools virtually provide the `mail` binary executed
> by the notification scripts below.

#### mail-host-notification <a id="mail-host-notification"></a>

The `mail-host-notification` NotificationCommand object uses the
example notification script located in `/etc/icinga2/scripts/mail-host-notification.sh`.

Here is a quick overview of the arguments that can be used. See also [host runtime
macros](03-monitoring-basics.md#-host-runtime-macros) for further
information.

  Name                           | Description
  -------------------------------|---------------------------------------
  `notification_date`            | **Required.** Date and time. Defaults to `$icinga.long_date_time$`.
  `notification_hostname`        | **Required.** The host's `FQDN`. Defaults to `$host.name$`.
  `notification_hostdisplayname` | **Required.** The host's display name. Defaults to `$host.display_name$`.
  `notification_hostoutput`      | **Required.** Output from host check. Defaults to `$host.output$`.
  `notification_useremail`       | **Required.** The notification's recipient(s). Defaults to `$user.email$`.
  `notification_hoststate`       | **Required.** Current state of host. Defaults to `$host.state$`.
  `notification_type`            | **Required.** Type of notification. Defaults to `$notification.type$`.
  `notification_address`         | **Optional.** The host's IPv4 address. Defaults to `$address$`.
  `notification_address6`        | **Optional.** The host's IPv6 address. Defaults to `$address6$`.
  `notification_author`          | **Optional.** Comment author. Defaults to `$notification.author$`.
  `notification_comment`         | **Optional.** Comment text. Defaults to `$notification.comment$`.
  `notification_from`            | **Optional.** Define a valid From: string (e.g. `"Icinga 2 Host Monitoring <icinga@example.com>"`). Requires `GNU mailutils` (Debian/Ubuntu) or `mailx` (RHEL/SUSE).
  `notification_icingaweb2url`   | **Optional.** Define URL to your Icinga Web 2 (e.g. `"https://www.example.com/icingaweb2"`)
  `notification_logtosyslog`     | **Optional.** Set `true` to log notification events to syslog; useful for debugging. Defaults to `false`.

#### mail-service-notification <a id="mail-service-notification"></a>

The `mail-service-notification` NotificationCommand object uses the
example notification script located in `/etc/icinga2/scripts/mail-service-notification.sh`.

Here is a quick overview of the arguments that can be used. See also [service runtime
macros](03-monitoring-basics.md#-service-runtime-macros) for further
information.

  Name                              | Description
  ----------------------------------|---------------------------------------
  `notification_date`               | **Required.** Date and time. Defaults to `$icinga.long_date_time$`.
  `notification_hostname`           | **Required.** The host's `FQDN`. Defaults to `$host.name$`.
  `notification_servicename`        | **Required.** The service name. Defaults to `$service.name$`.
  `notification_hostdisplayname`    | **Required.** Host display name. Defaults to `$host.display_name$`.
  `notification_servicedisplayname` | **Required.** Service display name. Defaults to `$service.display_name$`.
  `notification_serviceoutput`      | **Required.** Output from service check. Defaults to `$service.output$`.
  `notification_useremail`          | **Required.** The notification's recipient(s). Defaults to `$user.email$`.
  `notification_servicestate`       | **Required.** Current state of host. Defaults to `$service.state$`.
  `notification_type`               | **Required.** Type of notification. Defaults to `$notification.type$`.
  `notification_address`            | **Optional.** The host's IPv4 address. Defaults to `$address$`.
  `notification_address6`           | **Optional.** The host's IPv6 address. Defaults to `$address6$`.
  `notification_author`             | **Optional.** Comment author. Defaults to `$notification.author$`.
  `notification_comment`            | **Optional.** Comment text. Defaults to `$notification.comment$`.
  `notification_from`               | **Optional.** Define a valid From: string (e.g. `"Icinga 2 Host Monitoring <icinga@example.com>"`). Requires `GNU mailutils` (Debian/Ubuntu) or `mailx` (RHEL/SUSE).
  `notification_icingaweb2url`      | **Optional.** Define URL to your Icinga Web 2 (e.g. `"https://www.example.com/icingaweb2"`)
  `notification_logtosyslog`        | **Optional.** Set `true` to log notification events to syslog; useful for debugging. Defaults to `false`.

### Event Commands <a id="event-commands"></a>

Unlike notifications, event commands for hosts/services are called on every
check execution if one of these conditions matches:

* The host/service is in a [soft state](03-monitoring-basics.md#hard-soft-states)
* The host/service state changes into a [hard state](03-monitoring-basics.md#hard-soft-states)
* The host/service state recovers from a [soft or hard state](03-monitoring-basics.md#hard-soft-states) to [OK](03-monitoring-basics.md#service-states)/[Up](03-monitoring-basics.md#host-states)

[EventCommand](09-object-types.md#objecttype-eventcommand) objects are referenced by
[Host](09-object-types.md#objecttype-host) and [Service](09-object-types.md#objecttype-service) objects
with the `event_command` attribute.

Therefore the `EventCommand` object should define a command line
evaluating the current service state and other service runtime attributes
available through runtime variables. Runtime macros such as `$service.state_type$`
and `$service.state$` will be processed by Icinga 2 and help with fine-granular
triggered events

If the host/service is located on a client as [command endpoint](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint)
the event command will be executed on the client itself (similar to the check
command).

Common use case scenarios are a failing HTTP check which requires an immediate
restart via event command. Another example would be an application that is not
responding and therefore requires a restart. You can also use event handlers
to forward more details on state changes and events than the typical notification
alerts provide.

#### Use Event Commands to Send Information from the Master <a id="event-command-send-information-from-master"></a>

This example sends a web request from the master node to an external tool
for every event triggered on a `businessprocess` service.

Define an [EventCommand](09-object-types.md#objecttype-eventcommand)
object `send_to_businesstool` which sends state changes to the external tool.

```
object EventCommand "send_to_businesstool" {
  command = [
    "/usr/bin/curl",
    "-s",
    "-X PUT"
  ]

  arguments = {
    "-H" = {
      value ="$businesstool_url$"
      skip_key = true
    }
    "-d" = "$businesstool_message$"
  }

  vars.businesstool_url = "http://localhost:8080/businesstool"
  vars.businesstool_message = "$host.name$ $service.name$ $service.state$ $service.state_type$ $service.check_attempt$"
}
```

Set the `event_command` attribute to `send_to_businesstool` on the Service.

```
object Service "businessprocess" {
  host_name = "businessprocess"

  check_command = "icingacli-businessprocess"
  vars.icingacli_businessprocess_process = "icinga"
  vars.icingacli_businessprocess_config = "training"

  event_command = "send_to_businesstool"
}
```

In order to test this scenario you can run:

```bash
nc -l 8080
```

This allows to catch the web request. You can also enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output)
and search for the event command execution log message.

```bash
tail -f /var/log/icinga2/debug.log | grep EventCommand
```

Feed in a check result via REST API action [process-check-result](12-icinga2-api.md#icinga2-api-actions-process-check-result)
or via Icinga Web 2.

Expected Result:

```
# nc -l 8080
PUT /businesstool HTTP/1.1
User-Agent: curl/7.29.0
Host: localhost:8080
Accept: */*
Content-Length: 47
Content-Type: application/x-www-form-urlencoded

businessprocess businessprocess CRITICAL SOFT 1
```

#### Use Event Commands to Restart Service Daemon via Command Endpoint on Linux <a id="event-command-restart-service-daemon-command-endpoint-linux"></a>

This example triggers a restart of the `httpd` service on the local system
when the `procs` service check executed via Command Endpoint fails. It only
triggers if the service state is `Critical` and attempts to restart the
service before a notification is sent.

Requirements:

* Icinga 2 as client on the remote node
* icinga user with sudo permissions to the httpd daemon

Example on RHEL:

```
# visudo
icinga  ALL=(ALL) NOPASSWD: /usr/bin/systemctl restart httpd
```

Note: Distributions might use a different name. On Debian/Ubuntu the service is called `apache2`.

Define an [EventCommand](09-object-types.md#objecttype-eventcommand) object `restart_service`
which allows to trigger local service restarts. Put it into a [global zone](06-distributed-monitoring.md#distributed-monitoring-global-zone-config-sync)
to sync its configuration to all clients.

```
[root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/global-templates/eventcommands.conf

object EventCommand "restart_service" {
  command = [ PluginDir + "/restart_service" ]

  arguments = {
    "-s" = "$service.state$"
    "-t" = "$service.state_type$"
    "-a" = "$service.check_attempt$"
    "-S" = "$restart_service$"
  }

  vars.restart_service = "$procs_command$"
}
```

This event command triggers the following script which restarts the service.
The script only is executed if the service state is `CRITICAL`. Warning and Unknown states
are ignored as they indicate not an immediate failure.

```
[root@icinga2-agent1.localdomain /]# vim /usr/lib64/nagios/plugins/restart_service

#!/bin/bash

while getopts "s:t:a:S:" opt; do
  case $opt in
    s)
      servicestate=$OPTARG
      ;;
    t)
      servicestatetype=$OPTARG
      ;;
    a)
      serviceattempt=$OPTARG
      ;;
    S)
      service=$OPTARG
      ;;
  esac
done

if ( [ -z $servicestate ] || [ -z $servicestatetype ] || [ -z $serviceattempt ] || [ -z $service ] ); then
  echo "USAGE: $0 -s servicestate -z servicestatetype -a serviceattempt -S service"
  exit 3;
else
  # Only restart on the third attempt of a critical event
  if ( [ $servicestate == "CRITICAL" ] && [ $servicestatetype == "SOFT" ] && [ $serviceattempt -eq 3 ] ); then
    sudo /usr/bin/systemctl restart $service
  fi
fi

[root@icinga2-agent1.localdomain /]# chmod +x /usr/lib64/nagios/plugins/restart_service
```

Add a service on the master node which is executed via command endpoint on the client.
Set the `event_command` attribute to `restart_service`, the name of the previously defined
EventCommand object.

```
[root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/master/icinga2-agent1.localdomain.conf

object Service "Process httpd" {
  check_command = "procs"
  event_command = "restart_service"
  max_check_attempts = 4

  host_name = "icinga2-agent1.localdomain"
  command_endpoint = "icinga2-agent1.localdomain"

  vars.procs_command = "httpd"
  vars.procs_warning = "1:10"
  vars.procs_critical = "1:"
}
```

In order to test this configuration just stop the `httpd` on the remote host `icinga2-agent1.localdomain`.

```
[root@icinga2-agent1.localdomain /]# systemctl stop httpd
```

You can enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) and search for the
executed command line.

```
[root@icinga2-agent1.localdomain /]# tail -f /var/log/icinga2/debug.log | grep restart_service
```

#### Use Event Commands to Restart Service Daemon via Command Endpoint on Windows <a id="event-command-restart-service-daemon-command-endpoint-windows"></a>

This example triggers a restart of the `httpd` service on the remote system
when the `service-windows` service check executed via Command Endpoint fails.
It only triggers if the service state is `Critical` and attempts to restart the
service before a notification is sent.

Requirements:

* Icinga 2 as client on the remote node
* Icinga 2 service with permissions to execute Powershell scripts (which is the default)

Define an [EventCommand](09-object-types.md#objecttype-eventcommand) object `restart_service-windows`
which allows to trigger local service restarts. Put it into a [global zone](06-distributed-monitoring.md#distributed-monitoring-global-zone-config-sync)
to sync its configuration to all clients.

```
[root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/global-templates/eventcommands.conf

object EventCommand "restart_service-windows" {
  command = [
    "C:\\Windows\\SysWOW64\\WindowsPowerShell\\v1.0\\powershell.exe",
    PluginDir + "/restart_service.ps1"
  ]

  arguments = {
    "-ServiceState" = "$service.state$"
    "-ServiceStateType" = "$service.state_type$"
    "-ServiceAttempt" = "$service.check_attempt$"
    "-Service" = "$restart_service$"
    "; exit" = {
        order = 99
        value = "$$LASTEXITCODE"
    }
  }

  vars.restart_service = "$service_win_service$"
}
```

This event command triggers the following script which restarts the service.
The script only is executed if the service state is `CRITICAL`. Warning and Unknown states
are ignored as they indicate not an immediate failure.

Add the `restart_service.ps1` Powershell script into `C:\Program Files\Icinga2\sbin`:

```
param(
        [string]$Service                  = '',
        [string]$ServiceState             = '',
        [string]$ServiceStateType         = '',
        [int]$ServiceAttempt              = ''
    )

if (!$Service -Or !$ServiceState -Or !$ServiceStateType -Or !$ServiceAttempt) {
    $scriptName = GCI $MyInvocation.PSCommandPath | Select -Expand Name;
    Write-Host "USAGE: $scriptName -ServiceState servicestate -ServiceStateType servicestatetype -ServiceAttempt serviceattempt -Service service" -ForegroundColor red;
    exit 3;
}

# Only restart on the third attempt of a critical event
if ($ServiceState -eq "CRITICAL" -And $ServiceStateType -eq "SOFT" -And $ServiceAttempt -eq 3) {
    Restart-Service $Service;
}

exit 0;
```

Add a service on the master node which is executed via command endpoint on the client.
Set the `event_command` attribute to `restart_service-windows`, the name of the previously defined
EventCommand object.

```
[root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/master/icinga2-agent2.localdomain.conf

object Service "Service httpd" {
  check_command = "service-windows"
  event_command = "restart_service-windows"
  max_check_attempts = 4

  host_name = "icinga2-agent2.localdomain"
  command_endpoint = "icinga2-agent2.localdomain"

  vars.service_win_service = "httpd"
}
```

In order to test this configuration just stop the `httpd` on the remote host `icinga2-agent1.localdomain`.

```
C:> net stop httpd
```

You can enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) and search for the
executed command line in `C:\ProgramData\icinga2\var\log\icinga2\debug.log`.


#### Use Event Commands to Restart Service Daemon via SSH <a id="event-command-restart-service-daemon-ssh"></a>

This example triggers a restart of the `httpd` daemon
via SSH when the `http` service check fails.

Requirements:

* SSH connection allowed (firewall, packet filters)
* icinga user with public key authentication
* icinga user with sudo permissions to restart the httpd daemon.

Example on Debian:

```
# ls /home/icinga/.ssh/
authorized_keys

# visudo
icinga  ALL=(ALL) NOPASSWD: /etc/init.d/apache2 restart
```

Define a generic [EventCommand](09-object-types.md#objecttype-eventcommand) object `event_by_ssh`
which can be used for all event commands triggered using SSH:

```
[root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/master/local_eventcommands.conf

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
```

The actual event command only passes the `event_by_ssh_command` attribute.
The `event_by_ssh_service` custom variable takes care of passing the correct
daemon name, while `test $service.state_id$ -gt 0` makes sure that the daemon
is only restarted when the service is not in an `OK` state.

```
object EventCommand "event_by_ssh_restart_service" {
  import "event_by_ssh"

  //only restart the daemon if state > 0 (not-ok)
  //requires sudo permissions for the icinga user
  vars.event_by_ssh_command = "test $service.state_id$ -gt 0 && sudo systemctl restart $event_by_ssh_service$"
}
```


Now set the `event_command` attribute to `event_by_ssh_restart_service` and tell it
which service should be restarted using the `event_by_ssh_service` attribute.

```
apply Service "http" {
  import "generic-service"
  check_command = "http"

  event_command = "event_by_ssh_restart_service"
  vars.event_by_ssh_service = "$host.vars.httpd_name$"

  //vars.event_by_ssh_logname = "icinga"
  //vars.event_by_ssh_identity = "/home/icinga/.ssh/id_rsa.pub"

  assign where host.vars.httpd_name
}
```

Specify the `httpd_name` custom variable on the host to assign the
service and set the event handler service.

```
object Host "remote-http-host" {
  import "generic-host"
  address = "192.168.1.100"

  vars.httpd_name = "apache2"
}
```

In order to test this configuration just stop the `httpd` on the remote host `icinga2-agent1.localdomain`.

```
[root@icinga2-agent1.localdomain /]# systemctl stop httpd
```

You can enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) and search for the
executed command line.

```
[root@icinga2-agent1.localdomain /]# tail -f /var/log/icinga2/debug.log | grep by_ssh
```


## Dependencies <a id="dependencies"></a>

Icinga 2 uses host and service [Dependency](09-object-types.md#objecttype-dependency) objects
for determining their network reachability.

A service can depend on a host, and vice versa. A service has an implicit
dependency (parent) to its host. A host to host dependency acts implicitly
as host parent relation.
When dependencies are calculated, not only the immediate parent is taken into
account but all parents are inherited.

The `parent_host_name` and `parent_service_name` attributes are mandatory for
service dependencies, `parent_host_name` is required for host dependencies.
[Apply rules](03-monitoring-basics.md#using-apply) will allow you to
[determine these attributes](03-monitoring-basics.md#dependencies-apply-custom-variables) in a more
dynamic fashion if required.

```
parent_host_name = "core-router"
parent_service_name = "uplink-port"
```

Notifications are suppressed by default if a host or service becomes unreachable.
You can control that option by defining the `disable_notifications` attribute.

```
disable_notifications = false
```

If the dependency should be triggered in the parent object's soft state, you
need to set `ignore_soft_states` to `false`.

The dependency state filter must be defined based on the parent object being
either a host (`Up`, `Down`) or a service (`OK`, `Warning`, `Critical`, `Unknown`).

The following example will make the dependency fail and trigger it if the parent
object is **not** in one of these states:

```
states = [ OK, Critical, Unknown ]
```

> **In other words**
>
> If the parent service object changes into the `Warning` state, this
> dependency will fail and render all child objects (hosts or services) unreachable.

You can determine the child's reachability by querying the `last_reachable` attribute
via the [REST API](12-icinga2-api.md#icinga2-api).

> **Note**
>
> Reachability calculation depends on fresh and processed check results. If dependencies
> disable checks for child objects, this won't work reliably.

### Implicit Dependencies for Services on Host <a id="dependencies-implicit-host-service"></a>

Icinga 2 automatically adds an implicit dependency for services on their host. That way
service notifications are suppressed when a host is `DOWN` or `UNREACHABLE`. This dependency
does not overwrite other dependencies and implicitly sets `disable_notifications = true` and
`states = [ Up ]` for all service objects.

Service checks are still executed. If you want to prevent them from happening, you can
apply the following dependency to all services setting their host as `parent_host_name`
and disabling the checks. `assign where true` matches on all `Service` objects.

```
apply Dependency "disable-host-service-checks" to Service {
  disable_checks = true
  assign where true
}
```

### Dependencies for Network Reachability <a id="dependencies-network-reachability"></a>

A common scenario is the Icinga 2 server behind a router. Checking internet
access by pinging the Google DNS server `google-dns` is a common method, but
will fail in case the `dsl-router` host is down. Therefore the example below
defines a host dependency which acts implicitly as parent relation too.

Furthermore the host may be reachable but ping probes are dropped by the
router's firewall. In case the `dsl-router`'s `ping4` service check fails, all
further checks for the `ping4` service on host `google-dns` service should
be suppressed. This is achieved by setting the `disable_checks` attribute to `true`.

```
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
```

### Redundancy Groups <a id="dependencies-redundancy-groups"></a>

Sometimes you want dependencies to accumulate,
i.e. to consider the parent reachable only if no dependency is violated.
Sometimes you want them to be regarded as redundant,
i.e. to consider the parent unreachable only if no dependency is fulfilled.
Think of a host connected to both a network and a storage switch vs. a host connected to redundant routers.

Sometimes you even want a mixture of both.
Think of a service like SSH depeding on both LDAP and DNS to function,
while operating redundant LDAP servers as well as redundant DNS resolvers.

Before v2.12, Icinga regarded all dependecies as cumulative.
In v2.12 and v2.13, Icinga regarded all dependencies redundant.
The latter led to unrelated services being inadvertantly regarded to be redundant to each other.

v2.14 restored the former behavior and allowed to override it.
I.e. all dependecies are regarded as essential for the parent by default.
Specifying the `redundancy_group` attribute for two dependecies of a child object with the equal value
causes them to be regarded as redundant (only inside that redundancy group).

<!-- Keep this for compatibility -->
<a id="dependencies-apply-custom-attríbutes"></a>

### Apply Dependencies based on Custom Variables <a id="dependencies-apply-custom-variables"></a>

You can use [apply rules](03-monitoring-basics.md#using-apply) to set parent or
child attributes, e.g. `parent_host_name` to other objects'
attributes.

A common example are virtual machines hosted on a master. The object
name of that master is auto-generated from your CMDB or VMWare inventory
into the host's custom variables (or a generic template for your
cloud).

Define your master host object:

```
/* your master */
object Host "master.example.com" {
  import "generic-host"
}
```

Add a generic template defining all common host attributes:

```
/* generic template for your virtual machines */
template Host "generic-vm" {
  import "generic-host"
}
```

Add a template for all hosts on your example.com cloud setting
custom variable `vm_parent` to `master.example.com`:

```
template Host "generic-vm-example.com" {
  import "generic-vm"
  vars.vm_parent = "master.example.com"
}
```

Define your guest hosts:

```
object Host "www.example1.com" {
  import "generic-vm-master.example.com"
}

object Host "www.example2.com" {
  import "generic-vm-master.example.com"
}
```

Apply the host dependency to all child hosts importing the
`generic-vm` template and set the `parent_host_name`
to the previously defined custom variable `host.vars.vm_parent`.

```
apply Dependency "vm-host-to-parent-master" to Host {
  parent_host_name = host.vars.vm_parent
  assign where "generic-vm" in host.templates
}
```

You can extend this example, and make your services depend on the
`master.example.com` host too. Their local scope allows you to use
`host.vars.vm_parent` similar to the example above.

```
apply Dependency "vm-service-to-parent-master" to Service {
  parent_host_name = host.vars.vm_parent
  assign where "generic-vm" in host.templates
}
```

That way you don't need to wait for your guest hosts becoming
unreachable when the master host goes down. Instead the services
will detect their reachability immediately when executing checks.

> **Note**
>
> This method with setting locally scoped variables only works in
> apply rules, but not in object definitions.


### Dependencies for Agent Checks <a id="dependencies-agent-checks"></a>

Another good example are agent based checks. You would define a health check
for the agent daemon responding to your requests, and make all other services
querying that daemon depend on that health check.

```
apply Service "agent-health" {
  check_command = "cluster-zone"

  display_name = "cluster-health-" + host.name

  /* This follows the convention that the agent zone name is the FQDN which is the same as the host object name. */
  vars.cluster_zone = host.name

  assign where host.vars.agent_endpoint
}
```

Now, make all other agent based checks dependent on the OK state of the `agent-health`
service.

```
apply Dependency "agent-health-check" to Service {
  parent_service_name = "agent-health"

  states = [ OK ] // Fail if the parent service state switches to NOT-OK
  disable_notifications = true

  assign where host.vars.agent_endpoint // Automatically assigns all agent endpoint checks as child services on the matched host
  ignore where service.name == "agent-health" // Avoid a self reference from child to parent
}

```

This is described in detail in [this chapter](06-distributed-monitoring.md#distributed-monitoring-health-checks).
