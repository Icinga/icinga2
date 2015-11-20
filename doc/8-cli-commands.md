# <a id="cli-commands"></a> Icinga 2 CLI Commands

Icinga 2 comes with a number of CLI commands which support bash autocompletion.

These CLI commands will allow you to use certain functionality
provided by and around the Icinga 2 daemon.

Each CLI command provides its own help and usage information, so please
make sure to always run them with the `--help` parameter.

Run `icinga2` without any arguments to get a list of all available global
options.

    # icinga2
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
      * console (Icinga console)
      * daemon (starts Icinga 2)
      * feature disable (disables specified feature)
      * feature enable (enables specified feature)
      * feature list (lists all enabled features)
      * node add (add node)
      * node blacklist add (adds a new blacklist filter)
      * node blacklist list (lists all blacklist filters)
      * node blacklist remove (removes a blacklist filter)
      * node list (lists all nodes)
      * node remove (removes node)
      * node set (set node attributes)
      * node setup (set up node)
      * node update-config (update node config)
      * node whitelist add (adds a new whitelist filter)
      * node whitelist list (lists all whitelist filters)
      * node whitelist remove (removes a whitelist filter)
      * node wizard (wizard for node setup)
      * object list (lists all objects)
      * pki new-ca (sets up a new CA)
      * pki new-cert (creates a new CSR)
      * pki request (requests a certificate)
      * pki save-cert (saves another Icinga 2 instance's certificate)
      * pki sign-csr (signs a CSR)
      * pki ticket (generates a ticket)
      * repository clear-changes (clear uncommitted repository changes)
      * repository commit (commit repository changes)
      * repository endpoint add (adds a new Endpoint object)
      * repository endpoint list (lists all Endpoint objects)
      * repository endpoint remove (removes a Endpoint object)
      * repository host add (adds a new Host object)
      * repository host list (lists all Host objects)
      * repository host remove (removes a Host object)
      * repository service add (adds a new Service object)
      * repository service list (lists all Service objects)
      * repository service remove (removes a Service object)
      * repository zone add (adds a new Zone object)
      * repository zone list (lists all Zone objects)
      * repository zone remove (removes a Zone object)
      * troubleshoot (collect information for troubleshooting)
      * variable get (gets a variable)
      * variable list (lists all variables)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

	Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


## <a id="cli-commands-autocompletion"></a> Icinga 2 CLI Bash Autocompletion

Bash Auto-Completion (pressing `<TAB>`) is provided only for the corresponding context.

While `--config` will suggest and auto-complete files and directories on disk,
`feature enable` will only suggest disabled features. `repository` will know
about object specific attributes, and so on. Try it yourself.

RPM and Debian packages install the bash completion files into
`/etc/bash_completion.d/icinga2`.

You will need to install the `bash-completion` package if not already installed.

RHEL/CentOS/Fedora:

    # yum install bash-completion

SUSE:

    # zypper install bash-completion

Debian/Ubuntu:

    # apt-get install bash-completion

## <a id="cli-commands-global-options"></a> Icinga 2 CLI Global Options

### Application Type

By default the `icinga2` binary loads the `icinga` library. A different application type
can be specified with the `--app` command-line option.

### Libraries

Instead of loading libraries using the [`library` config directive](18-language-reference.md#library)
you can also use the `--library` command-line option.

### Constants

[Global constants](18-language-reference.md#constants) can be set using the `--define` command-line option.

### <a id="config-include-path"></a> Config Include Path

When including files you can specify that the include search path should be
checked. You can do this by putting your configuration file name in angle
brackets like this:

    include <test.conf>

This would cause Icinga 2 to search its include path for the configuration file
`test.conf`. By default the installation path for the Icinga Template Library
is the only search directory.

Using the `--include` command-line option additional search directories can be
added.


## <a id="cli-command-console"></a> CLI command: Console

The CLI command `console` can be used to evaluate Icinga config expressions, e.g. to test
[functions](18-language-reference.md#functions).

    $ icinga2 console
    Icinga 2 (version: v2.4.0)
    <1> => function test(name) {
    <1> ..   log("Hello " + name)
    <1> .. }
    null
    <2> => test("World")
    information/config: Hello World
    null
    <3> =>


On operating systems without the `libedit` library installed there is no
support for line-editing or a command history. However you can
use the `rlwrap` program if you require those features:

    $ rlwrap icinga2 console

The `console` can be used to connect to a running Icinga 2 instance using
the [REST API](9-icinga2-api.md#icinga2-api). [API permissions](9-icinga2-api.md#icinga2-api-permissions)
are required for executing config expressions and auto-completion.

> **Note**
> The console does not currently support SSL certificate verification.

You can specify the API URL using the `--connect` parameter.

Although the password can be specified there process arguments on UNIX platforms are usually visible to other users (e.g. through `ps`). In order to securely specify the user credentials the console supports two environment variables:

  Environment variable | Description
  ---------------------|-------------
  ICINGA2_API_USERNAME | The API username.
  ICINGA2_API_PASSWORD | The API password.

Here's an example:

    $ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/'
    Icinga 2 (version: v2.4.0)
    <1> =>

Once connected you can inspect variables and execute other expressions by entering them at the prompt:

    <1> => var h = get_host("example.localdomain")
    null
    <2> => h.last_check_result
    {
            active = true
            check_source = "example.localdomain"
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


You can use the `--eval` parameter to evaluate a single expression in batch mode. The output format for batch mode is JSON.

Here's an example that retrieves the command that was used by Icinga to check the `example.localdomain` host:

    $ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' --eval 'get_host("example.localdomain").last_check_result.command' | python -m json.tool
    [
        "/usr/local/sbin/check_ping",
        "-H",
        "127.0.0.1",
        "-c",
        "5000,100%",
        "-w",
        "3000,80%"
    ]

## <a id="cli-command-daemon"></a> CLI command: Daemon

The CLI command `daemon` provides the functionality to start/stop Icinga 2.
Furthermore it provides the [configuration validation](8-cli-commands.md#config-validation).

    # icinga2 daemon --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 daemon [<arguments>]

    Starts Icinga 2.

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:
      -c [ --config ] arg   parse a configuration file
      -z [ --no-config ]    start without a configuration file
      -C [ --validate ]     exit after validating the configuration
      -e [ --errorlog ] arg log fatal errors to the specified log file (only works
                            in combination with --daemonize)
      -d [ --daemonize ]    detach from the controlling terminal

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>

### Config Files

Using the `--config` option you can specify one or more configuration files.
Config files are processed in the order they're specified on the command-line.

When no configuration file is specified and the `--no-config` is not used
Icinga 2 automatically falls back to using the configuration file
`SysconfDir + "/icinga2/icinga2.conf"` (where SysconfDir is usually `/etc`).

### Config Validation

The `--validate` option can be used to check if your configuration files
contain errors. If any errors are found the exit status is 1, otherwise 0
is returned. More details in the [configuration validation](8-cli-commands.md#config-validation) chapter.


## <a id="cli-command-feature"></a> CLI command: Feature

The `feature enable` and `feature disable` commands can be used to enable and disable features:

    # icinga2 feature disable <tab>
    checker       --color       --define      --help        --include     --library     --log-level   mainlog       notification  --version

    # icinga2 feature enable <tab>
    api           command       debuglog      graphite      icingastatus  ido-pgsql     --library     --log-level   statusdata    --version
    --color       compatlog     --define      --help        ido-mysql     --include     livestatus    perfdata      syslog

The `feature list` command shows which features are currently enabled:

    # icinga2 feature list
    Disabled features: agent command compatlog debuglog gelf graphite icingastatus notification perfdata statusdata syslog
    Enabled features: api checker livestatus mainlog


## <a id="cli-command-node"></a> CLI command: Node

Provides the functionality to install and manage master and client
nodes in a [remote monitoring ](11-icinga2-client.md#icinga2-client) or
[distributed cluster](13-distributed-monitoring-ha.md#distributed-monitoring-high-availability) scenario.


    # icinga2 node --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
      * node add (add node)
      * node blacklist add (adds a new blacklist filter)
      * node blacklist list (lists all blacklist filters)
      * node blacklist remove (removes a blacklist filter)
      * node list (lists all nodes)
      * node remove (removes node)
      * node set (set node attributes)
      * node setup (set up node)
      * node update-config (update node config)
      * node whitelist add (adds a new whitelist filter)
      * node whitelist list (lists all whitelist filters)
      * node whitelist remove (removes a whitelist filter)
      * node wizard (wizard for node setup)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


## <a id="cli-command-object"></a> CLI command: Object

The `object` CLI command can be used to list all configuration objects and their
attributes. The command also shows where each of the attributes was modified.
That way you can also identify which objects have been created from your [apply rules](18-language-reference.md#apply).

More information can be found in the [troubleshooting](16-troubleshooting.md#list-configuration-objects) section.

    # icinga2 object --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
      * object list (lists all objects)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>



## <a id="cli-command-pki"></a> CLI command: Pki

Provides the CLI commands to

* generate a new local CA
* generate a new CSR or self-signed certificate
* sign a CSR and return a certificate
* save a master certificate manually
* request a signed certificate from the master
* generate a new ticket for the client setup

This functionality is used by the [node setup/wizard](8-cli-commands.md#cli-command-pki) CLI commands too.

    # icinga2 pki --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
      * pki new-ca (sets up a new CA)
      * pki new-cert (creates a new CSR)
      * pki request (requests a certificate)
      * pki save-cert (saves another Icinga 2 instance's certificate)
      * pki sign-csr (signs a CSR)
      * pki ticket (generates a ticket)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


## <a id="cli-command-repository"></a> CLI command: Repository

Provides the functionality to manage the Icinga 2 configuration repository in
`/etc/icinga2/repository.d`. All changes are logged and must be committed or
cleared after review.


> **Note**
>
> The CLI command `repository` only supports basic configuration manipulation (add, remove)
> and a limited set of objects required for the [remote client] integration. Future
> versions will support more options (set, etc.).
>
> Please check the Icinga 2 development roadmap for updates.


    # icinga2 repository --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
      * repository clear-changes (clear uncommitted repository changes)
      * repository commit (commit repository changes)
      * repository endpoint add (adds a new Endpoint object)
      * repository endpoint list (lists all Endpoint objects)
      * repository endpoint remove (removes a Endpoint object)
      * repository host add (adds a new Host object)
      * repository host list (lists all Host objects)
      * repository host remove (removes a Host object)
      * repository service add (adds a new Service object)
      * repository service list (lists all Service objects)
      * repository service remove (removes a Service object)
      * repository zone add (adds a new Zone object)
      * repository zone list (lists all Zone objects)
      * repository zone remove (removes a Zone object)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>



## <a id="cli-command-troubleshoot"></a> CLI command: Troubleshoot

Collects basic information like version, paths, log files and crash reports for troubleshooting purposes and prints them to a file or the console. See [troubleshooting](16-troubleshooting.md#troubleshooting-information-required).

Its output defaults to a file named `troubleshooting-[TIMESTAMP].log` so it won't overwrite older troubleshooting files.

> **Note**
> Keep in mind that this tool can not collect information from other icinga2 nodes, you will have to run it on
> each of one of you instances.
> This is only a tool to collect information to help others help you, it will not attempt to fix anything.


    # icinga2 troubleshoot --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.2.0-551-g1d0f6ed)

    Usage:
      icinga2 troubleshoot [<arguments>]

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]	     show version information
      --color                use VT100 color codes even when stdout is not aterminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:
      -c [ --console ]       print to console instead of file
      -o [ --output ] arg    path to output file
      --include-vars         print variables to separate file
      --inluce-objects       print object to separate file

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>

## <a id="cli-command-variable"></a> CLI command: Variable

Lists all configured variables (constants) in a similar fasion like [object list](8-cli-commands.md#cli-command-object).

    # icinga2 variable --help
    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
      * variable get (gets a variable)
      * variable list (lists all variables)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -a [ --app ] arg       application library name (default: icinga)
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


## <a id="enable-features"></a> Enabling/Disabling Features

Icinga 2 provides configuration files for some commonly used features. These
are installed in the `/etc/icinga2/features-available` directory and can be
enabled and disabled using the `icinga2 feature enable` and `icinga2 feature disable`
[CLI commands](8-cli-commands.md#cli-command-feature), respectively.

The `icinga2 feature enable` CLI command creates symlinks in the
`/etc/icinga2/features-enabled` directory which is included by default
in the example configuration file.

You can view a list of enabled and disabled features:

    # icinga2 feature list
    Disabled features: api command compatlog debuglog graphite icingastatus ido-mysql ido-pgsql livestatus notification perfdata statusdata syslog
    Enabled features: checker mainlog notification

Using the `icinga2 feature enable` command you can enable features:

    # icinga2 feature enable graphite
    Enabling feature graphite. Make sure to restart Icinga 2 for these changes to take effect.


You can disable features using the `icinga2 feature disable` command:

    # icinga2 feature disable ido-mysql livestatus
    Disabling feature ido-mysql. Make sure to restart Icinga 2 for these changes to take effect.
    Disabling feature livestatus. Make sure to restart Icinga 2 for these changes to take effect.


The `icinga2 feature enable` and `icinga2 feature disable` commands do not
restart Icinga 2. You will need to restart Icinga 2 using the init script
after enabling or disabling features.



## <a id="config-validation"></a> Configuration Validation

Once you've edited the configuration files make sure to tell Icinga 2 to validate
the configuration changes. Icinga 2 will log any configuration error including
a hint on the file, the line number and the affected configuration line itself.

The following example creates an apply rule without any `assign` condition.

    apply Service "5872-ping4" {
      import "generic-service"
      check_command = "ping4"
      //assign where match("5872-*", host.name)
    }

Validate the configuration with the init script option `checkconfig`:

    # /etc/init.d/icinga2 checkconfig

> **Note**
>
> Using [systemd](2-getting-started.md#systemd-service) you need to manually validate the configuration using
> the CLI command below.

Or manually passing the `-C` argument:

    # /usr/sbin/icinga2 daemon -c /etc/icinga2/icinga2.conf -C

    [2014-05-22 17:07:25 +0200] critical/ConfigItem: Location:
    /etc/icinga2/conf.d/tests/5872.conf(5): }
    /etc/icinga2/conf.d/tests/5872.conf(6):
    /etc/icinga2/conf.d/tests/5872.conf(7): apply Service "5872-ping4" {
                                            ^^^^^^^^^^^^^
    /etc/icinga2/conf.d/tests/5872.conf(8):   import "test-generic-service"
    /etc/icinga2/conf.d/tests/5872.conf(9):   check_command = "ping4"

    Config error: 'apply' is missing 'assign'
    [2014-05-22 17:07:25 +0200] critical/ConfigItem: 1 errors, 0 warnings.
    Icinga 2 detected configuration errors.

> **Tip**
>
> Icinga 2 will automatically detect the default path for `icinga2.conf`
> in `SysconfDir + /icinga2/icinga2.conf` and you can safely omit this parameter.
>
> `# icinga2 daemon -C`

If you encounter errors during configuration validation, please make sure
to read the [troubleshooting](16-troubleshooting.md#troubleshooting) chapter.

You can also use the [CLI command](8-cli-commands.md#cli-command-object) `icinga2 object list`
after validation passes to analyze object attributes, inheritance or created
objects by apply rules.
Find more on troubleshooting with `object list` in [this chapter](16-troubleshooting.md#list-configuration-objects).

Example filtered by `Service` objects with the name `ping*`:

    # icinga2 object list --type Service --name *ping*
    Object 'icinga.org!ping4' of type 'Service':
      * __name = 'icinga.org!ping4'
      * check_command = 'ping4'
        % = modified in '/etc/icinga2/conf.d/services.conf', lines 17:3-17:25
      * check_interval = 60
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 28:3-28:21
      * host_name = 'icinga.org'
        % = modified in '/etc/icinga2/conf.d/services.conf', lines 14:1-14:21
      * max_check_attempts = 3
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 27:3-27:24
      * name = 'ping4'
        % = modified in '/etc/icinga2/conf.d/services.conf', lines 14:1-14:21
      * retry_interval = 30
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 29:3-29:22
      * templates = [ 'ping4', 'generic-service' ]
        % += modified in '/etc/icinga2/conf.d/services.conf', lines 14:1-14:21
        % += modified in '/etc/icinga2/conf.d/templates.conf', lines 26:1-30:1
      * type = 'Service'
      * vars
        % += modified in '/etc/icinga2/conf.d/services.conf', lines 18:3-18:19
        * sla = '24x7'
          % = modified in '/etc/icinga2/conf.d/services.conf', lines 18:3-18:19



## <a id="config-change-reload"></a> Reload on Configuration Changes

Everytime you have changed your configuration you should first tell Icinga 2
to [validate](8-cli-commands.md#config-validation). If there are no validation errors you can
safely reload the Icinga 2 daemon.

    # /etc/init.d/icinga2 reload

> **Note**
>
> The `reload` action will send the `SIGHUP` signal to the Icinga 2 daemon
> which will validate the configuration in a separate process and not stop
> the other events like check execution, notifications, etc.
>
> Details can be found [here](22-migrating-from-icinga-1x.md#differences-1x-2-real-reload).

