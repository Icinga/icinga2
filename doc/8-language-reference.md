# <a id="language-reference"></a> Language Reference

## <a id="object-definition"></a> Object Definition

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

Alternatively you can write multiple statements on a single line by separating
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

## Expressions

The following expressions can be used on the right-hand side of assignments.

### <a id="numeric-literals"></a> Numeric Literals

A floating-point number.

Example:

    -27.3

### <a id="duration-literals"></a> Duration Literals

Similar to floating-point numbers except for the fact that they support
suffixes to help with specifying time durations.

Example:

    2.5m

Supported suffixes include ms (milliseconds), s (seconds), m (minutes),
h (hours) and d (days).

Duration literals are converted to seconds by the config parser and
are treated like numeric literals.

### <a id="string-literals"></a> String Literals

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

### <a id="multiline-string-literals"></a> Multi-line String Literals

Strings spanning multiple lines can be specified by enclosing them in
{{{ and }}}.

Example:

    {{{This
    is
    a multi-line
    string.}}}

Unlike in ordinary strings special characters do not have to be escaped
in multi-line string literals.

### <a id="boolean-literals"></a> Boolean Literals

The keywords `true` and `false` are used to denote truth values.

### <a id="null-value"></a> Null Value

The `null` keyword can be used to specify an empty value.

### <a id="dictionary"></a> Dictionary

An unordered list of key-value pairs. Keys must be unique and are
compared in a case-sensitive manner.

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

### <a id="array"></a> Array

An ordered list of values.

Individual array elements must be comma-separated.
The comma after the last element is optional.

Example:

    [ "hello", 42 ]

An array may simultaneously contain values of different types, such as
strings and numbers.

### <a id="expression-operators"></a> Operators

The following operators are supported in expressions:

Operator | Examples (Result)                             | Description
---------|-----------------------------------------------|--------------------------------
!        | !"Hello" (false), !false (true)               | Logical negation of the operand
~        | ~true (false)                                 | Bitwise negation of the operand
+        | 1 + 3 (4), "hello " + "world" ("hello world") | Adds two numbers; concatenates strings
-        | 3 - 1 (2)                                     | Subtracts two numbers
*        | 5m * 10 (3000)                                | Multiplies two numbers
/        | 5m / 5 (60)                                   | Divides two numbers
%        | 17 % 12 (5)                                   | Remainder after division
^        | 17 ^ 12 (29)                                  | Bitwise XOR
&        | 7 & 3 (3)                                     | Binary AND
&#124;   | 2 &#124; 3 (3)                                | Binary OR
&&       | true && false (false)                         | Logical AND
&#124;&#124; | true &#124;&#124; false (true)            | Logical OR
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
()       | Math.random()                                 | Calls a function

### <a id="function-calls"></a> Function Calls

Functions can be called using the `()` operator:

    const MyGroups = [ "test1", "test" ]

    {
      check_interval = len(MyGroups) * 1m
    }

> **Tip**
>
> Use these functions in [apply](#using-apply) rule expressions.

    assign where match("192.168.*", host.address)

A list of available functions is available in the [Built-in functions and methods](#builtin-functions) chapter.

## <a id="dictionary-operators"></a> Assignments

In addition to the `=` operator shown above a number of other operators
to manipulate attributes are supported. Here's a list of all
available operators:

### <a id="operator-assignment"></a> Operator =

Sets an attribute to the specified value.

Example:

    {
      a = 5
      a = 7
    }

In this example `a` has the value `7` after both instructions are executed.

### <a id="operator-additive-assignment"></a> Operator +=

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

### <a id="operator-substractive-assignment"></a> Operator -=

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

### <a id="operator-multiply-assignment"></a> Operator \*=

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

### <a id="operator-dividing-assignment"></a> Operator /=

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

## <a id="indexer"></a> Indexer

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

If the `hello` attribute does not already have a value it is automatically initialized to an empty dictionary.

## <a id="template-imports"></a> Template Imports

Objects can import attributes from other objects.

Example:

    template Host "default-host" {
      vars.colour = "red"
    }

    template Host "test-host" {
      import "default-host"

      vars.colour = "blue"
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
custom attributes and the custom attribute `colour` has the value `"blue"`.

Parent objects are resolved in the order they're specified using the
`import` keyword.

## <a id="constants"></a> Constants

Global constants can be set using the `const` keyword:

    const VarName = "some value"

Once defined a constant can be accessed from any file. Constants cannot be changed
once they are set.

Icinga 2 provides a number of special global constants. Some of them can be overridden using the `--define` command line parameter:

Variable            |Description
--------------------|-------------------
PrefixDir           |**Read-only.** Contains the installation prefix that was specified with cmake -DCMAKE_INSTALL_PREFIX. Defaults to "/usr/local".
SysconfDir          |**Read-only.** Contains the path of the sysconf directory. Defaults to PrefixDir + "/etc".
ZonesDir            |**Read-only.** Contains the path of the zones.d directory. Defaults to SysconfDir + "/zones.d".
LocalStateDir       |**Read-only.** Contains the path of the local state directory. Defaults to PrefixDir + "/var".
RunDir              |**Read-only.** Contains the path of the run directory. Defaults to LocalStateDir + "/run".
PkgDataDir          |**Read-only.** Contains the path of the package data directory. Defaults to PrefixDir + "/share/icinga2".
StatePath           |**Read-write.** Contains the path of the Icinga 2 state file. Defaults to LocalStateDir + "/lib/icinga2/icinga2.state".
ObjectsPath         |**Read-write.** Contains the path of the Icinga 2 objects file. Defaults to LocalStateDir + "/cache/icinga2/icinga2.debug".
PidPath             |**Read-write.** Contains the path of the Icinga 2 PID file. Defaults to RunDir + "/icinga2/icinga2.pid".
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
RunAsUser	        |**Read-write.** Defines the user the Icinga 2 daemon is running as. Used in [init.conf](#init-conf).
RunAsGroup	        |**Read-write.** Defines the group the Icinga 2 daemon is running as. Used in [init.conf](#init-conf).

## <a id="apply"></a> Apply

The `apply` keyword can be used to create new objects which are associated with
another group of objects.

    apply Service "ping" to Host {
      import "generic-service"

      check_command = "ping4"

      assign where host.name == "localhost"
    }

In this example the `assign where` condition is a boolean expression which is
evaluated for all objects of type `Host` and a new service with name "ping"
is created for each matching host. [Expression operators](#expression-operators)
may be used in `assign where` conditions.

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

## <a id="group-assign"></a> Group Assign

Group objects can be assigned to specific member objects using the `assign where`
and `ignore where` conditions.

    object HostGroup "linux-servers" {
      display_name = "Linux Servers"

      assign where host.vars.os == "Linux"
    }

In this example the `assign where` condition is a boolean expression which is evaluated
for all objects of the type `Host`. Each matching host is added as member to the host group
with the name "linux-servers". Membership exclusion can be controlled using the `ignore where`
condition. [Expression operators](#expression-operators) may be used in `assign where` and
`ignore where` conditions.

Source Type       | Variables
------------------|--------------
HostGroup         | host
ServiceGroup      | host, service
UserGroup         | user


## <a id="boolean-values"></a> Boolean Values

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

For a list of supported expression operators for `assign where` and `ignore where`
statements, see [expression operators](#expression-operators).

## <a id="comments"></a> Comments

The Icinga 2 configuration format supports C/C++-style and shell-style comments.

Example:

    /*
     This is a comment.
     */
    object Host "localhost" {
      check_interval = 30 // this is also a comment.
      retry_interval = 15 # yet another comment
    }

## <a id="includes"></a> Includes

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
file. By default $PREFIX/share/icinga2/include is included in the list of search
paths. Additional include search paths can be added using
[command-line options](#cmdline).

Wildcards are not permitted when using angle brackets.

## <a id="recursive-includes"></a> Recursive Includes

The `include_recursive` directive can be used to recursively include all
files in a directory which match a certain pattern.

Example:

    include_recursive "conf.d", "*.conf"
    include_recursive "templates"

The first parameter specifies the directory from which files should be
recursively included.

The file names need to match the pattern given in the second parameter.
When no pattern is specified the default pattern "*.conf" is used.

## <a id="library"></a> Library directive

The `library` directive can be used to manually load additional
libraries. Libraries can be used to provide additional object types and
functions.

Example:

    library "snmphelper"

## <a id="functions"></a> Functions

Functions can be defined using the `function` keyword.

Example:

    function multiply(a, b) {
	  return a * b
	}

When encountering the `return` keyword further execution of the function is terminated and
the specified value is supplied to the caller of the function:

    log(multiply(3, 5))

In this example the `multiply` function we declared earlier is invoked with two arguments (3 and 5).
The function computes the product of those arguments and makes the result available to the
function's caller.

When no value is supplied for the `return` statement the function returns `null`.

Functions which do not have a `return` statement have their return value set to the value of the
last expression which was performed by the function. For example, we could have also written our
`multiply` function like this:

    function multiply(a, b) {
	  a * b
	}

Anonymous functions can be created by omitting the name in the function definition. The
resulting function object can be used like any other value:

    var fn = function() { 3 }
	
	fn() /* Returns 3 */

## <a id="lambdas"></a> Lambda Expressions

Functions can also be declared using the alternative lambda syntax.

Example:

    f = (x) => x * x

Multiple statements can be used by putting the function body into braces:

    f = (x) => {
	  log("Lambda called")
	  x * x
    }

Just like with ordinary functions the return value is the value of the last statement.

For lambdas which take exactly one argument the braces around the arguments can be omitted:

    f = x => x * x

## <a id="variable-scopes"></a> Variable Scopes

When setting a variable Icinga checks the following scopes in this order whether the variable
already exists there:

* Local Scope
* `this` Scope
* Global Scope

The local scope contains variables which only exist during the invocation of the current function,
object or apply statement. Local variables can be declared using the `var` keyword:

    function multiply(a, b) {
	  var temp = a * b
	  return temp
	}

Each time the `multiply` function is invoked a new `temp` variable is used which is in no way
related to previous invocations of the function.

When setting a variable which has not previously been declared as local using the `var` keyword
the `this` scope is used.

The `this` scope refers to the current object which the function or object/apply statement
operates on.

    object Host "localhost" {
	  check_interval = 5m
    }

In this example the `this` scope refers to the "localhost" object. The `check_interval` attribute
is set for this particular host.

You can explicitly access the `this` scope using the `this` keyword:

    object Host "localhost" {
	  var check_interval = 5m
  
      /* This explicitly specifies that the attribute should be set
       * for the host, if we had omitted `this.` the (poorly named)
       * local variable `check_interval` would have been modified instead.
       */
      this.check_interval = 1m 
  }

Similarly the keywords `locals` and `globals` are available to access the local and global scope.

Functions also have a `this` scope. However unlike for object/apply statements the `this` scope for
a function is set to whichever object was used to invoke the function. Here's an example:

     hm = {
	   h_word = null
		 
	   function init(word) {
	     h_word = word
	   }
	 }
	 
	 /* Let's invoke the init() function */
	 hm.init("hello")

We're using `hm.init` to invoke the function which causes the value of `hm` to become the `this`
scope for this function call.

## <a id="conditional-statements"></a> Conditional Statements

Sometimes it can be desirable to only evaluate statements when certain conditions are met. The if/else
construct can be used to accomplish this.

Example:

    a = 3

	if (a < 5) {
      a *= 7
	} else {
	  a *= 2
	}

An if/else construct can also be used in place of any other value. The value of an if/else statement
is the value of the last statement which was evaluated for the branch which was taken:

    a = if (true) {
	  log("Taking the 'true' branch")
	  7 * 3
	} else {
	  log("Taking the 'false' branch")
	  9
	}

This example prints the log message "Taking the 'true' branch" and the `a` variable is set to 21 (7 * 3).

The value of an if/else construct is null if the condition evaluates to false and no else branch is given.

## <a id="for-loops"></a> For Loops

The `for` statement can be used to iterate over arrays and dictionaries.

Example:

    var list = [ "a", "b", "c" ]
	
	for (item in list) {
	  log("Item: " + item)
	}

The loop body is evaluated once for each item in the array. The variable `item` is declared as a local
variable just as if the `var` keyword had been used.

Iterating over dictionaries can be accomplished in a similar manner:

    var dict = { a = 3, b = 7 }
	
	for (key => value in dict) {
	  log("Key: " + key + ", Value: " + value)
    }

## <a id="types"></a> Types

All values have a static type. The `typeof` function can be used to determine the type of a value:

    typeof(3) /* Returns an object which represents the type for numbers */

The following built-in types are available:

Type       | Examples          | Description
-----------|-------------------|------------------------
Number     | 3.7               | A numerical value.
Boolean    | true, false       | A boolean value.
String     | "hello"           | A string.
Array      | [ "a", "b" ]      | An array.
Dictionary | { a = 3 }         | A dictionary.

Depending on which libraries are loaded additional types may become available. The `icinga`
library implements a whole bunch of other types, e.g. Host, Service, CheckCommand, etc.

Each type has an associated type object which describes the type's semantics. These
type objects are made available using global variables which match the type's name:

    /* This logs 'true' */
    log(typeof(3) == Number)

The type object's `prototype` property can be used to find out which methods a certain type
supports:

	/* This returns: ["find","len","lower","replace","split","substr","to_string","upper"] */
    keys(String.prototype)

## <a id="reserved-keywords"></a> Reserved Keywords

These keywords are reserved and must not be used as constants or custom attributes.

    object
    template
    include
    include_recursive
    library
    null
    true
    false
    const
    var
    this
    use
    apply
    to
    where
    import
    assign
    ignore
    function
    return
    for
    if
    else
    in

You can escape reserved keywords using the `@` character. The following example
tries to set `vars.include` which references a reserved keyword and generates
an error:

    [2014-09-15 17:24:00 +0200] critical/config: Location:
    /etc/icinga2/conf.d/hosts/localhost.conf(13):   vars.sla = "24x7"
    /etc/icinga2/conf.d/hosts/localhost.conf(14):
    /etc/icinga2/conf.d/hosts/localhost.conf(15):   vars.include = "some cmdb export field"
                                                         ^^^^^^^
    /etc/icinga2/conf.d/hosts/localhost.conf(16): }
    /etc/icinga2/conf.d/hosts/localhost.conf(17):

    Config error: in /etc/icinga2/conf.d/hosts/localhost.conf: 15:8-15:14: syntax error, unexpected include (T_INCLUDE), expecting T_IDENTIFIER
    [2014-09-15 17:24:00 +0200] critical/config: 1 errors, 0 warnings.

You can escape the `include` keyword by prefixing it with an additional `@` character:

    object Host "localhost" {
      import "generic-host"

      address = "127.0.0.1"
      address6 = "::1"

      vars.os = "Linux"
      vars.sla = "24x7"

      vars.@include = "some cmdb export field"
    }

## <a id="builtin-functions"></a> Built-in functions and methods

### <a id="global-functions"></a> Global functions

Function                        | Description
--------------------------------|-----------------------
regex(pattern, text)            | Returns true if the regex pattern matches the text, false otherwise.
match(pattern, text)            | Returns true if the wildcard pattern matches the text, false otherwise.
len(value)                      | Returns the length of the value, i.e. the number of elements for an array or dictionary, or the length of the string in bytes.
union(array, array, ...)        | Returns an array containing all unique elements from the specified arrays.
intersection(array, array, ...) | Returns an array containing all unique elements which are common to all specified arrays.
keys(dict)                      | Returns an array containing the dictionary's keys.
string(value)                   | Converts the value to a string.
number(value)                   | Converts the value to a number.
bool(value)                     | Converts the value to a bool.
random()                        | Returns a random value between 0 and RAND_MAX (as defined in stdlib.h).
log(value)                      | Writes a message to the log. Non-string values are converted to a JSON string.
log(severity, facility, value)  | Writes a message to the log. `severity` can be one of `LogDebug`, `LogNotice`, `LogInformation`, `LogWarning`, and `LogCritical`. Non-string values are converted to a JSON string.
exit(integer)                   | Terminates the application.

### <a id="math-object"></a> Math object

The global `Math` object can be used to access a number of mathematical constants
and functions.

#### <a id="math-e"></a> Math.E

Euler's constant.

#### <a id="math-ln2"></a> Math.LN2

Natural logarithm of 2.

#### <a id="math-ln10"></a> Math.LN10

Natural logarithm of 10.

#### <a id="math-log2e"></a> Math.LOG2E

Base 2 logarithm of E.

#### <a id="math-pi"></a> Math.PI

The mathematical constant Pi.

#### <a id="math-sqrt1_2"></a> Math.SQRT1_2

Square root of 1/2.

#### <a id="math-sqrt2"></a> Math.SQRT2

Square root of 2.

#### <a id="math-abs"></a> Math.abs

Signature:

    function abs(x);

Returns the absolute value of `x`.

#### <a id="math-acos"></a> Math.acos

Signature:

    function acos(x);

Returns the arccosine of `x`.

#### <a id="math-asin"></a> Math.asin

Signature:

    function asin(x);

Returns the arcsine of `x`.

#### <a id="math-atan"></a> Math.atan

Signature:

    function atan(x);

Returns the arctangent of `x`.

#### <a id="math-atan2"></a> Math.atan2

Signature:

    function atan2(y, x);

Returns the arctangent of the quotient of `y` and `x`.

#### <a id="math-ceil"></a> Math.ceil

Signature:

    function ceil(x);

Returns the smallest integer value not less than `x`.

#### <a id="math-cos"></a> Math.cos

Signature:

    function cos(x);

Returns the cosine of `x`.

#### <a id="math-exp"></a> Math.exp

Signature:

    function exp(x);

Returns E raised to the `x`th power.

#### <a id="math-floor"></a> Math.floor

Signature:

    function floor(x);

Returns the largest integer value not greater than `x`.

#### <a id="math-isinf"></a> Math.isinf

Signature:

    function isinf(x);

Returns whether `x` is infinite.

#### <a id="math-isnan"></a> Math.isnan

Signature:

    function isnan(x);

Returns whether `x` is NaN (not-a-number).

#### <a id="math-log"></a> Math.log

Signature:

    function log(x);

Returns the natural logarithm of `x`.

#### <a id="math-max"></a> Math.max

Signature:

    function max(...);

Returns the largest argument. A variable number of arguments can be specified.
If no arguments are given -Infinity is returned.

#### <a id="math-min"></a> Math.min

Signature:

    function min(...);

Returns the smallest argument. A variable number of arguments can be specified.
If no arguments are given +Infinity is returned.

#### <a id="math-pow"></a> Math.pow

Signature:

    function pow(x, y);

Returns `x` raised to the `y`th power.

#### <a id="math-random"></a> Math.random

Signature:

    function random();

Returns a pseudo-random number between 0 and 1.

#### <a id="math-round"></a> Math.round

Signature:

    function round(x);

Returns `x` rounded to the nearest integer value.

#### <a id="math-sign"></a> Math.sign

Signature:

    function sign(x);

Returns -1 if `x` is negative, 1 if `x` is positive
and 0 if `x` is 0.

#### <a id="math-sin"></a> Math.sin

Signature:

    function sin(x);

Returns the sine of `x`.

#### <a id="math-sqrt"></a> Math.sqrt

Signature:

    function sqrt(x);

Returns the square root of `x`.

#### <a id="math-tan"></a> Math.tan

Signature:

    function tan(x);

Returns the tangent of `x`.

### <a id="number-type"></a> Number type

#### <a id="number-to_string"></a> Number#to_string

Signature:

    function to_string();

The `to_string` method returns a string representation of the number.

Example:

    var example = 7
	example.to_string() /* Returns "7" */

### <a id="boolean-type"></a> Boolean type

#### <a id="boolean-to_string"></a> Boolean#to_string

Signature:

    function to_string();

The `to_string` method returns a string representation of the boolean value.

Example:

    var example = true
	example.to_string() /* Returns "true" */

### <a id="string-type"></a> String type

#### <a id="string-find"></a> String#find

Signature:

    function find(str, start);

Returns the zero-based index at which the string `str` was found in the string. If the string
was not found -1 is returned. `start` specifies the zero-based index at which `find` should
start looking for the string (defaults to 0 when not specified).

Example:

    "Hello World".find("World") /* Returns 6 */

#### <a id="string-len"></a> String#len

Signature

    function len();

Returns the length of the string in bytes. Note that depending on the encoding type of the string
this is not necessarily the number of characters.

Example:

    "Hello World".len() /* Returns 11 */

#### <a id="string-lower"></a> String#lower

Signature:

    function lower();

Returns a copy of the string with all of its characters converted to lower-case.

Example:

    "Hello World".lower() /* Returns "hello world" */

#### <a id="string-upper"></a> String#upper

Signature:

    function upper();

Returns a copy of the string with all of its characters converted to upper-case.

Example:

    "Hello World".upper() /* Returns "HELLO WORLD" */

#### <a id="string-replace"></a> String#replace

Signature:

    function replace(search, replacement);

Returns a copy of the string with all occurences of the string specified in `search` replaced
with the string specified in `replacement`.

#### <a id="string-split"></a> String#split

Signature:

    function split(delimiters);

Splits a string into individual parts and returns them as an array. The `delimiters` argument
specifies the characters which should be used as delimiters between parts.

Example:

    "x-7,y".split("-,") /* Returns [ "x", "7", "y" ] */

#### <a id="string-substr"></a> String#substr

Signature:

    function substr(start, len);

Returns a part of a string. The `start` argument specifies the zero-based index at which the part begins.
The optional `len` argument specifies the length of the part ("until the end of the string" if omitted).

Example:

    "Hello World".substr(6) /* Returns "World" */

#### <a id="string-to_string"></a> String#to_string

Signature:

    function to_string();

Returns a copy of the string.

### <a id="array-type"> Array type

#### <a id="array-add"> Array#add
#### <a id="array-clear"> Array#clear
#### <a id="array-clone"> Array#clone
#### <a id="array-contains"> Array#contains
#### <a id="array-len"> Array#len
#### <a id="array-remove"> Array#remove
#### <a id="array-set"> Array#set
#### <a id="array-sort"> Array#sort

Signature:

    function sort(less_cmp);

Returns a copy of the array where all items are sorted. The items are
compared using the `<` (less-than) operator. A custom comparator function
can be specified with the `less_cmp` argument.

### <a id="dictionary-type"> Dictionary type

#### <a id="dictionary-clone"> Dictionary#clone
#### <a id="dictionary-contains"> Dictionary#contains
#### <a id="dictionary-len"> Dictionary#len
#### <a id="dictionary-remove"> Dictionary#remove
#### <a id="dictionary-set"> Dictionary#set

### <a id="scriptfunction-type"> Function type

#### <a id="scriptfunction-call"> Function#call

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

#### <a id="scriptfunction-callv"> Function#callv

Signature:

    function call(thisArg, args);

Invokes the function using an alternative `this` scope. The `thisArg` argument specifies the `this`
scope for the function. The items in the `args` array are passed to the function as individual arguments.

Example:

    function set_x(val) {
	  this.x = val
	}
	
	var dict = {}
	
	var args = [ 7 ]
	
	set_x.callv(dict, args) /* Invokes set_x using `dict` as `this` */

