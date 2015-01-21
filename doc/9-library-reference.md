# <a id="library-reference"></a> Library Reference

## <a id="global-functions"></a> Global functions

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
If no arguments are given -Infinity is returned.

### <a id="math-min"></a> Math.min

Signature:

    function min(...);

Returns the smallest argument. A variable number of arguments can be specified.
If no arguments are given +Infinity is returned.

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
was not found -1 is returned. `start` specifies the zero-based index at which `find` should
start looking for the string (defaults to 0 when not specified).

Example:

    "Hello World".find("World") /* Returns 6 */

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

## <a id="array-type"></a> Array type

### <a id="array-add"></a> Array#add

Signature:

    function add(value);

TODO

### <a id="array-clear"></a> Array#clear

Signature:

    function clear();

TODO

### <a id="array-clone"></a> Array#clone

    function clone();

TODO

### <a id="array-contains"></a> Array#contains

Signature:

    function contains(value);

TODO

### <a id="array-len"></a> Array#len

Signature:

    function len();

TODO

### <a id="array-remove"></a> Array#remove

Signature:

    function remove(index);

TODO

### <a id="array-set"></a> Array#set

Signature:

    function set(index, value);

TODO

### <a id="array-sort"></a> Array#sort

Signature:

    function sort(less_cmp);

Returns a copy of the array where all items are sorted. The items are
compared using the `<` (less-than) operator. A custom comparator function
can be specified with the `less_cmp` argument.

## <a id="dictionary-type"></a> Dictionary type

### <a id="dictionary-clone"></a> Dictionary#clone

Signature:

    function clone();

TODO

### <a id="dictionary-contains"></a> Dictionary#contains

Signature:

    function contains(key);

TODO

### <a id="dictionary-len"></a> Dictionary#len

Signature:

    function len();

TODO

### <a id="dictionary-remove"></a> Dictionary#remove

Signature:

    function remove(key);

TODO

### <a id="dictionary-set"></a> Dictionary#set

Signature:

    function set(key, value);

TODO

## <a id="scriptfunction-type"></a> ScriptFunction type

### <a id="scriptfunction-call"></a> ScriptFunction#call

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

### <a id="scriptfunction-callv"></a> ScriptFunction#callv

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

