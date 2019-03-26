# Library Reference <a id="library-reference"></a>

## Global functions <a id="global-functions"></a>

These functions are globally available in [assign/ignore where expressions](03-monitoring-basics.md#using-apply-expressions),
[functions](17-language-reference.md#functions), [API filters](12-icinga2-api.md#icinga2-api-filters)
and the [Icinga 2 debug console](11-cli-commands.md#cli-command-console).

You can use the [Icinga 2 debug console](11-cli-commands.md#cli-command-console)
as a sandbox to test these functions before implementing
them in your scenarios.

### regex <a id="global-functions-regex"></a>

Signature:

```
function regex(pattern, value, mode)
```

Returns true if the regular expression `pattern` matches the `value`, false otherwise.
The `value` can be of the type [String](18-library-reference.md#string-type) or [Array](18-library-reference.md#array-type) (which
contains string elements).

The `mode` argument is optional and can be either `MatchAll` (in which case all elements
for an array have to match) or `MatchAny` (in which case at least one element has to match).
The default mode is `MatchAll`.

**Tip**: In case you are looking for regular expression tests try [regex101](https://regex101.com).

Example for string values:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => host.vars.os_type = "Linux/Unix"
null
<2> => regex("^Linux", host.vars.os_type)
true
<3> => regex("^Linux$", host.vars.os_type)
false
```

Example for an array of string values:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => host.vars.databases = [ "db-prod1", "db-prod2", "db-dev" ]
null
<2> => regex("^db-prod\\d+", host.vars.databases, MatchAny)
true
<3> => regex("^db-prod\\d+", host.vars.databases, MatchAll)
false
```

### match <a id="global-functions-match"></a>

Signature:

```
function match(pattern, text, mode)
```

Returns true if the wildcard (`?*`) `pattern` matches the `value`, false otherwise.
The `value` can be of the type [String](18-library-reference.md#string-type) or [Array](18-library-reference.md#array-type) (which
contains string elements).

The `mode` argument is optional and can be either `MatchAll` (in which case all elements
for an array have to match) or `MatchAny` (in which case at least one element has to match).
The default mode is `MatchAll`.

Example for string values:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var name = "db-prod-sfo-657"
null
<2> => match("*prod-sfo*", name)
true
<3> => match("*-dev-*", name)
false
```

Example for an array of string values:

```
$ icinga2 console
Icinga 2 (version: v2.7.0-28)
<1> => host.vars.application_types = [ "web-wp", "web-rt", "db-local" ]
null
<2> => match("web-*", host.vars.application_types, MatchAll)
false
<3> => match("web-*", host.vars.application_types, MatchAny)
true
```

### cidr_match <a id="global-functions-cidr_match"></a>

Signature:

```
function cidr_match(pattern, ip, mode)
```

Returns true if the CIDR pattern matches the IP address, false otherwise.

IPv4 addresses are converted to IPv4-mapped IPv6 addresses before being
matched against the pattern. The `mode` argument is optional and can be
either `MatchAll` (in which case all elements for an array have to match) or `MatchAny`
(in which case at least one element has to match). The default mode is `MatchAll`.


Example for a single IP address:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => host.address = "192.168.56.101"
null
<2> => cidr_match("192.168.56.0/24", host.address)
true
<3> => cidr_match("192.168.56.0/26", host.address)
false
```

Example for an array of IP addresses:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => host.vars.vhost_ips = [ "192.168.56.101", "192.168.56.102", "10.0.10.99" ]
null
<2> => cidr_match("192.168.56.0/24", host.vars.vhost_ips, MatchAll)
false
<3> => cidr_match("192.168.56.0/24", host.vars.vhost_ips, MatchAny)
true
```

### range <a id="global-functions-range"></a>

Signature:

```
function range(end)
function range(start, end)
function range(start, end, increment)
```

Returns an array of numbers in the specified range.
If you specify one parameter, the first element starts at `0`.
The following array numbers are incremented by `1` and stop before
the specified end.
If you specify the start and end numbers, the returned array
number are incremented by `1`. They start at the specified start
number and stop before the end number.
Optionally you can specify the incremented step between numbers
as third parameter.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => range(5)
[ 0.000000, 1.000000, 2.000000, 3.000000, 4.000000 ]
<2> => range(2,4)
[ 2.000000, 3.000000 ]
<3> => range(2,10,2)
[ 2.000000, 4.000000, 6.000000, 8.000000 ]
```

### len <a id="global-functions-len"></a>

Signature:

```
function len(value)
```

Returns the length of the value, i.e. the number of elements for an array
or dictionary, or the length of the string in bytes.

**Note**: Instead of using this global function you are advised to use the type's
prototype method: [Array#len](18-library-reference.md#array-len), [Dictionary#len](18-library-reference.md#dictionary-len) and
[String#len](18-library-reference.md#string-len).

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => host.groups = [ "linux-servers", "db-servers" ]
null
<2> => host.groups.len()
2.000000
<3> => host.vars.disks["/"] = {}
null
<4> => host.vars.disks["/var"] = {}
null
<5> => host.vars.disks.len()
2.000000
<6> => host.vars.os_type = "Linux/Unix"
null
<7> => host.vars.os_type.len()
10.000000
```

### union <a id="global-functions-union"></a>

Signature:

```
function union(array, array, ...)
```

Returns an array containing all unique elements from the specified arrays.

Example:
```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var dev_notification_groups = [ "devs", "slack" ]
null
<2> => var host_notification_groups = [ "slack", "noc" ]
null
<3> => union(dev_notification_groups, host_notification_groups)
[ "devs", "noc", "slack" ]
```

### intersection <a id="global-functions-intersection"></a>

Signature:

```
function intersection(array, array, ...)
```

Returns an array containing all unique elements which are common to all
specified arrays.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var dev_notification_groups = [ "devs", "slack" ]
null
<2> => var host_notification_groups = [ "slack", "noc" ]
null
<3> => intersection(dev_notification_groups, host_notification_groups)
[ "slack" ]
```

### keys <a id="global-functions-keys"></a>

Signature:

```
function keys(dict)
```

Returns an array containing the dictionary's keys.

**Note**: Instead of using this global function you are advised to use the type's
prototype method: [Dictionary#keys](18-library-reference.md#dictionary-keys).

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => host.vars.disks["/"] = {}
null
<2> => host.vars.disks["/var"] = {}
null
<3> => host.vars.disks.keys()
[ "/", "/var" ]
```

### string <a id="global-functions-string"></a>

Signature:

```
function string(value)
```

Converts the value to a string.

**Note**: Instead of using this global function you are advised to use the type's
prototype method:

* [Number#to_string](18-library-reference.md#number-to_string)
* [Boolean#to_string](18-library-reference.md#boolean-to_string)
* [String#to_string](18-library-reference.md#string-to_string)
* [Object#to_string](18-library-reference.md#object-to-string) for Array and Dictionary types
* [DateTime#to_string](18-library-reference.md#datetime-tostring)

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => 5.to_string()
"5"
<2> => false.to_string()
"false"
<3> => "abc".to_string()
"abc"
<4> => [ "dev", "slack" ].to_string()
"[ \"dev\", \"slack\" ]"
<5> => { "/" = {}, "/var" = {} }.to_string()
"{\n\t\"/\" = {\n\t}\n\t\"/var\" = {\n\t}\n}"
<6> => DateTime(2016, 11, 25).to_string()
"2016-11-25 00:00:00 +0100"
```

### number <a id="global-functions-number"></a>

Signature:

```
function number(value)
```

Converts the value to a number.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => number(false)
0.000000
<2> => number("78")
78.000000
```

### bool <a id="global-functions-bool"></a>

Signature:

```
function bool(value)
```

Converts the value to a bool.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => bool(1)
true
<2> => bool(0)
false
```

### random <a id="global-functions-random"></a>

Signature:

```
function random()
```

Returns a random value between 0 and RAND\_MAX (as defined in stdlib.h).

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => random()
1263171996.000000
<2> => random()
108402530.000000
```

### log <a id="global-functions-log"></a>

Signature:

```
function log(value)
```

Writes a message to the log. Non-string values are converted to a JSON string.

Signature:

```
function log(severity, facility, value)
```

Writes a message to the log. `severity` can be one of `LogDebug`, `LogNotice`,
`LogInformation`, `LogWarning`, and `LogCritical`.

Non-string values are converted to a JSON string.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => log(LogCritical, "Console", "First line")
critical/Console: First line
null
<2> => var groups = [ "devs", "slack" ]
null
<3> => log(LogCritical, "Console", groups)
critical/Console: ["devs","slack"]
null
```

### typeof <a id="global-functions-typeof"></a>

Signature:

```
function typeof(value)
```

Returns the [Type](18-library-reference.md#type-type) object for a value.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => typeof(3) == Number
true
<2> => typeof("str") == String
true
<3> => typeof(true) == Boolean
true
<4> => typeof([ 1, 2, 3]) == Array
true
<5> => typeof({ a = 2, b = 3 }) == Dictionary
true
```

### get_time <a id="global-functions-get_time"></a>

Signature:

```
function get_time()
```

Returns the current UNIX timestamp as floating point number.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => get_time()
1480072135.633008
<2> => get_time()
1480072140.401207
```

### parse_performance_data <a id="global-functions-parse_performance_data"></a>

Signature:

```
function parse_performance_data(pd)
```

Parses a performance data string and returns an array describing the values.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var pd = "'time'=1480074205.197363;;;"
null
<2> => parse_performance_data(pd)
{
	counter = false
	crit = null
	label = "time"
	max = null
	min = null
	type = "PerfdataValue"
	unit = ""
	value = 1480074205.197363
	warn = null
}
```

### getenv <a id="global-functions-getenv"></a>

Signature:

```
function getenv(key)
```

Returns the value from the specified environment variable key.

Example:

```
$ MY_ENV_VAR=icinga2 icinga2 console
Icinga 2 (version: v2.11.0)
Type $help to view available commands.
<1> => getenv("MY_ENV_VAR")
"icinga2"
```

### dirname <a id="global-functions-dirname"></a>

Signature:

```
function dirname(path)
```

Returns the directory portion of the specified path.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var path = "/etc/icinga2/scripts/xmpp-notification.pl"
null
<2> => dirname(path)
"/etc/icinga2/scripts"
```

### basename <a id="global-functions-basename"></a>

Signature:

```
function basename(path)
```

Returns the filename portion of the specified path.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var path = "/etc/icinga2/scripts/xmpp-notification.pl"
null
<2> => basename(path)
"xmpp-notification.pl"
```

### path\_exists <a id="global-functions-path-exists"></a>

Signature:

```
function path_exists(path)
```

Returns true if the specified path exists, false otherwise.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var path = "/etc/icinga2/scripts/xmpp-notification.pl"
null
<2> => path_exists(path)
true
```

### glob <a id="global-functions-glob"></a>

Signature:

```
function glob(pathSpec, type)
```

Returns an array containing all paths which match the
`pathSpec` argument.

The `type` argument is optional and specifies which types
of paths are matched. This can be a combination of the `GlobFile`
and `GlobDirectory` constants. The default value is `GlobFile | GlobDirectory`.

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var pathSpec = "/etc/icinga2/conf.d/*.conf"
null
<2> => glob(pathSpec)
[ "/etc/icinga2/conf.d/app.conf", "/etc/icinga2/conf.d/commands.conf", ... ]
```

### glob\_recursive <a id="global-functions-glob-recursive"></a>

Signature:

```
function glob_recursive(path, pattern, type)
```

Recursively descends into the specified directory and returns an array containing
all paths which match the `pattern` argument.

The `type` argument is optional and specifies which types
of paths are matched. This can be a combination of the `GlobFile`
and `GlobDirectory` constants. The default value is `GlobFile | GlobDirectory`.

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => var path = "/etc/icinga2/zones.d/"
null
<2> => var pattern = "*.conf"
null
<3> => glob_recursive(path, pattern)
[ "/etc/icinga2/zones.d/global-templates/templates.conf", "/etc/icinga2/zones.d/master/hosts.conf", ... ]
```

### escape_shell_arg <a id="global-functions-escape_shell_arg"></a>

Signature:

```
function escape_shell_arg(text)
```

Escapes a string for use as a single shell argument.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => escape_shell_arg("'$host.name$' '$service.name$'")
"''\\''$host.name$'\\'' '\\''$service.name$'\\'''"
```

### escape_shell_cmd <a id="global-functions-escape_shell_cmd"></a>

Signature:

```
function escape_shell_cmd(text)
```

Escapes shell meta characters in a string.

Example:

```
$ icinga2 console
Icinga 2 (version: v2.7.0)
<1> => escape_shell_cmd("/bin/echo 'shell test' $ENV")
"/bin/echo 'shell test' \\$ENV"
```

### escape_create_process_arg <a id="global-functions-escape_create_process_arg"></a>

Signature:

```
function escape_create_process_arg(text)
```

Escapes a string for use as an argument for CreateProcess(). Windows only.

### sleep <a id="global-functions-sleep"></a>

Signature:

```
function sleep(interval)
```

Sleeps for the specified amount of time (in seconds).


## Scoped Functions <a id="scoped-functions"></a>

This chapter describes functions which are only available
in a specific scope.

### macro <a id="scoped-functions-macro"></a>

Signature:

```
function macro("$macro_name$")
```

The `macro` function can be used to resolve [runtime macro](03-monitoring-basics.md#runtime-macros)
strings into their values.
The returned value depends on the attribute value which is resolved
from the specified runtime macro.

This function is only available in runtime evaluated functions, e.g.
for [custom attributes](03-monitoring-basics.md#custom-attributes-functions) which
use the [abbreviated lambda syntax](17-language-reference.md#nullary-lambdas).

This example sets the `snmp_address` custom attribute
based on `$address$` and `$address6$`.

```
  vars.snmp_address = {{
    var addr_v4 = macro("$address$")
    var addr_v6 = macro("$address6$")

    if (addr_v4) {
    	return addr_v4
    } else {
    	return "udp6:[" + addr_v6 + "]"
    }
  }}
```

More reference examples are available inside the [Icinga Template Library](10-icinga-template-library.md#icinga-template-library)
and the [object accessors chapter](08-advanced-topics.md#access-object-attributes-at-runtime).

## Object Accessor Functions <a id="object-accessor-functions"></a>

These functions can be used to retrieve a reference to another object by name.

### get_check_command <a id="objref-get_check_command"></a>

Signature:

```
function get_check_command(name);
```

Returns the CheckCommand object with the specified name, or `null` if no such CheckCommand object exists.

### get_event_command <a id="objref-get_event_command"></a>

Signature:

```
function get_event_command(name);
```

Returns the EventCommand object with the specified name, or `null` if no such EventCommand object exists.

### get_notification_command <a id="objref-get_notification_command"></a>

Signature:

```
function get_notification_command(name);
```

Returns the NotificationCommand object with the specified name, or `null` if no such NotificationCommand object exists.

### get_host <a id="objref-get_host"></a>

Signature:

```
function get_host(host_name);
```

Returns the Host object with the specified name, or `null` if no such Host object exists.


### get_service <a id="objref-get_service"></a>

Signature:

```
function get_service(host_name, service_name);
function get_service(host, service_name);
```

Returns the Service object with the specified host name or object and service name pair,
or `null` if no such Service object exists.

Example in the [debug console](11-cli-commands.md#cli-command-console)
which fetches the `disk` service object from the current Icinga 2 node:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/'
Icinga 2 (version: v2.7.0)

<1> => get_service(NodeName, "disk")
<2> => get_service(NodeName, "disk").__name
"icinga2-master1.localdomain!disk"

<3> => get_service(get_host(NodeName), "disk").__name
"icinga2-master1.localdomain!disk"
```

### get_services <a id="objref-get_services"></a>

Signature:

```
function get_services(host_name);
function get_services(host);
```

Returns an [array](17-language-reference.md#array) of service objects for the specified host name or object,
or `null` if no such host object exists.

Example in the [debug console](11-cli-commands.md#cli-command-console)
which fetches all service objects from the current Icinga 2 node:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/'
Icinga 2 (version: v2.7.0)

<1> => get_services(NodeName).map(s => s.name)
[ "disk", "disk /", "http", "icinga", "load", "ping4", "ping6", "procs", "ssh", "users" ]
```

Note: [map](18-library-reference.md#array-map) takes a [lambda function](17-language-reference.md#lambdas) as argument. In this example
we only want to collect and print the `name` attribute with `s => s.name`.

This works in a similar fashion for a host object where you can extract all service states
in using the [map](18-library-reference.md#array-map) functionality:

```
<2> => get_services(get_host(NodeName)).map(s => s.state)
[ 2.000000, 2.000000, 2.000000, 0.000000, 0.000000, 0.000000, 2.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000 ]
```

### get_user <a id="objref-get_user"></a>

Signature:

```
function get_user(name);
```

Returns the User object with the specified name, or `null` if no such User object exists.

### get_host_group <a id="objref-get_host_group"></a>

Signature:

```
function get_host_group(name);
```

Returns the HostGroup object with the specified name, or `null` if no such HostGroup object exists.


### get_service_group <a id="objref-get_service_group"></a>

Signature:

```
function get_service_group(name);
```

Returns the ServiceGroup object with the specified name, or `null` if no such ServiceGroup object exists.

### get_user_group <a id="objref-get_user_group"></a>

Signature:

```
function get_user_group(name);
```

Returns the UserGroup object with the specified name, or `null` if no such UserGroup object exists.


### get_time_period <a id="objref-get_time_period"></a>

Signature:

```
function get_time_period(name);
```

Returns the TimePeriod object with the specified name, or `null` if no such TimePeriod object exists.


### get_object <a id="objref-get_object"></a>

Signature:

```
function get_object(type, name);
```

Returns the object with the specified type and name, or `null` if no such object exists. `type` must refer
to a type object.


### get_objects <a id="objref-get_objects"></a>

Signature:

```
function get_objects(type);
```

Returns an array of objects whose type matches the specified type. `type` must refer
to a type object.


## Math object <a id="math-object"></a>

The global `Math` object can be used to access a number of mathematical constants
and functions.

### Math.E <a id="math-e"></a>

Euler's constant.

### Math.LN2 <a id="math-ln2"></a>

Natural logarithm of 2.

### Math.LN10 <a id="math-ln10"></a>

Natural logarithm of 10.

### Math.LOG2E <a id="math-log2e"></a>

Base 2 logarithm of E.

### Math.PI <a id="math-pi"></a>

The mathematical constant Pi.

### Math.SQRT1_2 <a id="math-sqrt1_2"></a>

Square root of 1/2.

### Math.SQRT2 <a id="math-sqrt2"></a>

Square root of 2.

### Math.abs <a id="math-abs"></a>

Signature:

```
function abs(x);
```

Returns the absolute value of `x`.

### Math.acos <a id="math-acos"></a>

Signature:

```
function acos(x);
```

Returns the arccosine of `x`.

### Math.asin <a id="math-asin"></a>

Signature:

```
function asin(x);
```

Returns the arcsine of `x`.

### Math.atan <a id="math-atan"></a>

Signature:

```
function atan(x);
```

Returns the arctangent of `x`.

### Math.atan2 <a id="math-atan2"></a>

Signature:

```
function atan2(y, x);
```
Returns the arctangent of the quotient of `y` and `x`.

### Math.ceil <a id="math-ceil"></a>

Signature:

```
function ceil(x);
```

Returns the smallest integer value not less than `x`.

### Math.cos <a id="math-cos"></a>

Signature:

```
function cos(x);
```

Returns the cosine of `x`.

### Math.exp <a id="math-exp"></a>

Signature:

```
function exp(x);
```

Returns E raised to the `x`th power.

### Math.floor <a id="math-floor"></a>

Signature:

```
function floor(x);
```

Returns the largest integer value not greater than `x`.

### Math.isinf <a id="math-isinf"></a>

Signature:

```
function isinf(x);
```

Returns whether `x` is infinite.

### Math.isnan <a id="math-isnan"></a>

Signature:

```
function isnan(x);
```

Returns whether `x` is NaN (not-a-number).

### Math.log <a id="math-log"></a>

Signature:

```
function log(x);
```

Returns the natural logarithm of `x`.

### Math.max <a id="math-max"></a>

Signature:

```
function max(...);
```

Returns the largest argument. A variable number of arguments can be specified.
If no arguments are given, -Infinity is returned.

### Math.min <a id="math-min"></a>

Signature:

```
function min(...);
```

Returns the smallest argument. A variable number of arguments can be specified.
If no arguments are given, +Infinity is returned.

### Math.pow <a id="math-pow"></a>

Signature:

```
function pow(x, y);
```

Returns `x` raised to the `y`th power.

### Math.random <a id="math-random"></a>

Signature:

```
function random();
```

Returns a pseudo-random number between 0 and 1.

### Math.round <a id="math-round"></a>

Signature:

```
function round(x);
```

Returns `x` rounded to the nearest integer value.

### Math.sign <a id="math-sign"></a>

Signature:

```
function sign(x);
```

Returns -1 if `x` is negative, 1 if `x` is positive
and 0 if `x` is 0.

### Math.sin <a id="math-sin"></a>

Signature:

```
function sin(x);
```

Returns the sine of `x`.

### Math.sqrt <a id="math-sqrt"></a>

Signature:

```
function sqrt(x);
```

Returns the square root of `x`.

### Math.tan <a id="math-tan"></a>

Signature:

```
function tan(x);
```

Returns the tangent of `x`.

## Json object <a id="json-object"></a>

The global `Json` object can be used to encode and decode JSON.

### Json.encode <a id="json-encode"></a>

Signature:

```
function encode(x);
```

Encodes an arbitrary value into JSON.

### Json.decode <a id="json-decode"></a>

Signature:

```
function decode(x);
```

Decodes a JSON string.

## Number type <a id="number-type"></a>

### Number#to_string <a id="number-to_string"></a>

Signature:

```
function to_string();
```

The `to_string` method returns a string representation of the number.

Example:

```
var example = 7
	example.to_string() /* Returns "7" */
```

## Boolean type <a id="boolean-type"></a>

### Boolean#to_string <a id="boolean-to_string"></a>

Signature:

```
function to_string();
```

The `to_string` method returns a string representation of the boolean value.

Example:

```
var example = true
	example.to_string() /* Returns "true" */
```

## String type <a id="string-type"></a>

### String#find <a id="string-find"></a>

Signature:

```
function find(str, start);
```

Returns the zero-based index at which the string `str` was found in the string. If the string
was not found, -1 is returned. `start` specifies the zero-based index at which `find` should
start looking for the string (defaults to 0 when not specified).

Example:

```
"Hello World".find("World") /* Returns 6 */
```

### String#contains <a id="string-contains"></a>

Signature:

```
function contains(str);
```

Returns `true` if the string `str` was found in the string. If the string
was not found, `false` is returned. Use [find](18-library-reference.md#string-find)
for getting the index instead.

Example:

```
"Hello World".contains("World") /* Returns true */
```

### String#len <a id="string-len"></a>

Signature

```
function len();
```

Returns the length of the string in bytes. Note that depending on the encoding type of the string
this is not necessarily the number of characters.

Example:

```
"Hello World".len() /* Returns 11 */
```

### String#lower <a id="string-lower"></a>

Signature:

```
function lower();
```

Returns a copy of the string with all of its characters converted to lower-case.

Example:

```
"Hello World".lower() /* Returns "hello world" */
```

### String#upper <a id="string-upper"></a>

Signature:

```
function upper();
```

Returns a copy of the string with all of its characters converted to upper-case.

Example:

```
"Hello World".upper() /* Returns "HELLO WORLD" */
```

### String#replace <a id="string-replace"></a>

Signature:

```
function replace(search, replacement);
```

Returns a copy of the string with all occurences of the string specified in `search` replaced
with the string specified in `replacement`.

### String#split <a id="string-split"></a>

Signature:

```
function split(delimiters);
```

Splits a string into individual parts and returns them as an array. The `delimiters` argument
specifies the characters which should be used as delimiters between parts.

Example:

```
"x-7,y".split("-,") /* Returns [ "x", "7", "y" ] */
```

### String#substr <a id="string-substr"></a>

Signature:

```
function substr(start, len);
```

Returns a part of a string. The `start` argument specifies the zero-based index at which the part begins.
The optional `len` argument specifies the length of the part ("until the end of the string" if omitted).

Example:

```
"Hello World".substr(6) /* Returns "World" */
```

### String#to_string <a id="string-to_string"></a>

Signature:

```
function to_string();
```

Returns a copy of the string.

### String#reverse <a id="string-reverse"></a>

Signature:

```
function reverse();
```

Returns a copy of the string in reverse order.

### String#trim <a id="string-trim"></a>

Signature:

```
function trim();
```

Removes trailing whitespaces and returns the string.

## Object type <a id="object-type"></a>

This is the base type for all types in the Icinga application.

### Object#clone <a id="object-clone"></a>

Signature:

```
 function clone();
```

Returns a copy of the object. Note that for object elements which are
reference values (e.g. objects such as arrays or dictionaries) the entire
object is recursively copied.

### Object#to_string <a id="object-to-string"></a>

Signature:

```
function to_string();
```

Returns a string representation for the object. Unless overridden this returns a string
of the format "Object of type '<typename>'" where <typename> is the name of the
object's type.

Example:

```
[ 3, true ].to_string() /* Returns "[ 3.000000, true ]" */
```

### Object#type <a id="object-type-field"></a>

Signature:

String type;

Returns the object's type name. This attribute is read-only.

Example:

```
get_host("localhost").type /* Returns "Host" */
```

## Type type <a id="type-type"></a>

Inherits methods from the [Object type](18-library-reference.md#object-type).

The `Type` type provides information about the underlying type of an object or scalar value.

All types are registered as global variables. For example, in order to obtain a reference to the `String` type the global variable `String` can be used.

### Type#base <a id="type-base"></a>

Signature:

```
Type base;
```

Returns a reference to the type's base type. This attribute is read-only.

Example:

```
Dictionary.base == Object /* Returns true, because the Dictionary type inherits directly from the Object type. */
```

### Type#name <a id="type-name"></a>

Signature:

```
String name;
```

Returns the name of the type.

### Type#prototype <a id="type-prototype"></a>

Signature:

```
Object prototype;
```

Returns the prototype object for the type. When an attribute is accessed on an object that doesn't exist the prototype object is checked to see if an attribute with the requested name exists. If it does, the attribute's value is returned.

The prototype functionality is used to implement methods.

Example:

```
3.to_string() /* Even though '3' does not have a to_string property the Number type's prototype object does. */
```

## Array type <a id="array-type"></a>

Inherits methods from the [Object type](18-library-reference.md#object-type).

### Array#add <a id="array-add"></a>

Signature:

```
function add(value);
```

Adds a new value after the last element in the array.

### Array#clear <a id="array-clear"></a>

Signature:

```
function clear();
```

Removes all elements from the array.

### Array#shallow_clone <a id="array-shallow-clone"></a>

```
function shallow_clone();
```

Returns a copy of the array. Note that for elements which are reference values (e.g. objects such
as arrays and dictionaries) only the references are copied.

### Array#contains <a id="array-contains"></a>

Signature:

```
function contains(value);
```

Returns true if the array contains the specified value, false otherwise.

### Array#freeze <a id="array-freeze"></a>

Signature:

```
function freeze()
```

Disallows further modifications to this array. Trying to modify the array will result in an exception.

### Array#len <a id="array-len"></a>

Signature:

```
function len();
```

Returns the number of elements contained in the array.

### Array#remove <a id="array-remove"></a>

Signature:

```
function remove(index);
```

Removes the element at the specified zero-based index.

### Array#set <a id="array-set"></a>

Signature:

```
function set(index, value);
```

Sets the element at the zero-based index to the specified value. The `index` must refer to an element
which already exists in the array.

### Array#get <a id="array-get"></a>

Signature:

```
function get(index);
```

Retrieves the element at the specified zero-based index.

### Array#sort <a id="array-sort"></a>

Signature:

```
function sort(less_cmp);
```

Returns a copy of the array where all items are sorted. The items are
compared using the `<` (less-than) operator. A custom comparator function
can be specified with the `less_cmp` argument.

### Array#join <a id="array-join"></a>

Signature:

```
function join(separator);
```

Joins all elements of the array using the specified separator.

### Array#reverse <a id="array-reverse"></a>

Signature:

```
function reverse();
```

Returns a new array with all elements of the current array in reverse order.

### Array#map <a id="array-map"></a>

Signature:

```
function map(func);
```

Calls `func(element)` for each of the elements in the array and returns
a new array containing the return values of these function calls.

### Array#reduce <a id="array-reduce"></a>

Signature:

```
function reduce(func);
```

Reduces the elements of the array into a single value by calling the provided
function `func` as `func(a, b)` repeatedly where `a` and `b` are elements of the array
or results from previous function calls.

### Array#filter <a id="array-filter"></a>

Signature:

```
function filter(func);
```

Returns a copy of the array containing only the elements for which `func(element)`
is true.

### Array#any <a id="array-any"></a>

Signature:

```
function any(func);
```

Returns true if the array contains at least one element for which `func(element)`
is true, false otherwise.

### Array#all <a id="array-all"></a>

Signature:

```
function all(func);
```

Returns true if the array contains only elements for which `func(element)`
is true, false otherwise.

### Array#unique <a id="array-unique"></a>

Signature:

```
function unique();
```

Returns a copy of the array with all duplicate elements removed. The original order
of the array is not preserved.

## Dictionary type <a id="dictionary-type"></a>

Inherits methods from the [Object type](18-library-reference.md#object-type).

### Dictionary#shallow_clone <a id="dictionary-shallow-clone"></a>

Signature:

```
function shallow_clone();
```

Returns a copy of the dictionary. Note that for elements which are reference values (e.g. objects such
as arrays and dictionaries) only the references are copied.

### Dictionary#contains <a id="dictionary-contains"></a>

Signature:

```
function contains(key);
```

Returns true if a dictionary item with the specified `key` exists, false otherwise.

### Dictionary#freeze <a id="dictionary-freeze"></a>

Signature:

```
function freeze()
```

Disallows further modifications to this dictionary. Trying to modify the dictionary will result in an exception.

### Dictionary#len <a id="dictionary-len"></a>

Signature:

```
function len();
```

Returns the number of items contained in the dictionary.

### Dictionary#remove <a id="dictionary-remove"></a>

Signature:

```
function remove(key);
```

Removes the item with the specified `key`. Trying to remove an item which does not exist
is a no-op.

### Dictionary#clear <a id="dictionary-clear"></a>

Signature:

```
function clear();
```

Removes all items from the dictionary.

### Dictionary#set <a id="dictionary-set"></a>

Signature:

```
function set(key, value);
```

Creates or updates an item with the specified `key` and `value`.

### Dictionary#get <a id="dictionary-get"></a>

Signature:

```
function get(key);
```

Retrieves the value for the specified `key`. Returns `null` if they `key` does not exist
in the dictionary.

### Dictionary#keys <a id="dictionary-keys"></a>

Signature:

```
function keys();
```

Returns a list of keys for all items that are currently in the dictionary.

### Dictionary#values <a id="dictionary-values"></a>

Signature:

```
function values();
```

Returns a list of values for all items that are currently in the dictionary.

## Function type <a id="scriptfunction-type"></a>

Inherits methods from the [Object type](18-library-reference.md#object-type).

### Function#call <a id="scriptfunction-call"></a>

Signature:

```
function call(thisArg, ...);
```

Invokes the function using an alternative `this` scope. The `thisArg` argument specifies the `this`
scope for the function. All other arguments are passed directly to the function.

Example:

```
function set_x(val) {
  this.x = val
}
	
dict = {}
	
set_x.call(dict, 7) /* Invokes set_x using `dict` as `this` */
```

### Function#callv <a id="scriptfunction-callv"></a>

Signature:

```
function callv(thisArg, args);
```

Invokes the function using an alternative `this` scope. The `thisArg` argument specifies the `this`
scope for the function. The items in the `args` array are passed to the function as individual arguments.

Example:

```
function set_x(val) {
  this.x = val
}
	
var dict = {}

var args = [ 7 ]

set_x.callv(dict, args) /* Invokes set_x using `dict` as `this` */
```

## DateTime type <a id="datetime-type"></a>

Inherits methods from the [Object type](18-library-reference.md#object-type).

### DateTime constructor <a id="datetime-ctor"></a>

Signature:

```
function DateTime()
function DateTime(unixTimestamp)
function DateTime(year, month, day)
function DateTime(year, month, day, hours, minutes, seconds)
```

Constructs a new DateTime object. When no arguments are specified for the constructor a new
DateTime object representing the current time is created.

Example:

```
var d1 = DateTime() /* current time */
var d2 = DateTime(2016, 5, 21) /* midnight April 21st, 2016 (local time) */
```

### DateTime arithmetic <a id="datetime-arithmetic"></a>

Subtracting two DateTime objects yields the interval between them, in seconds.

Example:

```
var delta = DateTime() - DateTime(2016, 5, 21) /* seconds since midnight April 21st, 2016 */
```

Subtracting a number from a DateTime object yields a new DateTime object that is further in the past:

Example:

```
var dt = DateTime() - 2 * 60 * 60 /* Current time minus 2 hours */
```

Adding a number to a DateTime object yields a new DateTime object that is in the future:

Example:

```
var dt = DateTime() + 24 * 60 60 /* Current time plus 24 hours */
```

### DateTime#format <a id="datetime-format"></a>

Signature:

```
function format(fmt)
```

Returns a string representation for the DateTime object using the specified format string.
The format string may contain format conversion placeholders as specified in strftime(3).

Example:

```
var s = DateTime(2016, 4, 21).format("%A") /* Sets s to "Thursday". */
```

### DateTime#to_string <a id="datetime-tostring"></a>

Signature:

```
function to_string()
```

Returns a string representation for the DateTime object. Uses a suitable default format.

Example:

```
var s = DateTime(2016, 4, 21).to_string() /* Sets s to "2016-04-21 00:00:00 +0200". */
```