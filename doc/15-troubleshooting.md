# Icinga 2 Troubleshooting <a id="troubleshooting"></a>

## Required Information <a id="troubleshooting-information-required"></a>

Please ensure to provide any detail which may help reproduce and understand your issue.
Whether you ask on the community channels or you create an issue at [GitHub](https://github.com/Icinga), make sure
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
	* Your [icinga2.conf](04-configuring-icinga-2.md#icinga2-conf) file
	* If you run multiple Icinga 2 instances, the [zones.conf](04-configuring-icinga-2.md#zones-conf) file (or `icinga2 object list --type Endpoint` and `icinga2 object list --type Zone`) from all affected nodes.
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
* Add graphs and screenshots to your issue description

Install tools which help you to do so. Opinions differ, let us know if you have any additions here!

### Analyse your Linux/Unix Environment <a id="troubleshooting-analyze-environment-linux"></a>

[htop](https://hisham.hm/htop/) is a better replacement for `top` and helps to analyze processes
interactively.

```
yum install htop
apt-get install htop
```

If you are for example experiencing performance issues, open `htop` and take a screenshot.
Add it to your question and/or bug report.

Analyse disk I/O performance in Grafana, take a screenshot and obfuscate any sensitive details.
Attach it when posting a question to the community channels.

The [sysstat](https://github.com/sysstat/sysstat) package provides a number of tools to
analyze the performance on Linux. On FreeBSD you could use `systat` for example.

```
yum install sysstat
apt-get install sysstat
```

Example for `vmstat` (summary of memory, processes, etc.):

```
// summary
vmstat -s
// print timestamps, format in MB, stats every 1 second, 5 times
vmstat -t -S M 1 5
```

Example for `iostat`:

```
watch -n 1 iostat
```

Example for `sar`:
```
sar //cpu
sar -r //ram
sar -q //load avg
sar -b //I/O
```

`sysstat` also provides the `iostat` binary. On FreeBSD you could use `systat` for example.

If you are missing checks and metrics found in your analysis, add them to your monitoring!

### Analyze your Windows Environment <a id="troubleshooting-analyze-environment-windows"></a>

A good tip for Windows are the tools found inside the [Sysinternals Suite](https://technet.microsoft.com/en-us/sysinternals/bb842062.aspx).

You can also start `perfmon` and analyze specific performance counters.
Keep notes which could be important for your monitoring, and add service
checks later on.

## Enable Debug Output <a id="troubleshooting-enable-debug-output"></a>

### Enable Debug Output on Linux/Unix <a id="troubleshooting-enable-debug-output-linux"></a>

Enable the `debuglog` feature:

```
# icinga2 feature enable debuglog
# service icinga2 restart
```

The debug log file can be found in `/var/log/icinga2/debug.log`.

Alternatively you may run Icinga 2 in the foreground with debugging enabled. Specify the console
log severity as an additional parameter argument to `-x`.

```
# /usr/sbin/icinga2 daemon -x notice
```

The [log severity](09-object-types.md#objecttype-filelogger) can be one of `critical`, `warning`, `information`, `notice`
and `debug`.

### Enable Debug Output on Windows <a id="troubleshooting-enable-debug-output-windows"></a>

Open a command prompt with administrative privileges and enable the debug log feature.

```
C:> icinga2.exe feature enable debuglog
```

Ensure that the Icinga 2 service already writes the main log into `C:\ProgramData\icinga2\var\log\icinga2`.
Restart the Icinga 2 service and open the newly created `debug.log` file.

```
C:> net stop icinga2
C:> net start icinga2
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

in the [icinga2.conf](04-configuring-icinga-2.md#icinga2-conf) configuration file. These files are not considered
configuration files and will be overridden on upgrade, so please send modifications as proposed patches upstream.
The default include path is set to `/usr/share/icinga2/includes` with the constant `IncludeConfDir`.

You should add your own command definitions to a new file in `conf.d/` called `commands.conf`
or similar.

### Configuration is ignored <a id="configuration-ignored"></a>

* Make sure that the line(s) are not [commented out](17-language-reference.md#comments) (starting with `//` or `#`, or
encapsulated by `/* ... */`).
* Is the configuration file included in [icinga2.conf](04-configuring-icinga-2.md#icinga2-conf)?

Run the [configuration validation](11-cli-commands.md#config-validation) and add `notice` as log severity.
Search for the file which should be included i.e. using the `grep` CLI command.

```
# icinga2 daemon -C -x notice | grep command
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

Correct the custom attribute value to

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

```
# icinga2 feature enable debuglog
# systemctl restart icinga2
# tail -f /var/log/icinga2/debug.log | grep "notice/Process"
```


### Checks are not executed <a id="checks-not-executed"></a>

* Check the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) to see if the check command gets executed.
* Verify that failed depedencies do not prevent command execution.
* Make sure that the plugin is executable by the Icinga 2 user (run a manual test).
* Make sure the [checker](11-cli-commands.md#enable-features) feature is enabled.
* Use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live check result streams.

Examples:

```
# sudo -u icinga /usr/lib/nagios/plugins/check_ping -4 -H 127.0.0.1 -c 5000,100% -w 3000,80%

# icinga2 feature enable checker
The feature 'checker' is already enabled.
```

Fetch all check result events matching the `event.service` name `random`:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -X POST 'https://localhost:5665/v1/events?queue=debugchecks&types=CheckResult&filter=match%28%22random*%22,event.service%29'
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
                "__name": "icinga2-client1.localdomain!disk",
                "last_check_result": {
                    "active": true,
                    "check_source": "icinga2-client1.localdomain",

  ...

                }
            },
            "joins": {},
            "meta": {},
            "name": "icinga2-client1.localdomain!disk",
            "type": "Service"
        }
    ]
}
```

Example for using the `icinga2 console` CLI command evaluation functionality:

```
$ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' \
--eval 'get_service("icinga2-client1.localdomain", "disk").last_check_result.check_source' | python -m json.tool

"icinga2-client1.localdomain"
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

If you have for example a client host `icinga2-client1.localdomain`
and a service `disk` check defined on the master, the warning and
critical thresholds are sometimes to applied and unwanted notification
alerts are raised.

This happens because the client itself includes a host object with
its `NodeName` and a basic set of checks in the [conf.d](04-configuring-icinga-2.md#conf-d)
directory, i.e. `disk` with the default thresholds.

Clients which have the `checker` feature enabled will attempt
to execute checks for local services and send their results
back to the master.

If you now have the same host and service objects on the
master you will receive wrong check results from the client.

Solution:

* Disable the `checker` feature on clients: `icinga2 feature disable checker`.
* Remove the inclusion of [conf.d](04-configuring-icinga-2.md#conf-d) as suggested in the [client setup docs](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint).

### Check Fork Errors <a id="check-fork-errors"></a>

Newer versions of Systemd on Linux limit spawned processes for
services.

* v227 introduces the `TasksMax` setting to units which allows to specify the spawned process limit.
* v228 adds `DefaultTasksMax` in the global `systemd-system.conf` with a default setting of 512 processes.
* v231 changes the default value to 15%

This can cause problems with Icinga 2 in large environments with many
commands executed in parallel starting with Systemd v228. Some distributions
also may have changed the defaults.

The error message could look like this:

```
2017-01-12T11:55:40.742685+01:00 icinga2-master1 kernel: [65567.582895] cgroup: fork rejected by pids controller in /system.slice/icinga2.service
```

In order to solve the problem, increase the value for `DefaultTasksMax`
or set it to `infinity`.

```
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
* [Systemd changelog](https://github.com/systemd/systemd/blob/master/NEWS)
* [Icinga 2 upstream issue](https://github.com/Icinga/icinga2/issues/5611)
* [Systemd upstream discussion](https://github.com/systemd/systemd/issues/3211)

### Systemd Watchdog <a id="check-systemd-watchdog"></a>

Usually Icinga 2 is a mission critical part of infrastructure and should be
online at all times. In case of a recoverable crash (e.g. OOM) you may want to
restart Icinga 2 automatically. With Systemd it is as easy as overriding some
settings of the Icinga 2 Systemd service by creating
`/etc/systemd/system/icinga2.service.d/override.conf` with the following
content:

    [Service]
    Restart=always
    RestartSec=1
    StartLimitInterval=10
    StartLimitBurst=3

Using the watchdog can also help with monitoring Icinga 2, to activate and use it add the following to the override:

    WatchdogSec=30s

This way Systemd will kill Icinga 2 if does not notify for over 30 seconds, a timout of less than 10 seconds is not
recommended. When the watchdog is activated, `Restart=` can be set to `watchdog` to restart Icinga 2 in the case of a
watchdog timeout.

Run `systemctl daemon-reload && systemctl restart icinga2` to apply the changes.
Now Systemd will always try to restart Icinga 2 (except if you run
`systemctl stop icinga2`). After three failures in ten seconds it will stop
trying because you probably have a problem that requires manual intervention.

### Late Check Results <a id="late-check-results"></a>

[Icinga Web 2](https://icinga.com/products/icinga-web-2/) provides
a dashboard overview for `overdue checks`.

The REST API provides the [status](12-icinga2-api.md#icinga2-api-status) URL endpoint with some generic metrics
on Icinga and its features.

```
# curl -k -s -u root:icinga 'https://localhost:5665/v1/status?pretty=1' | less
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

### Notifications are not sent <a id="notifications-not-sent"></a>

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

```
# icinga2 feature enable debuglog
# systemctl restart icinga2

# grep Notification /var/log/icinga2/debug.log > /root/analyze_notification_problem.log
```

You can use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live notification streams:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -X POST 'https://localhost:5665/v1/events?queue=debugnotifications&types=Notification'
```

## Feature Troubleshooting <a id="troubleshooting-features"></a>

### Feature is not working <a id="feature-not-working"></a>

* Make sure that the feature configuration is enabled by symlinking from `features-available/`
to `features-enabled` and that the latter is included in [icinga2.conf](04-configuring-icinga-2.md#icinga2-conf).
* Are the feature attributes set correctly according to the documentation?
* Any errors on the logs?

Look up the [object type](09-object-types.md#object-types) for the required feature and verify it is enabled:

```
# icinga2 object list --type <feature object type>
```

Example for the `graphite` feature:

```
# icinga2 object list --type GraphiteWriter
```

Look into the log and check whether the feature logs anything specific for this matter.

```
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

## REST API Troubleshooting: No Objects Found <a id="troubleshooting-api-no-objects-found"></a>

Please note that the `404` status with no objects being found can also originate
from missing or too strict object permissions for the authenticated user.

This is a security feature to disable object name guessing. If this would not be the
case, restricted users would be able to get a list of names of your objects just by
trying every character combination.

In order to analyse and fix the problem, please check the following:

- use an administrative account with full permissions to check whether the objects are actually there.
- verify the permissions on the affected ApiUser object and fix them.


## Certificate Troubleshooting <a id="troubleshooting-certificate"></a>

### Certificate Verification <a id="troubleshooting-certificate-verification"></a>

If the TLS handshake fails when a client connects to the cluster or the REST API,
ensure to verify the used certificates.

Print the CA and client certificate and ensure that the following attributes are set:

* Version must be 3.
* Serial number is a hex-encoded string.
* Issuer should be your certificate authority (defaults to `Icinga CA` for all CLI commands).
* Validity, meaning to say the certificate is not expired.
* Subject with the common name (CN) matches the client endpoint name and its FQDN.
* v3 extensions must set the basic constraint for `CA:TRUE` (ca.crt) or `CA:FALSE` (client certificate).
* Subject Alternative Name is set to a proper DNS name (required for REST API and browsers).


```
# cd /var/lib/icinga2/certs/
```

CA certificate:

```
# openssl x509 -in ca.crt -text

Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 1 (0x1)
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN=Icinga CA
        Validity
            Not Before: Feb 23 14:45:32 2016 GMT
            Not After : Feb 19 14:45:32 2031 GMT
        Subject: CN=Icinga CA
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (4096 bit)
                Modulus:
...
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints: critical
                CA:TRUE
    Signature Algorithm: sha256WithRSAEncryption
...
```

Client public certificate:

```
# openssl x509 -in icinga2-client1.localdomain.crt -text

Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            86:47:44:65:49:c6:65:6b:5e:6d:4f:a5:fe:6c:76:05:0b:1a:cf:34
    Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN=Icinga CA
        Validity
            Not Before: Aug 20 16:20:05 2016 GMT
            Not After : Aug 17 16:20:05 2031 GMT
        Subject: CN=icinga2-client1.localdomain
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (4096 bit)
                Modulus:
...
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints: critical
                CA:FALSE
            X509v3 Subject Alternative Name:
                DNS:icinga2-client1.localdomain
    Signature Algorithm: sha256WithRSAEncryption
...
```

Make sure to verify the client's certificate and its received `ca.crt` in `/var/lib/icinga2/certs` and ensure that
both instances are signed by the **same CA**.

```
# openssl verify -verbose -CAfile /var/lib/icinga2/certs/ca.crt /var/lib/icinga2/certs/icinga2-master1.localdomain.crt
icinga2-master1.localdomain.crt: OK

# openssl verify -verbose -CAfile /var/lib/icinga2/certs/ca.crt /var/lib/icinga2/certs/icinga2-client1.localdomain.crt
icinga2-client1.localdomain.crt: OK
```

Fetch the `ca.crt` file from the client node and compare it to your master's `ca.crt` file:

```
# scp icinga2-client1:/var/lib/icinga2/certs/ca.crt test-client-ca.crt
# diff -ur /var/lib/icinga2/certs/ca.crt test-client-ca.crt
```

On SLES11 you'll need to use the `openssl1` command instead of `openssl`.

<!--
### Certificate Signing <a id="troubleshooting-certificate-signing"></a>
-->


### Certificate Problems with OpenSSL 1.1.0 <a id="troubleshooting-certificate-openssl-1-1-0"></a>

Users have reported problems with SSL certificates inside a distributed monitoring setup when they

* updated their Icinga 2 package to 2.7.0 on Windows or
* upgraded their distribution which included an update to OpenSSL 1.1.0.

Example during startup on a Windows client:

```
critical/SSL: Error loading and verifying locations in ca key file 'C:\ProgramData\icinga2\etc/icinga2/pki/ca.crt': 219029726, "error:0D0E20DE:asn1 encoding routines:c2i_ibuf:illegal zero content"
critical/config: Error: Cannot make SSL context for cert path: 'C:\ProgramData\icinga2\etc/icinga2/pki/client.crt' key path: 'C:\ProgramData\icinga2\etc/icinga2/pki/client.key' ca path: 'C:\ProgramData\icinga2\etc/icinga2/pki/ca.crt'.
```

A technical analysis and solution for re-creating the public CA certificate is
available in [this advisory](https://icinga.com/2017/08/30/advisory-for-ssl-problems-with-leading-zeros-on-openssl-1-1-0/).


## Cluster and Clients Troubleshooting <a id="troubleshooting-cluster"></a>

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

```
# tcpdump -n port 5665 -i any

# netstat -tulpen | grep icinga

# nmap icinga2-client1.localdomain
```

### Cluster Troubleshooting SSL Errors <a id="troubleshooting-cluster-ssl-errors"></a>

If the cluster communication fails with SSL error messages, make sure to check
the following

* File permissions on the SSL certificate files
* Does the used CA match for all cluster endpoints?
  * Verify the `Issuer` being your trusted CA
  * Verify the `Subject` containing your endpoint's common name (CN)
  * Check the validity of the certificate itself

Try to manually connect from `icinga2-client1.localdomain` to the master node `icinga2-master1.localdomain`:

```
# openssl s_client -CAfile /var/lib/icinga2/certs/ca.crt -cert /var/lib/icinga2/certs/icinga2-client1.localdomain.crt -key /var/lib/icinga2/certs/icinga2-client1.localdomain.key -connect icinga2-master1.localdomain:5665

CONNECTED(00000003)
---
...
```

If the connection attempt fails or your CA does not match, [verify the certificates](15-troubleshooting.md#troubleshooting-certificate-verification).


#### Cluster Troubleshooting Unauthenticated Clients <a id="troubleshooting-cluster-unauthenticated-clients"></a>

Unauthenticated nodes are able to connect. This is required for client setups.

Master:

```
[2015-07-13 18:29:25 +0200] information/ApiListener: New client connection for identity 'icinga2-client1.localdomain' (unauthenticated)
```

Client as command execution bridge:

```
[2015-07-13 18:29:26 +1000] notice/ClusterEvents: Discarding 'execute command' message from 'icinga2-master1.localdomain': Invalid endpoint origin (client not allowed).
```

If these messages do not go away, make sure to [verify the master and client certificates](15-troubleshooting.md#troubleshooting-certificate-verification).

### Cluster Troubleshooting Message Errors <a id="troubleshooting-cluster-message-errors"></a>

When the network connection is broken or gone, the Icinga 2 instances will be disconnected.
If the connection can't be re-established between endpoints in the same HA zone,
they remain in a Split-Brain-mode and history may differ.

Although the Icinga 2 cluster protocol stores historical events in a [replay log](15-troubleshooting.md#troubleshooting-cluster-replay-log)
for later synchronisation, you should make sure to check why the network connection failed.

Ensure to setup [cluster health checks](06-distributed-monitoring.md#distributed-monitoring-health-checks)
to monitor all endpoints and zones connectivity.

### Cluster Troubleshooting Command Endpoint Errors <a id="troubleshooting-cluster-command-endpoint-errors"></a>

Command endpoints can be used [for clients](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint)
as well as inside an [High-Availability cluster](06-distributed-monitoring.md#distributed-monitoring-scenarios).

There is no cli command for manually executing the check, but you can verify
the following (e.g. by invoking a forced check from the web interface):

* `/var/log/icinga2/icinga2.log` contains connection and execution errors.
 * The ApiListener is not enabled to [accept commands](06-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint).
 * `CheckCommand` definition not found on the remote client.
 * Referenced check plugin not found on the remote client.
 * Runtime warnings and errors, e.g. unresolved runtime macros or configuration problems.
* Specific error messages are also populated into `UNKNOWN` check results including a detailed error message in their output.
* Verify the `check_source` object attribute. This is populated by the node executing the check.
* More verbose logs are found inside the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output).

* Use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live check result streams.

Fetch all check result events matching the `event.service` name `remote-client`:

```
$ curl -k -s -u root:icinga -H 'Accept: application/json' -X POST 'https://localhost:5665/v1/events?queue=debugcommandendpoint&types=CheckResult&filter=match%28%22remote-client*%22,event.service%29'
```



### Cluster Troubleshooting Config Sync <a id="troubleshooting-cluster-config-sync"></a>

If the cluster zones do not sync their configuration, make sure to check the following:

* Within a config master zone, only one configuration master is allowed to have its config in `/etc/icinga2/zones.d`.
** The master syncs the configuration to `/var/lib/icinga2/api/zones/` during startup and only syncs valid configuration to the other nodes.
** The other nodes receive the configuration into `/var/lib/icinga2/api/zones/`.
* The `icinga2.log` log file in `/var/log/icinga2` will indicate whether this ApiListener
[accepts config](06-distributed-monitoring.md#distributed-monitoring-top-down-config-sync), or not.

Verify the object's [version](09-object-types.md#object-types) attribute on all nodes to
check whether the config update and reload was successful or not.


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
> [Icinga Web 2](02-getting-started.md#setting-up-icingaweb2) provides a dashboard view
> for overdue check results.

Enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) on the master
for more verbose insights.

If the client cannot authenticate, it's a more general [problem](15-troubleshooting.md#troubleshooting-cluster-unauthenticated-clients).

The client's endpoint is not configured on nor trusted by the master node:

```
Discarding 'check result' message from 'icinga2-client1.localdomain': Invalid endpoint origin (client not allowed).
```

The check result message sent by the client does not belong to the zone the checkable object is
in on the master:

```
Discarding 'check result' message from 'icinga2-client1.localdomain': Unauthorized access.
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
