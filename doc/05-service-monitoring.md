# Service Monitoring <a id="service-monitoring"></a>

The power of Icinga 2 lies in its modularity. There are thousands of
community plugins available next to the standard plugins provided by
the [Monitoring Plugins project](https://www.monitoring-plugins.org).

## Requirements <a id="service-monitoring-requirements"></a>

### Plugins <a id="service-monitoring-plugins"></a>

All existing Nagios or Icinga 1.x plugins work with Icinga 2. Community
plugins can be found for example on [Icinga Exchange](https://exchange.icinga.com).

The recommended way of setting up these plugins is to copy them to a common directory
and create a new global constant, e.g. `CustomPluginDir` in your [constants.conf](04-configuring-icinga-2.md#constants-conf)
configuration file:

```
# cp check_snmp_int.pl /opt/monitoring/plugins
# chmod +x /opt/monitoring/plugins/check_snmp_int.pl

# cat /etc/icinga2/constants.conf
/**
 * This file defines global constants which can be used in
 * the other configuration files. At a minimum the
 * PluginDir constant should be defined.
 */

const PluginDir = "/usr/lib/nagios/plugins"
const CustomPluginDir = "/opt/monitoring/plugins"
```

Prior to using the check plugin with Icinga 2 you should ensure that it is working properly
by trying to run it on the console using whichever user Icinga 2 is running as:

```
# su - icinga -s /bin/bash
$ /opt/monitoring/plugins/check_snmp_int.pl --help
```

Additional libraries may be required for some plugins. Please consult the plugin
documentation and/or the included README file for installation instructions.
Sometimes plugins contain hard-coded paths to other components. Instead of changing
the plugin it might be easier to create a symbolic link to make sure it doesn't get overwritten during the next update.

Sometimes there are plugins which do not exactly fit your requirements.
In that case you can modify an existing plugin or just write your own.

### CheckCommand Definition <a id="service-monitoring-plugin-checkcommand"></a>

Each plugin requires a [CheckCommand](09-object-types.md#objecttype-checkcommand) object in your
configuration which can be used in the [Service](09-object-types.md#objecttype-service) or
[Host](09-object-types.md#objecttype-host) object definition.

Please check if the Icinga 2 package already provides an
[existing CheckCommand definition](10-icinga-template-library.md#icinga-template-library).
If that's the case, throroughly check the required parameters and integrate the check command
into your host and service objects.

Please make sure to follow these conventions when adding a new command object definition:

* Use [command arguments](03-monitoring-basics.md#command-arguments) whenever possible. The `command` attribute
must be an array in `[ ... ]` for shell escaping.
* Define a unique `prefix` for the command's specific arguments. That way you can safely
set them on host/service level and you'll always know which command they control.
* Use command argument default values, e.g. for thresholds.
* Use [advanced conditions](09-object-types.md#objecttype-checkcommand) like `set_if` definitions.

This is an example for a custom `my-snmp-int` check command:

```
object CheckCommand "my-snmp-int" {
  command = [ CustomPluginDir + "/check_snmp_int.pl" ]

  arguments = {
    "-H" = "$snmp_address$"
    "-C" = "$snmp_community$"
    "-p" = "$snmp_port$"
    "-2" = {
      set_if = "$snmp_v2$"
    }
    "-n" = "$snmp_interface$"
    "-f" = {
      set_if = "$snmp_perf$"
    }
    "-w" = "$snmp_warn$"
    "-c" = "$snmp_crit$"
  }

  vars.snmp_v2 = true
  vars.snmp_perf = true
  vars.snmp_warn = "300,400"
  vars.snmp_crit = "0,600"
}
```

For further information on your monitoring configuration read the
[Monitoring Basics](03-monitoring-basics.md#monitoring-basics) chapter.

If you have created your own `CheckCommand` definition, please kindly
[send it upstream](https://github.com/Icinga/icinga2/blob/master/CONTRIBUTING.md).

### Plugin API <a id="service-monitoring-plugin-api"></a>

Currently Icinga 2 supports the native plugin API specification from the Monitoring Plugins project. It is defined in the [Monitoring Plugins Development Guidelines](https://www.monitoring-plugins.org/doc/guidelines.html).

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

Common best practices when creating a new plugin are for example:

* Choose the programming language wisely
 * Scripting languages (Bash, Python, Perl, Ruby, PHP, etc.) are easier to write and setup but their check execution might take longer (invoking the script interpreter as overhead, etc.).
 * Plugins written in C/C++, Go, etc. improve check execution time but may generate an overhead with installation and packaging.
* Use a modern VCS such as Git for developing the plugin (e.g. share your plugin on GitHub).
* Add parameters with key-value pairs to your plugin. They should allow long names (e.g. `--host localhost`) and also short parameters (e.g. `-H localhost`)
 * `-h|--help` should print the version and all details about parameters and runtime invocation.
* Add a verbose/debug output functionality for detailed on-demand logging.
* Respect the exit codes required by the [Plugin API](05-service-monitoring.md#service-monitoring-plugin-api).
* Always add performance data to your plugin output

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
* performance: [iostat](10-icinga-template-library.md#plugin-contrib-command-iostat), [check_sar_perf](https://github.com/dnsmichi/icinga-plugins/blob/master/scripts/check_sar_perf.py)

### Windows Monitoring <a id="service-monitoring-windows"></a>

* [check_wmi_plus](http://www.edcint.co.nz/checkwmiplus/)
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
* [check_logstash](https://github.com/widhalmt/check_logstash)
* [check_graylog2_stream](https://github.com/Graylog2/check-graylog2-stream)

### Virtualization Monitoring <a id="service-monitoring-virtualization"></a>

### VMware Monitoring <a id="service-monitoring-virtualization-vmware"></a>

* [esxi_hardware](10-icinga-template-library.md#plugin-contrib-command-esxi-hardware)
* [VMware](10-icinga-template-library.md#plugin-contrib-vmware)

**Tip**: If you are encountering timeouts using the VMware Perl SDK,
check [this blog entry](https://www.claudiokuenzler.com/blog/650/slow-vmware-perl-sdk-soap-request-error-libwww-version).
Ubuntu 16.04 LTS can have troubles with random entropy in Perl asked [here](https://monitoring-portal.org/t/check-vmware-api-slow-when-run-multiple-times/2868).
In that case, [haveged](http://issihosts.com/haveged/) may help.

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
