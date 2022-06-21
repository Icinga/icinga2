# Icinga 2 Troubleshooting <a id="troubleshooting"></a>

## Required Information <a id="troubleshooting-information-required"></a>

Please ensure to provide any detail which may help reproduce and understand your issue.
Whether you ask on the [community channels](https://community.icinga.com) or you
create an issue at [GitHub](https://github.com/Icinga), make sure
that others can follow your explanations. If necessary, draw a picture and attach it for
better illustration. This is especially helpful if you are troubleshooting a distributed
setup.

We've come around many community questions and compiled this list. Add your own
findings and details please.

* Describe the expected behavior in your own words.
* Describe the actual behavior in one or two sentences.
* Ensure to provide general information such as:
	* How was Icinga 2 installed (and which repository in case) and which distribution are you using
	* `icinga2 --version`
	* `icinga2 feature list`
	* `icinga2 daemon -C`
	* [Icinga Web 2](https://icinga.com/products/icinga-web-2/) version (screenshot from System - About)
	* [Icinga Web 2 modules](https://icinga.com/products/icinga-web-2-modules/) e.g. the Icinga Director (optional)
* Configuration insights:
	* Provide complete configuration snippets explaining your problem in detail
	* Your [icinga2.conf](04-configuration.md#icinga2-conf) file
	* If you run multiple Icinga 2 instances, the [zones.conf](04-configuration.md#zones-conf) file (or `icinga2 object list --type Endpoint` and `icinga2 object list --type Zone`) from all affected nodes.
* Logs
	* Relevant output from your main and [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) in `/var/log/icinga2`. Please add step-by-step explanations with timestamps if required.
	* The newest Icinga 2 crash log if relevant, located in `/var/log/icinga2/crash`
* Additional details
	* If the check command failed, what's the output of your manual plugin tests?
	* In case of [debugging](21-development.md#development) Icinga 2, the full back traces and outputs

## Analyze your Environment <a id="troubleshooting-analyze-environment"></a>

There are many components involved on a server running Icinga 2. When you
analyze a problem, keep in mind that basic system administration knowledge
is also key to identify bottlenecks and issues.

> **Tip**
>
> [Monitor Icinga 2](08-advanced-topics.md#monitoring-icinga) and use the hints for further analysis.

* Analyze the system's performance and dentify bottlenecks and issues.
* Collect details about all applications (e.g. Icinga 2, MySQL, Apache, Graphite, Elastic, etc.).
* If data is exchanged via network (e.g. central MySQL cluster) ensure to monitor the bandwidth capabilities too.
* Add graphs from Grafana or Graphite as screenshots to your issue description

Install tools which help you to do so. Opinions differ, let us know if you have any additions here!

### Analyse your Linux/Unix Environment <a id="troubleshooting-analyze-environment-linux"></a>

[htop](https://hisham.hm/htop/) is a better replacement for `top` and helps to analyze processes
interactively.

```bash
yum install htop
apt-get install htop
```

If you are for example experiencing performance issues, open `htop` and take a screenshot.
Add it to your question and/or bug report.

Analyse disk I/O performance in Grafana, take a screenshot and obfuscate any sensitive details.
Attach it when posting a question to the community channels.

The [sysstat](https://github.com/sysstat/sysstat) package provides a number of tools to
analyze the performance on Linux. On FreeBSD you could use `systat` for example.

```bash
yum install sysstat
apt-get install sysstat
```

Example for `vmstat` (summary of memory, processes, etc.):

```bash
# summary
vmstat -s
# print timestamps, format in MB, stats every 1 second, 5 times
vmstat -t -S M 1 5
```

Example for `iostat`:

```bash
watch -n 1 iostat
```

Example for `sar`:

```bash
sar # cpu
sar -r # ram
sar -q # load avg
sar -b # I/O
```

`sysstat` also provides the `iostat` binary. On FreeBSD you could use `systat` for example.

If you are missing checks and metrics found in your analysis, add them to your monitoring!

### Analyze your Windows Environment <a id="troubleshooting-analyze-environment-windows"></a>

A good tip for Windows are the tools found inside the [Sysinternals Suite](https://technet.microsoft.com/en-us/sysinternals/bb842062.aspx).

You can also start `perfmon` and analyze specific performance counters.
Keep notes which could be important for your monitoring, and add service
checks later on.

> **Tip**
>
> Use an administrative Powershell to gain more insights.

```
cd C:\ProgramData\icinga2\var\log\icinga2

Get-Content .\icinga2.log -tail 10 -wait
```

## Enable Debug Output <a id="troubleshooting-enable-debug-output"></a>

### Enable Debug Output on Linux/Unix <a id="troubleshooting-enable-debug-output-linux"></a>

Enable the `debuglog` feature:

```bash
icinga2 feature enable debuglog
service icinga2 restart
```

The debug log file can be found in `/var/log/icinga2/debug.log`.

You can tail the log files with an administrative shell:

```bash
cd /var/log/icinga2
tail -f debug.log
```

Alternatively you may run Icinga 2 in the foreground with debugging enabled. Specify the console
log severity as an additional parameter argument to `-x`.

```bash
/usr/sbin/icinga2 daemon -x notice
```

The [log severity](09-object-types.md#objecttype-filelogger) can be one of `critical`, `warning`, `information`, `notice`
and `debug`.

### Enable Debug Output on Windows <a id="troubleshooting-enable-debug-output-windows"></a>

Open a Powershell with administrative privileges and enable the debug log feature.

```
C:\> cd C:\Program Files\ICINGA2\sbin

C:\Program Files\ICINGA2\sbin> .\icinga2.exe feature enable debuglog
```

Ensure that the Icinga 2 service already writes the main log into `C:\ProgramData\icinga2\var\log\icinga2`.
Restart the Icinga 2 service in an administrative Powershell and open the newly created `debug.log` file.

```
C:\> Restart-Service icinga2

C:\> Get-Service icinga2
```

You can tail the log files with an administrative Powershell:

```
C:\> cd C:\ProgramData\icinga2\var\log\icinga2

C:\ProgramData\icinga2\var\log\icinga2> Get-Content .\debug.log -tail 10 -wait
```

## Configuration Troubleshooting <a id="troubleshooting-configuration"></a>

### List Configuration Objects <a id="troubleshooting-list-configuration-objects"></a>

The `icinga2 object list` CLI command can be used to list all configuration objects and their
attributes. The tool also shows where each of the attributes was modified.

> **Tip**
>
> Use the Icinga 2 API to access [config objects at runtime](12-icinga2-api.md#icinga2-api-config-objects) directly.

That way you can also identify which objects have been created from your [apply rules](17-language-reference.md#apply).

```
# icinga2 object list

Object 'localhost!ssh' of type 'Service':
  * __name = 'localhost!ssh'
  * check_command = 'ssh'
    % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 5:3-5:23
  * check_interval = 60
    % = modified in '/etc/icinga2/conf.d/templates.conf', lines 24:3-24:21
  * host_name = 'localhost'
    % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 4:3-4:25
  * max_check_attempts = 3
    % = modified in '/etc/icinga2/conf.d/templates.conf', lines 23:3-23:24
  * name = 'ssh'
  * retry_interval = 30
    % = modified in '/etc/icinga2/conf.d/templates.conf', lines 25:3-25:22
  * templates = [ 'ssh', 'generic-service' ]
    % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 1:0-7:1
    % += modified in '/etc/icinga2/conf.d/templates.conf', lines 22:1-26:1
  * type = 'Service'
  * vars
    % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19
    * sla = '24x7'
      % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19

[...]
```

On Windows, use an administrative Powershell:

```
C:\> cd C:\Program Files\ICINGA2\sbin

C:\Program Files\ICINGA2\sbin> .\icinga2.exe object list
```

You can also filter by name and type:

```
# icinga2 object list --name *ssh* --type Service
Object 'localhost!ssh' of type 'Service':
  * __name = 'localhost!ssh'
  * check_command = 'ssh'
    % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 5:3-5:23
  * check_interval = 60
    % = modified in '/etc/icinga2/conf.d/templates.conf', lines 24:3-24:21
  * host_name = 'localhost'
    % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 4:3-4:25
  * max_check_attempts = 3
    % = modified in '/etc/icinga2/conf.d/templates.conf', lines 23:3-23:24
  * name = 'ssh'
  * retry_interval = 30
    % = modified in '/etc/icinga2/conf.d/templates.conf', lines 25:3-25:22
  * templates = [ 'ssh', 'generic-service' ]
    % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 1:0-7:1
    % += modified in '/etc/icinga2/conf.d/templates.conf', lines 22:1-26:1
  * type = 'Service'
  * vars
    % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19
    * sla = '24x7'
      % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19

Found 1 Service objects.

[2014-10-15 14:27:19 +0200] information/cli: Parsed 175 objects.
```

Runtime modifications via the [REST API](12-icinga2-api.md#icinga2-api-config-objects)
are not immediately updated. Furthermore there is a known issue with
[group assign expressions](17-language-reference.md#group-assign) which are not reflected in the host object output.
You need to restart Icinga 2 in order to update the `icinga2.debug` cache file.

### Apply rules do not match <a id="apply-rules-do-not-match"></a>

You can analyze apply rules and matching objects by using the [script debugger](20-script-debugger.md#script-debugger).

### Where are the check command definitions? <a id="check-command-definitions"></a>

Icinga 2 features a number of built-in [check command definitions](10-icinga-template-library.md#icinga-template-library) which are
included with

```
include <itl>
include <plugins>
```

in the [icinga2.conf](04-configuration.md#icinga2-conf) configuration file. These files are not considered
configuration files and will be overridden on upgrade, so please send modifications as proposed patches upstream.
The default include path is set to `/usr/share/icinga2/includes` with the constant `IncludeConfDir`.

You should add your own command definitions to a new file in `conf.d/` called `commands.conf`
or similar.

### Configuration is ignored <a id="configuration-ignored"></a>

* Make sure that the line(s) are not [commented out](17-language-reference.md#comments) (starting with `//` or `#`, or
encapsulated by `/* ... */`).
* Is the configuration file included in [icinga2.conf](04-configuration.md#icinga2-conf)?

Run the [configuration validation](11-cli-commands.md#config-validation) and add `notice` as log severity.
Search for the file which should be included i.e. using the `grep` CLI command.

```bash
icinga2 daemon -C -x notice | grep command
```

### Configuration attributes are inherited from <a id="configuration-attribute-inheritance"></a>

Icinga 2 allows you to import templates using the [import](17-language-reference.md#template-imports) keyword. If these templates
contain additional attributes, your objects will automatically inherit them. You can override
or modify these attributes in the current object.

The [object list](15-troubleshooting.md#troubleshooting-list-configuration-objects) CLI command allows you to verify the attribute origin.

### Configuration Value with Single Dollar Sign <a id="configuration-value-dollar-sign"></a>

In case your configuration validation fails with a missing closing dollar sign error message, you
did not properly escape the single dollar sign preventing its usage as [runtime macro](03-monitoring-basics.md#runtime-macros).

```
critical/config: Error: Validation failed for Object 'ping4' (Type: 'Service') at /etc/icinga2/zones.d/global-templates/windows.conf:24: Closing $ not found in macro format string 'top-syntax=${list}'.
```

Correct the custom variable value to

```
"top-syntax=$${list}"
```


## Checks Troubleshooting <a id="troubleshooting-checks"></a>

### Executed Command for Checks <a id="checks-executed-command"></a>

* Use the Icinga 2 API to [query](12-icinga2-api.md#icinga2-api-config-objects-query) host/service objects
for their check result containing the executed shell command.
* Use the Icinga 2 [console cli command](11-cli-commands.md#cli-command-console)
to fetch the checkable object, its check result and the executed shell command.
* Alternatively enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) and look for the executed command.

Example for a service object query using a [regex match](18-library-reference.md#global-functions-regex)
on the name:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -H 'X-HTTP-Method-Override: GET' -X POST 'https://localhost:5665/v1/objects/services' \
-d '{ "filter": "regex(pattern, service.name)", "filter_vars": { "pattern": "^http" }, "attrs": [ "__name", "last_check_result" ], "pretty": true }'
{
    "results": [
        {
            "attrs": {
                "__name": "example.localdomain!http",
                "last_check_result": {
                    "active": true,
                    "check_source": "example.localdomain",
                    "command": [
                        "/usr/local/sbin/check_http",
                        "-I",
                        "127.0.0.1",
                        "-u",
                        "/"
                    ],

  ...

                }
            },
            "joins": {},
            "meta": {},
            "name": "example.localdomain!http",
            "type": "Service"
        }
    ]
}
```

Alternatively when using the Director, navigate into the Service Detail View
in Icinga Web and pick `Inspect` to query the details.

Example for using the `icinga2 console` CLI command evaluation functionality:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' \
--eval 'get_service("example.localdomain", "http").last_check_result.command' | python -m json.tool
[
    "/usr/local/sbin/check_http",
    "-I",
    "127.0.0.1",
    "-u",
    "/"
]
```

Example for searching the debug log:

```bash
icinga2 feature enable debuglog
systemctl restart icinga2
tail -f /var/log/icinga2/debug.log | grep "notice/Process"
```


### Checks are not executed <a id="checks-not-executed"></a>

* First off, decide whether the checks are executed locally, or remote in a distributed setup.

If the master does not receive check results from the satellite, move your analysis to the satellite
and verify why the checks are not executed there.

* Check the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) to see if the check command gets executed.
* Verify that failed dependencies do not prevent command execution.
* Make sure that the plugin is executable by the Icinga 2 user (run a manual test).
* Make sure the [checker](11-cli-commands.md#enable-features) feature is enabled.
* Use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live check result streams.

Test a plugin as icinga user.

```bash
sudo -u icinga /usr/lib/nagios/plugins/check_ping -4 -H 127.0.0.1 -c 5000,100% -w 3000,80%
```

> **Note**
>
> **Never test plugins as root, but the icinga daemon user.** The environment and permissions differ.
>
> Also, the daemon user **does not** spawn a terminal shell (Bash, etc.) so it won't read anything from .bashrc
> and variants. The Icinga daemon only relies on sysconfig environment variables being set.


Enable the checker feature.

```
# icinga2 feature enable checker
The feature 'checker' is already enabled.
```

Fetch all check result events matching the `event.service` name `random`:

```bash
curl -k -s -u root:icinga -H 'Accept: application/json' -X POST \
 'https://localhost:5665/v1/events?queue=debugchecks&types=CheckResult&filter=match%28%22random*%22,event.service%29'
```


### Analyze Check Source <a id="checks-check-source"></a>

Sometimes checks are not executed on the remote host, but on the master and so on.
This could lead into unwanted results or NOT-OK states.

The `check_source` attribute is the best indication where a check command
was actually executed. This could be a satellite with synced configuration
or a client as remote command bridge -- both will return the check source
as where the plugin is called.

Example for retrieving the check source from all `disk` services using a
[regex match](18-library-reference.md#global-functions-regex) on the name:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -H 'X-HTTP-Method-Override: GET' -X POST 'https://localhost:5665/v1/objects/services' \
-d '{ "filter": "regex(pattern, service.name)", "filter_vars": { "pattern": "^disk" }, "attrs": [ "__name", "last_check_result" ], "pretty": true }'
{
    "results": [
        {
            "attrs": {
                "__name": "icinga2-agent1.localdomain!disk",
                "last_check_result": {
                    "active": true,
                    "check_source": "icinga2-agent1.localdomain",

  ...

                }
            },
            "joins": {},
            "meta": {},
            "name": "icinga2-agent1.localdomain!disk",
            "type": "Service"
        }
    ]
}
```

Alternatively when using the Director, navigate into the Service Detail View
in Icinga Web and pick `Inspect` to query the details.

Example with the debug console:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' \
--eval 'get_service("icinga2-agent1.localdomain", "disk").last_check_result.check_source' | python -m json.tool

"icinga2-agent1.localdomain"
```


### NSClient++ Check Errors with nscp-local <a id="nsclient-check-errors-nscp-local"></a>

The [nscp-local](10-icinga-template-library.md#nscp-check-local) CheckCommand object definitions call the local `nscp.exe` command.
If a Windows client service check fails to find the `nscp.exe` command, the log output would look like this:

```
Command ".\nscp.exe" "client" "-a" "drive=d" "-a" "show-all" "-b" "-q" "check_drivesize" failed to execute: 2, "The system cannot find the file specified."
```

or

```
Command ".
scp.exe" "client" "-a" "drive=d" "-a" "show-all" "-b" "-q" "check_drivesize" failed to execute: 2, "The system cannot find the file specified."
```

The above actually prints `.\\nscp.exe` where the escaped `\n` character gets interpreted as new line.

Both errors lead to the assumption that the `NscpPath` constant is empty or set to a `.` character.
This could mean the following:

* The command is **not executed on the Windows client**. Check the [check_source](15-troubleshooting.md#checks-check-source) attribute from the check result.
* You are using an outdated NSClient++ version (0.3.x or 0.4.x) which is not compatible with Icinga 2.
* You are using a custom NSClient++ installer which does not register the correct GUID for NSClient++

More troubleshooting:

Retrieve the `NscpPath` constant on your Windows client:

```
C:\Program Files\ICINGA2\sbin\icinga2.exe variable get NscpPath
```

If the variable is returned empty, manually test how Icinga 2 would resolve
its path (this can be found inside the ITL):

```
C:\Program Files\ICINGA2\sbin\icinga2.exe console --eval "dirname(msi_get_component_path(\"{5C45463A-4AE9-4325-96DB-6E239C034F93}\"))"
```

If this command does not return anything, NSClient++ is not properly installed.
Verify that inside the `Programs and Features` (`appwiz.cpl`) control panel.

You can run the bundled NSClient++ installer from the Icinga 2 Windows package.
The msi package is located in `C:\Program Files\ICINGA2\sbin`.

The bundled NSClient++ version has properly been tested with Icinga 2. Keep that
in mind when using a different package.


### Check Thresholds Not Applied <a id="check-thresholds-not-applied"></a>

This could happen with [clients as command endpoint execution](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint).

If you have for example a client host `icinga2-agent1.localdomain`
and a service `disk` check defined on the master, the warning and
critical thresholds are sometimes to applied and unwanted notification
alerts are raised.

This happens because the client itself includes a host object with
its `NodeName` and a basic set of checks in the [conf.d](04-configuration.md#conf-d)
directory, i.e. `disk` with the default thresholds.

Clients which have the `checker` feature enabled will attempt
to execute checks for local services and send their results
back to the master.

If you now have the same host and service objects on the
master you will receive wrong check results from the client.

Solution:

* Disable the `checker` feature on clients: `icinga2 feature disable checker`.
* Remove the inclusion of [conf.d](04-configuration.md#conf-d) as suggested in the [client setup docs](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint).

### Check Fork Errors <a id="check-fork-errors"></a>

Newer versions of systemd on Linux limit spawned processes for
services.

* v227 introduces the `TasksMax` setting to units which allows to specify the spawned process limit.
* v228 adds `DefaultTasksMax` in the global `systemd-system.conf` with a default setting of 512 processes.
* v231 changes the default value to 15%

This can cause problems with Icinga 2 in large environments with many
commands executed in parallel starting with systemd v228. Some distributions
also may have changed the defaults.

The error message could look like this:

```
2017-01-12T11:55:40.742685+01:00 icinga2-master1 kernel: [65567.582895] cgroup: fork rejected by pids controller in /system.slice/icinga2.service
```

In order to solve the problem, increase the value for `DefaultTasksMax`
or set it to `infinity`.

```bash
mkdir /etc/systemd/system/icinga2.service.d
cat >/etc/systemd/system/icinga2.service.d/limits.conf <<EOF
[Service]
DefaultTasksMax=infinity
EOF

systemctl daemon-reload
systemctl restart icinga2
```

An example is available inside the GitHub repository in [etc/initsystem](https://github.com/Icinga/icinga2/tree/master/etc/initsystem).

External Resources:

* [Fork limit for cgroups](https://lwn.net/Articles/663873/)
* [systemd changelog](https://github.com/systemd/systemd/blob/master/NEWS)
* [Icinga 2 upstream issue](https://github.com/Icinga/icinga2/issues/5611)
* [systemd upstream discussion](https://github.com/systemd/systemd/issues/3211)

### Systemd Watchdog <a id="check-systemd-watchdog"></a>

Usually Icinga 2 is a mission critical part of infrastructure and should be
online at all times. In case of a recoverable crash (e.g. OOM) you may want to
restart Icinga 2 automatically. With systemd it is as easy as overriding some
settings of the Icinga 2 systemd service by creating
`/etc/systemd/system/icinga2.service.d/override.conf` with the following
content:

```
[Service]
Restart=always
RestartSec=1
StartLimitInterval=10
StartLimitBurst=3
```

Using the watchdog can also help with monitoring Icinga 2, to activate and use it add the following to the override:

```
WatchdogSec=30s
```

This way systemd will kill Icinga 2 if it does not notify for over 30 seconds. A timeout of less than 10 seconds is not
recommended. When the watchdog is activated, `Restart=` can be set to `watchdog` to restart Icinga 2 in the case of a
watchdog timeout.

Run `systemctl daemon-reload && systemctl restart icinga2` to apply the changes.
Now systemd will always try to restart Icinga 2 (except if you run
`systemctl stop icinga2`). After three failures in ten seconds it will stop
trying because you probably have a problem that requires manual intervention.

### Late Check Results <a id="late-check-results"></a>

[Icinga Web 2](https://icinga.com/products/icinga-web-2/) provides
a dashboard overview for `overdue checks`.

The REST API provides the [status](12-icinga2-api.md#icinga2-api-status) URL endpoint with some generic metrics
on Icinga and its features.

```bash
curl -k -s -u root:icinga 'https://localhost:5665/v1/status?pretty=1' | less
```

You can also calculate late check results via the REST API:

* Fetch the `last_check` timestamp from each object
* Compare the timestamp with the current time and add `check_interval` multiple times (change it to see which results are really late, like five times check_interval)

You can use the [icinga2 console](11-cli-commands.md#cli-command-console) to connect to the instance, fetch all data
and calculate the differences. More infos can be found in [this blogpost](https://icinga.com/2016/08/11/analyse-icinga-2-problems-using-the-console-api/).

```
# ICINGA2_API_USERNAME=root ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://localhost:5665/'

<1> => var res = []; for (s in get_objects(Service).filter(s => s.last_check < get_time() - 2 * s.check_interval)) { res.add([s.__name, DateTime(s.last_check).to_string()]) }; res

[ [ "10807-host!10807-service", "2016-06-10 15:54:55 +0200" ], [ "mbmif.int.netways.de!disk /", "2016-01-26 16:32:29 +0100" ] ]
```

Or if you are just interested in numbers, call [len](18-library-reference.md#array-len) on the result array `res`:

```
<2> => var res = []; for (s in get_objects(Service).filter(s => s.last_check < get_time() - 2 * s.check_interval)) { res.add([s.__name, DateTime(s.last_check).to_string()]) }; res.len()

2.000000
```

If you need to analyze that problem multiple times, just add the current formatted timestamp
and repeat the commands.

```
<23> => DateTime(get_time()).to_string()

"2017-04-04 16:09:39 +0200"

<24> => var res = []; for (s in get_objects(Service).filter(s => s.last_check < get_time() - 2 * s.check_interval)) { res.add([s.__name, DateTime(s.last_check).to_string()]) }; res.len()

8287.000000
```

More details about the Icinga 2 DSL and its possibilities can be
found in the [language](17-language-reference.md#language-reference) and [library](18-library-reference.md#library-reference) reference chapters.

### Late Check Results in Distributed Environments <a id="late-check-results-distributed"></a>

When it comes to a distributed HA setup, each node is responsible for a load-balanced amount of checks.
Host and Service objects provide the attribute `paused`. If this is set to `false`, the current node
actively attempts to schedule and execute checks. Otherwise the node does not feel responsible.

```
<3> => var res = {}; for (s in get_objects(Service).filter(s => s.last_check < get_time() - 2 * s.check_interval)) { res[s.paused] += 1 }; res
{
  @false = 2.000000
  @true = 1.000000
}
```

You may ask why this analysis is important? Fair enough - if the numbers are not inverted in a HA zone
with two members, this may give a hint that the cluster nodes are in a split-brain scenario, or you've
found a bug in the cluster.


If you are running a cluster setup where the master/satellite executes checks on the client via
[top down command endpoint](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint) mode,
you might want to know which zones are affected.

This analysis assumes that clients which are not connected, have the string `connected` in their
service check result output and their state is `UNKNOWN`.

```
<4> => var res = {}; for (s in get_objects(Service)) { if (s.state==3) { if (match("*connected*", s.last_check_result.output)) { res[s.zone] += [s.host_name] } } };  for (k => v in res) { res[k] = len(v.unique()) }; res

{
  Asia = 31.000000
  Europe = 214.000000
  USA = 207.000000
}
```

The result set shows the configured zones and their affected hosts in a unique list. The output also just prints the numbers
but you can adjust this by omitting the `len()` call inside the for loop.

## Notifications Troubleshooting <a id="troubleshooting-notifications"></a>

### Notifications are not sent <a id="troubleshooting-notifications-not-sent"></a>

* Check the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) to see if a notification is triggered.
* If yes, verify that all conditions are satisfied.
* Are any errors on the notification command execution logged?

Please ensure to add these details with your own description
to any question or issue posted to the community channels.

Verify the following configuration:

* Is the host/service `enable_notifications` attribute set, and if so, to which value?
* Do the [notification](09-object-types.md#objecttype-notification) attributes `states`, `types`, `period` match the notification conditions?
* Do the [user](09-object-types.md#objecttype-user) attributes `states`, `types`, `period` match the notification conditions?
* Are there any notification `begin` and `end` times configured?
* Make sure the [notification](11-cli-commands.md#enable-features) feature is enabled.
* Does the referenced NotificationCommand work when executed as Icinga user on the shell?

If notifications are to be sent via mail, make sure that the mail program specified inside the
[NotificationCommand object](09-object-types.md#objecttype-notificationcommand) exists.
The name and location depends on the distribution so the preconfigured setting might have to be
changed on your system.


Examples:

```
# icinga2 feature enable notification
The feature 'notification' is already enabled.
```

```bash
icinga2 feature enable debuglog
systemctl restart icinga2

grep Notification /var/log/icinga2/debug.log > /root/analyze_notification_problem.log
```

You can use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live notification streams:

```bash
curl -k -s -u root:icinga -H 'Accept: application/json' -X POST 'https://localhost:5665/v1/events?queue=debugnotifications&types=Notification'
```


### Analyze Notification Result <a id="troubleshooting-notifications-result"></a>

> **Note**
>
> This feature is available since v2.11 and requires all endpoints
> being updated.

Notifications inside a HA enabled zone are balanced between the endpoints,
just like checks.

Sometimes notifications may fail, and with looking into the (debug) logs
for both masters, you cannot correlate this correctly.

The `last_notification_result` runtime attribute is stored and synced for Notification
objects and can be queried via REST API.

Example for retrieving the notification object and result from all `disk` services using a
[regex match](18-library-reference.md#global-functions-regex) on the name:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -H 'X-HTTP-Method-Override: GET' -X POST 'https://localhost:5665/v1/objects/notifications' \
-d '{ "filter": "regex(pattern, service.name)", "filter_vars": { "pattern": "^disk" }, "attrs": [ "__name", "last_notification_result" ], "pretty": true }'
{
    "results": [

        {
            "attrs": {
                "last_notification_result": {
                    "active": true,
                    "command": [
                        "/etc/icinga2/scripts/mail-service-notification.sh",
                        "-4",
                        "",
                        "-6",
                        "",
                        "-b",
                        "",
                        "-c",
                        "",
                        "-d",
                        "2019-08-02 10:54:16 +0200",
                        "-e",
                        "disk",
                        "-l",
                        "icinga2-agent1.localdomain",
                        "-n",
                        "icinga2-agent1.localdomain",
                        "-o",
                        "DISK OK - free space: / 38108 MB (90.84% inode=100%);",
                        "-r",
                        "user@localdomain",
                        "-s",
                        "OK",
                        "-t",
                        "RECOVERY",
                        "-u",
                        "disk"
                    ],
                    "execution_end": 1564736056.186217,
                    "execution_endpoint": "icinga2-master1.localdomain",
                    "execution_start": 1564736056.132323,
                    "exit_status": 0.0,
                    "output": "",
                    "type": "NotificationResult"
                }
            },
            "joins": {},
            "meta": {},
            "name": "icinga2-agent1.localdomain!disk!mail-service-notification",
            "type": "Notification"
        }

...

    ]
}
```

Example with the debug console:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' --eval 'get_object(Notification, "icinga2-agent1.localdomain!disk!mail-service-notification").last_notification_result.execution_endpoint' | jq

"icinga2-agent1.localdomain"
```

Whenever a notification command failed to execute, you can fetch the output as well.


## Feature Troubleshooting <a id="troubleshooting-features"></a>

### Feature is not working <a id="feature-not-working"></a>

* Make sure that the feature configuration is enabled by symlinking from `features-available/`
to `features-enabled` and that the latter is included in [icinga2.conf](04-configuration.md#icinga2-conf).
* Are the feature attributes set correctly according to the documentation?
* Any errors on the logs?

Look up the [object type](09-object-types.md#object-types) for the required feature and verify it is enabled:

```bash
icinga2 object list --type <feature object type>
```

Example for the `graphite` feature:

```bash
icinga2 object list --type GraphiteWriter
```

Look into the log and check whether the feature logs anything specific for this matter.

```bash
grep GraphiteWriter /var/log/icinga2/icinga2.log
```

## REST API Troubleshooting <a id="troubleshooting-api"></a>

In order to analyse errors on API requests, you can explicitly enable the [verbose parameter](12-icinga2-api.md#icinga2-api-parameters-global).

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -X DELETE 'https://localhost:5665/v1/objects/hosts/example-cmdb?pretty=1&verbose=1'
{
    "diagnostic_information": "Error: Object does not exist.\n\n ....",
    "error": 404.0,
    "status": "No objects found."
}
```

### REST API Troubleshooting: No Objects Found <a id="troubleshooting-api-no-objects-found"></a>

Please note that the `404` status with no objects being found can also originate
from missing or too strict object permissions for the authenticated user.

This is a security feature to disable object name guessing. If this would not be the
case, restricted users would be able to get a list of names of your objects just by
trying every character combination.

In order to analyse and fix the problem, please check the following:

- use an administrative account with full permissions to check whether the objects are actually there.
- verify the permissions on the affected ApiUser object and fix them.

### Missing Runtime Objects (Hosts, Downtimes, etc.) <a id="troubleshooting-api-missing-runtime-objects"></a>

Runtime objects consume the internal config packages shared with
the REST API config packages. Each host, downtime, comment, service, etc. created
via the REST API is stored in the `_api` package.

This includes downtimes and comments, which where sometimes stored in the wrong
directory path, because the active-stage file was empty/truncated/unreadable at
this point.

Wrong:

```
/var/lib/icinga2/api/packages/_api//conf.d/downtimes/1234-5678-9012-3456.conf
```

Correct:

```
/var/lib/icinga2/api/packages/_api/dbe0bef8-c72c-4cc9-9779-da7c4527c5b2/conf.d/downtimes/1234-5678-9012-3456.conf
```

At creation time, the object lives in memory but its storage is broken. Upon restart,
it is missing and e.g. a missing downtime will re-enable unwanted notifications.

`abcd-ef12-3456-7890` is the active stage name which wasn't correctly
read by the Icinga daemon. This information is stored in `/var/lib/icinga2/api/packages/_api/active-stage`.

2.11 now limits the direct active-stage file access (this is hidden from the user),
and caches active stages for packages in-memory.

It also tries to repair the broken package, and logs a new message:

```
systemctl restart icinga2

tail -f /var/log/icinga2/icinga2.log

[2019-05-10 12:27:15 +0200] information/ConfigObjectUtility: Repairing config package '_api' with stage 'dbe0bef8-c72c-4cc9-9779-da7c4527c5b2'.
```

If this does not happen, you can manually fix the broken config package, and mark a deployed stage as active
again, carefully do the following steps with creating a backup before:

Navigate into the API package prefix.

```bash
cd /var/lib/icinga2/api/packages
```

Change into the broken package directory and list all directories and files
ordered by latest changes.

```
cd _api
ls -lahtr

drwx------  4 michi  wheel   128B Mar 27 14:39 ..
-rw-r--r--  1 michi  wheel    25B Mar 27 14:39 include.conf
-rw-r--r--  1 michi  wheel   405B Mar 27 14:39 active.conf
drwx------  7 michi  wheel   224B Mar 27 15:01 dbe0bef8-c72c-4cc9-9779-da7c4527c5b2
drwx------  5 michi  wheel   160B Apr 26 12:47 .
```

As you can see, the `active-stage` file is missing. When it is there, verify that its content
is set to the stage directory as follows.

If you have more than one stage directory here, pick the latest modified
directory. Copy the directory name `abcd-ef12-3456-7890` and
add it into a new file `active-stage`. This can be done like this:

```bash
echo "dbe0bef8-c72c-4cc9-9779-da7c4527c5b2" > active-stage
```

`active.conf` needs to have the correct active stage too, add it again
like this. Note: This is deep down in the code, use with care!

```bash
sed -i 's/ActiveStages\["_api"\] = .*/ActiveStages\["_api"\] = "dbe0bef8-c72c-4cc9-9779-da7c4527c5b2"/g' /var/lib/icinga2/api/packages/_api/active.conf
```

Restart Icinga 2.

```bash
systemctl restart icinga2
```


> **Note**
>
> The internal `_api` config package structure may change in the future. Do not modify
> things in there manually or with scripts unless guided here or asked by a developer.


## Certificate Troubleshooting <a id="troubleshooting-certificate"></a>

Tools for analysing certificates and TLS connections:

- `openssl` binary on Linux/Unix, `openssl.exe` on Windows ([download](https://slproweb.com/products/Win32OpenSSL.html))
- `sslscan` tool, available [here](https://github.com/rbsec/sslscan) (Linux/Windows)

Note: You can also execute sslscan on Windows using Powershell.


### Certificate Verification <a id="troubleshooting-certificate-verification"></a>

Whenever the TLS handshake fails when a client connects to the cluster or the REST API,
ensure to verify the used certificates.

Print the CA and client certificate and ensure that the following attributes are set:

* Version must be 3.
* Serial number is a hex-encoded string.
* Issuer should be your certificate authority (defaults to `Icinga CA` for all certificates generated by CLI commands and automated signing requests).
* Validity: The certificate must not be expired.
* Subject with the common name (CN) matches the client endpoint name and its FQDN.
* v3 extensions must set the basic constraint for `CA:TRUE` (ca.crt) or `CA:FALSE` (client certificate).
* Subject Alternative Name is set to the resolvable DNS name (required for REST API and browsers).

Navigate into the local certificate store:

```bash
cd /var/lib/icinga2/certs/
```

Make sure to verify the agents' certificate and its stored `ca.crt` in `/var/lib/icinga2/certs` and ensure that
all instances (master, satellite, agent) are signed by the **same CA**.

Compare the `ca.crt` file from the agent node and compare it to your master's `ca.crt` file.


Since 2.12, you can use the built-in CLI command `pki verify` to perform TLS certificate validation tasks.

> **Hint**
>
> The CLI command uses exit codes aligned to the [Plugin API specification](05-service-monitoring.md#service-monitoring-plugin-api).
> Run the commands followed with `echo $?` to see the exit code.

These CLI commands can be used on Windows agents too without requiring the OpenSSL binary.

#### Print TLS Certificate <a id="troubleshooting-certificate-verification-print"></a>

Pass the certificate file to the `--cert` CLI command parameter to print its details.
This prints a shorter version of `openssl x509 -in <file> -text`.

```
$ icinga2 pki verify --cert icinga2-agent2.localdomain.crt

information/cli: Printing certificate 'icinga2-agent2.localdomain.crt'

 Version:             3
 Subject:             CN = icinga2-agent2.localdomain
 Issuer:              CN = Icinga CA
 Valid From:          Feb 14 11:29:36 2020 GMT
 Valid Until:         Feb 10 11:29:36 2035 GMT
 Serial:              12:fe:a6:22:f5:e3:db:a2:95:8e:92:b2:af:1a:e3:01:44:c4:70:e0

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:   icinga2-agent2.localdomain
 Fingerprint:         40 98 A0 77 58 4F CA D1 05 AC 18 53 D7 52 8D D7 9C 7F 5A 23 B4 AF 63 A4 92 9D DC FF 89 EF F1 4C
```

You can also print the `ca.crt` certificate without any further checks using the `--cert` parameter.

#### Print and Verify CA Certificate <a id="troubleshooting-certificate-verification-print-verify-ca"></a>

The `--cacert` CLI parameter allows to check whether the given certificate file is a public CA certificate.

```
$ icinga2 pki verify --cacert ca.crt

information/cli: Checking whether certificate 'ca.crt' is a valid CA certificate.

 Version:             3
 Subject:             CN = Icinga CA
 Issuer:              CN = Icinga CA
 Valid From:          Jul 31 12:26:08 2019 GMT
 Valid Until:         Jul 27 12:26:08 2034 GMT
 Serial:              89:fe:d6:12:66:25:3a:c5:07:c1:eb:d4:e6:f2:df:ca:13:6e:dc:e7

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:
 Fingerprint:         9A 11 29 A8 A3 89 F8 56 30 1A E4 0A B2 6B 28 46 07 F0 14 17 BD 19 A4 FC BD 41 40 B5 1A 8F BF 20

information/cli: OK: CA certificate file 'ca.crt' was verified successfully.
```

In case you pass a wrong certificate, an error is shown and the exit code is `2` (Critical).

```
$ icinga2 pki verify --cacert icinga2-agent2.localdomain.crt

information/cli: Checking whether certificate 'icinga2-agent2.localdomain.crt' is a valid CA certificate.

 Version:             3
 Subject:             CN = icinga2-agent2.localdomain
 Issuer:              CN = Icinga CA
 Valid From:          Feb 14 11:29:36 2020 GMT
 Valid Until:         Feb 10 11:29:36 2035 GMT
 Serial:              12:fe:a6:22:f5:e3:db:a2:95:8e:92:b2:af:1a:e3:01:44:c4:70:e0

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:   icinga2-agent2.localdomain
 Fingerprint:         40 98 A0 77 58 4F CA D1 05 AC 18 53 D7 52 8D D7 9C 7F 5A 23 B4 AF 63 A4 92 9D DC FF 89 EF F1 4C

critical/cli: CRITICAL: The file 'icinga2-agent2.localdomain.crt' does not seem to be a CA certificate file.
```

#### Verify Certificate is signed by CA Certificate <a id="troubleshooting-certificate-verification-signed-by-ca"></a>

Pass the certificate file to the `--cert` CLI parameter, and the `ca.crt` file to the `--cacert` parameter.
Common troubleshooting scenarios involve self-signed certificates and untrusted agents resulting in disconnects.

```
$ icinga2 pki verify --cert icinga2-agent2.localdomain.crt --cacert ca.crt

information/cli: Verifying certificate 'icinga2-agent2.localdomain.crt'

 Version:             3
 Subject:             CN = icinga2-agent2.localdomain
 Issuer:              CN = Icinga CA
 Valid From:          Feb 14 11:29:36 2020 GMT
 Valid Until:         Feb 10 11:29:36 2035 GMT
 Serial:              12:fe:a6:22:f5:e3:db:a2:95:8e:92:b2:af:1a:e3:01:44:c4:70:e0

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:   icinga2-agent2.localdomain
 Fingerprint:         40 98 A0 77 58 4F CA D1 05 AC 18 53 D7 52 8D D7 9C 7F 5A 23 B4 AF 63 A4 92 9D DC FF 89 EF F1 4C

information/cli:  with CA certificate 'ca.crt'.

 Version:             3
 Subject:             CN = Icinga CA
 Issuer:              CN = Icinga CA
 Valid From:          Jul 31 12:26:08 2019 GMT
 Valid Until:         Jul 27 12:26:08 2034 GMT
 Serial:              89:fe:d6:12:66:25:3a:c5:07:c1:eb:d4:e6:f2:df:ca:13:6e:dc:e7

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:
 Fingerprint:         9A 11 29 A8 A3 89 F8 56 30 1A E4 0A B2 6B 28 46 07 F0 14 17 BD 19 A4 FC BD 41 40 B5 1A 8F BF 20

information/cli: OK: Certificate with CN 'icinga2-agent2.localdomain' is signed by CA.
```

#### Verify Certificate matches Common Name (CN) <a id="troubleshooting-certificate-verification-common-name-match"></a>

This allows to verify the common name inside the certificate with a given string parameter.
Typical troubleshooting involve upper/lower case CNs (Windows).

```
$ icinga2 pki verify --cert icinga2-agent2.localdomain.crt --cn icinga2-agent2.localdomain

information/cli: Verifying common name (CN) 'icinga2-agent2.localdomain in certificate 'icinga2-agent2.localdomain.crt'.

 Version:             3
 Subject:             CN = icinga2-agent2.localdomain
 Issuer:              CN = Icinga CA
 Valid From:          Feb 14 11:29:36 2020 GMT
 Valid Until:         Feb 10 11:29:36 2035 GMT
 Serial:              12:fe:a6:22:f5:e3:db:a2:95:8e:92:b2:af:1a:e3:01:44:c4:70:e0

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:   icinga2-agent2.localdomain
 Fingerprint:         40 98 A0 77 58 4F CA D1 05 AC 18 53 D7 52 8D D7 9C 7F 5A 23 B4 AF 63 A4 92 9D DC FF 89 EF F1 4C

information/cli: OK: CN 'icinga2-agent2.localdomain' matches certificate CN 'icinga2-agent2.localdomain'.
```

In the example below, the certificate uses an upper case CN.

```
$ icinga2 pki verify --cert icinga2-agent2.localdomain.crt --cn icinga2-agent2.localdomain

information/cli: Verifying common name (CN) 'icinga2-agent2.localdomain in certificate 'icinga2-agent2.localdomain.crt'.

 Version:             3
 Subject:             CN = ICINGA2-agent2.localdomain
 Issuer:              CN = Icinga CA
 Valid From:          Feb 14 11:29:36 2020 GMT
 Valid Until:         Feb 10 11:29:36 2035 GMT
 Serial:              12:fe:a6:22:f5:e3:db:a2:95:8e:92:b2:af:1a:e3:01:44:c4:70:e0

 Signature Algorithm: sha256WithRSAEncryption
 Subject Alt Names:   ICINGA2-agent2.localdomain
 Fingerprint:         40 98 A0 77 58 4F CA D1 05 AC 18 53 D7 52 8D D7 9C 7F 5A 23 B4 AF 63 A4 92 9D DC FF 89 EF F1 4C

critical/cli: CRITICAL: CN 'icinga2-agent2.localdomain' does NOT match certificate CN 'icinga2-agent2.localdomain'.
```



### Certificate Signing <a id="troubleshooting-certificate-signing"></a>

Icinga offers two methods:

* [CSR Auto-Signing](06-distributed-monitoring.md#distributed-monitoring-setup-csr-auto-signing) which uses a client (an agent or a satellite) ticket generated on the master as trust identifier.
* [On-Demand CSR Signing](06-distributed-monitoring.md#distributed-monitoring-setup-on-demand-csr-signing) which allows to sign pending certificate requests on the master.

Whenever a signed certificate is not received on the requesting clients, ensure to check the following:

* The ticket was valid and the master's log shows nothing different (CSR Auto-Signing only)
* If the agent/satellite is directly connected to the CA master, check whether the master actually has performance problems to process the request. If the connection is closed without certificate response, analyse the master's health. It is also advised to upgrade to v2.11 where network stack problems have been fixed.
* If you're using a 3+ level cluster, check whether the satellite really forwarded the CSR signing request and the master processed it.

Other common errors:

* The generated ticket is invalid. The client receives this error message, as well as the master logs a warning message.
* The [api](09-object-types.md#objecttype-apilistener) feature does not have the `ticket_salt` attribute set to the generated `TicketSalt` constant by the CLI wizards.

In case you are using On-Demand CSR Signing, `icinga2 ca list` on the master only lists
pending requests since v2.11. Add `--all` to also see signed requests. Keep in mind that
old requests are purged after 1 week automatically.


### TLS Handshake: Ciphers <a id="troubleshooting-certificate-handshake-ciphers"></a>

Starting with v2.11, the default configured ciphers have been hardened to modern
standards. This includes TLS v1.2 as minimum protocol version too.

In case the TLS handshake fails with `no shared cipher`, first analyse whether both
instances support the same ciphers.

#### Client connects to Server <a id="troubleshooting-certificate-handshake-ciphers-client"></a>

Connect using `openssl s_client` and try to reproduce the connection problem.

> **Important**
>
> The endpoint with the server role **accepting** the connection picks the preferred
> cipher. E.g. when a satellite connects to the master, the master chooses the cipher.
>
> Keep this in mind where to simulate the client role connecting to a server with
> CLI tools such as `openssl s_client`.


`openssl s_client` tells you about the supported and shared cipher suites
on the remote server. `openssl ciphers` lists locally available ciphers.

```
$ openssl s_client -connect 192.168.33.5:5665
...

---
SSL handshake has read 2899 bytes and written 786 bytes
---
New, TLSv1/SSLv3, Cipher is AES256-GCM-SHA384
Server public key is 4096 bit
Secure Renegotiation IS supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
SSL-Session:
    Protocol  : TLSv1.2
    Cipher    : AES256-GCM-SHA384

...
```

You can specifically use one cipher or a list with the `-cipher` parameter:

```bash
openssl s_client -connect 192.168.33.5:5665 -cipher 'ECDHE-RSA-AES256-GCM-SHA384'
```

In order to fully simulate a connecting client, provide the certificates too:

```bash
CERTPATH='/var/lib/icinga2/certs'
HOSTNAME='icinga2.vagrant.demo.icinga.com'
openssl s_client -connect 192.168.33.5:5665 -cert "${CERTPATH}/${HOSTNAME}.crt" -key "${CERTPATH}/${HOSTNAME}.key" -CAfile "${CERTPATH}/ca.crt" -cipher 'ECDHE-RSA-AES256-GCM-SHA384'
```

In case to need to change the default cipher list,
set the [cipher_list](09-object-types.md#objecttype-apilistener) attribute
in the `api` feature configuration accordingly.

Beware of using insecure ciphers, this may become a
security risk in your organisation.

#### Server Accepts Client <a id="troubleshooting-certificate-handshake-ciphers-server"></a>

If the master node does not actively connect to the satellite/agent node(s), but instead
the child node actively connectsm, you can still simulate a TLS handshake.

Use `openssl s_server` instead of `openssl s_client` on the master during the connection
attempt.

```bash
openssl s_server -connect 192.168.56.101:5665
```

Since the server role chooses the preferred cipher suite in Icinga,
you can test-drive the "agent connects to master" mode here, granted that
the TCP connection is not blocked by the firewall.


#### Cipher Scan Tools <a id="troubleshooting-certificate-handshake-ciphers-scantools"></a>

You can also use different tools to test the available cipher suites, this is what SSL Labs, etc.
provide for TLS enabled websites as well. [This post](https://superuser.com/questions/109213/how-do-i-list-the-ssl-tls-cipher-suites-a-particular-website-offers)
highlights some tools and scripts such as [sslscan](https://github.com/rbsec/sslscan) or [testssl.sh](https://github.com/drwetter/testssl.sh/)

Example for sslscan on macOS against a Debian 10 Buster instance
running v2.11:

```
$ brew install sslscan

$ sslscan 192.168.33.22:5665
Version: 1.11.13-static
OpenSSL 1.0.2f  28 Jan 2016

Connected to 192.168.33.22

Testing SSL server 192.168.33.22 on port 5665 using SNI name 192.168.33.22

  TLS Fallback SCSV:
Server supports TLS Fallback SCSV

  TLS renegotiation:
Session renegotiation not supported

  TLS Compression:
Compression disabled

  Heartbleed:
TLS 1.2 not vulnerable to heartbleed
TLS 1.1 not vulnerable to heartbleed
TLS 1.0 not vulnerable to heartbleed

  Supported Server Cipher(s):
Preferred TLSv1.2  256 bits  ECDHE-RSA-AES256-GCM-SHA384   Curve P-256 DHE 256
Accepted  TLSv1.2  128 bits  ECDHE-RSA-AES128-GCM-SHA256   Curve P-256 DHE 256
Accepted  TLSv1.2  256 bits  ECDHE-RSA-AES256-SHA384       Curve P-256 DHE 256
Accepted  TLSv1.2  128 bits  ECDHE-RSA-AES128-SHA256       Curve P-256 DHE 256

  SSL Certificate:
Signature Algorithm: sha256WithRSAEncryption
RSA Key Strength:    4096

Subject:  icinga2-debian10.vagrant.demo.icinga.com
Altnames: DNS:icinga2-debian10.vagrant.demo.icinga.com
Issuer:   Icinga CA

Not valid before: Jul 12 07:39:55 2019 GMT
Not valid after:  Jul  8 07:39:55 2034 GMT
```

## Distributed Troubleshooting <a id="troubleshooting-cluster"></a>

This applies to any Icinga 2 node in a [distributed monitoring setup](06-distributed-monitoring.md#distributed-monitoring-scenarios).

You should configure the [cluster health checks](06-distributed-monitoring.md#distributed-monitoring-health-checks) if you haven't
done so already.

> **Note**
>
> Some problems just exist due to wrong file permissions or applied packet filters. Make
> sure to check these in the first place.

### Cluster Troubleshooting Connection Errors <a id="troubleshooting-cluster-connection-errors"></a>

General connection errors could be one of the following problems:

* Incorrect network configuration
* Packet loss
* Firewall rules preventing traffic

Use tools like `netstat`, `tcpdump`, `nmap`, etc. to make sure that the cluster communication
works (default port is `5665`).

```bash
tcpdump -n port 5665 -i any

netstat -tulpen | grep icinga

nmap icinga2-agent1.localdomain
```

### Cluster Troubleshooting TLS Errors <a id="troubleshooting-cluster-tls-errors"></a>

If the cluster communication fails with TLS/SSL error messages, make sure to check
the following

* File permissions on the TLS certificate files
* Does the used CA match for all cluster endpoints?
  * Verify the `Issuer` being your trusted CA
  * Verify the `Subject` containing your endpoint's common name (CN)
  * Check the validity of the certificate itself

Try to manually connect from `icinga2-agent1.localdomain` to the master node `icinga2-master1.localdomain`:

```
$ openssl s_client -CAfile /var/lib/icinga2/certs/ca.crt -cert /var/lib/icinga2/certs/icinga2-agent1.localdomain.crt -key /var/lib/icinga2/certs/icinga2-agent1.localdomain.key -connect icinga2-master1.localdomain:5665

CONNECTED(00000003)
---
...
```

If the connection attempt fails or your CA does not match, [verify the certificates](15-troubleshooting.md#troubleshooting-certificate-verification).


#### Cluster Troubleshooting Unauthenticated Clients <a id="troubleshooting-cluster-unauthenticated-clients"></a>

Unauthenticated nodes are able to connect. This is required for agent/satellite setups.

Master:

```
[2015-07-13 18:29:25 +0200] information/ApiListener: New client connection for identity 'icinga2-agent1.localdomain' (unauthenticated)
```

Agent as command execution bridge:

```
[2015-07-13 18:29:26 +1000] notice/ClusterEvents: Discarding 'execute command' message from 'icinga2-master1.localdomain': Invalid endpoint origin (client not allowed).
```

If these messages do not go away, make sure to [verify the master and agent certificates](15-troubleshooting.md#troubleshooting-certificate-verification).


### Cluster Troubleshooting Message Errors <a id="troubleshooting-cluster-message-errors"></a>

When the network connection is broken or gone, the Icinga 2 instances will be disconnected.
If the connection can't be re-established between endpoints in the same HA zone,
they remain in a Split-Brain-mode and history may differ.

Although the Icinga 2 cluster protocol stores historical events in a [replay log](15-troubleshooting.md#troubleshooting-cluster-replay-log)
for later synchronisation, you should make sure to check why the network connection failed.

Ensure to setup [cluster health checks](06-distributed-monitoring.md#distributed-monitoring-health-checks)
to monitor all endpoints and zones connectivity.


### Cluster Troubleshooting Command Endpoint Errors <a id="troubleshooting-cluster-command-endpoint-errors"></a>

Command endpoints can be used [for agents](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint)
as well as inside an [High-Availability cluster](06-distributed-monitoring.md#distributed-monitoring-scenarios).

There is no CLI command for manually executing the check, but you can verify
the following (e.g. by invoking a forced check from the web interface):

* `/var/log/icinga2/icinga2.log` shows connection and execution errors.
 * The ApiListener is not enabled to [accept commands](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint). This is visible as `UNKNOWN` check result output.
 * `CheckCommand` definition not found on the remote client. This is visible as `UNKNOWN` check result output.
 * Referenced check plugin not found on the remote agent.
 * Runtime warnings and errors, e.g. unresolved runtime macros or configuration problems.
* Specific error messages are also populated into `UNKNOWN` check results including a detailed error message in their output.
* Verify the [check source](15-troubleshooting.md#checks-check-source). This is populated by the node executing the check. You can see that in Icinga Web's detail view or by querying the REST API for this checkable object.

Additional tasks:

* More verbose logs are found inside the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output).

* Use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live check result streams.

Fetch all check result events matching the `event.service` name `remote-client`:

```bash
curl -k -s -u root:icinga -H 'Accept: application/json' -X POST 'https://localhost:5665/v1/events?queue=debugcommandendpoint&types=CheckResult&filter=match%28%22remote-client*%22,event.service%29'
```


#### Agent Hosts with Command Endpoint require a Zone <a id="troubleshooting-cluster-command-endpoint-errors-agent-hosts-command-endpoint-zone"></a>

2.11 fixes bugs where agent host checks would never be scheduled on
the master. One requirement is that the checkable host/service
is put into a zone.

By default, the Director puts the agent host in `zones.d/master`
and you're good to go. If you manually manage the configuration,
the config compiler now throws an error with `command_endpoint`
being set but no `zone` defined.

In case you previously managed the configuration outside of `zones.d`,
follow along with the following instructions.

The most convenient way with e.g. managing the objects in `conf.d`
is to move them into the `master` zone.

First, verify the name of your endpoint's zone. The CLI wizards
use `master` by default.

```
vim /etc/icinga2/zones.conf

object Zone "master" {
  ...
}
```

Then create a new directory in `zones.d` called `master`, if not existing.

```bash
mkdir -p /etc/icinga2/zones.d/master
```

Now move the directory tree from `conf.d` into the `master` zone.

```bash
mv conf.d/* /etc/icinga2/zones.d/master/
```

Validate the configuration and reload Icinga.

```bash
icinga2 daemon -C
systemctl restart icinga2
```

Another method is to specify the `zone` attribute manually, but since
this may lead into other unwanted "not checked" scenarios, we don't
recommend this for your production environment.

### Cluster Troubleshooting Config Sync <a id="troubleshooting-cluster-config-sync"></a>

In order to troubleshoot this, remember the key things with the config sync:

* Within a config master zone, only one configuration master is allowed to have its config in `/etc/icinga2/zones.d`.
    * The config master copies the zone configuration from `/etc/icinga2/zones.d` to `/var/lib/icinga2/api/zones`. This storage is the same for all cluster endpoints, and the source for all config syncs.
    * The config master puts the `.authoritative` marker on these zone files locally. This is to ensure that it doesn't receive config updates from other endpoints. If you have copied the content from `/var/lib/icinga2/api/zones` to another node, ensure to remove them.
* During startup, the master validates the entire configuration and only syncs valid configuration to other zone endpoints.

Satellites/Agents < 2.11 store the received configuration directly in `/var/lib/icinga2/api/zones`, validating it and reloading the daemon.
Satellites/Agents >= 2.11 put the received configuration into the staging directory `/var/lib/icinga2/api/zones-stage` first, and will only copy this to the production directory `/var/lib/icinga2/api/zones` once the validation was successful.

The configuration sync logs the operations during startup with the `information` severity level. Received zone configuration is also logged.

Typical errors are:

* The api feature doesn't [accept config](06-distributed-monitoring.md#distributed-monitoring-top-down-config-sync). This is logged into `/var/lib/icinga2/icinga2.log`.
* The received configuration zone is not configured in [zones.conf](04-configuration.md#zones-conf) and Icinga denies it. This is logged into `/var/lib/icinga2/icinga2.log`.
* The satellite/agent has local configuration in `/etc/icinga2/zones.d` and thinks it is authoritive for this zone. It then denies the received update. Purge the content from `/etc/icinga2/zones.d`, `/var/lib/icinga2/api/zones/*` and restart Icinga to fix this.

#### New configuration does not trigger a reload <a id="troubleshooting-cluster-config-sync-no-reload"></a>

The debug/notice log dumps the calculated checksums for all files and the comparison. Analyse this to troubleshoot further.

A complete sync for the `director-global` global zone can look like this:

```
[2019-08-01 09:20:25 +0200] notice/JsonRpcConnection: Received 'config::Update' message from 'icinga2-master1.localdomain'
[2019-08-01 09:20:25 +0200] information/ApiListener: Applying config update from endpoint 'icinga2-master1.localdomain' of zone 'master'.
[2019-08-01 09:20:25 +0200] notice/ApiListener: Creating config update for file '/var/lib/icinga2/api/zones/director-global/.checksums'.
[2019-08-01 09:20:25 +0200] notice/ApiListener: Creating config update for file '/var/lib/icinga2/api/zones/director-global/.timestamp'.
[2019-08-01 09:20:25 +0200] notice/ApiListener: Creating config update for file '/var/lib/icinga2/api/zones/director-global/director/001-director-basics.conf'.
[2019-08-01 09:20:25 +0200] notice/ApiListener: Creating config update for file '/var/lib/icinga2/api/zones/director-global/director/host_templates.conf'.
[2019-08-01 09:20:25 +0200] information/ApiListener: Received configuration for zone 'director-global' from endpoint 'icinga2-master1.localdomain'. Comparing the checksums.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Checking for config change between stage and production. Old (4): '{"/.checksums":"c4dd1237e36dcad9142f4d9a81324a7cae7d01543a672299
b8c1bb08b629b7d1","/.timestamp":"f21c0e6551328812d9f5176e5e31f390de0d431d09800a85385630727b404d83","/director/001-director-basics.conf":"f86583eec81c9bf3a1823a761991fb53d640bd0dc
6cd12bf8c5e6a275359970f","/director/host_templates.conf":"831e9b7e3ec1e33288e56a51e63c688da1d6316155349382a101f7fce6229ecc"}' vs. new (4): '{"/.checksums":"c4dd1237e36dcad9142f4d
9a81324a7cae7d01543a672299b8c1bb08b629b7d1","/.timestamp":"f21c0e6551328812d9f5176e5e31f390de0d431d09800a85385630727b404d83","/director/001-director-basics.conf":"f86583eec81c9bf
3a1823a761991fb53d640bd0dc6cd12bf8c5e6a275359970f","/director/host_templates.conf":"831e9b7e3ec1e33288e56a51e63c688da1d6316155349382a101f7fce6229ecc"}'.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Ignoring old internal file '/.checksums'.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Ignoring old internal file '/.timestamp'.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Checking /director/001-director-basics.conf for old checksum: f86583eec81c9bf3a1823a761991fb53d640bd0dc6cd12bf8c5e6a275359970f.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Checking /director/host_templates.conf for old checksum: 831e9b7e3ec1e33288e56a51e63c688da1d6316155349382a101f7fce6229ecc.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Ignoring new internal file '/.checksums'.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Ignoring new internal file '/.timestamp'.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Checking /director/001-director-basics.conf for new checksum: f86583eec81c9bf3a1823a761991fb53d640bd0dc6cd12bf8c5e6a275359970f.
[2019-08-01 09:20:25 +0200] debug/ApiListener: Checking /director/host_templates.conf for new checksum: 831e9b7e3ec1e33288e56a51e63c688da1d6316155349382a101f7fce6229ecc.
[2019-08-01 09:20:25 +0200] information/ApiListener: Stage: Updating received configuration file '/var/lib/icinga2/api/zones-stage/director-global//director/001-director-basics.c
onf' for zone 'director-global'.
[2019-08-01 09:20:25 +0200] information/ApiListener: Stage: Updating received configuration file '/var/lib/icinga2/api/zones-stage/director-global//director/host_templates.conf'
for zone 'director-global'.
[2019-08-01 09:20:25 +0200] information/ApiListener: Applying configuration file update for path '/var/lib/icinga2/api/zones-stage/director-global' (2209 Bytes).

...

[2019-08-01 09:20:25 +0200] information/ApiListener: Received configuration updates (4) from endpoint 'icinga2-master1.localdomain' are different to production, triggering validation and reload.
[2019-08-01 09:20:25 +0200] notice/Process: Running command '/usr/lib/x86_64-linux-gnu/icinga2/sbin/icinga2' '--no-stack-rlimit' 'daemon' '--close-stdio' '-e' '/var/log/icinga2/e
rror.log' '--validate' '--define' 'System.ZonesStageVarDir=/var/lib/icinga2/api/zones-stage/': PID 4532
[2019-08-01 09:20:25 +0200] notice/Process: PID 4532 ('/usr/lib/x86_64-linux-gnu/icinga2/sbin/icinga2' '--no-stack-rlimit' 'daemon' '--close-stdio' '-e' '/var/log/icinga2/error.l
og' '--validate' '--define' 'System.ZonesStageVarDir=/var/lib/icinga2/api/zones-stage/') terminated with exit code 0
[2019-08-01 09:20:25 +0200] information/ApiListener: Config validation for stage '/var/lib/icinga2/api/zones-stage/' was OK, replacing into '/var/lib/icinga2/api/zones/' and trig
gering reload.
[2019-08-01 09:20:26 +0200] information/ApiListener: Copying file 'director-global//.checksums' from config sync staging to production zones directory.
[2019-08-01 09:20:26 +0200] information/ApiListener: Copying file 'director-global//.timestamp' from config sync staging to production zones directory.
[2019-08-01 09:20:26 +0200] information/ApiListener: Copying file 'director-global//director/001-director-basics.conf' from config sync staging to production zones directory.
[2019-08-01 09:20:26 +0200] information/ApiListener: Copying file 'director-global//director/host_templates.conf' from config sync staging to production zones directory.

...

[2019-08-01 09:20:26 +0200] notice/Application: Got reload command, forwarding to umbrella process (PID 4236)
```

In case the received configuration updates are equal to what is running in production, a different message is logged and the validation/reload is skipped.

```
[2020-02-05 15:18:19 +0200] information/ApiListener: Received configuration updates (4) from endpoint 'icinga2-master1.localdomain' are equal to production, skipping validation and reload.
```


#### Syncing Binary Files is Denied <a id="troubleshooting-cluster-config-sync-binary-denied"></a>

The config sync is built for syncing text configuration files, wrapped into JSON-RPC messages.
Some users have started to use this as binary file sync instead of using tools built for this:
rsync, git, Puppet, Ansible, etc.

Starting with 2.11, this attempt is now prohibited and logged.

```
[2019-08-02 16:03:19 +0200] critical/ApiListener: Ignoring file '/etc/icinga2/zones.d/global-templates/forbidden.exe' for cluster config sync: Does not contain valid UTF8. Binary files are not supported.
Context:
	(0) Creating config update for file '/etc/icinga2/zones.d/global-templates/forbidden.exe'
	(1) Activating object 'api' of type 'ApiListener'
```

In order to solve this problem, remove the mentioned files from `zones.d` and use an alternate way
of syncing plugin binaries to your satellites and agents.


#### Zones in Zones <a id="troubleshooting-cluster-config-zones-in-zones"></a>

The cluster config sync works in a such manner that any `/etc/icinga2/zones.d/` subdirectory is included only when it is
named after a known zone by the local `Endpoint`.

If you for example add some configs in to `zones.d/satellite` and forgot to define the `satellite` zone
in `zones.d/master` or outside in `/etc/icinga2/zones.conf`, the config compiler will not include
this config from the `zones.d/satellite` zone directory.

Since v2.11, the config compiler is only including directories where a
zone has been configured. Otherwise, it would include renamed old zones,
broken zones, etc. and those long-lasting bugs have been now fixed.

Here are some working examples:

**Example: Everything in `zones.conf`**

Each instance needs to know the `Zone` and `Endpoint` definitions for itself and all directly connected instances in order
to successfully establish a connection with each other. This can be achieved by placing all `Endpoint` and `Zone` definitions
of all Icinga 2 instances known by the local endpoint in this single file.

```
vim /etc/icinga2/zones.conf

object Endpoint "icinga2-master1.localdomain" { ... }
object Endpoint "icinga2-master2.localdomain" { ... }

object Zone "master" {
  endpoints = [ "icinga2-master1.localdomain", "icinga2-master2.localdomain" ]
}

object Endpoint "icinga2-satellite1.localdomain" { ... }
object Endpoint "icinga2-satellite2.localdomain" { ... }

object Zone "satellite" {
  endpoints = [ "icinga2-satellite1.localdomain", "icinga2-satellite1.localdomain" ]
  parent = "master"
}
```

**Example: Child zones in `zones.d/`**

An additional option that Icinga 2 offers is the possibility to outsource all *child* `Endpoint` definitions of the
local Icinga 2 instance to the `zones.d/` directory. As an example, we can place the satellite `Zone` and `Endpoint` definition
from the above example into `zones.d/` underneath a directory named exactly like the local endpoint `Zone` name, which
in our case is `master`.

```
mkdir /etc/icinga2/zones.d/master
vim /etc/icinga2/zones.d/master/satellite.conf

object Endpoint "icinga2-satellite1.localdomain" { ... }
object Endpoint "icinga2-satellite2.localdomain" { ... }

object Zone "satellite" {
  endpoints = [ "icinga2-satellite1.localdomain", "icinga2-satellite1.localdomain" ]
  parent = "master"
}

...
```

Once done, you can start deploying actual monitoring objects into the satellite zone.

```
vim /etc/icinga2/zones.d/satellite/satellite-hosts.conf

object Host "agent" { ... }
```

Keep in mind that the `agent` host object will never reach the satellite, when the master does not have the
`satellite` zone configured either in `zones.d/master` nor outside the `zones.d` directory. That's also explained and
described in the [documentation](06-distributed-monitoring.md#distributed-monitoring-scenarios-master-satellite-agents).

The thing you can do: For `command_endpoint` agents like inside the Director:
Host -> Agent -> yes, there is no config sync for this zone in place. Therefore,
it is valid to just sync their zones via the config sync.

#### Director Changes

The following restores the Zone/Endpoint objects as config objects outside of `zones.d`
in your master/satellite's zones.conf with rendering them as external objects in the Director.

[Example](06-distributed-monitoring.md#distributed-monitoring-scenarios-master-satellite-agents)
for a 3 level setup with the masters and satellites knowing about the zone hierarchy
outside defined in [zones.conf](04-configuration.md#zones-conf):

```
object Endpoint "icinga-master1.localdomain" {
  //define 'host' attribute to control the connection direction on each instance
}

object Endpoint "icinga-master2.localdomain" {
  //...
}

object Endpoint "icinga-satellite1.localdomain" {
  //...
}

object Endpoint "icinga-satellite2.localdomain" {
  //...
}

//--------------
// Zone hierarchy with endpoints, required for the trust relationship and that the cluster config sync knows which zone directory defined in zones.d needs to be synced to which endpoint.
// That's no different to what is explained in the docs as basic zone trust hierarchy, and is intentionally managed outside in zones.conf there.

object Zone "master" {
  endpoints = [ "icinga-master1.localdomain", "icinga-master2.localdomain" ]
}

object Zone "satellite" {
  endpoints = [ "icinga-satellite1.localdomain", "icinga-satellite2.localdomain" ]
  parent = "master" // trust
}
```

Prepare the above configuration on all affected nodes, satellites are likely uptodate already.
Then continue with the steps below.

> * backup your database, just to be on the safe side
> * create all non-external Zone/Endpoint-Objects on all related Icinga Master/Satellite-Nodes (manually in your local zones.conf)
> * while doing so please do NOT restart Icinga, no deployments
> * change the type in the Director DB:
>
> ```sql
> UPDATE icinga_zone SET object_type = 'external_object' WHERE object_type = 'object';
> UPDATE icinga_endpoint SET object_type = 'external_object' WHERE object_type = 'object';
> ```
>
> * render and deploy a new configuration in the Director. It will state that there are no changes. Ignore it, deploy anyways
>
> That's it. All nodes should automatically restart, triggered by the deployed configuration via cluster protocol.


### Cluster Troubleshooting Overdue Check Results <a id="troubleshooting-cluster-check-results"></a>

If your master does not receive check results (or any other events) from the child zones
(satellite, clients, etc.), make sure to check whether the client sending in events
is allowed to do so.

> **Tip**
>
> General troubleshooting hints on late check results are documented [here](15-troubleshooting.md#late-check-results).

The [distributed monitoring conventions](06-distributed-monitoring.md#distributed-monitoring-conventions)
apply. So, if there's a mismatch between your client node's endpoint name and its provided
certificate's CN, the master will deny all events.

> **Tip**
>
> [Icinga Web 2](https://icinga.com/docs/icinga-web-2/latest/doc/01-About/) provides a dashboard view
> for overdue check results.

Enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) on the master
for more verbose insights.

If the client cannot authenticate, it's a more general [problem](15-troubleshooting.md#troubleshooting-cluster-unauthenticated-clients).

The client's endpoint is not configured on nor trusted by the master node:

```
Discarding 'check result' message from 'icinga2-agent1.localdomain': Invalid endpoint origin (client not allowed).
```

The check result message sent by the client does not belong to the zone the checkable object is
in on the master:

```
Discarding 'check result' message from 'icinga2-agent1.localdomain': Unauthorized access.
```


### Cluster Troubleshooting Replay Log <a id="troubleshooting-cluster-replay-log"></a>

If your `/var/lib/icinga2/api/log` directory grows, it generally means that your cluster
cannot replay the log on connection loss and re-establishment. A master node for example
will store all events for not connected endpoints in the same and child zones.

Check the following:

* All clients are connected? (e.g. [cluster health check](06-distributed-monitoring.md#distributed-monitoring-health-checks)).
* Check your [connection](15-troubleshooting.md#troubleshooting-cluster-connection-errors) in general.
* Does the log replay work, e.g. are all events processed and the directory gets cleared up over time?
* Decrease the `log_duration` attribute value for that specific [endpoint](09-object-types.md#objecttype-endpoint).

The cluster health checks also measure the `slave_lag` metric. Use this data to correlate
graphs with other events (e.g. disk I/O, network problems, etc).


### Cluster Troubleshooting: Windows Agents <a id="troubleshooting-cluster-windows-agents"></a>


#### Windows Service Exe Path <a id="troubleshooting-cluster-windows-agents-service-exe-path"></a>

Icinga agents can be installed either as x86 or x64 package. If you enable features, or wonder why
logs are not written, the first step is to analyse which path the Windows service `icinga2` is using.

Start a new administrative Powershell and ensure that the `icinga2` service is running.

```
C:\Program Files\ICINGA2\sbin> net start icinga2
```

Use the `Get-WmiObject` function to extract the windows service and its path name.

```
C:\Program Files\ICINGA2\sbin> Get-WmiObject win32_service | ?{$_.Name -like '*icinga*'} | select Name, DisplayName, State, PathName

Name    DisplayName State   PathName
----    ----------- -----   --------
icinga2 Icinga 2    Running "C:\Program Files\ICINGA2\sbin\icinga2.exe" --scm "daemon"
```

If you have used the `icinga2.exe` from a different path to enable e.g. the `debuglog` feature,
navigate into `C:\Program Files\ICINGA2\sbin\` and use the correct exe to control the feature set.


#### Windows Agents consuming 100% CPU <a id="troubleshooting-cluster-windows-agents-cpu"></a>

> **Note**
>
> The network stack was rewritten in 2.11. This fixes several hanging connections and threads
> on older Windows agents and master/satellite nodes. Prior to testing the below, plan an upgrade.

Icinga 2 requires the `NodeName` [constant](17-language-reference.md#constants) in various places to run.
This includes loading the TLS certificates, setting the proper check source,
and so on.

Typically the Windows setup wizard and also the CLI commands populate the [constants.conf](04-configuration.md#constants-conf)
file with the auto-detected or user-provided FQDN/Common Name.

If this constant is not set during startup, Icinga will try to resolve the
FQDN, if that fails, fetch the hostname. If everything fails, it logs
an error and sets this to `localhost`. This results in undefined behaviour
if ignored by the admin.

Querying the DNS when not reachable is CPU consuming, and may look like Icinga
is doing lots of checks, etc. but actually really is just starting up.

In order to fix this, edit the `constants.conf` file and populate
the `NodeName` constant with the FQDN. Ensure this is the same value
as the local endpoint object name.

```
const NodeName = "windows-agent1.domain.com"
```



#### Windows blocking Icinga 2 with ephemeral port range <a id="troubleshooting-cluster-windows-agents-ephemeral-port-range"></a>

When you see a message like this in your Windows agent logs:

```
critical/TcpSocket: Invalid socket: 10055, "An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full."
```

Windows is blocking Icinga 2 and as such, no more TCP connection handling is possible.

Depending on the version, patch level and installed applications, Windows is changing its
range of [ephemeral ports](https://en.wikipedia.org/wiki/Ephemeral_port#Range).

In order to solve this, raise the `MaxUserPort` value in the registry.

```
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters

Value Name: MaxUserPort Value
Type: DWORD
Value data: 65534
```

More details in [this blogpost](https://www.netways.de/blog/2019/01/24/windows-blocking-icinga-2-with-ephemeral-port-range/)
and this [MS help entry](https://support.microsoft.com/en-us/help/196271/when-you-try-to-connect-from-tcp-ports-greater-than-5000-you-receive-t).
