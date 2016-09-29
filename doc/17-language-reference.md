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

All objects have at least the following attributes:

Attribute            | Description
---------------------|-----------------------------
name                 | The name of the object. This attribute can be modified in the object definition to override the name specified with the `object` directive.
type                 | The type of the object.

## <a id="expressions"></a> Expressions

The following expressions can be used on the right-hand side of assignments.

### <a id="numeric-literals"></a> Numeric Literals

A floating-point number.

Example:

    27.3

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
key that is not a valid identifier, you can enclose the key in double
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

The following operators are supported in expressions. The operators are by descending precedence.

Operator | Precedence | Examples (Result)                             | Description
---------|------------|-----------------------------------------------|--------------------------------
()       | 1          | (3 + 3) * 5                                   | Groups sub-expressions
()       | 1          | Math.random()                                 | Calls a function
[]       | 1          | a[3]                                          | Array subscript
.        | 1          | a.b                                           | Element access
!        | 2          | !"Hello" (false), !false (true)               | Logical negation of the operand
~        | 2          | ~true (false)                                 | Bitwise negation of the operand
+        | 2          | +3                                            | Unary plus
-        | 2          | -3                                            | Unary minus
*        | 3          | 5m * 10 (3000)                                | Multiplies two numbers
/        | 3          | 5m / 5 (60)                                   | Divides two numbers
%        | 3          | 17 % 12 (5)                                   | Remainder after division
+        | 4          | 1 + 3 (4), "hello " + "world" ("hello world") | Adds two numbers; concatenates strings
-        | 4          | 3 - 1 (2)                                     | Subtracts two numbers
<<       | 5          | 4 << 8 (1024)                                 | Left shift
>>       | 5          | 1024 >> 4 (64)                                | Right shift
<        | 6         | 3 < 5 (true)                                  | Less than
>        | 6         | 3 > 5 (false)                                 | Greater than
<=       | 6         | 3 <= 3 (true)                                 | Less than or equal
>=       | 6         | 3 >= 3 (true)                                 | Greater than or equal
in       | 7          | "foo" in [ "foo", "bar" ] (true)              | Element contained in array
!in      | 7          | "foo" !in [ "bar", "baz" ] (true)             | Element not contained in array
==       | 8         | "hello" == "hello" (true), 3 == 5 (false)     | Equal to
!=       | 8         | "hello" != "world" (true), 3 != 3 (false)     | Not equal to
&        | 9          | 7 & 3 (3)                                     | Binary AND
^        | 10          | 17 ^ 12 (29)                                  | Bitwise XOR
&#124;   | 11          | 2 &#124; 3 (3)                                | Binary OR
&&       | 13         | true && false (false), 3 && 7 (7), 0 && 7 (0) | Logical AND
&#124;&#124; | 14     | true &#124;&#124; false (true), 0 &#124;&#124; 7 (7)| Logical OR
=        | 12         | a = 3                                         | Assignment
=>       | 15         | x => x * x (function with arg x)              | Lambda, for loop

### <a id="function-calls"></a> Function Calls

Functions can be called using the `()` operator:

    const MyGroups = [ "test1", "test" ]

    {
      check_interval = len(MyGroups) * 1m
    }

A list of available functions is available in the [Library Reference](18-library-reference.md#library-reference) chapter.

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

If the `hello` attribute does not already have a value, it is automatically initialized to an empty dictionary.

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

Default templates which are automatically imported into all object definitions
can be specified using the `default` keyword:

    template CheckCommand "plugin-check-command" default {
      // ...
    }

Default templates are imported before any other user-specified statement in an
object definition is evaluated.

If there are multiple default templates the order in which they are imported
is unspecified.

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
EventEngine         |**Read-write.** The name of the socket event engine, can be "poll" or "epoll". The epoll interface is only supported on Linux.
AttachDebugger      |**Read-write.** Whether to attach a debugger when Icinga 2 crashes. Defaults to false.
RunAsUser           |**Read-write.** Defines the user the Icinga 2 daemon is running as. Used in the `init.conf` configuration file.
RunAsGroup          |**Read-write.** Defines the group the Icinga 2 daemon is running as. Used in the `init.conf` configuration file.
PlatformName        |**Read-only.** The name of the operating system, e.g. "Ubuntu".
PlatformVersion     |**Read-only.** The version of the operating system, e.g. "14.04.3 LTS".
PlatformKernel      |**Read-only.** The name of the operating system kernel, e.g. "Linux".
PlatformKernelVersion|**Read-only.** The version of the operating system kernel, e.g. "3.13.0-63-generic".
BuildCompilerName   |**Read-only.** The name of the compiler Icinga was built with, e.g. "Clang".
BuildCompilerVersion|**Read-only.** The version of the compiler Icinga was built with, e.g. "7.3.0.7030031".
BuildHostName       |**Read-only.** The name of the host Icinga was built on, e.g. "acheron".

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
is created for each matching host. [Expression operators](17-language-reference.md#expression-operators)
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
"address" attribute -- or null if that attribute isn't set.

More usage examples are documented in the [monitoring basics](3-monitoring-basics.md#using-apply-expressions)
chapter.

## <a id="apply-for"></a> Apply For

[Apply](17-language-reference.md#apply) rules can be extended with the
[for loop](17-language-reference.md#for-loops) keyword.

    apply Service "prefix-" for (key => value in host.vars.dictionary) to Host {
      import "generic-service"

      check_command = "ping4"
      vars.host_value = value
    }


Any valid config attribute can be accessed using the `host` and `service`
variables. The attribute must be of the Array or Dictionary type. In this example
`host.vars.dictionary` is of the Dictionary type which needs a key-value-pair
as iterator.

In this example all generated service object names consist of `prefix-` and
the value of the `key` iterator. The prefix string can be omitted if not required.

The `key` and `value` variables can be used for object attribute assignment, e.g. for
setting the `check_command` attribute or custom attributes as command parameters.

`apply for` rules are first evaluated against all objects matching the `for loop` list
and afterwards the `assign where` and `ignore where` conditions are evaluated.

It is not necessary to check attributes referenced in the `for loop` expression
for their existance using an additional `assign where` condition.

More usage examples are documented in the [monitoring basics](3-monitoring-basics.md#using-apply-for)
chapter.

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
condition. [Expression operators](17-language-reference.md#expression-operators) may be used in `assign where` and
`ignore where` conditions.

Source Type       | Variables
------------------|--------------
HostGroup         | host
ServiceGroup      | host, service
UserGroup         | user


## <a id="boolean-values"></a> Boolean Values

The `assign where`, `ignore where`, `if` and `while`  statements, the `!` operator as
well as the `bool()` function convert their arguments to a boolean value based on the
following rules:

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
statements, see [expression operators](17-language-reference.md#expression-operators).

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
[command-line options](11-cli-commands.md#config-include-path).

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

## <a id="zone-includes"></a> Zone Includes

The `include_zones` recursively includes all subdirectories for the
given path.

In addition to that it sets the `zone` attribute for all objects created
in these subdirectories to the name of the subdirectory.

Example:

    include_zones "etc", "zones.d", "*.conf"
    include_zones "puppet", "puppet-zones"

The first parameter specifies a tag name for this directive. Each `include_zones`
invocation should use a unique tag name. When copying the zones' configuration
files Icinga uses the tag name as the name for the destination directory in
`/var/lib/icinga2/api/config`.

The second parameter specifies the directory which contains the subdirectories.

The file names need to match the pattern given in the third parameter.
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

## <a id="nullary-lambdas"></a> Abbreviated Lambda Syntax

Lambdas which take no arguments can also be written using the abbreviated lambda syntax.

Example:

    f = {{ 3 }}

This creates a new function which returns the value 3.

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

## <a id="closures"></a> Closures

By default `function`s, `object`s and `apply` rules do not have access to variables declared
outside of their scope (except for global variables).

In order to access variables which are defined in the outer scope the `use` keyword can be used:

    function MakeHelloFunction(name) {
      return function() use(name) {
        log("Hello, " + name)
      }
    }

In this case a new variable `name` is created inside the inner function's scope which has the
value of the `name` function argument.

Alternatively a different value for the inner variable can be specified:

    function MakeHelloFunction(name) {
      return function() use (greeting = "Hello, " + name) {
        log(greeting)
      }
    }

## <a id="conditional-statements"></a> Conditional Statements

Sometimes it can be desirable to only evaluate statements when certain conditions are met. The if/else
construct can be used to accomplish this.

Example:

    a = 3

    if (a < 5) {
      a *= 7
    } else if (a > 10) {
      a *= 5
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

## <a id="while-loops"></a> While Loops

The `while` statement checks a condition and executes the loop body when the condition evaluates to `true`.
This is repeated until the condition is no longer true.

Example:

    var num = 5

    while (num > 5) {
        log("Test")
        num -= 1
    }

The `continue` and `break` keywords can be used to control how the loop is executed: The `continue` keyword
skips over the remaining expressions for the loop body and begins the next loop evaluation. The `break` keyword
breaks out of the loop.

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

The `continue` and `break` keywords can be used to control how the loop is executed: The `continue` keyword
skips over the remaining expressions for the loop body and begins the next loop evaluation. The `break` keyword
breaks out of the loop.

## <a id="constructor"></a> Constructors

In order to create a new value of a specific type constructor calls may be used.

Example:

    var pd = PerfdataValue()
    pd.label = "test"
    pd.value = 10

You can also try to convert an existing value to another type by specifying it as an argument for the constructor call.

Example:

    var s = String(3) /* Sets s to "3". */

## <a id="throw"></a> Exceptions

Built-in commands may throw exceptions to signal errors such as invalid arguments. User scripts can throw exceptions
using the `throw` keyword.

Example:

    throw "An error occurred."

There is currently no way for scripts to catch exceptions.

## <a id="breakpoints"></a> Breakpoints

The `debugger` keyword can be used to insert a breakpoint. It may be used at any place where an assignment would also be a valid expression.

By default breakpoints have no effect unless Icinga is started with the `--script-debugger` command-line option. When the script debugger is enabled Icinga stops execution of the script when it encounters a breakpoint and spawns a console which lets the user inspect the current state of the execution environment.

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
library implements a whole bunch of other [object types](9-object-types.md#object-types),
e.g. Host, Service, CheckCommand, etc.

Each type has an associated type object which describes the type's semantics. These
type objects are made available using global variables which match the type's name:

    /* This logs 'true' */
    log(typeof(3) == Number)

The type object's `prototype` property can be used to find out which methods a certain type
supports:

    /* This returns: ["contains","find","len","lower","replace","reverse","split","substr","to_string","trim","upper"] */
    keys(String.prototype)

Additional documentation on type methods is available in the
[library reference](18-library-reference.md#library-reference).

## <a id="location-information"></a> Location Information

The location of the currently executing script can be obtained using the
`current_filename` and `current_line` keywords.

Example:

    log("Hello from '" + current_filename + "' in line " + current_line)

## <a id="reserved-keywords"></a> Reserved Keywords

These keywords are reserved and must not be used as constants or custom attributes.

    object
    template
    include
    include_recursive
    ignore_on_error
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
    current_filename
    current_line

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

