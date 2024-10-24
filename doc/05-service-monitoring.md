# Service Monitoring <a id="service-monitoring"></a>

The power of Icinga 2 lies in its modularity. There are thousands of
community plugins available next to the standard plugins provided by
the [Monitoring Plugins project](https://www.monitoring-plugins.org).

Start your research on [Icinga Exchange](https://exchange.icinga.com)
and look which services are already [covered](05-service-monitoring.md#service-monitoring-overview).

The [requirements chapter](05-service-monitoring.md#service-monitoring-requirements) guides you
through the plugin setup, tests and their integration with an [existing](05-service-monitoring.md#service-monitoring-plugin-checkcommand)
or [new](05-service-monitoring.md#service-monitoring-plugin-checkcommand-new) CheckCommand object
and host/service objects inside the [Director](05-service-monitoring.md#service-monitoring-plugin-checkcommand-integration-director)
or [Icinga config files](05-service-monitoring.md#service-monitoring-plugin-checkcommand-integration-config-files).
It also adds hints on [modifying](05-service-monitoring.md#service-monitoring-plugin-checkcommand-modify) existing commands.

Plugins follow the [Plugin API specification](05-service-monitoring.md#service-monitoring-plugin-api)
which is enriched with examples and also code examples to get you started with
[your own plugin](05-service-monitoring.md#service-monitoring-plugin-new).



## Requirements <a id="service-monitoring-requirements"></a>

### Plugins <a id="service-monitoring-plugins"></a>

All existing Icinga or Nagios plugins work with Icinga 2. Community
plugins can be found for example on [Icinga Exchange](https://exchange.icinga.com).

The recommended way of setting up these plugins is to copy them
into the `PluginDir` directory.

If you have plugins with many dependencies, consider creating a
custom RPM/DEB package which handles the required libraries and binaries.

Configuration management tools such as Puppet, Ansible, Chef or Saltstack
also help with automatically installing the plugins on different
operating systems. They can also help with installing the required
dependencies, e.g. Python libraries, Perl modules, etc.

### Plugin Setup <a id="service-monitoring-plugins-setup"></a>

Good plugins provide installations and configuration instructions
in their docs and/or README on GitHub.

Sometimes dependencies are not listed, or your distribution differs from the one
described. Try running the plugin after setup and [ensure it works](05-service-monitoring.md#service-monitoring-plugins-it-works).

#### Ensure it works <a id="service-monitoring-plugins-it-works"></a>

Prior to using the check plugin with Icinga 2 you should ensure that it is working properly
by trying to run it on the console using whichever user Icinga 2 is running as:

RHEL/Fedora

```bash
sudo -u icinga /usr/lib64/nagios/plugins/check_mysql_health --help
```

Debian/Ubuntu

```bash
sudo -u nagios /usr/lib/nagios/plugins/check_mysql_health --help
```

Additional libraries may be required for some plugins. Please consult the plugin
documentation and/or the included README file for installation instructions.
Sometimes plugins contain hard-coded paths to other components. Instead of changing
the plugin it might be easier to create a symbolic link to make sure it doesn't get
overwritten during the next update.

Sometimes there are plugins which do not exactly fit your requirements.
In that case you can modify an existing plugin or just write your own.

#### Plugin Dependency Errors <a id="service-monitoring-plugins-setup-dependency-errors"></a>

Plugins can be scripts (Shell, Python, Perl, Ruby, PHP, etc.)
or compiled binaries (C, C++, Go).

These scripts/binaries may require additional libraries
which must be installed on every system they are executed.

> **Tip**
>
> Don't test the plugins on your master instance, instead
> do that on the satellites and clients which execute the
> checks.

There are errors, now what? Typical errors are missing libraries,
binaries or packages.

##### Python Example

Example for a Python plugin which uses the `tinkerforge` module
to query a network service:

```
ImportError: No module named tinkerforge.ip_connection
```

Its [documentation](https://github.com/NETWAYS/check_tinkerforge#installation)
points to installing the `tinkerforge` Python module.

##### Perl Example

Example for a Perl plugin which uses SNMP:

```
Can't locate Net/SNMP.pm in @INC (you may need to install the Net::SNMP module)
```

Prior to installing the Perl module via CPAN, look for a distribution
specific package, e.g. `libnet-snmp-perl` on Debian/Ubuntu or `perl-Net-SNMP`
on RHEL.


#### Optional: Custom Path <a id="service-monitoring-plugins-custom-path"></a>

If you are not using the default `PluginDir` directory, you
can create a custom plugin directory and constant
and reference this in the created CheckCommand objects.

Create a common directory e.g. `/opt/monitoring/plugins`
and install the plugin there.

```bash
mkdir -p /opt/monitoring/plugins
cp check_snmp_int.pl /opt/monitoring/plugins
chmod +x /opt/monitoring/plugins/check_snmp_int.pl
```

Next create a new global constant, e.g. `CustomPluginDir`
in your [constants.conf](04-configuration.md#constants-conf)
configuration file:

```
vim /etc/icinga2/constants.conf

const PluginDir = "/usr/lib/nagios/plugins"
const CustomPluginDir = "/opt/monitoring/plugins"
```

### CheckCommand Definition <a id="service-monitoring-plugin-checkcommand"></a>

Each plugin requires a [CheckCommand](09-object-types.md#objecttype-checkcommand) object in your
configuration which can be used in the [Service](09-object-types.md#objecttype-service) or
[Host](09-object-types.md#objecttype-host) object definition.

Please check if the Icinga 2 package already provides an
[existing CheckCommand definition](10-icinga-template-library.md#icinga-template-library).

If that's the case, thoroughly check the required parameters and integrate the check command
into your host and service objects. Best practice is to run the plugin on the CLI
with the required parameters first.

Example for database size checks with [check_mysql_health](10-icinga-template-library.md#plugin-contrib-command-mysql_health).

```bash
/usr/lib64/nagios/plugins/check_mysql_health --hostname '127.0.0.1' --username root --password icingar0xx --mode sql --name 'select sum(data_length + index_length) / 1024 / 1024 from information_schema.tables where table_schema = '\''icinga'\'';' '--name2' 'db_size' --units 'MB' --warning 4096 --critical 8192
```

The parameter names inside the ITL commands follow the
`<command name>_<parameter name>` schema.

#### Icinga Director Integration <a id="service-monitoring-plugin-checkcommand-integration-director"></a>

Navigate into `Commands > External Commands` and search for `mysql_health`.
Select `mysql_health` and navigate into the `Fields` tab.

In order to access the parameters, the Director requires you to first
define the needed custom data fields:

* `mysql_health_hostname`
* `mysql_health_username` and `mysql_health_password`
* `mysql_health_mode`
* `mysql_health_name`, `mysql_health_name2` and `mysql_health_units`
* `mysql_health_warning` and `mysql_health_critical`

Create a new host template and object where you'll generic
settings like `mysql_health_hostname` (if it differs from the host's
`address` attribute) and `mysql_health_username` and `mysql_health_password`.

Create a new service template for `mysql-health` and set the `mysql_health`
as check command. You can also define a default for `mysql_health_mode`.

Next, create a service apply rule or a new service set which gets assigned
to matching host objects.


#### Icinga Config File Integration <a id="service-monitoring-plugin-checkcommand-integration-config-files"></a>

Create or modify a host object which stores
the generic database defaults and prepares details
for a service apply for rule.

```
object Host "icinga2-master1.localdomain" {
  check_command = "hostalive"
  address = "..."

  // Database listens locally, not external
  vars.mysql_health_hostname = "127.0.0.1"

  // Basic database size checks for Icinga DBs
  vars.databases["icinga"] = {
    mysql_health_warning = 4096 //MB
    mysql_health_critical = 8192 //MB
  }
  vars.databases["icingaweb2"] = {
    mysql_health_warning = 4096 //MB
    mysql_health_critical = 8192 //MB
  }
}
```

The host object prepares the database details and thresholds already
for advanced [apply for](03-monitoring-basics.md#using-apply-for) rules. It also uses
conditions to fetch host specified values, or set default values.

```
apply Service "db-size-" for (db_name => config in host.vars.databases) {
  check_interval = 1m
  retry_interval = 30s

  check_command = "mysql_health"

  if (config.mysql_health_username) {
    vars.mysql_health_username = config.mysql_health_username
  } else {
    vars.mysql_health_username = "root"
  }
  if (config.mysql_health_password) {
    vars.mysql_health_password = config.mysql_health_password
  } else {
    vars.mysql_health_password = "icingar0xx"
  }

  vars.mysql_health_mode = "sql"
  vars.mysql_health_name = "select sum(data_length + index_length) / 1024 / 1024 from information_schema.tables where table_schema = '" + db_name + "';"
  vars.mysql_health_name2 = "db_size"
  vars.mysql_health_units = "MB"

  if (config.mysql_health_warning) {
    vars.mysql_health_warning = config.mysql_health_warning
  }
  if (config.mysql_health_critical) {
    vars.mysql_health_critical = config.mysql_health_critical
  }

  vars += config
}
```

#### New CheckCommand <a id="service-monitoring-plugin-checkcommand-new"></a>

This chapter describes how to add a new CheckCommand object for a plugin.

Please make sure to follow these conventions when adding a new command object definition:

* Use [command arguments](03-monitoring-basics.md#command-arguments) whenever possible. The `command` attribute
must be an array in `[ ... ]` for shell escaping.
* Define a unique `prefix` for the command's specific arguments. Best practice is to follow this schema:

```
<command name>_<parameter name>
```

That way you can safely set them on host/service level and you'll always know which command they control.
* Use command argument default values, e.g. for thresholds.
* Use [advanced conditions](09-object-types.md#objecttype-checkcommand) like `set_if` definitions.

Before starting with the CheckCommand definition, please check
the existing objects available inside the ITL. They follow best
practices and are maintained by developers and our community.

This example picks a new plugin called [check_systemd](https://exchange.icinga.com/joseffriedrich/check_systemd)
uploaded to Icinga Exchange in June 2019.

First, [install](05-service-monitoring.md#service-monitoring-plugins-setup) the plugin and ensure
that [it works](05-service-monitoring.md#service-monitoring-plugins-it-works). Then run it with the
`--help` parameter to see the actual parameters (docs might be outdated).

```
./check_systemd.py --help

usage: check_systemd.py [-h] [-c SECONDS] [-e UNIT | -u UNIT] [-v] [-V]
                        [-w SECONDS]

...

optional arguments:
  -h, --help            show this help message and exit
  -c SECONDS, --critical SECONDS
                        Startup time in seconds to result in critical status.
  -e UNIT, --exclude UNIT
                        Exclude a systemd unit from the checks. This option
                        can be applied multiple times. For example: -e mnt-
                        data.mount -e task.service.
  -u UNIT, --unit UNIT  Name of the systemd unit that is beeing tested.
  -v, --verbose         Increase output verbosity (use up to 3 times).
  -V, --version         show program's version number and exit
  -w SECONDS, --warning SECONDS
                        Startup time in seconds to result in warning status.
```

The argument description is important, based on this you need to create the
command arguments.

> **Tip**
>
> When you are using the Director, you can prepare the commands as files
> e.g. inside the `global-templates` zone. Then run the kickstart wizard
> again to import the commands as external reference.
>
> If you prefer to use the Director GUI/CLI, please apply the steps
> in the `Add Command` form.

Start with the basic plugin call without any parameters.

```
object CheckCommand "systemd" { // Plugin name without 'check_' prefix
  command = [ PluginContribDir + "/check_systemd.py" ] // Use the 'PluginContribDir' constant, see the contributed ITL commands
}
```

Run a config validation to see if that works, `icinga2 daemon -C`

Next, analyse the plugin parameters. Plugins with a good help output show
optional parameters in square brackes. This is the case for all parameters
for this plugin. If there are required parameters, use the `required` key
inside the argument.

The `arguments` attribute is a dictionary which takes the parameters as keys.

```
  arguments = {
    "--unit" = { ... }
  }
```

If there a long parameter names available, prefer them. This increases
readability in both the configuration as well as the executed command line.

The argument value itself is a sub dictionary which has additional keys:

* `value` which references the runtime macro string
* `description` where you copy the plugin parameter help text into
* `required`, `set_if`, etc. for advanced parameters, check the [CheckCommand object](09-object-types.md#objecttype-checkcommand) chapter.

The runtime macro syntax is required to allow value extraction when
the command is executed.

> **Tip**
>
> Inside the Director, store the new command first in order to
> unveil the `Arguments` tab.

Best practice is to use the command name as prefix, in this specific
case e.g. `systemd_unit`.

```
  arguments = {
    "--unit" = {
      value = "$systemd_unit$" // The service parameter would then be defined as 'vars.systemd_unit = "icinga2"'
      description = "Name of the systemd unit that is beeing tested."
    }
    "--warning" = {
      value = "$systemd_warning$"
      description = "Startup time in seconds to result in warning status."
    }
    "--critical" = {
      value = "$systemd_critical$"
      description = "Startup time in seconds to result in critical status."
    }
  }
```

This may take a while -- validate the configuration in between up until
the CheckCommand definition is done.

Then test and integrate it into your monitoring configuration.

Remember: Do it once and right, and never touch the CheckCommand again.
Optional arguments allow different use cases and scenarios.


Once you have created your really good CheckCommand, please consider
sharing it with our community by creating a new PR on [GitHub](https://github.com/Icinga/icinga2/blob/master/CONTRIBUTING.md).
_Please also update the documentation for the ITL._


> **Tip**
>
> Inside the Director, you can render the configuration in the Deployment
> section. Extract the static configuration object and use that as a source
> for sending it upstream.



#### Modify Existing CheckCommand <a id="service-monitoring-plugin-checkcommand-modify"></a>

Sometimes an existing CheckCommand inside the ITL is missing a parameter.
Or you don't need a default parameter value being set.

Instead of copying the entire configuration object, you can import
an object into another new object.

```
object CheckCommand "http-custom" {
  import "http" // Import existing http object

  arguments += { // Use additive assignment to add missing parameters
    "--key" = {
      value = "$http_..." // Keep the parameter name the same as with http
    }
  }

  // Override default parameters
  vars.http_address = "..."
}
```

This CheckCommand can then be referenced in your host/service object
definitions.


### Plugin API <a id="service-monitoring-plugin-api"></a>

Icinga 2 supports the native plugin API specification from the Monitoring Plugins project.
It is defined in the [Monitoring Plugins](https://www.monitoring-plugins.org) guidelines.

The Icinga documentation revamps the specification into our
own guideline enriched with examples and best practices.

#### Output <a id="service-monitoring-plugin-api-output"></a>

The output should be as short and as detailed as possible. The
most common cases include:

- Viewing a problem list in Icinga Web and dashboards
- Getting paged about a problem
- Receiving the alert on the CLI or forwarding it to external (ticket) systems

Examples:

```
<STATUS>: <A short description what happened>

OK: MySQL connection time is fine (0.0002s)
WARNING: MySQL connection time is slow (0.5s > 0.1s threshold)
CRITICAL: MySQL connection time is causing degraded performance (3s > 0.5s threshold)
```

Icinga supports reading multi-line output where Icinga Web
only shows the first line in the listings and everything in the detail view.

Example for an end2end check with many smaller test cases integrated:

```
OK: Online banking works.
Testcase 1: Site reached.
Testcase 2: Attempted login, JS loads.
Testcase 3: Login succeeded.
Testcase 4: View current state works.
Testcase 5: Transactions fine.
```

If the extended output shouldn't be visible in your monitoring, but only for testing,
it is recommended to implement the `--verbose` plugin parameter to allow
developers and users to debug further. Check [here](05-service-monitoring.md#service-monitoring-plugin-api-verbose)
for more implementation tips.

> **Tip**
>
> More debug output also helps when implementing your plugin.
>
> Best practice is to have the plugin parameter and handling implemented first,
> then add it anywhere you want to see more, e.g. from initial database connections
> to actual query results.


#### Status <a id="service-monitoring-plugin-api-status"></a>

Value | Status    | Description
------|-----------|-------------------------------
0     | OK        | The check went fine and everything is considered working.
1     | Warning   | The check is above the given warning threshold, or anything else is suspicious requiring attention before it breaks.
2     | Critical  | The check exceeded the critical threshold, or something really is broken and will harm the production environment.
3     | Unknown   | Invalid parameters, low level resource errors (IO device busy, no fork resources, TCP sockets, etc.) preventing the actual check. Higher level errors such as DNS resolving, TCP connection timeouts should be treated as `Critical` instead. Whenever the plugin reaches its timeout (best practice) it should also terminate with `Unknown`.

Keep in mind that these are service states. Icinga automatically maps
the [host state](03-monitoring-basics.md#check-result-state-mapping) from the returned plugin states.

#### Thresholds <a id="service-monitoring-plugin-api-thresholds"></a>

A plugin calculates specific values and may decide about the exit state on its own.
This is done with thresholds - warning and critical values which are compared with
the actual value. Upon this logic, the exit state is determined.

Imagine the following value and defined thresholds:

```
ptc_value = 57.8

warning = 50
critical = 60
```

Whenever `ptc_value` is higher than warning or critical, it should return
the appropriate [state](05-service-monitoring.md#service-monitoring-plugin-api-status).

The threshold evaluation order also is important:

* Critical thresholds are evaluated first and superseed everything else.
* Warning thresholds are evaluated second
* If no threshold is matched, return the OK state

Avoid using hardcoded threshold values in your plugins, always
add them to the argument parser.

Example for Python:

```python
import argparse
import signal
import sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument("-w", "--warning", help="Warning threshold. Single value or range, e.g. '20:50'.")
    parser.add_argument("-c", "--critical", help="Critical threshold. Single vluae or range, e.g. '25:45'.")

    args = parser.parse_args()
```

Users might call plugins only with the critical threshold parameter,
leaving out the warning parameter. Keep this in mind when evaluating
the thresholds, always check if the parameters have been defined before.

```python
    if args.critical:
        if ptc_value > args.critical:
            print("CRITICAL - ...")
            sys.exit(2) # Critical

    if args.warning:
        if ptc_value > args.warning:
            print("WARNING - ...")
            sys.exit(1) # Warning

    print("OK - ...")
    sys.exit(0) # OK
```

The above is a simplified example for printing the [output](05-service-monitoring.md#service-monitoring-plugin-api-output)
and using the [state](05-service-monitoring.md#service-monitoring-plugin-api-status)
as exit code.

Before diving into the implementation, learn more about required
[performance data metrics](05-service-monitoring.md#service-monitoring-plugin-api-performance-data-metrics)
and more best practices below.

##### Threshold Ranges <a id="service-monitoring-plugin-api-thresholds-ranges"></a>

Threshold ranges can be used to specify an alert window, e.g. whenever a calculated
value is between a lower and higher critical threshold.

The schema for threshold ranges looks as follows. The `@` character in square brackets
is optional.

```
[@]start:end
```

There are a few requirements for ranges:

* `start <= end`. Add a check in your code and let the user know about problematic values.

```
10:20 	# OK

30:10 	# Error
```

* `start:` can be omitted if its value is 0. This is the default handling for single threshold values too.

```
10 	# Every value > 10 and < 0, outside of 0..10
```

* If `end` is omitted, assume end is infinity.

```
10: 	# < 10, outside of 10..∞
```

* In order to specify negative infinity, use the `~` character.

```
~:10	# > 10, outside of -∞..10
```

* Raise alert if value is outside of the defined range.

```
10:20 	# < 10 or > 20, outside of 10..20
```

* Start with `@` to raise an alert if the value is **inside** the defined range, inclusive start/end values.

```
@10:20	# >= 10 and <= 20, inside of 10..20
```

Best practice is to either implement single threshold values, or fully support ranges.
This requires parsing the input parameter values, therefore look for existing libraries
already providing this functionality.

[check_tinkerforge](https://github.com/NETWAYS/check_tinkerforge/blob/master/check_tinkerforge.py)
implements a simple parser to avoid dependencies.


#### Performance Data Metrics <a id="service-monitoring-plugin-api-performance-data-metrics"></a>

Performance data metrics must be appended to the plugin output with a preceding `|` character.
The schema is as follows:

```
<output> | 'label'=value[UOM];[warn];[crit];[min];[max]
```

The label should be encapsulated with single quotes. Avoid spaces or special characters such
as `%` in there, this could lead to problems with metric receivers such as Graphite.

Labels must not include `'` and `=` characters. Keep the label length as short and unique as possible.

Example:

```
'load1'=4.7
```

Values must respect the C/POSIX locale and not implement e.g. German locale for floating point numbers with `,`.
Icinga sets `LC_NUMERIC=C` to enforce this locale on plugin execution.

##### Unit of Measurement (UOM) <a id="service-monitoring-plugin-api-performance-data-metrics-uom"></a>

```
'rta'=12.445000ms 'pl'=0%
```

The UoMs are written as-is into the [core backends](14-features.md#core-backends)
(IDO, API). I.e. 12.445000ms remain 12.445000ms.

In contrast, the [metric backends](14-features.md#metrics)
(Graphite, InfluxDB, etc.) get perfdata (including warn, crit, min, max)
normalized by Icinga. E.g. 12.445000ms become 0.012445 seconds.

Some plugins change the UoM for different sizing, e.g. returning the disk usage in MB and later GB
for the same performance data label. This is to ensure that graphs always look the same.

[Icinga DB](14-features.md#core-backends-icingadb) gets both the as-is and the normalized perfdata.

What metric backends get... | ... from which perfdata UoMs (case-insensitive if possible)
----------------------------|---------------------------------------
bytes (B)                   | B, KB, MB, ..., YB, KiB, MiB, ..., YiB
bits (b)                    | b, kb, mb, ..., yb, kib, mib, ..., yib
packets                     | packets
seconds (s)                 | ns, us, ms, s, m, h, d
percent                     | %
amperes (A)                 | nA, uA, mA, A, kA, MA, GA, ..., YA
ohms (O)                    | nO, uO, mO, O, kO, MO, GO, ..., YO
volts (V)                   | nV, uV, mV, V, kV, MV, GV, ..., YV
watts (W)                   | nW, uW, mW, W, kW, MW, GW, ..., YW
ampere seconds (As)         | nAs, uAs, mAs, As, kAs, MAs, GAs, ..., YAs
ampere seconds              | nAm, uAm, mAm, Am (ampere minutes), kAm, MAm, GAm, ..., YAm
ampere seconds              | nAh, uAh, mAh, Ah (ampere hours), kAh, MAh, GAh, ..., YAh
watt hours                  | nWs, uWs, mWs, Ws (watt seconds), kWs, MWs, GWs, ..., YWs
watt hours                  | nWm, uWm, mWm, Wm (watt minutes), kWm, MWm, GWm, ..., YWm
watt hours (Wh)             | nWh, uWh, mWh, Wh, kWh, MWh, GWh, ..., YWh
lumens                      | lm
decibel-milliwatts          | dBm
grams (g)                   | ng, ug, mg, g, kg, t
degrees Celsius             | C
degrees Fahrenheit          | F
degrees Kelvin              | K
liters (l)                  | ml, l, hl

The UoM "c" represents a continuous counter (e.g. interface traffic counters).

Unknown UoMs are discarted (as if none was given).
A value without any UoM may be an integer or floating point number
for any type (processes, users, etc.).

##### Thresholds and Min/Max <a id="service-monitoring-plugin-api-performance-data-metrics-thresholds-min-max"></a>

Next to the performance data value, warn, crit, min, max can optionally be provided. They must be separated
with the semi-colon `;` character. They share the same UOM with the performance data value.

```
$ check_ping -4 -H icinga.com -c '200,15%' -w '100,5%'

PING OK - Packet loss = 0%, RTA = 12.44 ms|rta=12.445000ms;100.000000;200.000000;0.000000 pl=0%;5;15;0
```

##### Multiple Performance Data Values <a id="service-monitoring-plugin-api-performance-data-metrics-multiple"></a>

Multiple performance data values must be joined with a space character. The below example
is from the [check_load](10-icinga-template-library.md#plugin-check-command-load) plugin.

```
load1=4.680;1.000;2.000;0; load5=0.000;5.000;10.000;0; load15=0.000;10.000;20.000;0;
```

#### Timeout <a id="service-monitoring-plugin-api-timeout"></a>

Icinga has a safety mechanism where it kills processes running for too
long. The timeout can be specified in [CheckCommand objects](09-object-types.md#objecttype-checkcommand)
or on the host/service object.

Best practice is to control the timeout in the plugin itself
and provide a clear message followed by the Unknown state.

Example in Python taken from [check_tinkerforge](https://github.com/NETWAYS/check_tinkerforge/blob/master/check_tinkerforge.py):

```python
import argparse
import signal
import sys

def handle_sigalrm(signum, frame, timeout=None):
    output('Plugin timed out after %d seconds' % timeout, 3)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    # ... add more arguments
    parser.add_argument("-t", "--timeout", help="Timeout in seconds (default 10s)", type=int, default=10)
    args = parser.parse_args()

    signal.signal(signal.SIGALRM, partial(handle_sigalrm, timeout=args.timeout))
    signal.alarm(args.timeout)

    # ... perform the check and generate output/status
```

#### Versions <a id="service-monitoring-plugin-api-versions"></a>

Plugins should provide a version via `-V` or `--version` parameter
which is bumped on releases. This allows to identify problems with
too old or new versions on the community support channels.

Example in Python taken from [check_tinkerforge](https://github.com/NETWAYS/check_tinkerforge/blob/master/check_tinkerforge.py):

```python
import argparse
import signal
import sys

__version__ = '0.9.1'

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('-V', '--version', action='version', version='%(prog)s v' + sys.modules[__name__].__version__)
```

#### Verbose <a id="service-monitoring-plugin-api-verbose"></a>

Plugins should provide a verbose mode with `-v` or `--verbose` in order
to show more detailed log messages. This helps to debug and analyse the
flow and execution steps inside the plugin.

Ensure to add the parameter prior to implementing the check logic into
the plugin.

Example in Python taken from [check_tinkerforge](https://github.com/NETWAYS/check_tinkerforge/blob/master/check_tinkerforge.py):

```python
import argparse
import signal
import sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('-v', '--verbose', action='store_true')

    if args.verbose:
        print("Verbose debug output")
```


### Create a new Plugin <a id="service-monitoring-plugin-new"></a>

Sometimes an existing plugin does not satisfy your requirements. You
can either kindly contact the original author about plans to add changes
and/or create a patch.

If you just want to format the output and state of an existing plugin
it might also be helpful to write a wrapper script. This script
could pass all configured parameters, call the plugin script, parse
its output/exit code and return your specified output/exit code.

On the other hand plugins for specific services and hardware might not yet
exist.

> **Tip**
>
> Watch this presentation from Icinga Camp Berlin to learn more
> about [How to write checks that don't suck](https://www.youtube.com/watch?v=Ey_APqSCoFQ).

Common best practices:

* Choose the programming language wisely
 * Scripting languages (Bash, Python, Perl, Ruby, PHP, etc.) are easier to write and setup but their check execution might take longer (invoking the script interpreter as overhead, etc.).
 * Plugins written in C/C++, Go, etc. improve check execution time but may generate an overhead with installation and packaging.
* Use a modern VCS such as Git for developing the plugin, e.g. share your plugin on GitHub and let it sync to [Icinga Exchange](https://exchange.icinga.com).
* **Look into existing plugins endorsed by community members.**

Implementation hints:

* Add parameters with key-value pairs to your plugin. They should allow long names (e.g. `--host localhost`) and also short parameters (e.g. `-H localhost`)
 * `-h|--help` should print the version and all details about parameters and runtime invocation. Note: Python's ArgParse class provides this OOTB.
 * `--version` should print the plugin [version](05-service-monitoring.md#service-monitoring-plugin-api-versions).
* Add a [verbose/debug output](05-service-monitoring.md#service-monitoring-plugin-api-verbose) functionality for detailed on-demand logging.
* Respect the exit codes required by the [Plugin API](05-service-monitoring.md#service-monitoring-plugin-api).
* Always add [performance data](05-service-monitoring.md#service-monitoring-plugin-api-performance-data-metrics) to your plugin output.
* Allow to specify [warning/critical thresholds](05-service-monitoring.md#service-monitoring-plugin-api-thresholds) as parameters.

Example skeleton:

```
# 1. include optional libraries
# 2. global variables
# 3. helper functions and/or classes
# 4. define timeout condition

if (<timeout_reached>) then
  print "UNKNOWN - Timeout (...) reached | 'time'=30.0
endif

# 5. main method

<execute and fetch data>

if (<threshold_critical_condition>) then
  print "CRITICAL - ... | 'time'=0.1 'myperfdatavalue'=5.0
  exit(2)
else if (<threshold_warning_condition>) then
  print "WARNING - ... | 'time'=0.1 'myperfdatavalue'=3.0
  exit(1)
else
  print "OK - ... | 'time'=0.2 'myperfdatavalue'=1.0
endif
```

There are various plugin libraries available which will help
with plugin execution and output formatting too, for example
[nagiosplugin from Python](https://pypi.python.org/pypi/nagiosplugin/).

> **Note**
>
> Ensure to test your plugin properly with special cases before putting it
> into production!

Once you've finished your plugin please upload/sync it to [Icinga Exchange](https://exchange.icinga.com/new).
Thanks in advance!


## Service Monitoring Overview <a id="service-monitoring-overview"></a>

The following examples should help you to start implementing your own ideas.
There is a variety of plugins available. This collection is not complete --
if you have any updates, please send a documentation patch upstream.

Please visit our [community forum](https://community.icinga.com) which
may provide an answer to your use case already. If not, do not hesitate
to create a new topic.

### General Monitoring <a id="service-monitoring-general"></a>

If the remote service is available (via a network protocol and port),
and if a check plugin is also available, you don't necessarily need a local client.
Instead, choose a plugin and configure its parameters and thresholds. The following examples are included in the [Icinga 2 Template Library](10-icinga-template-library.md#icinga-template-library):

* [ping4](10-icinga-template-library.md#plugin-check-command-ping4), [ping6](10-icinga-template-library.md#plugin-check-command-ping6),
[fping4](10-icinga-template-library.md#plugin-check-command-fping4), [fping6](10-icinga-template-library.md#plugin-check-command-fping6), [hostalive](10-icinga-template-library.md#plugin-check-command-hostalive)
* [tcp](10-icinga-template-library.md#plugin-check-command-tcp), [udp](10-icinga-template-library.md#plugin-check-command-udp), [ssl](10-icinga-template-library.md#plugin-check-command-ssl)
* [ntp_time](10-icinga-template-library.md#plugin-check-command-ntp-time)

### Linux Monitoring <a id="service-monitoring-linux"></a>

* [disk](10-icinga-template-library.md#plugin-check-command-disk)
* [mem](10-icinga-template-library.md#plugin-contrib-command-mem), [swap](10-icinga-template-library.md#plugin-check-command-swap)
* [procs](10-icinga-template-library.md#plugin-check-command-processes)
* [users](10-icinga-template-library.md#plugin-check-command-users)
* [running_kernel](10-icinga-template-library.md#plugin-contrib-command-running_kernel)
* package management: [apt](10-icinga-template-library.md#plugin-check-command-apt), [yum](10-icinga-template-library.md#plugin-contrib-command-yum), etc.
* [ssh](10-icinga-template-library.md#plugin-check-command-ssh)
* performance: [iostat](10-icinga-template-library.md#plugin-contrib-command-iostat), [check_sar_perf](https://github.com/NETWAYS/check-sar-perf)

### Windows Monitoring <a id="service-monitoring-windows"></a>

!!! important

    [Icinga for Windows](https://icinga.com/docs/icinga-for-windows/latest/doc/000-Introduction/)
    is the recommended way to monitor Windows via Icinga 2.
    Even if the plugins it ships out-of-the-box don't already cover your needs, you can
    [create your own](https://icinga.com/docs/icinga-for-windows/latest/doc/900-Developer-Guide/11-Custom-Plugins/).

Other (legacy) solutions include:

* [check_wmi_plus](https://edcint.co.nz/checkwmiplus/)
* [NSClient++](https://www.nsclient.org) (in combination with the Icinga 2 client and either [check_nscp_api](10-icinga-template-library.md#nscp-check-api) or [nscp-local](10-icinga-template-library.md#nscp-plugin-check-commands) check commands)
* [Icinga 2 Windows Plugins](10-icinga-template-library.md#windows-plugins) (disk, load, memory, network, performance counters, ping, procs, service, swap, updates, uptime, users
* vbs and Powershell scripts

### Database Monitoring <a id="service-monitoring-database"></a>

* MySQL/MariaDB: [mysql_health](10-icinga-template-library.md#plugin-contrib-command-mysql_health), [mysql](10-icinga-template-library.md#plugin-check-command-mysql), [mysql_query](10-icinga-template-library.md#plugin-check-command-mysql-query)
* PostgreSQL: [postgres](10-icinga-template-library.md#plugin-contrib-command-postgres)
* Oracle: [oracle_health](10-icinga-template-library.md#plugin-contrib-command-oracle_health)
* MSSQL: [mssql_health](10-icinga-template-library.md#plugin-contrib-command-mssql_health)
* DB2: [db2_health](10-icinga-template-library.md#plugin-contrib-command-db2_health)
* MongoDB: [mongodb](10-icinga-template-library.md#plugin-contrib-command-mongodb)
* Elasticsearch: [elasticsearch](10-icinga-template-library.md#plugin-contrib-command-elasticsearch)
* Redis: [redis](10-icinga-template-library.md#plugin-contrib-command-redis)

### SNMP Monitoring <a id="service-monitoring-snmp"></a>

* [Manubulon plugins](10-icinga-template-library.md#snmp-manubulon-plugin-check-commands) (interface, storage, load, memory, process)
* [snmp](10-icinga-template-library.md#plugin-check-command-snmp), [snmpv3](10-icinga-template-library.md#plugin-check-command-snmpv3)

### Network Monitoring <a id="service-monitoring-network"></a>

* [nwc_health](10-icinga-template-library.md#plugin-contrib-command-nwc_health)
* [interfaces](10-icinga-template-library.md#plugin-contrib-command-interfaces)
* [interfacetable](10-icinga-template-library.md#plugin-contrib-command-interfacetable)
* [iftraffic](10-icinga-template-library.md#plugin-contrib-command-iftraffic), [iftraffic64](10-icinga-template-library.md#plugin-contrib-command-iftraffic64)

### Web Monitoring <a id="service-monitoring-web"></a>

* [http](10-icinga-template-library.md#plugin-check-command-http)
* [ftp](10-icinga-template-library.md#plugin-check-command-ftp)
* [webinject](10-icinga-template-library.md#plugin-contrib-command-webinject)
* [squid](10-icinga-template-library.md#plugin-contrib-command-squid)
* [apache-status](10-icinga-template-library.md#plugin-contrib-command-apache-status)
* [nginx_status](10-icinga-template-library.md#plugin-contrib-command-nginx_status)
* [kdc](10-icinga-template-library.md#plugin-contrib-command-kdc)
* [rbl](10-icinga-template-library.md#plugin-contrib-command-rbl)

* [Icinga Certificate Monitoring](https://icinga.com/products/icinga-certificate-monitoring/)

### Java Monitoring <a id="service-monitoring-java"></a>

* [jmx4perl](10-icinga-template-library.md#plugin-contrib-command-jmx4perl)

### DNS Monitoring <a id="service-monitoring-dns"></a>

* [dns](10-icinga-template-library.md#plugin-check-command-dns)
* [dig](10-icinga-template-library.md#plugin-check-command-dig)
* [dhcp](10-icinga-template-library.md#plugin-check-command-dhcp)

### Backup Monitoring <a id="service-monitoring-backup"></a>

* [check_bareos](https://github.com/widhalmt/check_bareos)

### Log Monitoring <a id="service-monitoring-log"></a>

* [check_logfiles](https://labs.consol.de/nagios/check_logfiles/)
* [check_logstash](https://github.com/NETWAYS/check_logstash)
* [check_graylog2_stream](https://github.com/Graylog2/check-graylog2-stream)

### Virtualization Monitoring <a id="service-monitoring-virtualization"></a>

### VMware Monitoring <a id="service-monitoring-virtualization-vmware"></a>

* [Icinga Module for vSphere](https://icinga.com/products/icinga-module-for-vsphere/)
* [esxi_hardware](10-icinga-template-library.md#plugin-contrib-command-esxi-hardware)
* [VMware](10-icinga-template-library.md#plugin-contrib-vmware)

**Tip**: If you are encountering timeouts using the VMware Perl SDK,
check [this blog entry](https://www.claudiokuenzler.com/blog/650/slow-vmware-perl-sdk-soap-request-error-libwww-version).
Ubuntu 16.04 LTS can have troubles with random entropy in Perl asked [here](https://monitoring-portal.org/t/check-vmware-api-slow-when-run-multiple-times/2868).
In that case, [haveged](https://issihosts.com/haveged/) may help.

### SAP Monitoring <a id="service-monitoring-sap"></a>

* [check_sap_health](https://labs.consol.de/nagios/check_sap_health/index.html)
* [SAP CCMS](https://sourceforge.net/projects/nagios-sap-ccms/)

### Mail Monitoring <a id="service-monitoring-mail"></a>

* [smtp](10-icinga-template-library.md#plugin-check-command-smtp), [ssmtp](10-icinga-template-library.md#plugin-check-command-ssmtp)
* [imap](10-icinga-template-library.md#plugin-check-command-imap), [simap](10-icinga-template-library.md#plugin-check-command-simap)
* [pop](10-icinga-template-library.md#plugin-check-command-pop), [spop](10-icinga-template-library.md#plugin-check-command-spop)
* [mailq](10-icinga-template-library.md#plugin-check-command-mailq)

### Hardware Monitoring <a id="service-monitoring-hardware"></a>

* [hpasm](10-icinga-template-library.md#plugin-contrib-command-hpasm)
* [ipmi-sensor](10-icinga-template-library.md#plugin-contrib-command-ipmi-sensor)

### Metrics Monitoring <a id="service-monitoring-metrics"></a>

* [graphite](10-icinga-template-library.md#plugin-contrib-command-graphite)
