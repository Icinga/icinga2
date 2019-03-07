# Script Debugger <a id="script-debugger"></a>

You can run the Icinga 2 daemon with the `-X` (`--script-debugger`)
parameter to enable the script debugger:

```
# icinga2 daemon -X
```

When an exception occurs or the [debugger](17-language-reference.md#breakpoints)
keyword is encountered in a user script, Icinga 2 launches a console that
allows the user to debug the script.

You can also attach the script debugger to the [configuration validation](11-cli-commands.md#config-validation):

```
# icinga2 daemon -C -X
```

Here is a list of common errors which can be diagnosed with the script debugger:

* Configuration errors e.g. [apply rules](03-monitoring-basics.md#using-apply)
* Errors in user-defined [functions](17-language-reference.md#functions)

## Debugging Configuration Errors <a id="script-debugger-config-errors"></a>

The following example illustrates the problem of a service [apply rule](03-monitoring-basics.md#using-apply-for)
which expects a dictionary value for `config`, but the host custom attribute only
provides a string value:

```
object Host "script-debugger-host" {
  check_command = "icinga"

  vars.http_vhosts["example.org"] = "192.168.1.100" // a string value
}

apply Service for (http_vhost => config in host.vars.http_vhosts) {
  import "generic-service"

  vars += config // expects a dictionary

  check_command = "http"
}
```

The error message on config validation will warn about the wrong value type,
but does not provide any context which objects are affected.

Enable the script debugger and run the config validation:

```
# icinga2 daemon -C -X

Breakpoint encountered in /etc/icinga2/conf.d/services.conf: 59:67-65:1
Exception: Error: Error while evaluating expression: Cannot convert value of type 'String' to an object.
Location:
/etc/icinga2/conf.d/services.conf(62):   check_command = "http"
/etc/icinga2/conf.d/services.conf(63):
/etc/icinga2/conf.d/services.conf(64):   vars += config
                                         ^^^^^^^^^^^^^^
/etc/icinga2/conf.d/services.conf(65): }
/etc/icinga2/conf.d/services.conf(66):
You can inspect expressions (such as variables) by entering them at the prompt.
To leave the debugger and continue the program use "$continue".
<1> =>
```

You can print the variables `vars` and `config` to get an idea about
their values:

```
<1> => vars
null
<2> => config
"192.168.1.100"
<3> =>
```

The `vars` attribute has to be a dictionary. Trying to set this attribute to a string caused
the error in our configuration example.

In order to determine the name of the host where the value of the `config` variable came from
you can inspect attributes of the service object:

```
<3> => host_name
"script-debugger-host-01"
<4> => name
"http"
```

Additionally you can view the service object attributes by printing the value of `this`.

## Using Breakpoints <a id="script-debugger-breakpoints"></a>

In order to halt execution in a script you can use the `debugger` keyword:

```
object Host "script-debugger-host-02" {
  check_command = "dummy"
  check_interval = 5s

  vars.dummy_text = {{
    var text = "Hello from " + macro("$name$")
    debugger
    return text
  }}
}
```

Icinga 2 will spawn a debugger console every time the function is executed:

```
# icinga2 daemon -X
...
Breakpoint encountered in /etc/icinga2/tests/script-debugger.conf: 7:5-7:12
You can inspect expressions (such as variables) by entering them at the prompt.
To leave the debugger and continue the program use "$continue".
<1> => text
"Hello from script-debugger-host-02"
<2> => $continue
```

## Debugging API Filters <a id="script-debugger-api-filters"></a>

Queries against the [Icinga 2 REST API](12-icinga2-api.md#icinga2-api) can use
filters, just like available in `assign where` expressions. If these filters cause
an internal error, they return an empty result to the caller.

In order to analyse these server-side errors, you can use the script debugger.

The following example tries filter for all host objects where the custom attribute
`os` is set. There are various possibilities to check that, one of them would be
`host.vars.os != ""`. Another idea is to use the [contains](18-library-reference.md#dictionary-contains) method on the custom
attribute dictionary like this: `host.vars.contains("os")`.

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -H 'X-HTTP-Method-Override: GET' \
 -X POST 'https://localhost:5665/v1/objects/services' \
 -d '{ "filter": "host.vars.contains(\"os\")", "attrs": [ "__name" ], "joins": [ "host.name", "host.vars" ], "pretty": true }'
```

This will fail on all hosts which don't have any custom attribute specified.

```
# icinga2 daemon -X

Breakpoint encountered.
Exception: Error: Argument is not a callable object.
Location: in <API query>: 1:0-1:23
You can inspect expressions (such as variables) by entering them at the prompt.
To leave the debugger and continue the program use "$continue".

<1> => this.host

...

    	vars = null

<2> => $continue
```

By definition, a type method can only be invoked on an actual object.

In order to stay safe, add more checks to the API filter:

- `host.vars && host.vars.contains("os")` or
- `host.vars && typeof(host.vars) == Dictionary && host.vars.contains("os")`

Example:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -H 'X-HTTP-Method-Override: GET' \
 -X POST 'https://localhost:5665/v1/objects/services' \
 -d '{ "filter": "host.vars && typeof(host.vars) == Dictionary && host.vars.contains(\"os\")", "attrs": [ "__name" ], "joins": [ "host.name", "host.vars" ], "pretty": true }'
```
