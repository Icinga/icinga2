# <a id="library-reference"></a> Library Reference

## <a id="global-functions"></a> Global functions

These functions are globally available in [assign/ignore where expressions](3-monitoring-basics.md#using-apply-expressions),
[functions](17-language-reference.md#functions), [API filters](12-icinga2-api.md#icinga2-api-filters)
and the [Icinga 2 console](11-cli-commands.md#cli-command-console).

You can use the [Icinga 2 console](11-cli-commands.md#cli-command-console)
as a sandbox to test these functions before implementing
them in your scenarios.

### <a id="global-functions-regex"></a> regex

Signature:

    function regex(pattern, text)

Returns true if the regular expression matches the text, false otherwise.
**Tip**: In case you are looking for regular expression tests try [regex101](https://regex101.com).

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => host.vars.os_type = "Linux/Unix"
    null
    <2> => regex("^Linux", host.vars.os_type)
    true
    <3> => regex("^Linux$", host.vars.os_type)
    false

### <a id="global-functions-match"></a> match

Signature:

    function match(pattern, text)

Returns true if the wildcard (`?*`) pattern matches the text, false otherwise.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => host.display_name = "NUE-DB-PROD-586"
    null
    <2> => match("NUE-*", host.display_name)
    true
    <3> => match("*NUE-*", host.display_name)
    true
    <4> => match("NUE-*-DEV-*", host.display_name)
    false

### <a id="global-functions-cidr_match"></a> cidr_match

Signature:

    function cidr_match(pattern, ip)

Returns true if the CIDR pattern matches the IP address, false otherwise.
IPv4 addresses are converted to IPv4-mapped IPv6 addresses before being
matched against the pattern.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => host.address = "192.168.56.101"
    null
    <2> => cidr_match("192.168.56.0/24", host.address)
    true
    <3> => cidr_match("192.168.56.0/26", host.address)
    false

### <a id="global-functions-range"></a> range

Signature:

    function range(end)
    function range(start, end)
    function range(start, end, increment)

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

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => range(5)
    [ 0.000000, 1.000000, 2.000000, 3.000000, 4.000000 ]
    <2> => range(2,4)
    [ 2.000000, 3.000000 ]
    <3> => range(2,10,2)
    [ 2.000000, 4.000000, 6.000000, 8.000000 ]

### <a id="global-functions-len"></a> len

Signature:

    function len(value)

Returns the length of the value, i.e. the number of elements for an array
or dictionary, or the length of the string in bytes.

**Note**: Instead of using this global function you are advised to use the type's
prototype method: [Array#len](18-library-reference.md#array-len), [Dictionary#len](18-library-reference.md#dictionary-len) and
[String#len](18-library-reference.md#string-len).

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
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


### <a id="global-functions-union"></a> union

Signature:

    function union(array, array, ...)

Returns an array containing all unique elements from the specified arrays.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => var dev_notification_groups = [ "devs", "slack" ]
    null
    <2> => var host_notification_groups = [ "slack", "noc" ]
    null
    <3> => union(dev_notification_groups, host_notification_groups)
    [ "devs", "noc", "slack" ]

### <a id="global-functions-intersection"></a> intersection

Signature:

    function intersection(array, array, ...)

Returns an array containing all unique elements which are common to all
specified arrays.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => var dev_notification_groups = [ "devs", "slack" ]
    null
    <2> => var host_notification_groups = [ "slack", "noc" ]
    null
    <3> => intersection(dev_notification_groups, host_notification_groups)
    [ "slack" ]

### <a id="global-functions-keys"></a> keys

Signature:

    function keys(dict)

Returns an array containing the dictionary's keys.

**Note**: Instead of using this global function you are advised to use the type's
prototype method: [Dictionary#keys](18-library-reference.md#dictionary-keys).

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => host.vars.disks["/"] = {}
    null
    <2> => host.vars.disks["/var"] = {}
    null
    <3> => host.vars.disks.keys()
    [ "/", "/var" ]

### <a id="global-functions-string"></a> string

Signature:

    function string(value)

Converts the value to a string.

**Note**: Instead of using this global function you are advised to use the type's
prototype method:

* [Number#to_string](18-library-reference.md#number-to_string)
* [Boolean#to_string](18-library-reference.md#boolean-to_string)
* [String#to_string](18-library-reference.md#string-to_string)
* [Object#to_string](18-library-reference.md#object-to-string) for Array and Dictionary types
* [DateTime#to_string](18-library-reference.md#datetime-tostring)

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
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

### <a id="global-functions-number"></a> number

Signature:

    function number(value)

Converts the value to a number.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => number(false)
    0.000000
    <2> => number("78")
    78.000000

### <a id="global-functions-bool"></a> bool

Signature:

    function bool(value)

Converts the value to a bool.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => bool(1)
    true
    <2> => bool(0)
    false

### <a id="global-functions-random"></a> random

Signature:

    function random()

Returns a random value between 0 and RAND\_MAX (as defined in stdlib.h).

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => random()
    1263171996.000000
    <2> => random()
    108402530.000000

### <a id="global-functions-log"></a> log

Signature:

    function log(value)

Writes a message to the log. Non-string values are converted to a JSON string.

Signature:

    function log(severity, facility, value)

Writes a message to the log. `severity` can be one of `LogDebug`, `LogNotice`,
`LogInformation`, `LogWarning`, and `LogCritical`.

Non-string values are converted to a JSON string.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => log(LogCritical, "Console", "First line")
    critical/Console: First line
    null
    <2> => var groups = [ "devs", "slack" ]
    null
    <3> => log(LogCritical, "Console", groups)
    critical/Console: ["devs","slack"]
    null

### <a id="global-functions-typeof"></a> typeof

Signature:

    function typeof(value)

Returns the [Type](18-library-reference.md#type-type) object for a value.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => typeof(3) == Number
    true
    <2> => typeof("str") == String
    true
    <3> => typeof(true) == Boolean
    true
    <4> => typeof([ 1, 2, 3]) == Array
    true
    <5> => typeof({ a = 2, b = 3}) == Dictionary

### <a id="global-functions-get_time"></a> get_time

Signature:

    function get_time()

Returns the current UNIX timestamp as floating point number.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => get_time()
    1480072135.633008
    <2> => get_time()
    1480072140.401207

### <a id="global-functions-parse_performance_data"></a> parse_performance_data

Signature:

    function parse_performance_data(pd)

Parses a performance data string and returns an array describing the values.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
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

### <a id="global-functions-dirname"></a> dirname

Signature:

    function dirname(path)

Returns the directory portion of the specified path.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => var path = "/etc/icinga2/scripts/xmpp-notification.pl"
    null
    <2> => dirname(path)
    "/etc/icinga2/scripts"

### <a id="global-functions-basename"></a> basename

Signature:

    function basename(path)

Returns the filename portion of the specified path.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => var path = "/etc/icinga2/scripts/xmpp-notification.pl"
    null
    <2> => basename(path)
    "xmpp-notification.pl"

### <a id="global-functions-escape_shell_arg"></a> escape_shell_arg

Signature:

    function escape_shell_arg(text)

Escapes a string for use as a single shell argument.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => escape_shell_arg("'$host.name$' '$service.name$'")
    "''\\''$host.name$'\\'' '\\''$service.name$'\\'''"

### <a id="global-functions-escape_shell_cmd"></a> escape_shell_cmd

Signature:

    function escape_shell_cmd(text)

Escapes shell meta characters in a string.

Example:

    $ icinga2 console
    Icinga 2 (version: v2.6.0)
    <1> => escape_shell_cmd("/bin/echo 'shell test' $ENV")
    "/bin/echo 'shell test' \\$ENV"

### <a id="global-functions-escape_create_process_arg"></a> escape_create_process_arg

Signature:

    function escape_create_process_arg(text)

Escapes a string for use as an argument for CreateProcess(). Windows only.

### <a id="global-functions-sleep"></a> sleep

Signature:

    function sleep(interval)

Sleeps for the specified amount of time (in seconds).

## <a id="object-accessor-functions"></a> Object Accessor Functions

These functions can be used to retrieve a reference to another object by name.

### <a id="objref-get_check_command"></a> get_check_command

Signature:

    function get_check_command(name);

Returns the CheckCommand object with the specified name, or `null` if no such CheckCommand object exists.

### <a id="objref-get_event_command"></a> get_event_command

Signature:

    function get_event_command(name);

Returns the EventCommand object with the specified name, or `null` if no such EventCommand object exists.

### <a id="objref-get_notification_command"></a> get_notification_command

Signature:

    function get_notification_command(name);

Returns the NotificationCommand object with the specified name, or `null` if no such NotificationCommand object exists.

### <a id="objref-get_host"></a> get_host

Signature:

    function get_host(host_name);

Returns the Host object with the specified name, or `null` if no such Host object exists.


### <a id="objref-get_service"></a> get_service

Signature:

    function get_service(host_name, service_name);

Returns the Service object with the specified name, or `null` if no such Service object exists.


### <a id="objref-get_user"></a> get_user

Signature:

    function get_user(name);

Returns the User object with the specified name, or `null` if no such User object exists.

### <a id="objref-get_host_group"></a> get_host_group

Signature:

    function get_host_group(name);

Returns the HostGroup object with the specified name, or `null` if no such HostGroup object exists.


### <a id="objref-get_service_group"></a> get_service_group

Signature:

    function get_service_group(name);

Returns the ServiceGroup object with the specified name, or `null` if no such ServiceGroup object exists.

### <a id="objref-get_user_group"></a> get_user_group

Signature:

    function get_user_group(name);

Returns the UserGroup object with the specified name, or `null` if no such UserGroup object exists.


### <a id="objref-get_time_period"></a> get_time_period

Signature:

    function get_time_period(name);

Returns the TimePeriod object with the specified name, or `null` if no such TimePeriod object exists.


### <a id="objref-get_object"></a> get_object

Signature:

    function get_object(type, name);

Returns the object with the specified type and name, or `null` if no such object exists. `type` must refer
to a type object.


### <a id="objref-get_objects"></a> get_objects

Signature:

    function get_objects(type);

Returns an array of objects whose type matches the specified type. `type` must refer
to a type object.


## <a id="math-object"></a> Math object

The global `Math` object can be used to access a number of mathematical constants
and functions.

### <a id="math-e"></a> Math.E

Euler's constant.

### <a id="math-ln2"></a> Math.LN2

Natural logarithm of 2.

### <a id="math-ln10"></a> Math.LN10

Natural logarithm of 10.

### <a id="math-log2e"></a> Math.LOG2E

Base 2 logarithm of E.

### <a id="math-pi"></a> Math.PI

The mathematical constant Pi.

### <a id="math-sqrt1_2"></a> Math.SQRT1_2

Square root of 1/2.

### <a id="math-sqrt2"></a> Math.SQRT2

Square root of 2.

### <a id="math-abs"></a> Math.abs

Signature:

    function abs(x);

Returns the absolute value of `x`.

### <a id="math-acos"></a> Math.acos

Signature:

    function acos(x);

Returns the arccosine of `x`.

### <a id="math-asin"></a> Math.asin

Signature:

    function asin(x);

Returns the arcsine of `x`.

### <a id="math-atan"></a> Math.atan

Signature:

    function atan(x);

Returns the arctangent of `x`.

### <a id="math-atan2"></a> Math.atan2

Signature:

    function atan2(y, x);

Returns the arctangent of the quotient of `y` and `x`.

### <a id="math-ceil"></a> Math.ceil

Signature:

    function ceil(x);

Returns the smallest integer value not less than `x`.

### <a id="math-cos"></a> Math.cos

Signature:

    function cos(x);

Returns the cosine of `x`.

### <a id="math-exp"></a> Math.exp

Signature:

    function exp(x);

Returns E raised to the `x`th power.

### <a id="math-floor"></a> Math.floor

Signature:

    function floor(x);

Returns the largest integer value not greater than `x`.

### <a id="math-isinf"></a> Math.isinf

Signature:

    function isinf(x);

Returns whether `x` is infinite.

### <a id="math-isnan"></a> Math.isnan

Signature:

    function isnan(x);

Returns whether `x` is NaN (not-a-number).

### <a id="math-log"></a> Math.log

Signature:

    function log(x);

Returns the natural logarithm of `x`.

### <a id="math-max"></a> Math.max

Signature:

    function max(...);

Returns the largest argument. A variable number of arguments can be specified.
If no arguments are given, -Infinity is returned.

### <a id="math-min"></a> Math.min

Signature:

    function min(...);

Returns the smallest argument. A variable number of arguments can be specified.
If no arguments are given, +Infinity is returned.

### <a id="math-pow"></a> Math.pow

Signature:

    function pow(x, y);

Returns `x` raised to the `y`th power.

### <a id="math-random"></a> Math.random

Signature:

    function random();

Returns a pseudo-random number between 0 and 1.

### <a id="math-round"></a> Math.round

Signature:

    function round(x);

Returns `x` rounded to the nearest integer value.

### <a id="math-sign"></a> Math.sign

Signature:

    function sign(x);

Returns -1 if `x` is negative, 1 if `x` is positive
and 0 if `x` is 0.

### <a id="math-sin"></a> Math.sin

Signature:

    function sin(x);

Returns the sine of `x`.

### <a id="math-sqrt"></a> Math.sqrt

Signature:

    function sqrt(x);

Returns the square root of `x`.

### <a id="math-tan"></a> Math.tan

Signature:

    function tan(x);

Returns the tangent of `x`.

## <a id="json-object"></a> Json object

The global `Json` object can be used to encode and decode JSON.

### <a id="json-encode"></a> Json.encode

Signature:

    function encode(x);

Encodes an arbitrary value into JSON.

### <a id="json-decode"></a> Json.decode

Signature:

    function decode(x);

Decodes a JSON string.

## <a id="number-type"></a> Number type

### <a id="number-to_string"></a> Number#to_string

Signature:

    function to_string();

The `to_string` method returns a string representation of the number.

Example:

    var example = 7
	example.to_string() /* Returns "7" */

## <a id="boolean-type"></a> Boolean type

### <a id="boolean-to_string"></a> Boolean#to_string

Signature:

    function to_string();

The `to_string` method returns a string representation of the boolean value.

Example:

    var example = true
	example.to_string() /* Returns "true" */

## <a id="string-type"></a> String type

### <a id="string-find"></a> String#find

Signature:

    function find(str, start);

Returns the zero-based index at which the string `str` was found in the string. If the string
was not found, -1 is returned. `start` specifies the zero-based index at which `find` should
start looking for the string (defaults to 0 when not specified).

Example:

    "Hello World".find("World") /* Returns 6 */

### <a id="string-contains"></a> String#contains

Signature:

    function contains(str);

Returns `true` if the string `str` was found in the string. If the string
was not found, `false` is returned. Use [find](18-library-reference.md#string-find)
for getting the index instead.

Example:

    "Hello World".contains("World") /* Returns true */

### <a id="string-len"></a> String#len

Signature

    function len();

Returns the length of the string in bytes. Note that depending on the encoding type of the string
this is not necessarily the number of characters.

Example:

    "Hello World".len() /* Returns 11 */

### <a id="string-lower"></a> String#lower

Signature:

    function lower();

Returns a copy of the string with all of its characters converted to lower-case.

Example:

    "Hello World".lower() /* Returns "hello world" */

### <a id="string-upper"></a> String#upper

Signature:

    function upper();

Returns a copy of the string with all of its characters converted to upper-case.

Example:

    "Hello World".upper() /* Returns "HELLO WORLD" */

### <a id="string-replace"></a> String#replace

Signature:

    function replace(search, replacement);

Returns a copy of the string with all occurences of the string specified in `search` replaced
with the string specified in `replacement`.

### <a id="string-split"></a> String#split

Signature:

    function split(delimiters);

Splits a string into individual parts and returns them as an array. The `delimiters` argument
specifies the characters which should be used as delimiters between parts.

Example:

    "x-7,y".split("-,") /* Returns [ "x", "7", "y" ] */

### <a id="string-substr"></a> String#substr

Signature:

    function substr(start, len);

Returns a part of a string. The `start` argument specifies the zero-based index at which the part begins.
The optional `len` argument specifies the length of the part ("until the end of the string" if omitted).

Example:

    "Hello World".substr(6) /* Returns "World" */

### <a id="string-to_string"></a> String#to_string

Signature:

    function to_string();

Returns a copy of the string.

### <a id="string-reverse"></a> String#reverse

Signature:

    function reverse();

Returns a copy of the string in reverse order.

### <a id="string-trim"></a> String#trim

Signature:

    function trim();

Removes trailing whitespaces and returns the string.

## <a id="object-type"></a> Object type

This is the base type for all types in the Icinga application.

### <a id="object-clone"></a> Object#clone

Signature:

     function clone();

Returns a copy of the object. Note that for object elements which are
reference values (e.g. objects such as arrays or dictionaries) the entire
object is recursively copied.

### <a id="object-to-string"></a> Object#to_string

Signature:

    function to_string();

Returns a string representation for the object. Unless overridden this returns a string
of the format "Object of type '<typename>'" where <typename> is the name of the
object's type.

Example:

    [ 3, true ].to_string() /* Returns "[ 3.000000, true ]" */

### <a id="object-type-field"></a> Object#type

Signature:

    String type;

Returns the object's type name. This attribute is read-only.

Example:

    get_host("localhost").type /* Returns "Host" */

## <a id="type-type"></a> Type type

Inherits methods from the [Object type](18-library-reference.md#object-type).

The `Type` type provides information about the underlying type of an object or scalar value.

All types are registered as global variables. For example, in order to obtain a reference to the `String` type the global variable `String` can be used.

### <a id="type-base"></a> Type#base

Signature:

    Type base;

Returns a reference to the type's base type. This attribute is read-only.

Example:

    Dictionary.base == Object /* Returns true, because the Dictionary type inherits directly from the Object type. */

### <a id="type-name"></a> Type#name

Signature:

    String name;

Returns the name of the type.

### <a id="type-prototype"></a> Type#prototype

Signature:

    Object prototype;

Returns the prototype object for the type. When an attribute is accessed on an object that doesn't exist the prototype object is checked to see if an attribute with the requested name exists. If it does, the attribute's value is returned.

The prototype functionality is used to implement methods.

Example:

    3.to_string() /* Even though '3' does not have a to_string property the Number type's prototype object does. */

## <a id="array-type"></a> Array type

Inherits methods from the [Object type](18-library-reference.md#object-type).

### <a id="array-add"></a> Array#add

Signature:

    function add(value);

Adds a new value after the last element in the array.

### <a id="array-clear"></a> Array#clear

Signature:

    function clear();

Removes all elements from the array.

### <a id="array-shallow-clone"></a> Array#shallow_clone

    function shallow_clone();

Returns a copy of the array. Note that for elements which are reference values (e.g. objects such
as arrays and dictionaries) only the references are copied.

### <a id="array-contains"></a> Array#contains

Signature:

    function contains(value);

Returns true if the array contains the specified value, false otherwise.

### <a id="array-len"></a> Array#len

Signature:

    function len();

Returns the number of elements contained in the array.

### <a id="array-remove"></a> Array#remove

Signature:

    function remove(index);

Removes the element at the specified zero-based index.

### <a id="array-set"></a> Array#set

Signature:

    function set(index, value);

Sets the element at the zero-based index to the specified value. The `index` must refer to an element
which already exists in the array.

### <a id="array-get"></a> Array#get

Signature:

    function get(index);

Retrieves the element at the specified zero-based index.

### <a id="array-sort"></a> Array#sort

Signature:

    function sort(less_cmp);

Returns a copy of the array where all items are sorted. The items are
compared using the `<` (less-than) operator. A custom comparator function
can be specified with the `less_cmp` argument.

### <a id="array-join"></a> Array#join

Signature:

    function join(separator);

Joins all elements of the array using the specified separator.

### <a id="array-reverse"></a> Array#reverse

Signature:

    function reverse();

Returns a new array with all elements of the current array in reverse order.

### <a id="array-map"></a> Array#map

Signature:

    function map(func);

Calls `func(element)` for each of the elements in the array and returns
a new array containing the return values of these function calls.

### <a id="array-reduce"></a> Array#reduce

Signature:

    function reduce(func);

Reduces the elements of the array into a single value by calling the provided
function `func` as `func(a, b)` repeatedly where `a` and `b` are elements of the array
or results from previous function calls.

### <a id="array-filter"></a> Array#filter

Signature:

    function filter(func);

Returns a copy of the array containing only the elements for which `func(element)`
is true.

### <a id="array-unique"></a> Array#unique

Signature:

    function unique();

Returns a copy of the array with all duplicate elements removed. The original order
of the array is not preserved.

## <a id="dictionary-type"></a> Dictionary type

Inherits methods from the [Object type](18-library-reference.md#object-type).

### <a id="dictionary-shallow-clone"></a> Dictionary#shallow_clone

Signature:

    function shallow_clone();

Returns a copy of the dictionary. Note that for elements which are reference values (e.g. objects such
as arrays and dictionaries) only the references are copied.

### <a id="dictionary-contains"></a> Dictionary#contains

Signature:

    function contains(key);

Returns true if a dictionary item with the specified `key` exists, false otherwise.

### <a id="dictionary-len"></a> Dictionary#len

Signature:

    function len();

Returns the number of items contained in the dictionary.

### <a id="dictionary-remove"></a> Dictionary#remove

Signature:

    function remove(key);

Removes the item with the specified `key`. Trying to remove an item which does not exist
is a no-op.

### <a id="dictionary-set"></a> Dictionary#set

Signature:

    function set(key, value);

Creates or updates an item with the specified `key` and `value`.

### <a id="dictionary-get"></a> Dictionary#get

Signature:

    function get(key);

Retrieves the value for the specified `key`. Returns `null` if they `key` does not exist
in the dictionary.

### <a id="dictionary-keys"></a> Dictionary#keys

Signature:

    function keys();

Returns a list of keys for all items that are currently in the dictionary.

## <a id="scriptfunction-type"></a> Function type

Inherits methods from the [Object type](18-library-reference.md#object-type).

### <a id="scriptfunction-call"></a> Function#call

Signature:

    function call(thisArg, ...);

Invokes the function using an alternative `this` scope. The `thisArg` argument specifies the `this`
scope for the function. All other arguments are passed directly to the function.

Example:

    function set_x(val) {
	  this.x = val
	}
	
	dict = {}
	
	set_x.call(dict, 7) /* Invokes set_x using `dict` as `this` */

### <a id="scriptfunction-callv"></a> Function#callv

Signature:

    function callv(thisArg, args);

Invokes the function using an alternative `this` scope. The `thisArg` argument specifies the `this`
scope for the function. The items in the `args` array are passed to the function as individual arguments.

Example:

    function set_x(val) {
	  this.x = val
	}
	
	var dict = {}

	var args = [ 7 ]

	set_x.callv(dict, args) /* Invokes set_x using `dict` as `this` */

## <a id="datetime-type"></a> DateTime type

Inherits methods from the [Object type](18-library-reference.md#object-type).

### <a id="datetime-ctor"></a> DateTime constructor

Signature:

    function DateTime()
    function DateTime(unixTimestamp)
    function DateTime(year, month, day)
    function DateTime(year, month, day, hours, minutes, seconds)

Constructs a new DateTime object. When no arguments are specified for the constructor a new
DateTime object representing the current time is created.

Example:

    var d1 = DateTime() /* current time */
    var d2 = DateTime(2016, 5, 21) /* midnight April 21st, 2016 (local time) */

### <a id="datetime-arithmetic"></a> DateTime arithmetic

Subtracting two DateTime objects yields the interval between them, in seconds.

Example:

    var delta = DateTime() - DateTime(2016, 5, 21) /* seconds since midnight April 21st, 2016 */

Subtracting a number from a DateTime object yields a new DateTime object that is further in the past:

Example:

    var dt = DateTime() - 2 * 60 * 60 /* Current time minus 2 hours */

Adding a number to a DateTime object yields a new DateTime object that is in the future:

Example:

    var dt = DateTime() + 24 * 60 60 /* Current time plus 24 hours */

### <a id="datetime-format"></a> DateTime#format

Signature:

    function format(fmt)

Returns a string representation for the DateTime object using the specified format string.
The format string may contain format conversion placeholders as specified in strftime(3).

Example:

    var s = DateTime(2016, 4, 21).format("%A") /* Sets s to "Thursday". */

### <a id="datetime-tostring"></a> DateTime#to_string

Signature:

    function to_string()

Returns a string representation for the DateTime object. Uses a suitable default format.

Example:

    var s = DateTime(2016, 4, 21).to_string() /* Sets s to "2016-04-21 00:00:00 +0200". */
