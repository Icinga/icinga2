# Icinga 2 CLI Commands <a id="cli-commands"></a>

Icinga 2 comes with a number of CLI commands which support bash autocompletion.

These CLI commands will allow you to use certain functionality
provided by and around Icinga 2.

Each CLI command provides its own help and usage information, so please
make sure to always run them with the `--help` parameter.

Run `icinga2` without any arguments to get a list of all available global
options.

```
# icinga2
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 <command> [<arguments>]

Supported commands:
  * api setup (setup for API)
  * ca list (lists all certificate signing requests)
  * ca restore (restores a removed certificate request)
  * ca remove (removes an outstanding certificate request)
  * ca sign (signs an outstanding certificate request)
  * console (Icinga debug console)
  * daemon (starts Icinga 2)
  * feature disable (disables specified feature)
  * feature enable (enables specified feature)
  * feature list (lists all available features)
  * node setup (set up node)
  * node wizard (wizard for node setup)
  * object list (lists all objects)
  * pki new-ca (sets up a new CA)
  * pki new-cert (creates a new CSR)
  * pki request (requests a certificate)
  * pki save-cert (saves another Icinga 2 instance's certificate)
  * pki sign-csr (signs a CSR)
  * pki ticket (generates a ticket)
  * pki verify (verify TLS certificates: CN, signed by CA, is CA; Print certificate)
  * variable get (gets a variable)
  * variable list (lists all variables)

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```


## Icinga 2 CLI Bash Autocompletion <a id="cli-commands-autocompletion"></a>

Bash Auto-Completion (pressing `<TAB>`) is provided only for the corresponding context.

While `--config` suggests and auto-completes files and directories on disk,
`feature enable` only suggests disabled features.

RPM and Debian packages install the bash completion files into
`/etc/bash_completion.d/icinga2`.

You need to install the `bash-completion` package if not already installed.

RHEL/CentOS/Fedora:

```bash
yum install bash-completion
```

SUSE:

```bash
zypper install bash-completion
```

Debian/Ubuntu:

```bash
apt-get install bash-completion
```

Ensure that the `bash-completion.d` directory is added to your shell
environment. You can manually source the icinga2 bash-completion file
into your current session and test it:

```bash
source /etc/bash-completion.d/icinga2
```


## Icinga 2 CLI Global Options <a id="cli-commands-global-options"></a>

### Application Type

By default the `icinga2` binary loads the `icinga` library. A different application type
can be specified with the `--app` command-line option.
Note: This is not needed by the average Icinga user, only developers.

### Libraries

Instead of loading libraries using the [`library` config directive](17-language-reference.md#library)
you can also use the `--library` command-line option.
Note: This is not needed by the average Icinga user, only developers.

### Constants

[Global constants](17-language-reference.md#constants) can be set using the `--define` command-line option.

### Config Include Path <a id="config-include-path"></a>

When including files you can specify that the include search path should be
checked. You can do this by putting your configuration file name in angle
brackets like this:

```
include <test.conf>
```

This causes Icinga 2 to search its include path for the configuration file
`test.conf`. By default the installation path for the [Icinga Template Library](10-icinga-template-library.md#icinga-template-library)
is the only search directory.

Using the `--include` command-line option additional search directories can be
added.

## CLI command: Api <a id="cli-command-api"></a>

Provides helper functions to enable and setup the
[Icinga 2 API](12-icinga2-api.md#icinga2-api-setup).

### CLI command: Api Setup <a id="cli-command-api-setup "></a>

```
# icinga2 api setup --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 api setup [<arguments>]

Setup for Icinga 2 API.

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Command options:
  --cn arg                  The certificate's common name

Report bugs at <https://github.com/Icinga/icinga2>
Get support: <https://icinga.com/support/>
Documentation: <https://icinga.com/docs/>
Icinga home page: <https://icinga.com/>
```

## CLI command: Ca <a id="cli-command-ca"></a>

List and manage incoming certificate signing requests. More details
can be found in the [signing methods](06-distributed-monitoring.md#distributed-monitoring-setup-sign-certificates-master)
chapter. This CLI command is available since v2.8.

```
# icinga2 ca --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 <command> [<arguments>]

Supported commands:
  * ca list (lists all certificate signing requests)
  * ca sign (signs an outstanding certificate request)
  * ca restore (restores a removed certificate request)
  * ca remove (removes an outstanding certificate request)

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```


### CLI command: Ca List <a id="cli-command-ca-list"></a>

```
icinga2 ca list --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 ca list [<arguments>]

Lists pending certificate signing requests.

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Command options:
  --all                     List all certificate signing requests, including
                            signed. Note: Old requests are automatically
                            cleaned by Icinga after 1 week.
  --removed                 List all removed CSRs (for use with 'ca restore')
  --json                    encode output as JSON

Report bugs at <https://github.com/Icinga/icinga2>
Get support: <https://icinga.com/support/>
Documentation: <https://icinga.com/docs/>
Icinga home page: <https://icinga.com/>
```

## CLI command: Console <a id="cli-command-console"></a>

The CLI command `console` can be used to debug and evaluate Icinga 2 config expressions,
e.g. to test [functions](17-language-reference.md#functions) in your local sandbox.

```
$ icinga2 console
Icinga 2 (version: v2.11.0)
<1> => function test(name) {
<1> ..   log("Hello " + name)
<1> .. }
null
<2> => test("World")
information/config: Hello World
null
<3> =>
```

Further usage examples can be found in the [library reference](18-library-reference.md#library-reference) chapter.

```
# icinga2 console --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 console [<arguments>]

Interprets Icinga script expressions.

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Command options:
  -c [ --connect ] arg      connect to an Icinga 2 instance
  -e [ --eval ] arg         evaluate expression and terminate
  -r [ --file ] arg         evaluate a file and terminate
  --syntax-only             only validate syntax (requires --eval or --file)
  --sandbox                 enable sandbox mode

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```


On operating systems without the `libedit` library installed there is no
support for line-editing or a command history. However you can
use the `rlwrap` program if you require those features:

```bash
rlwrap icinga2 console
```

The debug console can be used to connect to a running Icinga 2 instance using
the [REST API](12-icinga2-api.md#icinga2-api). [API permissions](12-icinga2-api.md#icinga2-api-permissions)
are required for executing config expressions and auto-completion.

> **Note**
>
> The debug console does not currently support TLS certificate verification.
>
> Runtime modifications are not validated and might cause the Icinga 2
> daemon to crash or behave in an unexpected way. Use these runtime changes
> at your own risk and rather *inspect and debug objects read-only*.

You can specify the API URL using the `--connect` parameter.

Although the password can be specified there process arguments on UNIX platforms are
usually visible to other users (e.g. through `ps`). In order to securely specify the
user credentials the debug console supports two environment variables:

  Environment variable | Description
  ---------------------|-------------
  ICINGA2_API_USERNAME | The API username.
  ICINGA2_API_PASSWORD | The API password.

Here's an example:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/'
Icinga 2 (version: v2.11.0)
<1> =>
```

Once connected you can inspect variables and execute other expressions by entering them at the prompt:

```
<1> => var h = get_host("icinga2-agent1.localdomain")
null
<2> => h.last_check_result
{
        active = true
        check_source = "icinga2-agent1.localdomain"
        command = [ "/usr/local/sbin/check_ping", "-H", "127.0.0.1", "-c", "5000,100%", "-w", "3000,80%" ]
        execution_end = 1446653527.174983
        execution_start = 1446653523.152673
        exit_status = 0.000000
        output = "PING OK - Packet loss = 0%, RTA = 0.11 ms"
        performance_data = [ "rta=0.114000ms;3000.000000;5000.000000;0.000000", "pl=0%;80;100;0" ]
        schedule_end = 1446653527.175133
        schedule_start = 1446653583.150000
        state = 0.000000
        type = "CheckResult"
        vars_after = {
                attempt = 1.000000
                reachable = true
                state = 0.000000
                state_type = 1.000000
        }
        vars_before = {
                attempt = 1.000000
                reachable = true
                state = 0.000000
                state_type = 1.000000
        }
}
<3> =>
```

You can use the `--eval` parameter to evaluate a single expression in batch mode.
Using the `--file` option you can specify a file which should be evaluated.
The output format for batch mode is JSON.

The `--syntax-only` option can be used in combination with `--eval` or `--file`
to check a script for syntax errors. In this mode the script is parsed to identify
syntax errors but not evaluated.

Here's an example that retrieves the command that was used by Icinga to check the `icinga2-agent1.localdomain` host:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' --eval 'get_host("icinga2-agent1.localdomain").last_check_result.command' | python -m json.tool
[
    "/usr/local/sbin/check_ping",
    "-H",
    "127.0.0.1",
    "-c",
    "5000,100%",
    "-w",
    "3000,80%"
]
```

## CLI command: Daemon <a id="cli-command-daemon"></a>

The CLI command `daemon` provides the functionality to start/stop Icinga 2.
Furthermore it allows to run the [configuration validation](11-cli-commands.md#config-validation).

```
# icinga2 daemon --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 daemon [<arguments>]

Starts Icinga 2.

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Command options:
  -c [ --config ] arg       parse a configuration file
  -z [ --no-config ]        start without a configuration file
  -C [ --validate ]         exit after validating the configuration
  --dump-objects            write icinga2.debug cache file for icinga2 object list
  -e [ --errorlog ] arg     log fatal errors to the specified log file (only
                            works in combination with --daemonize or
                            --close-stdio)
  -d [ --daemonize ]        detach from the controlling terminal
  --close-stdio             do not log to stdout (or stderr) after startup

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```

### Config Files <a id="cli-command-daemon-config-files"></a>

You can specify one or more configuration files with the `--config` option.
Configuration files are processed in the order they're specified on the command-line.

When no configuration file is specified and the `--no-config` is not used
Icinga 2 automatically falls back to using the configuration file
`ConfigDir + "/icinga2.conf"` (where ConfigDir is usually `/etc/icinga2`).

### Validation <a id="cli-command-daemon-validation"></a>

The `--validate` option can be used to check if configuration files
contain errors. If any errors are found, the exit status is 1, otherwise 0
is returned. More details in the [configuration validation](11-cli-commands.md#config-validation) chapter.

## CLI command: Feature <a id="cli-command-feature"></a>

The `feature enable` and `feature disable` commands can be used to enable and disable features:

```
# icinga2 feature disable <tab>
--app              --define           --include          --log-level        --version          checker            graphite           mainlog
--color            --help             --library          --script-debugger  api                command            ido-mysql          notification
```

```
# icinga2 feature enable <tab>
--app              --define           --include          --log-level        --version          debuglog           ido-pgsql          livestatus         perfdata           syslog
--color            --help             --library          --script-debugger  compatlog          gelf               influxdb           opentsdb           statusdata
```

The `feature list` command shows which features are currently enabled:

```
# icinga2 feature list
Disabled features: compatlog debuglog gelf ido-pgsql influxdb livestatus opentsdb perfdata statusdata syslog
Enabled features: api checker command graphite ido-mysql mainlog notification
```

## CLI command: Node <a id="cli-command-node"></a>

Provides the functionality to setup master and client
nodes in a [distributed monitoring](06-distributed-monitoring.md#distributed-monitoring) scenario.

```
# icinga2 node --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 <command> [<arguments>]

Supported commands:
  * node setup (set up node)
  * node wizard (wizard for node setup)

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```

## CLI command: Object <a id="cli-command-object"></a>

The `object` CLI command can be used to list all configuration objects and their
attributes. The command also shows where each of the attributes was modified and as such
provides debug information for further configuration problem analysis.
That way you can also identify which objects have been created from your [apply rules](17-language-reference.md#apply).

Configuration modifications are not immediately updated. Furthermore there is a known issue with
[group assign expressions](17-language-reference.md#group-assign) which are not reflected in the host object output.
You need to run `icinga2 daemon -C --dump-objects` in order to update the `icinga2.debug` cache file.

More information can be found in the [troubleshooting](15-troubleshooting.md#troubleshooting-list-configuration-objects) section.

```
# icinga2 object --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 <command> [<arguments>]

Supported commands:
  * object list (lists all objects)

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```

## CLI command: Pki <a id="cli-command-pki"></a>

Provides the CLI commands to

* generate a new certificate authority (CA)
* generate a new CSR or self-signed certificate
* sign a CSR and return a certificate
* save a master certificate manually
* request a signed certificate from the master
* generate a new ticket for the client setup

This functionality is used by the [node setup/wizard](11-cli-commands.md#cli-command-node) CLI commands.
You will need them in the [distributed monitoring chapter](06-distributed-monitoring.md#distributed-monitoring).

```
# icinga2 pki --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.12.0)

Usage:
  icinga2 <command> [<arguments>]

Supported commands:
  * pki new-ca (sets up a new CA)
  * pki new-cert (creates a new CSR)
  * pki request (requests a certificate)
  * pki save-cert (saves another Icinga 2 instance's certificate)
  * pki sign-csr (signs a CSR)
  * pki ticket (generates a ticket)
  * pki verify (verify TLS certificates: CN, signed by CA, is CA; Print certificate)

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```

## CLI command: Variable <a id="cli-command-variable"></a>

Lists all configured variables (constants) in a similar fashion like [object list](11-cli-commands.md#cli-command-object).

```
# icinga2 variable --help
icinga2 - The Icinga 2 network monitoring daemon (version: v2.11.0)

Usage:
  icinga2 <command> [<arguments>]

Supported commands:
  * variable get (gets a variable)
  * variable list (lists all variables)

Global options:
  -h [ --help ]             show this help message
  -V [ --version ]          show version information
  --color                   use VT100 color codes even when stdout is not a
                            terminal
  -D [ --define ] arg       define a constant
  -a [ --app ] arg          application library name (default: icinga)
  -l [ --library ] arg      load a library
  -I [ --include ] arg      add include search directory
  -x [ --log-level ] arg    specify the log level for the console log.
                            The valid value is either debug, notice,
                            information (default), warning, or critical
  -X [ --script-debugger ]  whether to enable the script debugger

Report bugs at <https://github.com/Icinga/icinga2>
Icinga home page: <https://icinga.com/>
```

## Enabling/Disabling Features <a id="enable-features"></a>

Icinga 2 provides configuration files for some commonly used features. These
are installed in the `/etc/icinga2/features-available` directory and can be
enabled and disabled using the `icinga2 feature enable` and `icinga2 feature disable`
[CLI commands](11-cli-commands.md#cli-command-feature), respectively.

The `icinga2 feature enable` CLI command creates symlinks in the
`/etc/icinga2/features-enabled` directory which is included by default
in the example configuration file.

You can view a list of enabled and disabled features:

```
# icinga2 feature list
Disabled features: api command compatlog debuglog graphite icingastatus ido-mysql ido-pgsql livestatus notification perfdata statusdata syslog
Enabled features: checker mainlog notification
```

Using the `icinga2 feature enable` command you can enable features:

```
# icinga2 feature enable graphite
Enabling feature graphite. Make sure to restart Icinga 2 for these changes to take effect.
```

You can disable features using the `icinga2 feature disable` command:

```
# icinga2 feature disable ido-mysql livestatus
Disabling feature ido-mysql. Make sure to restart Icinga 2 for these changes to take effect.
Disabling feature livestatus. Make sure to restart Icinga 2 for these changes to take effect.
```

The `icinga2 feature enable` and `icinga2 feature disable` commands do not
restart Icinga 2. You will need to restart Icinga 2 using the init script
after enabling or disabling features.



## Configuration Validation <a id="config-validation"></a>

Once you've edited the configuration files make sure to tell Icinga 2 to validate
the configuration changes. Icinga 2 will log any configuration error including
a hint on the file, the line number and the affected configuration line itself.

The following example creates an apply rule without any `assign` condition.

```
apply Service "my-ping4" {
  import "generic-service"
  check_command = "ping4"
  //assign where host.address
}
```

Validate the configuration:

```
# icinga2 daemon -C

[2014-05-22 17:07:25 +0200] critical/ConfigItem: Location:
/etc/icinga2/conf.d/tests/my.conf(5): }
/etc/icinga2/conf.d/tests/my.conf(6):
/etc/icinga2/conf.d/tests/my.conf(7): apply Service "my-ping4" {
                                        ^^^^^^^^^^^^^
/etc/icinga2/conf.d/tests/my.conf(8):   import "test-generic-service"
/etc/icinga2/conf.d/tests/my.conf(9):   check_command = "ping4"

Config error: 'apply' is missing 'assign'
[2014-05-22 17:07:25 +0200] critical/ConfigItem: 1 errors, 0 warnings.
Icinga 2 detected configuration errors.
```

If you encounter errors during configuration validation, please make sure
to read the [troubleshooting](15-troubleshooting.md#troubleshooting) chapter.

You can also use the [CLI command](11-cli-commands.md#cli-command-object) `icinga2 object list`
after validation passes to analyze object attributes, inheritance or created
objects by apply rules.
Find more on troubleshooting with `object list` in [this chapter](15-troubleshooting.md#troubleshooting-list-configuration-objects).


## Reload on Configuration Changes <a id="config-change-reload"></a>

Every time you have changed your configuration you should first tell Icinga 2
to [validate](11-cli-commands.md#config-validation). If there are no validation errors, you can
safely reload the Icinga 2 daemon.

```bash
systemctl reload icinga2
```

The `reload` action will send the `SIGHUP` signal to the Icinga 2 daemon
which will validate the configuration in a separate process and not stop
the other events like check execution, notifications, etc.
