# <a id="configuring-icinga2"></a> Configuring Icinga 2

## <a id="global-constants"></a> Global Constants

Icinga 2 provides a number of special global constants. Some of them can be overriden using the `--define` command line parameter:

Variable            |Description
--------------------|-------------------
PrefixDir           |**Read-only.** Contains the installation prefix that was specified with cmake -DCMAKE_INSTALL_PREFIX. Defaults to "/usr/local".
SysconfDir          |**Read-only.** Contains the path of the sysconf directory. Defaults to PrefixDir + "/etc".
ZonesDir            |**Read-only.** Contains the path of the zones.d directory. Defaults to SysconfDir + "/zones.d".
LocalStateDir       |**Read-only.** Contains the path of the local state directory. Defaults to PrefixDir + "/var".
PkgDataDir          |**Read-only.** Contains the path of the package data directory. Defaults to PrefixDir + "/share/icinga2".
StatePath           |**Read-write.** Contains the path of the Icinga 2 state file. Defaults to LocalStateDir + "/lib/icinga2/icinga2.state".
PidPath             |**Read-write.** Contains the path of the Icinga 2 PID file. Defaults to LocalStateDir + "/run/icinga2/icinga2.pid".
Vars                |**Read-write.** Contains a dictionary with global custom attributes. Not set by default.
NodeName            |**Read-write.** Contains the cluster node name. Set to the local hostname by default.
ApplicationType     |**Read-write.** Contains the name of the Application type. Defaults to "icinga/IcingaApplication".
EnableNotifications |**Read-write.** Whether notifications are globally enabled. Defaults to true.
EnableEventHandlers |**Read-write.** Whether event handlers are globally enabled. Defaults to true.
EnableFlapping      |**Read-write.** Whether flap detection is globally enabled. Defaults to true.
EnableHostChecks    |**Read-write.** Whether active host checks are globally enabled. Defaults to true.
EnableServiceChecks |**Read-write.** Whether active service checks are globally enabled. Defaults to true.
EnablePerfdata      |**Read-write.** Whether performance data processing is globally enabled. Defaults to true.
UseVfork            |**Read-write.** Whether to use vfork(). Only available on *NIX. Defaults to true.


## <a id="configuration-syntax"></a> Configuration Syntax

### <a id="object-definition"></a> Object Definition

Icinga 2 features an object-based configuration format. You can define new
objects using the `object` keyword:

    object Host "host1.example.org" {
      display_name = "host1"

      address = "192.168.0.1"
      address6 = "::1"
    }

In general you need to write each statement on a new line. Expressions started
with `{`, `(` and `[` extend until the matching closing character and can be broken
up into multiple lines.

Alternatively you can write multiple statements in a single line by separating
them with a semicolon:

    object Host "host1.example.org" {
      display_name = "host1"

      address = "192.168.0.1"; address6 = "::1"
    }

Each object is uniquely identified by its type (`Host`) and name
(`host1.example.org`). Some types have composite names, e.g. the
`Service` type which uses the `host_name` attribute and the name
you specified to generate its object name.

Exclamation marks (!) are not permitted in object names.

Objects can contain a comma-separated list of property
declarations. Instead of commas semicolons may also be used.
The following data types are available for property values:

### Expressions

The following expressions can be used in the right-hand side of dictionary
values.

#### <a id="numeric-literals"></a> Numeric Literals

A floating-point number.

Example:

    -27.3

#### <a id="duration-literals"></a> Duration Literals

Similar to floating-point numbers except for the fact that they support
suffixes to help with specifying time durations.

Example:

    2.5m

Supported suffixes include ms (milliseconds), s (seconds), m (minutes),
h (hours) and d (days).

Duration literals are converted to seconds by the config parser and
are treated like numeric literals.

#### <a id="string-literals"></a> String Literals

A string.

Example:

    "Hello World!"

Certain characters need to be escaped. The following escape sequences
are supported:

Character                 | Escape sequence
--------------------------|------------------------------------
"                         | \\"
\\                        | \\\\
&lt;TAB&gt;               | \\t
&lt;CARRIAGE-RETURN&gt;   | \\r
&lt;LINE-FEED&gt;         | \\n
&lt;BEL&gt;               | \\b
&lt;FORM-FEED&gt;         | \\f

In addition to these pre-defined escape sequences you can specify
arbitrary ASCII characters using the backslash character (\\) followed
by an ASCII character in octal encoding.

#### <a id="multiline-string-literals"></a> Multi-line String Literals

Strings spanning multiple lines can be specified by enclosing them in
{{{ and }}}.

Example.

    {{{This
    is
    a multi-line
    string.}}}

Unlike in ordinary strings special characters do not have to be escaped
in multi-line string literals.

#### <a id="boolean-literals"></a> Boolean Literals

The keywords `true` and `false` are equivalent to 1 and 0 respectively.

#### <a id="null-value"></a> Null Value

The `null` keyword can be used to specify an empty value.

#### <a id="dictionary"></a> Dictionary

An unordered list of key-value pairs. Keys must be unique and are
compared in a case-insensitive manner.

Individual key-value pairs must either be comma-separated or on separate lines.
The comma after the last key-value pair is optional.

Example:

    {
      address = "192.168.0.1"
      port = 443
    }

Identifiers may not contain certain characters (e.g. space) or start
with certain characters (e.g. digits). If you want to use a dictionary
key that is not a valid identifier you can enclose the key in double
quotes.

Setting a dictionary key to null causes the key and its value to be
removed from the dictionary.

#### <a id="array"></a> Array

An ordered list of values.

Individual array elements must be comma-separated.
The comma after the last element is optional.

Example:

    [ "hello", 42 ]

An array may simultaneously contain values of different types, such as
strings and numbers.

#### <a id="expression-operators"></a> Operators

The following operators are supported in expressions:

Operator | Examples (Result)                             | Description
---------|-----------------------------------------------|--------------------------------
!        | !"Hello" (false), !false (true)               | Log<!-- ignore BLACKLIST -->ical negation of the operand
~        | ~true (false)                                 | Bitwise negation of the operand
+        | 1 + 3 (4), "hello " + "world" ("hello world") | Adds two numbers; concatenates strings
-        | 3 - 1 (2)                                     | Subtracts two numbers
*        | 5m * 10 (3000)                                | Multiplies two numbers
/        | 5m / 5 (60)                                   | Divides two numbers
&        | 7 & 3 (3)                                     | Binary AND
&#124;   | 2 &#124; 3 (3)                                | Binary OR
<        | 3 < 5 (true)                                  | Less than
>        | 3 > 5 (false)                                 | Greater than
<=       | 3 <= 3 (true)                                 | Less than or equal
>=       | 3 >= 3 (true)                                 | Greater than or equal
<<       | 4 << 8 (1024)                                 | Left shift
>>       | 1024 >> 4 (64)                                | Right shift
==       | "hello" == "hello" (true), 3 == 5 (false)     | Equal to
!=       | "hello" != "world" (true), 3 != 3 (false)     | Not equal to
in       | "foo" in [ "foo", "bar" ] (true)              | Element contained in array
!in      | "foo" !in [ "bar", "baz" ] (true)             | Element not contained in array
()       | (3 + 3) * 5                                   | Groups sub-expressions

Constants may be used in expressions:

    const MyCheckInterval = 10m

    ...

    {
      check_interval = MyCheckInterval / 2.5
    }

#### <a id="function-calls"></a> Function Calls

Functions can be called using the `()` operator:

    const MyGroups = [ "test1", "test" ]

    {
      check_interval = len(MyGroups) * 1m
    }

Function                        | Description
--------------------------------|-----------------------
regex(pattern, text)            | Returns true if the regex pattern matches the text, false otherwise.
match(pattern, text)            | Returns true if the wildcard pattern matches the text, false otherwise.
len(value)                      | Returns the length of the value, i.e. the number of elements for an array or dictionary, or the length of the string in bytes.
union(array, array, ...)        | Returns an array containing all unique elements from the specified arrays.
intersection(array, array, ...) | Returns an array containing all unique elements which are common to all specified arrays.
string(value)                   | Converts the value to a string.
number(value)                   | Converts the value to a number.
bool(value)                     | Converts the value to a bool.
log(value)                      | Writes a message to the log. Non-string values are converted to a JSON string.
log(severity, facility, value)  | Writes a message to the log. `severity` can be one of `LogDebug`, `LogNotice`, `LogInformation`, `LogWarning`, and `LogCritical`. Non-string values are converted to a JSON string.
exit(integer)                   | Terminates the application.

### <a id="dictionary-operators"></a> Dictionary Operators

In addition to the `=` operator shown above a number of other operators
to manipulate dictionary elements are supported. Here's a list of all
available operators:

#### <a id="operator-assignment"></a> Operator =

Sets a dictionary element to the specified value.

Example:

    {
      a = 5,
      a = 7
    }

In this example a has the value 7 after both instructions are executed.

#### <a id="operator-additive-assignment"></a> Operator +=

The += operator is a shortcut. The following expression:

    {
      a = [ "hello" ]
      a += [ "world" ]
    }

is equivalent to:

    {
      a = [ "hello" ]
      a = a + [ "world" ]
    }

#### <a id="operator-substractive-assignment"></a> Operator -=

The -= operator is a shortcut. The following expression:

    {
      a = 10
      a -= 5
    }

is equivalent to:

    {
      a = 10
      a = a - 5
    }

#### <a id="operator-multiply-assignment"></a> Operator \*=

The *= operator is a shortcut. The following expression:

    {
      a = 60
      a *= 5
    }

is equivalent to:

    {
      a = 60
      a = a * 5
    }

#### <a id="operator-dividing-assignment"></a> Operator /=

The /= operator is a shortcut. The following expression:

    {
      a = 300
      a /= 5
    }

is equivalent to:

    {
      a = 300
      a = a / 5
    }

### <a id="indexer"></a> Indexer

The indexer syntax provides a convenient way to set dictionary elements.

Example:

    {
      hello.key = "world"
    }

Example (alternative syntax):

    {
      hello["key"] = "world"
    }

This is equivalent to writing:

    {
      hello += {
        key = "world"
      }
    }

### <a id="template-imports"></a> Template Imports

Objects can import attributes from other objects.

Example:

    template Host "default-host" {
      vars.color = "red"
    }

    template Host "test-host" {
      import "default-host"

      vars.color = "blue"
    }

    object Host "localhost" {
      import "test-host"

      address = "127.0.0.1"
      address6 = "::1"
    }

The `default-host` and `test-host` objects are marked as templates
using the `template` keyword. Unlike ordinary objects templates are not
instantiated at run-time. Parent objects do not necessarily have to be
templates, however in general they are.

The `vars` dictionary for the `localhost` object contains all three
custom attributes and the custom attribute `color` has the value `"blue"`.

Parent objects are resolved in the order they're specified using the
`import` keyword.

### <a id="constants"></a> Constants

Global constants can be set using the `const` keyword:

    const VarName = "some value"

Once defined a constant can be accessed from any file. Constants cannot be changed
once they are set.

There is a defined set of [global constants](#global-constants) which allow
you to specify application settings.

### <a id="apply"></a> Apply

The `apply` keyword can be used to create new objects which are associated with
another group of objects.

    apply Service "ping" to Host {
      import "generic-service"
 
      check_command = "ping4"
  
      assign where host.name == "localhost"
    }

In this example the `assign where` condition is a boolean expression which is
evaluated for all objects of type `Host` and a new service with name "ping"
is created for each matching host.

The `to` keyword and the target type may be omitted if there is only one target
type, e.g. for the `Service` type.

Depending on the object type used in the `apply` expression additional local
variables may be available for use in the `where` condition:

Source Type       | Target Type | Variables
------------------|-------------|--------------
Service           | Host        | host
Dependency        | Host        | host
Dependency        | Service     | host, service
Notification      | Host        | host
Notification      | Service     | host, service
ScheduledDowntime | Host        | host
ScheduledDowntime | Service     | host, service

Any valid config attribute can be accessed using the `host` and `service`
variables. For example, `host.address` would return the value of the host's
"address" attribute - or null if that attribute isn't set.

### <a id="group-assign"></a> Group Assign

Group objects can be assigned to specific member objects using the `assign where`
and `ignore where` conditions.

    object HostGroup "linux-servers" {
      display_name = "Linux Servers"

      assign where host.vars.os == "Linux"
    }

In this example the `assign where` condition is a boolean expression which is evaluated
for all objects of the type `Host`. Each matching host is added as member to the host group
with the name "linux-servers". Membership exclusion can be controlled using the `ignore where`
condition.

Source Type       | Variables
------------------|--------------
HostGroup         | host
ServiceGroup      | host, service
UserGroup         | user


### <a id="boolean-values"></a> Boolean Values

The `assign where` and `ignore where` statements, the `!`, `&&` and `||`
operators as well as the `bool()` function convert their arguments to a
boolean value based on the following rules:

Description          | Example Value     | Boolean Value
---------------------|-------------------|--------------
Empty value          | null              | false
Zero                 | 0                 | false
Non-zero integer     | -23945            | true
Empty string         | ""                | false
Non-empty string     | "Hello"           | true
Empty array          | []                | false
Non-empty array      | [ "Hello" ]       | true
Empty dictionary     | {}                | false
Non-empty dictionary | { key = "value" } | true

### <a id="comments"></a> Comments

The Icinga 2 configuration format supports C/C++-style and shell-style comments.

Example:

    /*
     This is a comment.
     */
    object Host "localhost" {
      check_interval = 30 // this is also a comment.
      retry_interval = 15 # yet another comment
    }

### <a id="includes"></a> Includes

Other configuration files can be included using the `include` directive.
Paths must be relative to the configuration file that contains the
`include` directive.

Example:

    include "some/other/file.conf"
    include "conf.d/*.conf"

Wildcard includes are not recursive.

Icinga also supports include search paths similar to how they work in a
C/C++ compiler:

    include <itl>

Note the use of angle brackets instead of double quotes. This causes the
config compiler to search the include search paths for the specified
file. By default $PREFIX/share/icinga2 is included in the list of search
paths. Additional include search paths can be added using
[command-line options](#cmdline).

Wildcards are not permitted when using angle brackets.

### <a id="recursive-includes"></a> Recursive Includes

The `include_recursive` directive can be used to recursively include all
files in a directory which match a certain pattern.

Example:

    include_recursive "conf.d", "*.conf"
    include_recursive "templates"

The first parameter specifies the directory from which files should be
recursively included.

The file names need to match the pattern given in the second parameter.
When no pattern is specified the default pattern "*.conf" is used.

### <a id="library"></a> Library directive

The `library` directive can be used to manually load additional
libraries. Libraries can be used to provide additional object types and
functions.

Example:

    library "snmphelper"



## <a id="object-types"></a> Object Types

### <a id="objecttype-host"></a> Host

A host.

Example:

    object Host "localhost" {
      display_name = "The best host there is"
      address = "127.0.0.1"
      address6 = "::1"

      groups = [ "all-hosts" ]

      check_command = "hostalive"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the host.
  address         |**Optional.** The host's address. Available as command runtime macro `$address$` if set.
  address6        |**Optional.** The host's address. Available as command runtime macro `$address6$` if set.
  groups          |**Optional.** A list of host groups this host belongs to.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this host.
  check\_command  |**Required.** The name of the check command.
  max\_check\_attempts|**Optional.** The number of times a host is re-checked before changing into a hard state. Defaults to 3.
  check\_period   |**Optional.** The name of a time period which determines when this host should be checked. Not set by default.
  check\_interval |**Optional.** The check interval (in seconds). This interval is used for checks when the host is in a `HARD` state. Defaults to 5 minutes.
  retry\_interval |**Optional.** The retry interval (in seconds). This interval is used for checks when the host is in a `SOFT` state. Defaults to 1 minute.
  enable\_notifications|**Optional.** Whether notifications are enabled. Defaults to true.
  enable\_active\_checks|**Optional.** Whether active checks are enabled. Defaults to true.
  enable\_passive\_checks|**Optional.** Whether passive checks are enabled. Defaults to true.
  enable\_event\_handler|**Optional.** Enables event handlers for this host. Defaults to true.
  enable\_flap\_detection|**Optional.** Whether flap detection is enabled. Defaults to true.
  enable\_perfdata|**Optional.** Whether performance data processing is enabled. Defaults to true.
  event\_command  |**Optional.** The name of an event command that should be executed every time the host's state changes and when the host is in a `SOFT` state.
  flapping\_threshold|**Optional.** The flapping threshold in percent when a host is considered to be flapping.
  volatile        |**Optional.** The volatile setting enables always `HARD` state types if `NOT-OK` state changes occur.
  notes           |**Optional.** Notes for the host.
  notes_url       |**Optional.** Url for notes for the host (for example, in notification commands).
  action_url      |**Optional.** Url for actions for the host (for example, an external graphing tool).
  icon_image      |**Optional.** Icon image for the host. Required for external interfaces only.
  icon_image_alt  |**Optional.** Icon image description for the host. Required for external interface only.

> **Best Practice**
>
> The `address` and `address6` attributes are required for running commands using
> the `$address$` and `$address6` runtime macros.


### <a id="objecttype-hostgroup"></a> HostGroup

A group of hosts.

> **Best Practice**
>
> Assign host group members using the [group assign](#group-assign) rules.

Example:

    object HostGroup "my-hosts" {
      display_name = "My hosts"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the host group.
  groups          |**Optional.** An array of nested group names.

### <a id="objecttype-service"></a> Service

Service objects describe network services and how they should be checked
by Icinga 2.

> **Best Practice**
>
> Rather than creating a `Service` object for a specific host it is usually easier
> to just create a `Service` template and use the `apply` keyword to assign the
> service to a number of hosts.
> Check the [apply](#using-apply) chapter for details.

Example:

    object Service "uptime" {
      host_name = "localhost"

      display_name = "localhost Uptime"

      check_command = "check_snmp"

      vars.community = "public"
      vars.oid = "DISMAN-EVENT-MIB::sysUpTimeInstance"

      check_interval = 60s
      retry_interval = 15s

      groups = [ "all-services", "snmp" ]
    }

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the service.
  host_name       |**Required.** The host this service belongs to. There must be a `Host` object with that name.
  name            |**Required.** The service name. Must be unique on a per-host basis (Similar to the service_description attribute in Icinga 1.x).
  groups          |**Optional.** The service groups this service belongs to.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this service.
  check\_command  |**Required.** The name of the check command.
  max\_check\_attempts|**Optional.** The number of times a service is re-checked before changing into a hard state. Defaults to 3.
  check\_period   |**Optional.** The name of a time period which determines when this service should be checked. Not set by default.
  check\_interval |**Optional.** The check interval (in seconds). This interval is used for checks when the service is in a `HARD` state. Defaults to 5 minutes.
  retry\_interval |**Optional.** The retry interval (in seconds). This interval is used for checks when the service is in a `SOFT` state. Defaults to 1 minute.
  enable\_notifications|**Optional.** Whether notifications are enabled. Defaults to true.
  enable\_active\_checks|**Optional.** Whether active checks are enabled. Defaults to true.
  enable\_passive\_checks|**Optional.** Whether passive checks are enabled. Defaults to true.
  enable\_event\_handler|**Optional.** Enables event handlers for this host. Defaults to true.
  enable\_flap\_detection|**Optional.** Whether flap detection is enabled. Defaults to true.
  enable\_perfdata|**Optional.** Whether performance data processing is enabled. Defaults to true.
  event\_command  |**Optional.** The name of an event command that should be executed every time the service's state changes.
  flapping\_threshold|**Optional.** The flapping threshold in percent when a service is considered to be flapping.
  volatile        |**Optional.** The volatile setting enables always `HARD` state types if `NOT-OK` state changes occur.
  notes           |**Optional.** Notes for the service.
  notes_url       |**Optional.** Url for notes for the service (for example, in notification commands).
  action_url      |**Optional.** Url for actions for the service (for example, an external graphing tool).
  icon_image      |**Optional.** Icon image for the service. Required for external interfaces only.
  icon_image_alt  |**Optional.** Icon image description for the service. Required for external interface only.


Service objects have composite names, i.e. their names are based on the host_name attribute and the name you specified. This means
you can define more than one object with the same (short) name as long as the `host_name` attribute has a different value. 

### <a id="objecttype-servicegroup"></a> ServiceGroup

A group of services.

> **Best Practice**
>
> Assign service group members using the [group assign](#group-assign) rules.

Example:

    object ServiceGroup "snmp" {
      display_name = "SNMP services"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the service group.
  groups          |**Optional.** An array of nested group names.


### <a id="objecttype-user"></a> User

A user.

Example:

    object User "icingaadmin" {
      display_name = "Icinga 2 Admin"
      groups = [ "icingaadmins" ]
      email = "icinga@localhost"
      pager = "icingaadmin@localhost.localdomain"

      period = "24x7"

      states = [ OK, Warning, Critical, Unknown ]
      types = [ Problem, Recovery ]

      vars.additional_notes = "This is the Icinga 2 Admin account."
    }

Available notification state filters:

    OK
    Warning
    Critical
    Unknown
    Up
    Down

Available notification type filters:

    DowntimeStart
    DowntimeEnd
    DowntimeRemoved
    Custom
    Acknowledgement
    Problem
    Recovery
    FlappingStart
    FlappingEnd

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the user.
  email           |**Optional.** An email string for this user. Useful for notification commands.
  pager           |**Optional.** A pager string for this user. Useful for notification commands.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this user.
  groups          |**Optional.** An array of group names.
  enable_notifications|**Optional.** Whether notifications are enabled for this user.
  period          |**Optional.** The name of a time period which determines when a notification for this user should be triggered. Not set by default.
  types           |**Optional.** A set of type filters when this notification should be triggered. By default everything is matched.
  states          |**Optional.** A set of state filters when this notification should be triggered. By default everything is matched.


### <a id="objecttype-usergroup"></a> UserGroup

A user group.

> **Best Practice**
>
> Assign user group members using the [group assign](#group-assign) rules.

Example:

    object UserGroup "icingaadmins" {
        display_name = "Icinga 2 Admin Group"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the user group.
  groups          |**Optional.** An array of nested group names.



### <a id="objecttype-checkcommand"></a> CheckCommand

A check command definition. Additional default command custom attributes can be
defined here.

Example:

    object CheckCommand "check_http" {
      import "plugin-check-command"

      command = PluginDir + "/check_http"

      arguments = {
        "-H" = "$http_vhost$"
        "-I" = "$http_address$"
        "-u" = "$http_uri$"
        "-p" = "$http_port$"
        "-S" = {
          set_if = "$http_ssl$"
        }
        "-w" = "$http_warn_time$"
        "-c" = "$http_critical_time$"
      }

      vars.http_address = "$address$"
      vars.http_ssl = false
    }


Attributes:

  Name            |Description
  ----------------|----------------
  methods         |**Required.** The "execute" script method takes care of executing the check. In virtually all cases you should import the "plugin-check-command" template to take care of this setting.
  command         |**Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command.
  env             |**Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout         |**Optional.** The command timeout in seconds. Defaults to 5 minutes.
  arguments       |**Optional.** A dictionary of command arguments.


Command arguments can be defined as key-value-pairs in the `arguments`
dictionary. If the argument requires additional configuration for example
a `description` attribute or an optional condition, the value can be defined
as dictionary specifying additional options.

Service:

    vars.x_val = "My command argument value."
    vars.have_x = "true"

CheckCommand:

    arguments = {
      "-X" = {
        value = "$x_val$"
        description = "My plugin requires this argument for doing X."
        required = false    /* optional, no error if not set */
        skip_key = false    /* always use "-X <value>" */
        set_if = "$have_x$" /* only set if variable defined */
        order = 0           /* first position */
      }
    }

  Option      | Description
  ------------|--------------
  value       | Optional argument value.
  description | Optional argument description.
  required    | Required argument. Execution error if not set. Defaults to false (optional).
  skip_key    | Use the value as argument and skip the key.
  set_if      | Argument added if value is set (macro resolves to a defined value).
  order       | Set if multiple arguments require a defined argument order.


### <a id="objecttype-notificationcommand"></a> NotificationCommand

A notification command definition.

Example:

    object NotificationCommand "mail-service-notification" {
      import "plugin-notification-command"

      command = [
        SysconfDir + "/icinga2/scripts/mail-notification.sh"
      ]

      env = {
        "NOTIFICATIONTYPE" = "$notification.type$"
        "SERVICEDESC" = "$service.name$"
        "HOSTALIAS" = "$host.display_name$",
        "HOSTADDRESS" = "$address$",
        "SERVICESTATE" = "$service.state$",
        "LONGDATETIME" = "$icinga.long_date_time$",
        "SERVICEOUTPUT" = "$service.output$",
        "NOTIFICATIONAUTHORNAME" = "$notification.author$",
        "NOTIFICATIONCOMMENT" = "$notification.comment$",
        "HOSTDISPLAYNAME" = "$host.display_name$",
        "SERVICEDISPLAYNAME" = "$service.display_name$",
        "USEREMAIL" = "$user.email$"
      }
    }

Attributes:

  Name            |Description
  ----------------|----------------
  methods         |**Required.** The "execute" script method takes care of executing the notification. In virtually all cases you should import the "plugin-notification-command" template to take care of this setting.
  command         |**Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command.
  env             |**Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout         |**Optional.** The command timeout in seconds. Defaults to 5 minutes.
  arguments       |**Optional.** A dictionary of command arguments.

Command arguments can be used the same way as for `CheckCommand` objects.


### <a id="objecttype-eventcommand"></a> EventCommand

An event command definition.

Example:

    object EventCommand "restart-httpd-event" {
      import "plugin-event-command"

      command = "/opt/bin/restart-httpd.sh"
    }


Attributes:

  Name            |Description
  ----------------|----------------
  methods         |**Required.** The "execute" script method takes care of executing the event handler. In virtually all cases you should import the "plugin-event-command" template to take care of this setting.
  command         |**Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command.
  env             |**Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout         |**Optional.** The command timeout in seconds. Defaults to 5 minutes.
  arguments       |**Optional.** A dictionary of command arguments.

Command arguments can be used the same way as for `CheckCommand` objects.


### <a id="objecttype-notification"></a> Notification

Notification objects are used to specify how users should be notified in case
of host and service state changes and other events.

> **Best Practice**
>
> Rather than creating a `Notification` object for a specific host or service it is
> usually easier to just create a `Notification` template and use the `apply` keyword
> to assign the notification to a number of hosts or services. Use the `to` keyword
> to set the specific target type for `Host` or `Service`.
> Check the [notifications](#notifications) chapter for detailed examples.

Example:

    object Notification "localhost-ping-notification" {
      host_name = "localhost"
      service_name = "ping4"

      command = "mail-notification"

      users = [ "user1", "user2" ]

      types = [ Problem, Recovery ]
    }

Attributes:

  Name                      | Description
  --------------------------|----------------
  host_name                 | **Required.** The name of the host this notification belongs to.
  service_name              | **Optional.** The short name of the service this notification belongs to. If omitted this notification object is treated as host notification.
  vars                      | **Optional.** A dictionary containing custom attributes that are specific to this notification object.
  users                     | **Optional.** A list of user names who should be notified.
  user_groups               | **Optional.** A list of user group names who should be notified.
  times                     | **Optional.** A dictionary containing `begin` and `end` attributes for the notification.
  command                   | **Required.** The name of the notification command which should be executed when the notification is triggered.
  interval                  | **Optional.** The notification interval (in seconds). This interval is used for active notifications. Defaults to 30 minutes.
  period                    | **Optional.** The name of a time period which determines when this notification should be triggered. Not set by default.
  types                     | **Optional.** A list of type filters when this notification should be triggered. By default everything is matched.
  states                    | **Optional.** A list of state filters when this notification should be triggered. By default everything is matched.

Available notification state filters:

    OK
    Warning
    Critical
    Unknown
    Up
    Down

Available notification type filters:

    DowntimeStart
    DowntimeEnd
    DowntimeRemoved
    Custom
    Acknowledgement
    Problem
    Recovery
    FlappingStart
    FlappingEnd



### <a id="objecttype-timeperiod"></a> TimePeriod

Time periods can be used to specify when services should be checked or to limit
when notifications should be sent out.

Example:

    object TimePeriod "24x7" {
      import "legacy-timeperiod"

      display_name = "Icinga 2 24x7 TimePeriod"

      ranges = {
        monday = "00:00-24:00"
        tuesday = "00:00-24:00"
        wednesday = "00:00-24:00"
        thursday = "00:00-24:00"
        friday = "00:00-24:00"
        saturday = "00:00-24:00"
        sunday = "00:00-24:00"
      }
    }

Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the time period.
  methods         |**Required.** The "update" script method takes care of updating the internal representation of the time period. In virtually all cases you should import the "legacy-timeperiod" template to take care of this setting.
  ranges          |**Required.** A dictionary containing information which days and durations apply to this timeperiod.

The `/etc/icinga2/conf.d/timeperiods.conf` file is usually used to define
timeperiods including this one.


### <a id="objecttype-scheduleddowntime"></a> ScheduledDowntime

ScheduledDowntime objects can be used to set up recurring downtimes for services.

> **Best Practice**
>
> Rather than creating a `ScheduledDowntime` object for a specific host or service it is usually easier
> to just create a `ScheduledDowntime` template and use the `apply` keyword to assign the
> scheduled downtime to a number of hosts or services. Use the `to` keyword to set the specific target
> type for `Host` or `Service`.
> Check the [recurring downtimes](#recurring-downtimes) example for details.

Example:

    object ScheduledDowntime "some-downtime" {
      host_name = "localhost"
      service_name = "ping4"

      author = "icingaadmin"
      comment = "Some comment"

      fixed = false
      duration = 30m

      ranges = {
        "sunday" = "02:00-03:00"
      }
    }

Attributes:

  Name            |Description
  ----------------|----------------
  host_name       |**Required.** The name of the host this scheduled downtime belongs to.
  service_name    |**Optional.** The short name of the service this scheduled downtime belongs to. If omitted this downtime object is treated as host downtime.
  author          |**Required.** The author of the downtime.
  comment         |**Required.** A comment for the downtime.
  fixed           |**Optional.** Whether this is a fixed downtime. Defaults to true.
  duration        |**Optional.** How long the downtime lasts. Only has an effect for flexible (non-fixed) downtimes.
  ranges          |**Required.** A dictionary containing information which days and durations apply to this timeperiod.

ScheduledDowntime objects have composite names, i.e. their names are based
on the `host_name` and `service_name` attributes and the
name you specified. This means you can define more than one object
with the same (short) name as long as one of the `host_name` and
`service_name` attributes has a different value.


### <a id="objecttype-dependency"></a> Dependency

Dependency objects are used to specify dependencies between hosts and services.

> **Best Practice**
>
> Rather than creating a `Dependency` object for a specific host or service it is usually easier
> to just create a `Dependency` template and use the `apply` keyword to assign the
> dependency to a number of hosts or services. Use the `to` keyword to set the specific target
> type for `Host` or `Service`.
> Check the [dependencies](#dependencies) chapter for detailed examples.

Example:

    object Dependency "webserver-internet" {
      child_host_name = "webserver"
      child_service_name = "ping4"

      parent_host_name = "internet"
      parent_service_name = "ping4"

      states = [ OK, Warning ]

      disable_checks = true
    }

Attributes:

  Name            |Description
  ----------------|----------------
  parent_host_name     |**Required.** The parent host.
  parent_service_name  |**Optional.** The parent service. If omitted this dependency object is treated as host dependency.
  child_host_name      |**Required.** The child host.
  child_service_name   |**Optional.** The child service. If omitted this dependency object is treated as host dependency.
  disable_checks  |**Optional.** Whether to disable checks when this dependency fails. Defaults to false.
  disable_notifications|**Optional.** Whether to disable notifications when this dependency fails. Defaults to true.
  period          |**Optional.** Time period during which this dependency is enabled.
  states    	  |**Optional.** A list of state filters when this dependency should be OK. Defaults to [ OK, Warning ] for services and [ Up ] for hosts.

Available state filters:

    OK
    Warning
    Critical
    Unknown
    Up
    Down

Dependency objects have composite names, i.e. their names are based on the `child_host_name` and `child_service_name` attributes and the
name you specified. This means you can define more than one object with the same (short) name as long as one of the `child_host_name` and
`child_service_name` attributes has a different value.


### <a id="objecttype-perfdatawriter"></a> PerfdataWriter

Writes check result performance data to a defined path using macro
pattern consisting of custom attributes and runtime macros.

Example:

    library "perfdata"

    object PerfdataWriter "pnp" {
      host_perfdata_path = "/var/spool/icinga2/perfdata/host-perfdata"

      service_perfdata_path = "/var/spool/icinga2/perfdata/service-perfdata"

      host_format_template = "DATATYPE::HOSTPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tHOSTPERFDATA::$host.perfdata$\tHOSTCHECKCOMMAND::$host.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$"
      service_format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tSERVICEDESC::$service.name$\tSERVICEPERFDATA::$service.perfdata$\tSERVICECHECKCOMMAND::$service.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$\tSERVICESTATE::$service.state$\tSERVICESTATETYPE::$service.state_type$"

      rotation_interval = 15s
    }

Attributes:

  Name                    |Description
  ------------------------|----------------
  host_perfdata\_path     |**Optional.** Path to the host performance data file. Defaults to LocalStateDir + "/spool/icinga2/perfdata/host-perfdata".
  service_perfdata\_path  |**Optional.** Path to the service performance data file. Defaults to LocalStateDir + "/spool/icinga2/perfdata/service-perfdata".
  host_temp\_path         |**Optional.** Path to the temporary host file. Defaults to LocalStateDir + "/spool/icinga2/tmp/host-perfdata".
  service_temp\_path      |**Optional.** Path to the temporary service file. Defaults to LocalStateDir + "/spool/icinga2/tmp/service-perfdata".
  host_format\_template   |**Optional.** Host Format template for the performance data file. Defaults to a template that's suitable for use with PNP4Nagios.
  service_format\_template|**Optional.** Service Format template for the performance data file. Defaults to a template that's suitable for use with PNP4Nagios.
  rotation\_interval      |**Optional.** Rotation interval for the files specified in `{host,service}\_perfdata\_path`. Defaults to 30 seconds.

When rotating the performance data file the current UNIX timestamp is appended to the path specified
in `host_perfdata\_path` and `service_perfdata\_path` to generate a unique filename.


### <a id="objecttype-graphitewriter"></a> GraphiteWriter

Writes check result metrics and performance data to a defined
Graphite Carbon host.

Example:

    library "perfdata"

    object GraphiteWriter "graphite" {
      host = "127.0.0.1"
      port = 2003
    }

Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Optional.** Graphite Carbon host address. Defaults to '127.0.0.1'.
  port            |**Optional.** Graphite Carbon port. Defaults to 2003.

### <a id="objecttype-idomysqlconnection"></a> IdoMySqlConnection

IDO database adapter for MySQL.

Example:

    library "db_ido_mysql"

    object IdoMysqlConnection "mysql-ido" {
      host = "127.0.0.1"
      port = 3306
      user = "icinga"
      password = "icinga"
      database = "icinga"
      table_prefix = "icinga_"
      instance_name = "icinga2"
      instance_description = "icinga2 instance"

      cleanup = {
        downtimehistory_age = 48h
        logentries_age = 31d
      }

      categories = DbCatConfig | DbCatState
    }

Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Optional.** MySQL database host address. Defaults to "localhost".
  port            |**Optional.** MySQL database port. Defaults to 3306.
  user            |**Optional.** MySQL database user with read/write permission to the icinga database. Defaults to "icinga".
  password        |**Optional.** MySQL database user's password. Defaults to "icinga".
  database        |**Optional.** MySQL database name. Defaults to "icinga".
  table\_prefix   |**Optional.** MySQL database table prefix. Defaults to "icinga\_".
  instance\_name  |**Optional.** Unique identifier for the local Icinga 2 instance. Defaults to "default".
  instance\_description|**Optional.** Description for the Icinga 2 instance.
  cleanup         |**Optional.** Dictionary with items for historical table cleanup.
  categories      |**Optional.** The types of information that should be written to the database.

Cleanup Items:

  Name            | Description
  ----------------|----------------
  acknowledgements_age |**Optional.** Max age for acknowledgements table rows (entry_time). Defaults to 0 (never).
  commenthistory_age |**Optional.** Max age for commenthistory table rows (entry_time). Defaults to 0 (never).
  contactnotifications_age |**Optional.** Max age for contactnotifications table rows (start_time). Defaults to 0 (never).
  contactnotificationmethods_age |**Optional.** Max age for contactnotificationmethods table rows (start_time). Defaults to 0 (never).
  downtimehistory_age |**Optional.** Max age for downtimehistory table rows (entry_time). Defaults to 0 (never).
  eventhandlers_age |**Optional.** Max age for eventhandlers table rows (start_time). Defaults to 0 (never).
  externalcommands_age |**Optional.** Max age for externalcommands table rows (entry_time). Defaults to 0 (never).
  flappinghistory_age |**Optional.** Max age for flappinghistory table rows (event_time). Defaults to 0 (never).
  hostchecks_age |**Optional.** Max age for hostalives table rows (start_time). Defaults to 0 (never).
  logentries_age |**Optional.** Max age for logentries table rows (logentry_time). Defaults to 0 (never).
  notifications_age |**Optional.** Max age for notifications table rows (start_time). Defaults to 0 (never).
  processevents_age |**Optional.** Max age for processevents table rows (event_time). Defaults to 0 (never).
  statehistory_age |**Optional.** Max age for statehistory table rows (state_time). Defaults to 0 (never).
  servicechecks_age |**Optional.** Max age for servicechecks table rows (start_time). Defaults to 0 (never).
  systemcommands_age |**Optional.** Max age for systemcommands table rows (start_time). Defaults to 0 (never).

Data Categories:

  Name                 | Description            | Required by
  ---------------------|------------------------|--------------------
  DbCatConfig          | Configuration data     | Icinga Web/Reporting
  DbCatState           | Current state data     | Icinga Web/Reporting
  DbCatAcknowledgement | Acknowledgements       | Icinga Web/Reporting
  DbCatComment         | Comments               | Icinga Web/Reporting
  DbCatDowntime        | Downtimes              | Icinga Web/Reporting
  DbCatEventHandler    | Event handler data     | Icinga Web/Reporting
  DbCatExternalCommand | External commands      | Icinga Web/Reporting
  DbCatFlapping        | Flap detection data    | Icinga Web/Reporting
  DbCatCheck           | Check results          | --
  DbCatLog             | Log messages           | Icinga Web/Reporting
  DbCatNotification    | Notifications          | Icinga Web/Reporting
  DbCatProgramStatus   | Program status data    | Icinga Web/Reporting
  DbCatRetention       | Retention data         | Icinga Web/Reporting
  DbCatStateHistory    | Historical state data  | Icinga Web/Reporting

Multiple categories can be combined using the `|` operator. In addition to
the category flags listed above the `DbCatEverything` flag may be used as
a shortcut for listing all flags.

External interfaces like Icinga Web require everything except `DbCatCheck`
which is the default value if `categories` is not set.

### <a id="objecttype-idomysqlconnection"></a> IdoPgSqlConnection

IDO database adapter for PostgreSQL.

Example:

    library "db_ido_pgsql"

    object IdoMysqlConnection "pgsql-ido" {
      host = "127.0.0.1"
      port = 5432
      user = "icinga"
      password = "icinga"
      database = "icinga"
      table_prefix = "icinga_"
      instance_name = "icinga2"
      instance_description = "icinga2 instance"

      cleanup = {
        downtimehistory_age = 48h
        logentries_age = 31d
      }

      categories = DbCatConfig | DbCatState
    }

Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Optional.** PostgreSQL database host address. Defaults to "localhost".
  port            |**Optional.** PostgreSQL database port. Defaults to "5432".
  user            |**Optional.** PostgreSQL database user with read/write permission to the icinga database. Defaults to "icinga".
  password        |**Optional.** PostgreSQL database user's password. Defaults to "icinga".
  database        |**Optional.** PostgreSQL database name. Defaults to "icinga".
  table\_prefix   |**Optional.** PostgreSQL database table prefix. Defaults to "icinga\_".
  instance\_name  |**Optional.** Unique identifier for the local Icinga 2 instance. Defaults to "default".
  instance\_description|**Optional.** Description for the Icinga 2 instance.
  cleanup         |**Optional.** Dictionary with items for historical table cleanup.
  categories      |**Optional.** The types of information that should be written to the database.

Cleanup Items:

  Name            | Description
  ----------------|----------------
  acknowledgements_age |**Optional.** Max age for acknowledgements table rows (entry_time). Defaults to 0 (never).
  commenthistory_age |**Optional.** Max age for commenthistory table rows (entry_time). Defaults to 0 (never).
  contactnotifications_age |**Optional.** Max age for contactnotifications table rows (start_time). Defaults to 0 (never).
  contactnotificationmethods_age |**Optional.** Max age for contactnotificationmethods table rows (start_time). Defaults to 0 (never).
  downtimehistory_age |**Optional.** Max age for downtimehistory table rows (entry_time). Defaults to 0 (never).
  eventhandlers_age |**Optional.** Max age for eventhandlers table rows (start_time). Defaults to 0 (never).
  externalcommands_age |**Optional.** Max age for externalcommands table rows (entry_time). Defaults to 0 (never).
  flappinghistory_age |**Optional.** Max age for flappinghistory table rows (event_time). Defaults to 0 (never).
  hostchecks_age |**Optional.** Max age for hostalives table rows (start_time). Defaults to 0 (never).
  logentries_age |**Optional.** Max age for logentries table rows (logentry_time). Defaults to 0 (never).
  notifications_age |**Optional.** Max age for notifications table rows (start_time). Defaults to 0 (never).
  processevents_age |**Optional.** Max age for processevents table rows (event_time). Defaults to 0 (never).
  statehistory_age |**Optional.** Max age for statehistory table rows (state_time). Defaults to 0 (never).
  servicechecks_age |**Optional.** Max age for servicechecks table rows (start_time). Defaults to 0 (never).
  systemcommands_age |**Optional.** Max age for systemcommands table rows (start_time). Defaults to 0 (never).

Data Categories:

  Name                 | Description            | Required by
  ---------------------|------------------------|--------------------
  DbCatConfig          | Configuration data     | Icinga Web/Reporting
  DbCatState           | Current state data     | Icinga Web/Reporting
  DbCatAcknowledgement | Acknowledgements       | Icinga Web/Reporting
  DbCatComment         | Comments               | Icinga Web/Reporting
  DbCatDowntime        | Downtimes              | Icinga Web/Reporting
  DbCatEventHandler    | Event handler data     | Icinga Web/Reporting
  DbCatExternalCommand | External commands      | Icinga Web/Reporting
  DbCatFlapping        | Flap detection data    | Icinga Web/Reporting
  DbCatCheck           | Check results          | --
  DbCatLog             | Log messages           | Icinga Web/Reporting
  DbCatNotification    | Notifications          | Icinga Web/Reporting
  DbCatProgramStatus   | Program status data    | Icinga Web/Reporting
  DbCatRetention       | Retention data         | Icinga Web/Reporting
  DbCatStateHistory    | Historical state data  | Icinga Web/Reporting

Multiple categories can be combined using the `|` operator. In addition to
the category flags listed above the `DbCatEverything` flag may be used as
a shortcut for listing all flags.

External interfaces like Icinga Web require everything except `DbCatCheck`
which is the default value if `categories` is not set.

### <a id="objecttype-livestatuslistener"></a> LiveStatusListener

Livestatus API interface available as TCP or UNIX socket. Historical table queries
require the `CompatLogger` feature enabled pointing to the log files using the
`compat_log_path` configuration attribute.

Example:

    library "livestatus"

    object LivestatusListener "livestatus-tcp" {
      socket_type = "tcp"
      bind_host = "127.0.0.1"
      bind_port = "6558"
    }

    object LivestatusListener "livestatus-unix" {
      socket_type = "unix"
      socket_path = "/var/run/icinga2/cmd/livestatus"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  socket\_type      |**Optional.** Specifies the socket type. Can be either "tcp" or "unix". Defaults to "unix".
  bind\_host        |**Optional.** Only valid when socket\_type is "tcp". Host address to listen on for connections. Defaults to "127.0.0.1".
  bind\_port        |**Optional.** Only valid when `socket\_type` is "tcp". Port to listen on for connections. Defaults to 6558.
  socket\_path      |**Optional.** Only valid when `socket\_type` is "unix". Specifies the path to the UNIX socket file. Defaults to LocalStateDir + "/run/icinga2/cmd/livestatus".
  compat\_log\_path |**Optional.** Required for historical table queries. Requires `CompatLogger` feature enabled. Defaults to LocalStateDir + "/log/icinga2/compat"

> **Note**
>
> UNIX sockets are not supported on Windows.

### <a id="objecttype-statusdatawriter"></a> StatusDataWriter

Periodically writes status data files which are used by the Classic UI and other third-party tools.

Example:

    library "compat"

    object StatusDataWriter "status" {
        status\_path = "/var/cache/icinga2/status.dat"
        objects\_path = "/var/cache/icinga2/objects.path"
        update\_interval = 30s
    }

Attributes:

  Name            |Description
  ----------------|----------------
  status\_path    |**Optional.** Path to the status.dat file. Defaults to LocalStateDir + "/cache/icinga2/status.dat".
  objects\_path   |**Optional.** Path to the objects.cache file. Defaults to LocalStateDir + "/cache/icinga2/objects.cache".
  update\_interval|**Optional.** The interval in which the status files are updated. Defaults to 15 seconds.


### <a id="objecttype-externalcommandlistener"></a> ExternalCommandListener

Implements the Icinga 1.x command pipe which can be used to send commands to Icinga.

Example:

    library "compat"

    object ExternalCommandListener "external" {
        command\_path = "/var/run/icinga2/cmd/icinga2.cmd"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  command\_path   |**Optional.** Path to the command pipe. Defaults to LocalStateDir + "/run/icinga2/cmd/icinga2.cmd".

### <a id="objecttype-compatlogger"></a> CompatLogger

Writes log files in a format that's compatible with Icinga 1.x.

Example:

    library "compat"

    object CompatLogger "my-log" {
      log\_dir = "/var/log/icinga2/compat"
      rotation\_method = "HOURLY"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  log\_dir        |**Optional.** Path to the compat log directory. Defaults to LocalStateDir + "/log/icinga2/compat".
  rotation\_method|**Optional.** Specifies when to rotate log files. Can be one of "HOURLY", "DAILY", "WEEKLY" or "MONTHLY". Defaults to "HOURLY".


### <a id="objecttype-checkresultreader"></a> CheckResultReader

Reads Icinga 1.x check results from a directory. This functionality is provided
to help existing Icinga 1.x users and might be useful for certain cluster
scenarios.

Example:

    library "compat"

    object CheckResultReader "reader" {
      spool_dir = "/data/check-results"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  spool\_dir      |**Optional.** The directory which contains the check result files. Defaults to LocalStateDir + "/lib/icinga2/spool/checkresults/".


### <a id="objecttype-checkcomponent"></a> CheckerComponent

The checker component is responsible for scheduling active checks. There are no configurable options.

Example:

    library "checker"

    object CheckerComponent "checker" { }

Can be enabled/disabled using

    # icinga2-enable-feature checker


### <a id="objecttype-notificationcomponent"></a> NotificationComponent

The notification component is responsible for sending notifications. There are no configurable options.

Example:

    library "notification"

    object NotificationComponent "notification" { }

Can be enabled/disabled using

    # icinga2-enable-feature notification


### <a id="objecttype-filelogger"></a> FileLogger

Specifies Icinga 2 logging to a file.

Example:

    object FileLogger "debug-file" {
      severity = "debug"
      path = "/var/log/icinga2/debug.log"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  path            |**Required.** The log path.
  severity        |**Optional.** The minimum severity for this log. Can be "debug", "notice", "information", "warning" or "critical". Defaults to "information".


### <a id="objecttype-sysloglogger"></a> SyslogLogger

Specifies Icinga 2 logging to syslog.

Example:

    object SyslogLogger "crit-syslog" {
      severity = "critical"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  severity        |**Optional.** The minimum severity for this log. Can be "debug", "notice", "information", "notice", "warning" or "critical". Defaults to "warning".



### <a id="objecttype-icingastatuswriter"></a> IcingaStatusWriter

The IcingaStatusWriter feature periodically dumps the current status
and performance data from Icinga 2 and all registered features into
a defined JSON file.

Example:

    object IcingaStatusWriter "status" {
      status_path = LocalStateDir + "/cache/icinga2/status.json"
      update_interval = 15s
    }

Attributes:

  Name                      |Description
  --------------------------|--------------------------
  status\_path              |**Optional.** Path to cluster status file. Defaults to LocalStateDir + "/cache/icinga2/status.json"
  update\_interval          |**Optional.** The interval in which the status files are updated. Defaults to 15 seconds.


### <a id="objecttype-apilistener"></a> ApiListener

ApiListener objects are used for distributed monitoring setups
specifying the certificate files used for ssl authorization.

The `NodeName` constant must be defined in [constants.conf](#constants-conf).

Example:

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
    }


Attributes:

  Name                      |Description
  --------------------------|--------------------------
  cert\_path                |**Required.** Path to the public key.
  key\_path                 |**Required.** Path to the private key.
  ca\_path                  |**Required.** Path to the CA certificate file.
  crl\_path                 |**Optional.** Path to the CRL file.
  bind\_host                |**Optional.** The IP address the api listener should be bound to. Defaults to `0.0.0.0`.
  bind\_port                |**Optional.** The port the api listener should be bound to. Defaults to `5665`.
  accept\_config            |**Optional.** Accept zone configuration. Defaults to `false`.


### <a id="objecttype-endpoint"></a> Endpoint

Endpoint objects are used to specify connection information for remote
Icinga 2 instances.

Example:

    object Endpoint "icinga2b" {
      host = "192.168.5.46"
      port = 5665
    }

Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Required.** The hostname/IP address of the remote Icinga 2 instance.
  port            |**Optional.** The service name/port of the remote Icinga 2 instance. Defaults to `5665`.
  log_duration    |**Optional.** Duration for keeping replay logs on connection loss. Defaults to `1d`.


### <a id="objecttype-zone"></a> Zone

Zone objects are used to specify which Icinga 2 instances are located in a zone.
All zone endpoints elect one active master instance among them (required for High-Availability setups).

Example:

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b" ]

    }

    object Zone "check-satellite" {
      endpoints = [ "icinga2c" ]
      parent = "config-ha-master"
    }

Attributes:

  Name            |Description
  ----------------|----------------
  endpoints       |**Optional.** Dictionary with endpoints located in this zone.
  parent          |**Optional.** Parent zone.



## <a id="icinga-template-library"></a> Icinga Template Library

### <a id="itl-overview"></a> Overview

The Icinga Template Library (ITL) implements standard templates and object
definitions for commonly used services.

You can include the ITL by using the `include` directive in your configuration
file:

    include <itl>

### <a id="itl-generic-templates"></a> Generic Templates

These templates are imported by the provided example configuration.

#### <a id="itl-plugin-check-command"></a> plugin-check-command

Command template for check plugins executed by Icinga 2.

The `plugin-check-command` command does not support any vars.

#### <a id="itl-plugin-notification-command"></a> plugin-notification-command

Command template for notification scripts executed by Icinga 2.

The `plugin-notification-command` command does not support any vars.

#### <a id="itl-plugin-event-command"></a> plugin-event-command

Command template for event handler scripts executed by Icinga 2.

The `plugin-event-command` command does not support any vars.

### <a id="itl-check-commands"></a> Check Commands

These check commands are embedded into Icinga 2 and do not require any external
plugin scripts.

#### <a id="itl-icinga"></a> icinga

Check command for the built-in `icinga` check. This check returns performance
data for the current Icinga instance.

The `icinga` check command does not support any vars.

#### <a id="itl-icinga-cluster"></a> cluster

Check command for the built-in `cluster` check. This check returns performance
data for the current Icinga instance and connected endpoints.

The `cluster` check command does not support any vars.

#### <a id="itl-icinga-cluster-zone"></a> cluster-zone

Check command for the built-in `cluster-zone` check.

Cluster Attributes:

Name         | Description
-------------|---------------
cluster_zone | **Optional.** The zone name. Defaults to "$host.name$".

## <a id="plugin-check-commands"></a> Plugin Check Commands

### <a id="plugin-check-command-overview"></a> Overview

The Plugin Check Commands provides example configuration for plugin check commands
provided by the the Monitoring Plugins project.

You can include the plugin check command definitions by using the `include`
directive in your configuration file:

    include <plugins>

The plugin check commands assume that there's a global constant named `PluginDir`
which contains the path of the plugins from the Monitoring Plugins project.


#### <a id="plugin-check-command-ping4"></a> ping4

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### <a id="plugin-check-command-ping6"></a> ping6

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv6 address. Defaults to "$address6$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### <a id="plugin-check-command-hostalive"></a> hostalive

Check command object for the `check_ping` plugin with host check default values.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 80.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 100.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### <a id="plugin-check-command-dummy"></a> dummy

Check command object for the `check_dummy` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
dummy_text      | **Optional.** Plugin output. Defaults to "Check was successful.".

#### <a id="plugin-check-command-passive"></a> passive

Specialised check command object for passive checks executing the `check_dummy` plugin with appropriate default values.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 3.
dummy_text      | **Optional.** Plugin output. Defaults to "No Passive Check Result Received.".

#### <a id="plugin-check-command-tcp"></a> tcp

Check command object for the `check_tcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
tcp_address     | **Optional.** The host's address. Defaults to "$address$".
tcp_port        | **Required.** The port that should be checked.

#### <a id="plugin-check-command-udp"></a> udp

Check command object for the `check_udp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
udp_address     | **Optional.** The host's address. Defaults to "$address$".
udp_port        | **Required.** The port that should be checked.

#### <a id="plugin-check-command-http"></a> http

Check command object for the `check_http` plugin.

Custom Attributes:

Name               | Description
-------------------|--------------
http_address       | **Optional.** The host's address. Defaults to "$address".
http_vhost         | **Optional.** The virtual host that should be sent in the "Host" header.
http_uri           | **Optional.** The request URI.
http_port          | **Optional.** The TCP port. Defaults to 80 when not using SSL, 443 otherwise.
http_ssl           | **Optional.** Whether to use SSL. Defaults to false.
http_warn_time     | **Optional.** The warning threshold.
http_critical_time | **Optional.** The critical threshold.

#### <a id="plugin-check-command-ftp"></a> ftp

Check command object for the `check_ftp` plugin.

Custom Attributes:

Name               | Description
-------------------|--------------
ftp_address        | **Optional.** The host's address. Defaults to "$address".

#### <a id="plugin-check-command-smtp"></a> smtp

Check command object for the `check_smtp` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
smtp_address         | **Optional.** The host's address. Defaults to "$address$".

#### <a id="plugin-check-command-ssmtp"></a> ssmtp

Check command object for the `check_ssmtp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssmtp_address   | **Required.** The host's address. Defaults to "$address$".
ssmtp_port      | **Optional.** The port that should be checked. Defaults to 465.

#### <a id="plugin-check-command-imap"></a> imap

Check command object for the `check_imap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
imap_address    | **Optional.** The host's address. Defaults to "$address$".

#### <a id="plugin-check-command-simap"></a> simap

Check command object for the `check_simap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
simap_address   | **Optional.** The host's address. Defaults to "$address$".
simap_port      | **Optional.** The host's port.

#### <a id="plugin-check-command-pop"></a> pop

Check command object for the `check_pop` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
pop_address     | **Optional.** The host's address. Defaults to "$address$".

#### <a id="plugin-check-command-spop"></a> spop 

Check command object for the `check_spop` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
spop_address    | **Optional.** The host's address. Defaults to "$address$".
spop_port       | **Optional.** The host's port.

#### <a id="plugin-check-command-ntp-time"></a> ntp_time

Check command object for the `check_ntp_time` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ntp_address     | **Optional.** The host's address. Defaults to "$address$".

#### <a id="plugin-check-command-ssh"></a> ssh

Check command object for the `check_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssh_address     | **Optional.** The host's address. Defaults to "$address$".
ssh_port        | **Optional.** The port that should be checked. Defaults to 22.

#### <a id="plugin-check-command-disk"></a> disk

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
disk_wfree      | **Optional.** The free space warning threshold in %. Defaults to 20.
disk_cfree      | **Optional.** The free space critical threshold in %. Defaults to 10.

#### <a id="plugin-check-command-users"></a> users

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
users_wgreater  | **Optional.** The user count warning threshold. Defaults to 20.
users_cgreater  | **Optional.** The user count critical threshold. Defaults to 50.

#### <a id="plugin-check-command-processes"></a> procs

Check command object for the `check_procs` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
procs_wgreater  | **Optional.** The process count warning threshold. Defaults to 250.
procs_cgreater  | **Optional.** The process count critical threshold. Defaults to 400.

#### <a id="plugin-check-command-swap"></a> swap

Check command object for the `check_swap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
swap_wfree      | **Optional.** The free swap space warning threshold in %. Defaults to 50.
swap_cfree      | **Optional.** The free swap space critical threshold in %. Defaults to 25.

#### <a id="plugin-check-command-load"></a> load

Check command object for the `check_load` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
load_wload1     | **Optional.** The 1-minute warning threshold. Defaults to 5.
load_wload5     | **Optional.** The 5-minute warning threshold. Defaults to 4.
load_wload15    | **Optional.** The 15-minute warning threshold. Defaults to 3.
load_cload1     | **Optional.** The 1-minute critical threshold. Defaults to 10.
load_cload5     | **Optional.** The 5-minute critical threshold. Defaults to 6.
load_cload15    | **Optional.** The 15-minute critical threshold. Defaults to 4.

#### <a id="plugin-check-command-snmp"></a> snmp

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Required.** The SNMP OID.
snmp_community  | **Optional.** The SNMP community. Defaults to "public".

#### <a id="plugin-check-command-snmp-uptime"></a> snmp-uptime

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
snmp_community  | **Optional.** The SNMP community. Defaults to "public".

#### <a id="plugin-check-command-dhcp"></a> dhcp

Check command object for the `check_dhcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
dhcp_serverip   | **Optional.** The IP address of the DHCP server which we should get a response from.
dhcp_requestedip| **Optional.** The IP address which we should be offered by a DHCP server.
dhcp_timeout    | **Optional.** The timeout in seconds.
dhcp_interface  | **Optional.** The interface to use.
dhcp_mac        | **Optional.** The MAC address to use in the DHCP request.
dhcp_unicast    | **Optional.** Whether to use unicast requests. Defaults to false.

#### <a id="plugin-check-command-nscp"></a> nscp

Check command object for the `check_nt` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
nscp_address    | **Optional.** The host's address. Defaults to "$address$".
nscp_port       | **Optional.** The NSClient++ port. Defaults to 12489.
nscp_password   | **Optional.** The NSClient++ password.
nscp_variable   | **Required.** The variable that should be checked.
nscp_params     | **Optional.** Parameters for the query.
nscp_warn       | **Optional.** The warning threshold.
nscp_crit       | **Optional.** The critical threshold.
nscp_timeout    | **Optional.** The query timeout in seconds.

#### <a id="plugin-check-command-by-ssh"></a> by_ssh

Check command object for the `check_by_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
by_ssh_address  | **Optional.** The host's address. Defaults to "$address$".
by_ssh_port     | **Optional.** The SSH port. Defaults to 22.
by_ssh_command  | **Optional.** The command that should be executed.
by_ssh_logname  | **Optional.** The SSH username.
by_ssh_identity | **Optional.** The SSH identity.
by_ssh_quiet    | **Optional.** Whether to suppress SSH warnings. Defaults to false.
by_ssh_warn     | **Optional.** The warning threshold.
by_ssh_crit     | **Optional.** The critical threshold.
by_ssh_timeout  | **Optional.** The timeout in seconds.

#### <a id="plugin-check-command-ups"></a> ups

Check command object for the `check_ups` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ups_address     | **Optional.** The host's address. Defaults to "$address$".
ups_name        | **Optional.** The UPS name. Defaults to `ups`.


#### <a id="plugin-check-command-nrpe"></a> nrpe

Check command object for the `check_nrpe` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
nrpe_address    | **Optional.** The host's address. Defaults to "$address$".
nrpe_port       | **Optional.** The NRPE port. Defaults to 5668.
nrpe_command    | **Optional.** The command that should be executed.
nrpe_no_ssl     | **Optional.** Whether to disable SSL or not. Defaults to `false`.
nrpe_timeout_unknown | **Optional.** Whether to set timeouts to unknown instead of critical state. Defaults to `false`.
nrpe_timeout    | **Optional.** The timeout in seconds.

#### <a id="plugin-check-command-apt"></a> apt

Check command for the `check_apt` plugin.

The `apt` check command does not support any vars.
