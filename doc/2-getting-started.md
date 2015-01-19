# <a id="getting-started"></a> Getting Started

This tutorial is a step-by-step introduction to installing Icinga 2 and
available Icinga web interfaces. It assumes that you are familiar with
the system you're installing Icinga 2 on.

Details on troubleshooting problems can be found [here](#troubleshooting).

## <a id="setting-up-icinga2"></a> Setting up Icinga 2

First off you will have to install Icinga 2. The preferred way of doing this
is to use the official package repositories depending on which operating system
and distribution you are running.

  Distribution            | Repository
  ------------------------|---------------------------
  Debian                  | [Upstream](https://packages.debian.org/sid/icinga2), [DebMon](http://debmon.org/packages/debmon-wheezy/icinga2), [Icinga Repository](http://packages.icinga.org/debian/)
  Ubuntu                  | [Upstream](https://launchpad.net/ubuntu/+source/icinga2), [Icinga PPA](https://launchpad.net/~formorer/+archive/ubuntu/icinga), [Icinga Repository](http://packages.icinga.org/ubuntu/)
  RHEL/CentOS             | [Icinga Repository](http://packages.icinga.org/epel/)
  OpenSUSE                | [Icinga Repository](http://packages.icinga.org/openSUSE/), [Server Monitoring Repository](https://build.opensuse.org/package/show/server:monitoring/icinga2)
  SLES                    | [Icinga Repository](http://packages.icinga.org/SUSE/)
  Gentoo                  | [Upstream](http://packages.gentoo.org/package/net-analyzer/icinga2)
  FreeBSD                 | [Upstream](http://www.freshports.org/net-mgmt/icinga2)
  ArchLinux               | [Upstream](https://aur.archlinux.org/packages/icinga2)

Packages for distributions other than the ones listed above may also be
available. Please contact your distribution packagers.

### <a id="installing-requirements"></a> Installing Requirements for Icinga 2

You need to add the Icinga repository to your package management configuration.
Below is a list with examples for the various distributions.

Debian (debmon):

    # wget -O - http://debmon.org/debmon/repo.key 2>/dev/null | apt-key add -
    # cat >/etc/apt/sources.list.d/debmon.list<<EOF
    deb http://debmon.org/debmon debmon-wheezy main
    EOF
    # apt-get update

Ubuntu (PPA):

    # add-apt-repository ppa:formorer/icinga
    # apt-get update

RHEL/CentOS:

    # rpm --import http://packages.icinga.org/icinga.key
    # wget http://packages.icinga.org/epel/ICINGA-release.repo -O /etc/yum.repos.d/ICINGA-release.repo
    # yum makecache

Fedora:

    # wget http://packages.icinga.org/fedora/ICINGA-release.repo -O /etc/yum.repos.d/ICINGA-release.repo
    # yum makecache

SLES:

    # zypper ar http://packages.icinga.org/SUSE/ICINGA-release.repo
    # zypper ref

OpenSUSE:

    # zypper ar http://packages.icinga.org/openSUSE/ICINGA-release.repo
    # zypper ref

The packages for RHEL/CentOS depend on other packages which are distributed
as part of the [EPEL repository](http://fedoraproject.org/wiki/EPEL). Please
make sure to enable this repository by following
[these instructions](#http://fedoraproject.org/wiki/EPEL#How_can_I_use_these_extra_packages.3F).

### <a id="installing-icinga2"></a> Installing Icinga 2

You can install Icinga 2 by using your distribution's package manager
to install the `icinga2` package.

Debian/Ubuntu:

    # apt-get install icinga2

RHEL/CentOS/Fedora:

    # yum install icinga2

SLES/OpenSUSE:

    # zypper install icinga2

On RHEL/CentOS and SLES you will need to use `chkconfig` to enable the
`icinga2` service. You can manually start Icinga 2 using `service icinga2 start`.

    # chkconfig icinga2 on
    # service icinga2 start

RHEL/CentOS 7 use [Systemd](#systemd-service) with `systemctl {enable,start} icinga2`.

    # systemctl enable icinga2
    # systemctl start icinga2

Some parts of Icinga 2's functionality are available as separate packages:

  Name                    | Description
  ------------------------|--------------------------------
  icinga2-ido-mysql       | [DB IDO](#configuring-db-ido) provider module for MySQL
  icinga2-ido-pgsql       | [DB IDO](#configuring-db-ido) provider module for PostgreSQL

If you're running a distribution for which Icinga 2 packages are
not yet available you will need to use the release tarball which you
can download from the [Icinga website](https://www.icinga.org/). The
release tarballs contain an `INSTALL` file with further instructions.

### <a id="installation-enabled-features"></a> Enabled Features during Installation

The default installation will enable three features required for a basic
Icinga 2 installation:

* `checker` for executing checks
* `notification` for sending notifications
* `mainlog` for writing the `icinga2.log` file

You can verify that by calling `icinga2 feature list` [cli command](#cli-command-feature)
to see which features are enabled and disabled.

    # icinga2 feature list
    Disabled features: api command compatlog debuglog graphite icingastatus ido-mysql ido-pgsql livestatus notification perfdata statusdata syslog
    Enabled features: checker mainlog notification


### <a id="installation-paths"></a> Installation Paths

By default Icinga 2 uses the following files and directories:

  Path                                | Description
  ------------------------------------|------------------------------------
  /etc/icinga2                        | Contains Icinga 2 configuration files.
  /etc/init.d/icinga2                 | The Icinga 2 init script.
  /usr/sbin/icinga2*                  | The Icinga 2 binary.
  /usr/share/doc/icinga2              | Documentation files that come with Icinga 2.
  /usr/share/icinga2/include          | The Icinga Template Library and plugin command configuration.
  /var/run/icinga2                    | PID file.
  /var/run/icinga2/cmd                | Command pipe and Livestatus socket.
  /var/cache/icinga2                  | status.dat/objects.cache, icinga2.debug files
  /var/spool/icinga2                  | Used for performance data spool files.
  /var/lib/icinga2                    | Icinga 2 state file, cluster log, local CA and configuration files.
  /var/log/icinga2                    | Log file location and compat/ directory for the CompatLogger feature.

## <a id="setting-up-check-plugins"></a> Setting up Check Plugins

Without plugins Icinga 2 does not know how to check external services. The
[Monitoring Plugins Project](https://www.monitoring-plugins.org/) provides
an extensive set of plugins which can be used with Icinga 2 to check whether
services are working properly.

The recommended way of installing these standard plugins is to use your
distribution's package manager.

> **Note**
>
> The `Nagios Plugins` project was renamed to `Monitoring Plugins`
> in January 2014. At the time of this writing some packages are still
> using the old name while some distributions have adopted the new package
> name `monitoring-plugins` already.

> **Note**
>
> EPEL for RHEL/CentOS 7 is still in beta mode at the time of writing and does
> not provide a `monitoring-plugins` package. You are required to manually install
> them.

For your convenience here is a list of package names for some of the more
popular operating systems/distributions:

OS/Distribution        | Package Name       | Installation Path
-----------------------|--------------------|---------------------------
RHEL/CentOS (EPEL)     | nagios-plugins-all | /usr/lib/nagios/plugins or /usr/lib64/nagios/plugins
Debian                 | nagios-plugins     | /usr/lib/nagios/plugins
FreeBSD                | nagios-plugins     | /usr/local/libexec/nagios
OS X (MacPorts)        | nagios-plugins     | /opt/local/libexec

Depending on which directory your plugins are installed into you may need to
update the global `PluginDir` constant in your Icinga 2 configuration. This macro is used
by the service templates contained in the Icinga Template Library to determine
where to find the plugin binaries.

### <a id="integrate-additional-plugins"></a> Integrate Additional Plugins

For some services you may need additional 'check plugins' which are not provided
by the official Monitoring Plugins project.

All existing Nagios or Icinga 1.x plugins work with Icinga 2. Here's a
list of popular community sites which host check plugins:

* [Icinga Exchange](https://exchange.icinga.org)
* [Icinga Wiki](https://wiki.icinga.org)

The recommended way of setting up these plugins is to copy them to a common directory
and create an extra global constant, e.g. `CustomPluginDir` in your [constants.conf](#constants-conf)
configuration file:

    # cp check_snmp_int.pl /opt/plugins
    # chmod +x /opt/plugins/check_snmp_int.pl

    # cat /etc/icinga2/constants.conf
    /**
     * This file defines global constants which can be used in
     * the other configuration files. At a minimum the
     * PluginDir constant should be defined.
     */

    const PluginDir = "/usr/lib/nagios/plugins"
    const CustomPluginDir = "/opt/monitoring"

Prior to using the check plugin with Icinga 2 you should ensure that it is working properly
by trying to run it on the console using whichever user Icinga 2 is running as:

    # su - icinga -s /bin/bash
    $ /opt/plugins/check_snmp_int.pl --help

Additional libraries may be required for some plugins. Please consult the plugin
documentation and/or plugin provided README for installation instructions.
Sometimes plugins contain hard-coded paths to other components. Instead of changing
the plugin it might be easier to create logical links which is (more) update-safe.

Each plugin requires a [CheckCommand](#objecttype-checkcommand) object in your
configuration which can be used in the [Service](#objecttype-service) or
[Host](#objecttype-host) object definition.

There are the following conventions to follow when adding a new command object definition:

* Always import the `plugin-check-command` template
* Use [command-arguments](#) whenever possible. The `command` attribute must be an array
in `[ ... ]` then for shell escaping.
* Define a unique `prefix` for the command's specific command arguments. That way you can safely
set them on host/service level and you'll always know which command they control.
* Use command argument default values, e.g. for thresholds
* Use [advanced conditions](#objecttype-checkcommand) like `set_if` definitions.

Example for a custom `my-snmp-int` check command:

    object CheckCommand "my-snmp-int" {
      import "plugin-check-command"

      command = [ PluginDir + "/check_snmp_int.pl" ]

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

You can find an existing `CheckCommand` definition for the `check_snmp_int.pl` plugin
shipped with the optional [Manubulon Plugin Check Command](#snmp-manubulon-plugin-check-commands)
definitions already.


For further information on your monitoring configuration read the
[monitoring basics](#monitoring-basics).


## <a id="configuring-icinga2-first-steps"></a> Configuring Icinga 2: First Steps

### <a id="icinga2-conf"></a> icinga2.conf

An example configuration file is installed for you in `/etc/icinga2/icinga2.conf`.

Here's a brief description of the example configuration:

    /**
     * Icinga 2 configuration file
     * - this is where you define settings for the Icinga application including
     * which hosts/services to check.
     *
     * For an overview of all available configuration options please refer
     * to the documentation that is distributed as part of Icinga 2.
     */

Icinga 2 supports [C/C++-style comments](#comments).

    /**
     * The constants.conf defines global constants.
     */
    include "constants.conf"

The `include` directive can be used to include other files.

    /**
     * The zones.conf defines zones for a cluster setup.
     * Not required for single instance setups.
     */
     include "zones.conf"

    /**
     * The Icinga Template Library (ITL) provides a number of useful templates
     * and command definitions.
     * Common monitoring plugin command definitions are included separately.
     */
    include <itl>
    include <plugins>

    /**
     * The features-available directory contains a number of configuration
     * files for features which can be enabled and disabled using the
     * icinga2 feature enable / icinga2 feature disable CLI commands.
     * These commands work by creating and removing symbolic links in
     * the features-enabled directory.
     */
    include "features-enabled/*.conf"

This `include` directive takes care of including the configuration files for all
the features which have been enabled with `icinga2 feature enable`. See
[Enabling/Disabling Features](#features) for more details.

    /**
     * The repository.d directory contains all configuration objects
     * managed by the 'icinga2 repository' CLI commands.
     */
    include_recursive "repository.d"

This `include_recursive` directive is used for discovery of services on remote clients
and their generated configuration described in
[this chapter](#icinga2-remote-monitoring-master-discovery-generate-config).


    /**
     * Although in theory you could define all your objects in this file
     * the preferred way is to create separate directories and files in the conf.d
     * directory. Each of these files must have the file extension ".conf".
     */
    include_recursive "conf.d"

You can put your own configuration files in the [conf.d](#conf-d) directory. This
directive makes sure that all of your own configuration files are included.

> **Tip**
>
> The example configuration is shipped in this directory too. You can either
> remove it entirely, or adapt the existing configuration structure with your
> own object configuration.

### <a id="init-conf"></a> init.conf

This initialization configuration file is automatically included by Icinga 2. It
defines the daemon user and group [constants](#constants) `RunAsUser` and
`RunAsGroup`.



### <a id="constants-conf"></a> constants.conf

The `constants.conf` configuration file can be used to define global constants.

By default, you need to make sure to set these constants:

* The `PluginDir` constant must be pointed to your [check plugins](#setting-up-check-plugins) path.
This constant is required by the shipped
[plugin check command configuration](#plugin-check-commands).
* The `NodeName` constant defines your local node name. Should be set to FQDN which is the default
if not set. This constant is required for local host configuration, monitoring remote clients and
cluster setup.

Example:

    /* The directory which contains the plugins from the Monitoring Plugins project. */
    const PluginDir = "/usr/lib64/nagios/plugins"


    /* The directory which contains the Manubulon plugins.
     * Check the documentation, chapter "SNMP Manubulon Plugin Check Commands", for details.
     */
    const ManubulonPluginDir = "/usr/lib64/nagios/plugins"

    /* Our local instance name. By default this is the server's hostname as returned by `hostname --fqdn`.
     * This should be the common name from the API certificate.
     */
    //const NodeName = "localhost"

    /* Our local zone name. */
    const ZoneName = NodeName

    /* Secret key for remote node tickets */
    const TicketSalt = ""

The `ZoneName` and `TicketSalt` constants are required for remote client
and distributed setups only.


### <a id="zones-conf"></a> zones.conf

The `zones.conf` configuration file can be used to configure `Endpoint` and `Zone` objects
required for a [distributed zone setup](#distributed-monitoring-high-availability). By default
a local dummy zone is defined based on the `NodeName` constant defined in
[constants.conf](#constants-conf).

> **Note**
>
> Not required for single instance installations.


### <a id="conf-d"></a> Configuration in conf.d Directory

This directory contains example configuration getting your started with monitoring
the local host and its services. It is included in [icinga2.conf](#icinga2-conf)
in the default installation.

It can be used as reference example for your own configuration strategy.
Just keep in mind to include the main directories in the
[icinga2.conf](#icinga2-conf) file.

You are certainly not bound to it. Remove it, if you prefer your own
way of deploying Icinga 2 configuration.

Further details on configuration best practice and how to build your
own strategy is described in [this chapter](#configuration-best-practice).

Available configuration files shipped by default:

* [hosts.conf](#hosts-conf)
* [services.conf](#services-conf)
* [users.conf](#users-conf)
* [notifications.conf](#notifications-conf)
* [commands.conf](#commands-conf)
* [groups.conf](#groups-conf)
* [templates.conf](#templates-conf)
* [downtimes.conf](#downtimes-conf)
* [timeperiods.conf](#timeperiods-conf)
* [satellite.conf](#satellite-conf)

#### <a id="hosts-conf"></a> hosts.conf

The `hosts.conf` file contains an example host based on your
`NodeName` setting in [constants.conf](#constants-conf). You
can use global constants for your object names instead of string
values.

The `import` keyword is used to import the `generic-host` template which
takes care of setting up the host check command to `hostalive`. If you
require a different check command, you can override it in the object definition.

The `vars` attribute can be used to define custom attributes which are available
for check and notification commands. Most of the [Plugin Check Commands](#plugin-check-commands)
in the Icinga Template Library require an `address` attribute.

The custom attribute `os` is evaluated by the `linux-servers` group in
[groups.conf](#groups-conf) making the local host a member.

The example host will show you how to

* define http vhost attributes for the `http` service apply rule defined
in [services.conf](#services.conf).
* define disks (all, specific `/`) and their attributes for the `disk`
service apply rule defined in [services.conf](#services.conf).
* define notification types (`mail`) and set the groups attribute. This
will be used by notification apply rules in [notifications.conf](notifications-conf).

If you've installed [Icinga Web 2](#setting-up-icingaweb2) you can
uncomment the http vhost attributes and relaod Icinga 2. The apply
rules in [services.conf](#services.conf) will automatically
generate a new service checking the `/icingaweb2` URI using the `http`
check.

    /*
     * Host definitions with object attributes
     * used for apply rules for Service, Notification,
     * Dependency and ScheduledDowntime objects.
     *
     * Tip: Use `icinga2 object list --type Host` to
     * list all host objects after running
     * configuration validation (`icinga2 daemon -C`).
     */

    /*
     * This is an example host based on your
     * local host's FQDN. Specify the NodeName
     * constant in `constants.conf` or use your
     * own description, e.g. "db-host-1".
     */

    object Host NodeName {
      /* Import the default host template defined in `templates.conf`. */
      import "generic-host"

      /* Specify the address attributes for checks e.g. `ssh` or `http`. */
      address = "127.0.0.1"
      address6 = "::1"

      /* Set custom attribute `os` for hostgroup assignment in `groups.conf`. */
      vars.os = "Linux"

      /* Define http vhost attributes for service apply rules in `services.conf`. */
      vars.http_vhosts["http"] = {
        http_uri = "/"
      }
      /* Uncomment if you've sucessfully installed Icinga Web 2. */
      //vars.http_vhosts["Icinga Web 2"] = {
      //  http_uri = "/icingaweb2"
      //}

      /* Define disks and attributes for service apply rules in `services.conf`. */
      vars.disks["disk"] = {
        /* No parameters. */
      }
      vars.disks["disk /"] = {
        disk_partitions = "/"
      }

      /* Define notification mail attributes for notification apply rules in `notifications.conf`. */
      vars.notification["mail"] = {
        /* The UserGroup `icingaadmins` is defined in `users.conf`. */
        groups = [ "icingaadmins" ]
      }
    }

This is only the host object definition. Now we'll need to make sure that this
host and your additional hosts are getting [services](#services-conf) applied.

> **Tip**
>
> If you don't understand all the attributes and how to use [apply rules](#apply)
> don't worry - the [monitoring basics](#monitoring-basics) chapter will explain
> that in detail.

#### <a id="services-conf"></a> services.conf

These service [apply rules](#apply) will show you how to monitor
the local host, but also allow you to re-use or modify them for
your own requirements.

You should define all your service apply rules in `services.conf`
or any other central location keeping them organized.

By default, the local host will be monitored by the following services

Service(s)                                  | Applied on host(s)
--------------------------------------------|------------------------
`load`, `procs`, `swap`, `users`, `icinga`  | The `NodeName` host only
`ping4`, `ping6`                            | All hosts with `address` resp. `address6` attribute
`ssh`                                       | All hosts with `address` and `vars.os` set to `Linux`
`http`, optional: `Icinga Web 2`            | All hosts with custom attribute `http_vhosts` defined as dictionary
`disk`, `disk /`                            | All hosts with custom attribute `disks` defined as dictionary

The Debian packages also ship an additional `apt` service check applied to the local host.

The command object `icinga` for the embedded health check is provided by the
[Icinga Template Library (ITL)](#itl) while `http_ip`, `ssh`, `load`, `processes`,
`users` and `disk` are all provided by the [Plugin Check Commands](#plugin-check-commands)
which we enabled earlier by including the `itl` and `plugins` configuration file.


Example `load` service apply rule:

    apply Service "load" {
      import "generic-service"

      check_command = "load"

      /* Used by the ScheduledDowntime apply rule in `downtimes.conf`. */
      vars.backup_downtime = "02:00-03:00"

      assign where host.name == NodeName
    }

The `apply` keyword can be used to create new objects which are associated with
another group of objects. You can `import` existing templates, define (custom)
attributes.

The custom attribe `backup_downtime` is defined to a specific timerange string.
This variable value will be used for applying a `ScheduledDowntime` object to
these services in [downtimes.conf](#downtimes-conf).

In this example the `assign where` condition is a boolean expression which is
evaluated for all objects of type `Host` and a new service with name "load"
is created for each matching host. [Expression operators](#expression-operators)
may be used in `assign where` conditions.

Multiple `assign where` condition can be combined with `AND` using the `&&` operator
as shown in the `ssh` example:

    apply Service "ssh" {
      import "generic-service"

      check_command = "ssh"

      assign where host.address && host.vars.os == "Linux"
    }

In this example, the service `ssh` is applied to all hosts having the `address`
attribute defined `AND` having the custom attribute `os` set to the string
`Linux`.
You can modify this condition to match multiple expressions by combinding `AND`
and `OR` using `&&` and `||` [operators](#expression-operators), for example
`assign where host.address && (vars.os == "Linux" || vars.os == "Windows")`.


A more advanced example is shown by the `http` and `disk` service apply
rules. While one `apply` rule for `ssh` will only create a service for matching
hosts, you can go one step further: Generate apply rules based on array items
or dictionary key-value pairs.

The idea is simple: Your host in [hosts.conf](#hosts-conf) defines the
`disks` dictionary as custom attribute in `vars`.

Remember the example from [hosts.conf](#hosts-conf):

    ...

      /* Define disks and attributes for service apply rules in `services.conf`. */
      vars.disks["disk"] = {
        /* No parameters. */
      }
      vars.disks["disk /"] = {
        disk_partition = "/"
      }
    ...


This dictionary contains multiple service names we want to monitor. `disk`
should just check all available disks, while `disk /` will pass an additional
parameter `disk_partition` to the check command.

You'll recognize that the naming is important - that's the very same name
as it is passed from a service to a check command argument. Read about services
and passing check commands in [this chapter](#command-passing-parameters).

Using `apply Service for` omits the service name, it will take the key stored in
the `disk` variable in `key => config` as new service object name.

The `for` keyword expects a loop definition, for example `key => value in dictionary`
as known from Perl and other scripting languages.

Once defined like this, the `apply` rule defined below will do the following:

* only match hosts with `host.vars.disks` defined through the `assign where` condition
* loop through all entries in the `host.vars.disks` dictionary. That's `disk` and `disk /` as keys.
* call `apply` on each, and set the service object name from the provided key
* inside apply, the `generic-service` template is imported
* defining the [disk](#plugin-check-command-disk) check command requiring command arguments like `disk_partition`
* adding the `config` dictionary items to `vars`. Simply said, there's now `vars.disk_partition` defined for the
generated service

Configuration example:

    apply Service for (disk => config in host.vars.disks) {
      import "generic-service"

      check_command = "disk"

      vars += config

      assign where host.vars.disks
    }

A similar example is used for the `http` services. That way you can make your
host the information provider for all apply rules. Define them once, and only
manage your hosts.

Look into [notifications.conf](#notifications-conf) how this technique is used
for applying notifications to hosts and services using their type and user
attributes.

Don't forget to install the [check plugins](#setting-up-check-plugins) required by
the hosts and services and their check commands.

Further details on the monitoring configuration can be found in the
[monitoring basics](#monitoring-basics) chapter.

#### <a id="users-conf"></a> users.conf

Defines the `icingaadmin` User and the `icingaadmins` UserGroup. The latter is used in
[hosts.conf](#hostss-conf) for defining a custom host attribute later used in
[notifications.conf](#notifications-conf) for notification apply rules.

    object User "icingaadmin" {
      import "generic-user"

      display_name = "Icinga 2 Admin"
      groups = [ "icingaadmins" ]

      email = "icinga@localhost"
    }

    object UserGroup "icingaadmins" {
      display_name = "Icinga 2 Admin Group"
    }


#### <a id="notifications-conf"></a> notifications.conf

Notifications for check alerts are an integral part or your
Icinga 2 monitoring stack.

The shipped example defines two notification apply rules for hosts and services.
Both `apply` rules match on the same condition: They are only applied if the
nested dictionary attribute `notification.mail` is set.

Please note that the `to` keyword is important in [notification apply rules](#using-apply-notifications)
defining whether these notifications are applies to hosts or services.
The `import` keyword imports the specific mail templates defined in [templates.conf](#templates-conf).

The `interval` attribute is not explicitly set - it [defaults to 30 minutes](#objecttype-notification).

By setting the `user_groups` to the value provided by the
respective [host.vars.notification.mail](#hosts-conf) attribute we'll
implicitely use the`icingaadmins` UserGroup defined in [users.conf](#users.conf).

    apply Notification "mail-icingaadmin" to Host {
      import "mail-host-notification"

      user_groups = host.vars.notification.mail.groups

      assign where host.vars.notification.mail
    }

    apply Notification "mail-icingaadmin" to Service {
      import "mail-service-notification"

      user_groups = host.vars.notification.mail.groups

      assign where host.vars.notification.mail
    }

More details on defining notifications and their additional attributes such as
filters can be read in [this chapter](#notifications).

#### <a id="commands-conf"></a> commands.conf

This is the place where your own command configuration can be defined. By default
only the notification commands used by the notification templates defined in [templates.conf](#templates-conf).

> **Tip**
>
> Icinga 2 ships the most common command definitions already in the
> [Plugin Check Commands](#plugin-check-commands) definitions. More details on
> that topic in the [troubleshooting section](#check-command-definitions).

You can freely customize these notification commands, and adapt them for your needs.
Read more on that topic [here](#notification-commands).

#### <a id="groups-conf"></a> groups.conf

The example host defined in [hosts.conf](hosts-conf) already has the
custom attribute `os` set to `Linux` and is therefore automatically
a member of the host group `linux-servers`.

This is done by using the [group assign](#group-assign) expressions similar
to previously seen [apply rules](#using-apply).

    object HostGroup "linux-servers" {
      display_name = "Linux Servers"

      assign where host.vars.os == "Linux"
    }

    object HostGroup "windows-servers" {
      display_name = "Windows Servers"

      assign where host.vars.os == "Windows"
    }

Service groups can be grouped together by similar pattern matches.
The [match() function](#function-calls) expects a wildcard match string
and the attribute string to match with.

    object ServiceGroup "ping" {
      display_name = "Ping Checks"

      assign where match("ping*", service.name)
    }

    object ServiceGroup "http" {
      display_name = "HTTP Checks"

      assign where match("http*", service.check_command)
    }

    object ServiceGroup "disk" {
      display_name = "Disk Checks"

      assign where match("disk*", service.check_command)
    }


#### <a id="templates-conf"></a> templates.conf

All shipped example configuration objects use generic global templates by
default. Be it setting a default `check_command` attribute in the `generic-host`
templates for your hosts defined in [hosts.conf](#hosts-conf), or defining
the default `states` and `types` filters for notification apply rules defined in
[notifications.conf](#notifications-conf).


    template Host "generic-host" {
      max_check_attempts = 5
      check_interval = 1m
      retry_interval = 30s

      check_command = "hostalive"
    }

    template Service "generic-service" {
      max_check_attempts = 3
      check_interval = 1m
      retry_interval = 30s
    }

The `hostalive` check command is shipped with Icinga 2 in the
[Plugin Check Commands](#plugin-check-commands).


    template Notification "mail-host-notification" {
      command = "mail-host-notification"

      states = [ Up, Down ]
      types = [ Problem, Acknowledgement, Recovery, Custom,
                FlappingStart, FlappingEnd,
                DowntimeStart, DowntimeEnd, DowntimeRemoved ]

      period = "24x7"
    }

    template Notification "mail-service-notification" {
      command = "mail-service-notification"

      states = [ OK, Warning, Critical, Unknown ]
      types = [ Problem, Acknowledgement, Recovery, Custom,
                FlappingStart, FlappingEnd,
                DowntimeStart, DowntimeEnd, DowntimeRemoved ]

      period = "24x7"
    }

More details on `Notification` object attributes can be found [here](#objecttype-notification).


#### <a id="downtimes-conf"></a> downtimes.conf

The `load` service apply rule defined in [services.conf](#services-conf) defines
the `backup_downtime` custom attribute.

The [ScheduledDowntime](#objecttype-scheduleddowntime) apply rule uses this attribute
to define the default value for the time ranges required for recurring downtime slots.

    apply ScheduledDowntime "backup-downtime" to Service {
      author = "icingaadmin"
      comment = "Scheduled downtime for backup"

      ranges = {
        monday = service.vars.backup_downtime
        tuesday = service.vars.backup_downtime
        wednesday = service.vars.backup_downtime
        thursday = service.vars.backup_downtime
        friday = service.vars.backup_downtime
        saturday = service.vars.backup_downtime
        sunday = service.vars.backup_downtime
      }

      assign where service.vars.backup_downtime != ""
    }


#### <a id="timeperiods-conf"></a> timeperiods.conf

This file ships the default timeperiod definitions for `24x7`, `9to5`
and `never`. Timeperiod objects are referenced by `*period`
objects such as hosts, services or notifications.


#### <a id="satellite-conf"></a> satellite.conf

Ships default templates and dependencies for [monitoring remote clients](#icinga2-remote-client-monitoring)
using service discovery and [config generation](#icinga2-remote-monitoring-master-discovery-generate-config)
on the master. Can be ignored/removed on setups not using this features.


Further details on the monitoring configuration can be found in the
[monitoring basics](#monitoring-basics) chapter.



## <a id="configuring-db-ido"></a> Configuring DB IDO

The DB IDO (Database Icinga Data Output) modules for Icinga 2 take care of exporting
all configuration and status information into a database. The IDO database is used
by a number of projects including [Icinga Web 2](#setting-up-icingaweb2),
Icinga Reporting or Icinga Web 1.x.

Icinga 2 does not read configuration or status data from the database backend
so this interface is fully optional, if not required by your user interfaces
or addons.

There is a separate module for each database back-end. At present support for
both MySQL and PostgreSQL is implemented.

Icinga 2 uses the Icinga 1.x IDOUtils database schema. Icinga 2 requires additional
features not yet released with older Icinga 1.x versions.

> **Note**
>
> Please check the [what's new](#whats-new) section for the required schema version.

> **Tip**
>
> Only install the IDO feature if your web interface or reporting tool requires
> you to do so (for example, [Icinga Web](#setting-up-icinga-web) or [Icinga Web 2](#setting-up-icingaweb2)).
> [Icinga Classic UI](#setting-up-icinga-classic-ui) does not use IDO as backend.

### <a id="installing-database"></a> Installing the Database Server

In order to use DB IDO you need to setup either [MySQL](#installing-database-mysql-server)
or [PostgreSQL](#installing-database-postgresql-server) as supported database server.

> **Note**
>
> It's up to you whether you choose to install it on the same server where Icinga 2 is running on,
> or on a dedicated database host (or cluster).

#### <a id="installing-database-mysql-server"></a> Installing MySQL database server

Debian/Ubuntu:

    # apt-get install mysql-server mysql-client

RHEL/CentOS 5/6:

    # yum install mysql-server mysql
    # chkconfig mysqld on
    # service mysqld start

RHEL/CentOS 7 and Fedora 20 prefer MariaDB over MySQL:

    # yum install mariadb-server mariadb
    # systemctl enable mariadb.service
    # systemctl start mariadb.service

SUSE:

    # zypper install mysql mysql-client
    # chkconfig mysqld on
    # service mysqld start

RHEL based distributions do not automatically set a secure root password. Do that **now**:

    # /usr/bin/mysql_secure_installation


#### <a id="installing-database-postgresql-server"></a> Installing PostgreSQL database server

Debian/Ubuntu:

    # apt-get install postgresql

RHEL/CentOS 5/6:

    # yum install postgresql-server postgresql
    # chkconfig postgresql on
    # service postgresql start

RHEL/CentOS 7 and Fedora 20 use [systemd](#systemd-service):

    # yum install postgresql-server postgresql
    # systemctl enable postgresql.service
    # systemctl start postgresql.service

SUSE:

    # zypper install postgresql postgresql-server
    # chkconfig postgresql on
    # service postgresql start

### <a id="configuring-db-ido-mysql"></a> Configuring DB IDO MySQL

First of all you have to install the `icinga2-ido-mysql` package using your
distribution's package manager.

Debian/Ubuntu:

    # apt-get install icinga2-ido-mysql

RHEL/CentOS:

    # yum install icinga2-ido-mysql

SUSE:

    # zypper install icinga2-ido-mysql



> **Note**
>
> Upstream Debian packages provide a database configuration wizard by default.
> You can skip the automated setup and install/upgrade the database manually
> if you prefer that.

#### <a id="setting-up-mysql-db"></a> Setting up the MySQL database

Set up a MySQL database for Icinga 2:

    # mysql -u root -p

    mysql>  CREATE DATABASE icinga;
            GRANT SELECT, INSERT, UPDATE, DELETE, DROP, CREATE VIEW, INDEX, EXECUTE ON icinga.* TO 'icinga'@'localhost' IDENTIFIED BY 'icinga';
            quit

After creating the database you can import the Icinga 2 IDO schema using the
following command:

    # mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/mysql.sql


#### <a id="upgrading-mysql-db"></a> Upgrading the MySQL database

Check the `/usr/share/icinga2-ido-mysql/schema/upgrade` directory for an
incremental schema upgrade file.

> **Note**
>
> If there isn't an upgrade file for your current version available there's nothing to do.

Apply all database schema upgrade files incrementially.

    # mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/<version>.sql

The Icinga 2 DB IDO module will check for the required database schema version on startup
and generate an error message if not satisfied.


**Example:** You are upgrading Icinga 2 from version `2.0.2` to `2.2.0`. Look into
the *upgrade* directory:

    $ ls /usr/share/icinga2-ido-mysql/schema/upgrade/
    2.0.2.sql  2.1.0.sql 2.2.0.sql

There are two new upgrade files called `2.1.0.sql` and `2.2.0.sql`
which must be applied incrementially to your IDO database.


#### <a id="installing-ido-mysql"></a> Installing the IDO MySQL module

The package provides a new configuration file that is installed in
`/etc/icinga2/features-available/ido-mysql.conf`. You will need to update the
database credentials in this file.
All available attributes are listed in the
[IdoMysqlConnection object][#objecttype-idomysqlconnection] configuration details.

You can enable the `ido-mysql` feature configuration file using `icinga2 feature enable`:

    # icinga2 feature enable ido-mysql
    Module 'ido-mysql' was enabled.
    Make sure to restart Icinga 2 for these changes to take effect.

After enabling the ido-mysql feature you have to restart Icinga 2:

Debian/Ubuntu, RHEL/CentOS 6 and SUSE:

    # service icinga2 restart

RHEL/CentOS 7 and Fedora 20:

    # systemctl restart icinga2.service

### <a id="configuring-db-ido-postgresql"></a> Configuring DB IDO PostgreSQL

First of all you have to install the `icinga2-ido-pgsql` package using your
distribution's package manager.

Debian/Ubuntu:

    # apt-get install icinga2-ido-pgsql

RHEL/CentOS:

    # yum install icinga2-ido-pgsql

SUSE:

    # zypper install icinga2-ido-pgsql

> **Note**
>
> Upstream Debian packages provide a database configuration wizard by default.
> You can skip the automated setup and install/upgrade the database manually
> if you prefer that.

#### Setting up the PostgreSQL database

Set up a PostgreSQL database for Icinga 2:

    # cd /tmp
    # sudo -u postgres psql -c "CREATE ROLE icinga WITH LOGIN PASSWORD 'icinga'";
    # sudo -u postgres createdb -O icinga -E UTF8 icinga
    # sudo -u postgres createlang plpgsql icinga

> **Note**
>
> Using PostgreSQL 9.x you can omit the `createlang` command.

Locate your pg_hba.conf (Debian: `/etc/postgresql/*/main/pg_hba.conf`,
RHEL/SUSE: `/var/lib/pgsql/data/pg_hba.conf`), add the icinga user with md5
authentication method and restart the postgresql server.

    # vim /var/lib/pgsql/data/pg_hba.conf

    # icinga
    local   icinga      icinga                            md5
    host    icinga      icinga      127.0.0.1/32          md5
    host    icinga      icinga      ::1/128               md5

    # "local" is for Unix domain socket connections only
    local   all         all                               ident
    # IPv4 local connections:
    host    all         all         127.0.0.1/32          ident
    # IPv6 local connections:
    host    all         all         ::1/128               ident

    # /etc/init.d/postgresql restart


After creating the database and permissions you can import the Icinga 2 IDO schema
using the following command:

    # export PGPASSWORD=icinga
    # psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/pgsql.sql

#### <a id="upgrading-postgresql-db"></a> Upgrading the PostgreSQL database

Check the `/usr/share/icinga2-ido-pgsql/schema/upgrade` directory for an
incremental schema upgrade file.

> **Note**
>
> If there isn't an upgrade file for your current version available there's nothing to do.

Apply all database schema upgrade files incrementially.

    # export PGPASSWORD=icinga
    # psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/<version>.sql

The Icinga 2 DB IDO module will check for the required database schema version on startup
and generate an error message if not satisfied.

**Example:** You are upgrading Icinga 2 from version `2.0.2` to `2.1.0`. Look into
the *upgrade* directory:

    $ ls /usr/share/icinga2-ido-pgsql/schema/upgrade/
    2.0.2.sql  2.1.0.sql 2.2.0.sql

There are two new upgrade files called `2.1.0.sql` and `2.2.0.sql`
which must be applied incrementially to your IDO database.


#### <a id="installing-ido-postgresql"></a> Installing the IDO PostgreSQL module

The package provides a new configuration file that is installed in
`/etc/icinga2/features-available/ido-pgsql.conf`. You will need to update the
database credentials in this file.
All available attributes are listed in the
[IdoPgsqlConnection object][#objecttype-idopgsqlconnection] configuration details.

You can enable the `ido-pgsql` feature configuration file using `icinga2 feature enable`:

    # icinga2 feature enable ido-pgsql
    Module 'ido-pgsql' was enabled.
    Make sure to restart Icinga 2 for these changes to take effect.

After enabling the ido-pgsql feature you have to restart Icinga 2:

Debian/Ubuntu, RHEL/CentOS 6 and SUSE:

    # service icinga2 restart

RHEL/CentOS 7 and Fedora 20:

    # systemctl restart icinga2.service


### <a id="setting-up-external-command-pipe"></a> Setting Up External Command Pipe

Web interfaces and other Icinga addons are able to send commands to
Icinga 2 through the external command pipe.

You can enable the External Command Pipe using icinga2 feature enable:

    # icinga2 feature enable command

After that you will have to restart Icinga 2:

Debian/Ubuntu, RHEL/CentOS 6 and SUSE:

    # service icinga2 restart

RHEL/CentOS 7 and Fedora 20:

    # systemctl restart icinga2.service

By default the command pipe file is owned by the group `icingacmd` with read/write
permissions. Add your webserver's user to the group `icingacmd` to
enable sending commands to Icinga 2 through your web interface:

    # usermod -a -G icingacmd www-data

Debian packages use `nagios` as the default user and group name. Therefore change `icingacmd` to
`nagios`.
The webserver's user is different between distributions so you might have to change `www-data` to
`wwwrun`, `www`, or `apache`.

Change "www-data" to the user you're using to run queries.

> **Note**
>
> Packages will do that automatically. Verify that by running `id <your-webserver-user>` and skip this
> step.

## <a id="setting-up-livestatus"></a> Setting up Livestatus

The [MK Livestatus](http://mathias-kettner.de/checkmk_livestatus.html) project
implements a query protocol that lets users query their Icinga instance for
status information. It can also be used to send commands.

> **Tip**
>
> Only install the Livestatus feature if your web interface or addon requires
> you to do so (for example, [Icinga Web 2](#setting-up-icingaweb2)).
> [Icinga Classic UI](#setting-up-icinga-classic-ui) and [Icinga Web](#setting-up-icinga-web)
> do not use Livestatus as backend.

The Livestatus component that is distributed as part of Icinga 2 is a
re-implementation of the Livestatus protocol which is compatible with MK
Livestatus.

Details on the available tables and attributes with Icinga 2 can be found
in the [Livestatus](#livestatus) section.

You can enable Livestatus using icinga2 feature enable:

    # icinga2 feature enable livestatus

After that you will have to restart Icinga 2:

Debian/Ubuntu, RHEL/CentOS 6 and SUSE:

    # service icinga2 restart

RHEL/CentOS 7 and Fedora 20:

    # systemctl restart icinga2.service

By default the Livestatus socket is available in `/var/run/icinga2/cmd/livestatus`.

In order for queries and commands to work you will need to add your query user
(e.g. your web server) to the `icingacmd` group:

    # usermod -a -G icingacmd www-data

The Debian packages use `nagios` as the user and group name. Make sure to change `icingacmd` to
`nagios` if you're using Debian.

Change "www-data" to the user you're using to run queries.

In order to use the historical tables provided by the livestatus feature (for example, the
`log` table) you need to have the `CompatLogger` feature enabled. By default these logs
are expected to be in `/var/log/icinga2/compat`. A different path can be set using the
`compat_log_path` configuration attribute.

    # icinga2 feature enable compatlog

## <a id="setting-up-icinga2-user-interfaces"></a> Setting up Icinga 2 User Interfaces

Icinga 2 can be used with [Icinga Web 2](#setting-up-icingaweb2), using
DB IDO or Livestatus as preferred backend provider, next to the command pipe.

Icinga 2 also is compatible with Icinga 1.x user interfaces
[Classic UI 1.x](#installing-icinga-classic-ui) and [Icinga Web 1.x](#setting-up-icinga-web)
by providing additional features required as backends.

Some Icinga 1.x interface features will only work in a limited manner due to
[compatibility reasons](#differences-1x-2), other features like the
statusmap parents are available by dumping the host dependencies as parents.
Special restrictions are noted specifically in the sections below.

You can also use other user interface addons, if you prefer. Keep in mind
that Icinga 2 core features might not be fully supported in these addons.

> **Tip**
>
> Choose your preferred interface. There's no need to install [Classic UI 1.x](#setting-up-icinga-classic-ui)
> if you prefer [Icinga Web 1.x](#setting-up-icinga-web) or [Icinga Web 2](#setting-up-icingaweb2) for example.

### <a id="icinga2-user-interface-requirements"></a> Requirements

* Web server (Apache2/Httpd, Nginx, Lighttp, etc)
* User credentials
* Firewall ports (tcp/80)

The Debian, RHEL and SUSE packages for Icinga [Classic UI](#setting-up-icinga-classic-ui),
[Web](#setting-up-icinga-web) and [Icingaweb 2](#setting-up-icingaweb2) depend on Apache2
as web server.

#### <a id="icinga2-user-interface-webserver"></a> Webserver

Debian/Ubuntu packages will automatically fetch and install the required packages.

RHEL/CentOS/Fedora:

    # yum install httpd
    # chkconfig httpd on && service httpd start
    ## RHEL7
    # systemctl enable httpd && systemctl start httpd

SUSE:

    # zypper install apache2
    # chkconfig on && service apache2 start

#### <a id="icinga2-user-interface-firewall-rules"></a> Firewall Rules

Example:

    # iptables -A INPUT -p tcp -m tcp --dport 80 -j ACCEPT
    # service iptables save

RHEL/CentOS 7 specific:

    # firewall-cmd --add-service=http
    # firewall-cmd --permanent --add-service=http

### <a id="setting-up-icingaweb2"></a> Setting up Icinga Web 2

Icinga Web 2 supports `DB IDO` or `Livestatus` as backends.

Using DB IDO as backend, you need to install and configure the
[DB IDO backend](#configuring-db-ido).
Once finished, you can enable the feature for DB IDO MySQL:

    # icinga2 feature enable ido-mysql

Furthermore [external commands](#external-commands) are supported through the external
command pipe.

    # icinga2 feature enable command

In order for commands to work you will need to [setup the external command pipe](#setting-up-external-command-pipe).

[Icinga Web 2](https://github.com/Icinga/icingaweb2) ships its own
web-based setup wizard which will guide you through the entire setup process.

Generate the Webserver configuration and a setup token using its local cli
and proceed with the web setup when accessing `/icingaweb2` after reloading
your webserver configuration.

Please consult the [installation documentation](https://github.com/Icinga/icingaweb2/blob/master/doc/installation.md)
shipped with `Icinga Web 2` for further instructions on how to install
`Icinga Web 2` and to configure backends, resources and instances manually.

Check the [Icinga website](https://www.icinga.org) for release announcements
and packages.


### <a id="setting-up-icinga-classic-ui"></a> Setting up Icinga Classic UI 1.x

Icinga 2 can write `status.dat` and `objects.cache` files in the format that
is supported by the Icinga 1.x Classic UI. [External commands](#external-commands)
(a.k.a. the "command pipe") are also supported. It also supports writing Icinga 1.x
log files which are required for the reporting functionality in the Classic UI.

#### <a id="installing-icinga-classic-ui"></a> Installing Icinga Classic UI 1.x

The Icinga package repository has both Debian and RPM packages. You can install
the Classic UI using the following packages:

  Distribution  | Packages
  --------------|---------------------
  Debian        | icinga2-classicui
  RHEL/SUSE     | icinga2-classicui-config icinga-gui

The Debian packages require additional packages which are provided by the
[Debian Monitoring Project](http://www.debmon.org) (`DebMon`) repository.

`libjs-jquery-ui` requires at least version `1.10.*` which is not available
in Debian 7 (Wheezy) and Ubuntu 12.04 LTS (Precise). Add the following repositories
to satisfy this dependency:

  Distribution  		| Package Repositories
  ------------------------------|------------------------------
  Debian Wheezy 		| [wheezy-backports](http://backports.debian.org/Instructions/) or [DebMon](http://www.debmon.org)
  Ubuntu 12.04 LTS (Precise)    | [Icinga PPA](https://launchpad.net/~formorer/+archive/icinga)

On all distributions other than Debian you may have to restart both your web
server as well as Icinga 2 after installing the Classic UI package.

Icinga Classic UI requires the [StatusDataWriter](#status-data), [CompatLogger](#compat-logging)
and [ExternalCommandListener](#external-commands) features.
Enable these features and restart Icinga 2.

    # icinga2 feature enable statusdata compatlog command

In order for commands to work you will need to [setup the external command pipe](#setting-up-external-command-pipe).

#### <a id="setting-up-icinga-classic-ui-summary"></a> Setting Up Icinga Classic UI 1.x Summary

Verify that your Icinga 1.x Classic UI works by browsing to your Classic
UI installation URL:

  Distribution  | URL                                                                      | Default Login
  --------------|--------------------------------------------------------------------------|--------------------------
  Debian        | [http://localhost/icinga2-classicui](http://localhost/icinga2-classicui) | asked during installation
  all others    | [http://localhost/icinga](http://localhost/icinga)                       | icingaadmin/icingaadmin

For further information on configuration, troubleshooting and interface documentation
please check the official [Icinga 1.x user interface documentation](http://docs.icinga.org/latest/en/ch06.html).

### <a id="setting-up-icinga-web"></a> Setting up Icinga Web 1.x

Icinga 2 can write to the same schema supplied by `Icinga IDOUtils 1.x` which
is an explicit requirement to run `Icinga Web` next to the external command pipe.
Therefore you need to setup the [DB IDO feature](#configuring-ido) remarked in the previous sections.

#### <a id="installing-icinga-web"></a> Installing Icinga Web 1.x

The Icinga package repository has both Debian and RPM packages. You can install
Icinga Web using the following packages (RPMs ship an additional configuration package):

  Distribution  | Packages
  --------------|-------------------------------------------------------
  RHEL/SUSE     | icinga-web icinga-web-{mysql,pgsql}
  Debian        | icinga-web icinga-web-config-icinga2-ido-{mysql,pgsql}

#### <a id="icinga-web-rpm-notes"></a> Icinga Web 1.x on RPM based systems

Additionally you need to setup the `icinga_web` database and import the database schema.
Details can be found in the package `README` files, for example [README.RHEL](https://github.com/Icinga/icinga-web/blob/master/doc/README.RHEL)

The Icinga Web RPM packages install the schema files into
`/usr/share/doc/icinga-web-*/schema` (`*` means package version).
The Icinga Web dist tarball ships the schema files in `etc/schema`.

On SuSE-based distributions the schema files are installed in
`/usr/share/doc/packages/icinga-web/schema`.

Example for RHEL and MySQL:

    # mysql -u root -p

    mysql> CREATE DATABASE icinga_web;
           GRANT SELECT, INSERT, UPDATE, DELETE, DROP, CREATE VIEW, INDEX, EXECUTE ON icinga_web.* TO 'icinga_web'@'localhost' IDENTIFIED BY 'icinga_web';
           quit

    # mysql -u root -p icinga_web <  /usr/share/doc/icinga-web-<version>/schema/mysql.sql

Icinga Web requires the IDO feature as database backend using MySQL or PostgreSQL.
Enable that feature, e.g. for MySQL.

    # icinga2 feature enable ido-mysql

If you've changed your default credentials you may either create a read-only user
or use the credentials defined in the IDO feature for Icinga Web backend configuration.
Edit `databases.xml` accordingly and clear the cache afterwards. Further details can be
found in the [Icinga Web documentation](http://docs.icinga.org/latest/en/icinga-web-config.html).

    # vim /etc/icinga-web/conf.d/databases.xml

    # icinga-web-clearcache

Additionally you need to enable the `command` feature for sending [external commands](#external-commands):

    # icinga2 feature enable command

In order for commands to work you will need to [setup the external command pipe](#setting-up-external-command-pipe).

Then edit the Icinga Web configuration for sending commands in `/etc/icinga-web/conf.d/access.xml`
(RHEL) or `/etc/icinga-web/access.xml` (SUSE) setting the command pipe path
to the default used in Icinga 2. Make sure to clear the cache afterwards.

    # vim /etc/icinga-web/conf.d/access.xml

                <write>
                    <files>
                        <resource name="icinga_pipe">/var/run/icinga2/cmd/icinga2.cmd</resource>
                    </files>
                </write>

    # icinga-web-clearcache

> **Note**
>
> The path to the Icinga Web `clearcache` script may differ. Please check the
> [Icinga Web documentation](https://docs.icinga.org) for details.

#### <a id="icinga-web-debian-notes"></a> Icinga Web on Debian systems

Since Icinga Web `1.11.1-2` the IDO auto-configuration has been moved into
additional packages on Debian and Ubuntu.

The package `icinga-web` no longer configures the IDO connection. You must now
use one of the config packages:

 - `icinga-web-config-icinga2-ido-mysql`
 - `icinga-web-config-icinga2-ido-pgsql`

These packages take care of setting up the [DB IDO](#configuring-db-ido) configuration,
enabling the external command pipe for Icinga Web and depend on
the corresponding packages of Icinga 2.

Please read the `README.Debian` files for details and advanced configuration:

 - `/usr/share/doc/icinga-web/README.Debian`
 - `/usr/share/doc/icinga-web-config-icinga2-ido-mysql/README.Debian`
 - `/usr/share/doc/icinga-web-config-icinga2-ido-pgsql/README.Debian`

When changing Icinga Web configuration files make sure to clear the config cache:

    # /usr/lib/icinga-web/bin/clearcache.sh

> **Note**
>
> If you are using an older version of Icinga Web, install it like this and adapt
> the configuration manually as shown in [the RPM notes](#icinga-web-rpm-notes):
>
> `apt-get install --no-install-recommends icinga-web`

#### <a id="setting-up-icinga-web-summary"></a> Setting Up Icinga Web 1.x Summary

Verify that your Icinga 1.x Web works by browsing to your Web installation URL:

  Distribution  | URL                                                         | Default Login
  --------------|-------------------------------------------------------------|--------------------------
  Debian        | [http://localhost/icinga-web](http://localhost/icinga-web)  | asked during installation
  all others    | [http://localhost/icinga-web](http://localhost/icinga-web)  | root/password

For further information on configuration, troubleshooting and interface documentation
please check the official [Icinga 1.x user interface documentation](http://docs.icinga.org/latest/en/ch06.html).


### <a id="additional-visualization"></a> Additional visualization

There are many visualization addons which can be used with Icinga 2.

Some of the more popular ones are [PNP](#addons-graphing-pnp), [inGraph](#addons-graphing-pnp)
(graphing performance data), [Graphite](#addons-graphing-pnp), and
[NagVis](#addons-visualization-nagvis) (network maps).


## <a id="configuration-tools"></a> Configuration Tools

If you require your favourite configuration tool to export Icinga 2 configuration, please get in
touch with their developers. The Icinga project does not provide a configuration web interface
or similar.

> **Tip**
>
> Get to know the new configuration format and the advanced [apply](#using-apply) rules and
> use [syntax highlighting](#configuration-syntax-highlighting) in vim/nano.

If you're looking for puppet manifests, chef cookbooks, ansible recipes, etc - we're happy
to integrate them upstream, so please get in touch at [https://support.icinga.org](https://support.icinga.org).

These tools are in development and require feedback and tests:

* [Ansible Roles](https://github.com/Icinga/icinga2-ansible)
* [Puppet Module](https://github.com/Icinga/puppet-icinga2)

## <a id="configuration-syntax-highlighting"></a> Configuration Syntax Highlighting

Icinga 2 ships configuration examples for syntax highlighting using the `vim` and `nano` editors.
The RHEL, SUSE and Debian package `icinga2-common` install these files into
`/usr/share/*/icinga2-common/syntax`. Sources provide these files in `tools/syntax`.

### <a id="configuration-syntax-highlighting-vim"></a> Configuration Syntax Highlighting using Vim

Create a new local vim configuration storage, if not already existing.
Edit `vim/ftdetect/icinga2.vim` if your paths to the Icinga 2 configuration
differ.

    $ PREFIX=~/.vim
    $ mkdir -p $PREFIX/{syntax,ftdetect}
    $ cp vim/syntax/icinga2.vim $PREFIX/syntax/
    $ cp vim/ftdetect/icinga2.vim $PREFIX/ftdetect/

Test it:

    $ vim /etc/icinga2/conf.d/templates.conf

### <a id="configuration-syntax-highlighting-nano"></a> Configuration Syntax Highlighting using Nano

Copy the `/etc/nanorc` sample file to your home directory. Create the `/etc/nano` directory
and copy the provided `icinga2.nanorc` into it.

    $ cp /etc/nanorc ~/.nanorc

    # mkdir -p /etc/nano
    # cp icinga2.nanorc /etc/nano/

Then include the icinga2.nanorc file in your ~/.nanorc by adding the following line:

    $ vim ~/.nanorc

    ## Icinga 2
    include "/etc/nano/icinga2.nanorc"

Test it:

    $ nano /etc/icinga2/conf.d/templates.conf


## <a id="running-icinga2"></a> Running Icinga 2

### <a id="init-script"></a> Init Script

Icinga 2's init script is installed in `/etc/init.d/icinga2` by default:

    # /etc/init.d/icinga2
    Usage: /etc/init.d/icinga2 {start|stop|restart|reload|checkconfig|status}

  Command             | Description
  --------------------|------------------------
  start               | The `start` action starts the Icinga 2 daemon.
  stop                | The `stop` action stops the Icinga 2 daemon.
  restart             | The `restart` action is a shortcut for running the `stop` action followed by `start`.
  reload              | The `reload` action sends the `HUP` signal to Icinga 2 which causes it to restart. Unlike the `restart` action `reload` does not wait until Icinga 2 has restarted.
  checkconfig         | The `checkconfig` action checks if the `/etc/icinga2/icinga2.conf` configuration file contains any errors.
  status              | The `status` action checks if Icinga 2 is running.

By default the Icinga 2 daemon is running as `icinga` user and group
using the init script. Using Debian packages the user and group are set to `nagios`
for historical reasons.

### <a id="systemd-service"></a> Systemd Service

Modern distributions (Fedora, OpenSUSE, etc.) already use `Systemd` natively. Enterprise-grade
distributions such as RHEL7 changed to `Systemd` recently. Icinga 2 Packages will install the
service automatically.

The Icinga 2 `Systemd` service can be (re)started, reloaded, stopped and also queried for its current status.

    # systemctl status icinga2
    icinga2.service - Icinga host/service/network monitoring system
       Loaded: loaded (/usr/lib/systemd/system/icinga2.service; disabled)
       Active: active (running) since Mi 2014-07-23 13:39:38 CEST; 15s ago
      Process: 21692 ExecStart=/usr/sbin/icinga2 -c ${ICINGA2_CONFIG_FILE} -d -e ${ICINGA2_ERROR_LOG} -u ${ICINGA2_USER} -g ${ICINGA2_GROUP} (code=exited, status=0/SUCCESS)
      Process: 21674 ExecStartPre=/usr/sbin/icinga2-prepare-dirs /etc/sysconfig/icinga2 (code=exited, status=0/SUCCESS)
     Main PID: 21727 (icinga2)
       CGroup: /system.slice/icinga2.service
               └─21727 /usr/sbin/icinga2 -c /etc/icinga2/icinga2.conf -d -e /var/log/icinga2/error.log -u icinga -g icinga --no-stack-rlimit

    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 309 Service(s).
    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 1 User(s).
    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 15 Notification(s).
    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 4 ScheduledDowntime(s).
    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 1 UserGroup(s).
    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 1 IcingaApplication(s).
    Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 8 Dependency(s).
    Jul 23 13:39:38 nbmif systemd[1]: Started Icinga host/service/network monitoring system.

`Systemd` supports the following command actions:

  Command             | Description
  --------------------|------------------------
  start               | The `start` action starts the Icinga 2 daemon.
  stop                | The `stop` action stops the Icinga 2 daemon.
  restart             | The `restart` action is a shortcut for running the `stop` action followed by `start`.
  reload              | The `reload` action sends the `HUP` signal to Icinga 2 which causes it to restart. Unlike the `restart` action `reload` does not wait until Icinga 2 has restarted.
  status              | The `status` action checks if Icinga 2 is running.
  enable              | The `enable` action enables the service being started at system boot time (similar to `chkconfig`)

If you're stuck with configuration errors, you can manually invoke the [configuration validation](#config-validation).

    # systemctl enable icinga2

    # systemctl restart icinga2
    Job for icinga2.service failed. See 'systemctl status icinga2.service' and 'journalctl -xn' for details.



### <a id="cli-commands"></a> Icinga 2 CLI Commands

Icinga 2 ships its own integrated cli commands supporting bash-autocompletion.

These cli commands will allow you to use certain functionality
provided by and around the Icinga 2 daemon.

> **Note**
>
> The cli commands are available in Icinga 2 starting with *2.2*.

Each cli command provides its own help and usage information, so please
make sure to always run them withthe  `--help` parameter.

Run `icinga2` without any arguments (or using `--help`) to get a list of
all available global options.

    # icinga2

    icinga2 - The Icinga 2 network monitoring daemon (version: v2.1.1-299-gf695275)

    Usage:
      icinga2 <command> [<arguments>]

    Supported commands:
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
      * variable get (gets a variable)
      * variable list (lists all variables)

    Global options:
      -h [ --help ]          show this help message
      -V [ --version ]       show version information
      --color                use VT100 color codes even when stdout is not a
                             terminal
      -D [ --define ] arg    define a constant
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


#### <a id="cli-commands-autocompletion"></a> Icinga 2 CLI Bash Autocompletion

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

#### <a id="cli-commands-global-options"></a> Icinga 2 CLI Global Options

##### Libraries

Instead of loading libraries using the [`library` config directive](#library)
you can also use the `--library` command-line option.

##### Constants

[Global constants](#constants) can be set using the `--define` command-line option.

##### Config Include Path

When including files you can specify that the include search path should be
checked. You can do this by putting your configuration file name in angle
brackets like this:

    include <test.conf>

This would cause Icinga 2 to search its include path for the configuration file
`test.conf`. By default the installation path for the Icinga Template Library
is the only search directory.

Using the `--include` command-line option additional search directories can be
added.



#### <a id="cli-command-daemon"></a> Cli command: Daemon

The cli command `daemon` provides the functionality to start/stop Icinga 2.
Furthermore it provides the [configuration validation](#config-validation).

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

##### Config Files

Using the `--config` option you can specify one or more configuration files.
Config files are processed in the order they're specified on the command-line.

When no configuration file is specified and the `--no-config` is not used
Icinga 2 automatically falls back to using the configuration file
`SysconfDir + "/icinga2/icinga2.conf"` (where SysconfDir is usually `/etc`).

##### Config Validation

The `--validate` option can be used to check if your configuration files
contain errors. If any errors are found the exit status is 1, otherwise 0
is returned. More details in the [configuration validation](#config-validation) chapter.


#### <a id="cli-command-feature"></a> Cli command: Feature

The cli commands for `enable` and `disable` feature support bash auto-completion
and will only suggest features for the corresponding context. Like disabling a
feature will only bring up all enabled features.

    # icinga2 feature disable <tab>
    checker       --color       --define      --help        --include     --library     --log-level   mainlog       notification  --version

    # icinga2 feature enable <tab>
    api           command       debuglog      graphite      icingastatus  ido-pgsql     --library     --log-level   statusdata    --version
    --color       compatlog     --define      --help        ido-mysql     --include     livestatus    perfdata      syslog

#### <a id="cli-command-node"></a> Cli command: Node

Provides the functionality to install and manage master and client
nodes in a [remote monitoring ](#icinga2-remote-client-monitoring) or
[distributed cluster](#distributed-monitoring-high-availability) scenario.


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
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


#### <a id="cli-command-object"></a> Cli command: Object

The `object` cli command can be used to list all configuration objects and their
attributes. The command also shows where each of the attributes was modified.
That way you can also identify which objects have been created from your [apply rules](#apply).

More information can be found in the [troubleshooting](#list-configuration-objects) section.

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
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>



#### <a id="cli-command-pki"></a> Cli command: Pki

Provides the cli commands to

* generate a new local CA
* generate a new CSR or self-signed certificate
* sign a CSR and return a certificate
* save a master certificate manually
* request a signed certificate from the master
* generate a new ticket for the client setup

This functionality is used by the [node setup/wizard](#cli-command-pki) cli commands too.

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
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>


#### <a id="cli-command-repository"></a> Cli command: Repository

Provides the functionality to manage the Icinga 2 configuration repository in
`/etc/icinga2/repository.d`. All changes are logged and must be committed or
cleared after review.


> **Note**
>
> The cli command `repository` only supports basic configuration manipulation (add, remove)
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
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>



#### <a id="cli-command-variable"></a> Cli command: Variable

Lists all configured variables (constants) in a similar fasion like [object list](#cli-command-object).

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
      -l [ --library ] arg   load a library
      -I [ --include ] arg   add include search directory
      -x [ --log-level ] arg specify the log level for the console log

    Command options:

    Report bugs at <https://dev.icinga.org/>
    Icinga home page: <https://www.icinga.org/>





### <a id="features"></a> Enabling/Disabling Features

Icinga 2 provides configuration files for some commonly used features. These
are installed in the `/etc/icinga2/features-available` directory and can be
enabled and disabled using the `icinga2 feature enable` and `icinga2 feature disable`
[cli commands](#cli-command-feature), respectively.

The `icinga2 feature enable` cli command creates symlinks in the
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



### <a id="config-validation"></a> Configuration Validation

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
> Using [Systemd](#systemd-service) you need to manually validate the configuration using
> the cli command below.

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

If you encouter errors during configuration validation, please make sure
to read the [troubleshooting](#troubleshooting) chapter.

You can also use the [cli command](#cli-command-object) `icinga2 object list`
after validation passes to analyze object attributes, inheritance or created
objects by apply rules.
Find more on troubleshooting with `object list` in [this chapter](#list-configuration-objects).

Example filtered by `Service` objects with the name `ping*`:

    # icinga2 object list --type Service --name *ping*
    Object 'nbmif.int.netways.de!ping4' of type 'Service':
      * __name = 'nbmif.int.netways.de!ping4'
      * check_command = 'ping4'
        % = modified in '/etc/icinga2/conf.d/services.conf', lines 17:3-17:25
      * check_interval = 60
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 28:3-28:21
      * host_name = 'nbmif.int.netways.de'
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



### <a id="config-change-reload"></a> Reload on Configuration Changes

Everytime you have changed your configuration you should first tell Icinga 2
to [validate](#config-validation). If there are no validation errors you can
safely reload the Icinga 2 daemon.

    # /etc/init.d/icinga2 reload

> **Note**
>
> The `reload` action will send the `SIGHUP` signal to the Icinga 2 daemon
> which will validate the configuration in a separate process and not stop
> the other events like check execution, notifications, etc.
>
> Details can be found [here](#differences-1x-2-real-reload).


## <a id="vagrant"></a> Vagrant Demo VM

The Icinga Vagrant Git repository contains support for [Vagrant](http://docs.vagrantup.com/v2/)
with VirtualBox. Please note that Vagrant version `1.0.x` is not supported. At least
version `1.2.x` is required.

In order to build the Vagrant VM first you will have to check out
the Git repository:

    $ git clone git://git.icinga.org/icinga-vagrant.git

For Icinga 2 there are currently two scenarios available:

* `icinga2x` bringing up a standalone box with Icinga 2
* `icinga2x-cluster` setting up two virtual machines in a master/slave cluster

> **Note**
>
> Please consult the `README.md` file for each project for further installation
> details at [https://github.com/Icinga/icinga-vagrant]

Once you have checked out the Git repository navigate to your required
vagrant box and build the VM using the following command:

    $ vagrant up

The Vagrant VMs are based on CentOS 6.x and are using the official
Icinga 2 RPM snapshot packages from `packages.icinga.org`. The check
plugins are installed from EPEL providing RPMs with sources from the
Monitoring Plugins project.
