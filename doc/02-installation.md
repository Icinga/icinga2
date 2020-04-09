# Installation <a id="installation"></a>

This tutorial is a step-by-step introduction to installing a basic Icinga Stack ([Icinga 2](02-installation.md#setting-up-icinga2), [Icinga DB](02-installation.md#setting-up-icingadb) and [Icinga Web 2](02-installation.md#setting-up-icingaweb2)).
It assumes that you are familiar with the operating system you're using to install Icinga 2.

> **WARNING**
>
> You are about to install a pre release version of Icinga.
> If that's not your intension, please use the [latest stable release](https://icinga.com/docs/icinga2/latest/doc/02-installation/).

In case you are upgrading an existing setup, please ensure to
follow the [upgrade documentation](16-upgrading-icinga-2.md#upgrading-icinga-2).

## Setting up Icinga 2 <a id="setting-up-icinga2"></a>

First off you have to install Icinga 2. The preferred way of doing this
is to use the official package repositories depending on which operating system
and distribution you are running.

Official repositories ([support matrix](https://icinga.com/subscription/support-details/)):

  Distribution            | Repository
  ------------------------|---------------------------
  Debian                  | [Icinga Repository](https://packages.icinga.com/debian/)
  Ubuntu                  | [Icinga Repository](https://packages.icinga.com/ubuntu/)
  Raspbian		  | [Icinga Repository](https://packages.icinga.com/raspbian/). Note that **Raspbian `icinga-buster` is required.**
  RHEL/CentOS             | [Icinga Repository](https://packages.icinga.com/epel/)
  openSUSE                | [Icinga Repository](https://packages.icinga.com/openSUSE/)
  SLES                    | [Icinga Repository](https://packages.icinga.com/SUSE/)

Community repositories:

  Distribution            | Repository
  ------------------------|---------------------------
  Gentoo                  | [Upstream](https://packages.gentoo.org/package/net-analyzer/icinga2)
  FreeBSD                 | [Upstream](https://www.freshports.org/net-mgmt/icinga2)
  OpenBSD                 | [Upstream](http://ports.su/net/icinga/core2,-main)
  ArchLinux               | [Upstream](https://aur.archlinux.org/packages/icinga2)
  Alpine Linux            | [Upstream](https://pkgs.alpinelinux.org/package/edge/community/x86_64/icinga2)

Packages for distributions other than the ones listed above may also be
available. Please contact your distribution packagers.

> **Note**
>
> Windows is only supported for agent installations. Please refer
> to the [distributed monitoring chapter](06-distributed-monitoring.md#distributed-monitoring-setup-client-windows).

### Package Repositories <a id="package-repositories"></a>

You need to add the Icinga repository to your package management configuration.
The following commands must be executed with `root` permissions unless noted otherwise.

> **Note**
>
> In this case we will add the Icinga testing repository to be able to install release canditate (pre release) versions

#### Debian/Ubuntu/Raspbian Repositories <a id="package-repositories-debian-ubuntu-raspbian"></a>

Debian:

```
apt-get update
apt-get -y install apt-transport-https wget gnupg

wget -O - https://packages.icinga.com/icinga.key | apt-key add -

DIST=$(awk -F"[)(]+" '/VERSION=/ {print $2}' /etc/os-release); \
 echo "deb http://packages.icinga.com/debian icinga-${DIST}-testing main" > \
 /etc/apt/sources.list.d/${DIST}-icinga-testing.list
 echo "deb-src http://packages.icinga.com/debian icinga-${DIST}-testing main" >> \
 /etc/apt/sources.list.d/${DIST}-icinga-testing.list

apt-get update
```

Ubuntu:

```
apt-get update
apt-get -y install apt-transport-https wget gnupg

wget -O - https://packages.icinga.com/icinga.key | apt-key add -

. /etc/os-release; if [ ! -z ${UBUNTU_CODENAME+x} ]; then DIST="${UBUNTU_CODENAME}"; else DIST="$(lsb_release -c| awk '{print $2}')"; fi; \
 echo "deb http://packages.icinga.com/ubuntu icinga-${DIST}-testing main" > \
 /etc/apt/sources.list.d/${DIST}-icinga-testing.list
 echo "deb-src http://packages.icinga.com/ubuntu icinga-${DIST}-testing main" >> \
 /etc/apt/sources.list.d/${DIST}-icinga-testing.list

apt-get update
```

Raspbian Buster:

```
apt-get update
apt-get -y install apt-transport-https wget gnupg

wget -O - https://packages.icinga.com/icinga.key | apt-key add -

DIST=$(awk -F"[)(]+" '/VERSION=/ {print $2}' /etc/os-release); \
 echo "deb http://packages.icinga.com/raspbian icinga-${DIST}-testing main" > \
 /etc/apt/sources.list.d/icinga-testing.list
 echo "deb-src http://packages.icinga.com/raspbian icinga-${DIST}-testing main" >> \
 /etc/apt/sources.list.d/icinga-testing.list

apt-get update
```

##### Debian Backports Repository <a id="package-repositories-debian-backports"></a>

> **Note**:
>
> This repository is required for Debian Stretch since v2.11.

Debian Stretch:

```
DIST=$(awk -F"[)(]+" '/VERSION=/ {print $2}' /etc/os-release); \
 echo "deb https://deb.debian.org/debian ${DIST}-backports main" > \
 /etc/apt/sources.list.d/${DIST}-backports.list

apt-get update
```

#### RHEL/CentOS/Fedora Repositories <a id="package-repositories-rhel-centos-fedora"></a>

RHEL/CentOS (6, 7, 8):

```
rpm --import https://packages.icinga.com/icinga.key

wget https://packages.icinga.com/epel/ICINGA-testing.repo -O /etc/yum.repos.d/ICINGA-testing.repo
```

Fedora 31:

```
rpm --import https://packages.icinga.com/icinga.key

wget https://packages.icinga.com/fedora/ICINGA-testing.repo -O /etc/yum.repos.d/ICINGA-testing.repo
```

##### RHEL/CentOS EPEL Repository <a id="package-repositories-rhel-epel"></a>

The packages for RHEL/CentOS depend on other packages which are distributed
as part of the [EPEL repository](https://fedoraproject.org/wiki/EPEL).

CentOS 8 additionally needs the PowerTools repository for EPEL:

```
dnf install 'dnf-command(config-manager)'
dnf config-manager --set-enabled PowerTools

dnf install epel-release
```

CentOS 7/6:

```
yum install epel-release
```

If you are using RHEL you need to additionally enable the `optional` and `codeready-builder`
repository before installing the [EPEL rpm package](https://fedoraproject.org/wiki/EPEL#How_can_I_use_these_extra_packages.3F).

RHEL 8:

```
ARCH=$( /bin/arch )

subscription-manager repos --enable rhel-8-server-optional-rpms
subscription-manager repos --enable "codeready-builder-for-rhel-8-${ARCH}-rpms"

dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
```

RHEL 7:

```
subscription-manager repos --enable rhel-7-server-optional-rpms
yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
```

RHEL 6:

```
subscription-manager repos --enable rhel-6-server-optional-rpms
yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-6.noarch.rpm
```

#### SLES/OpenSUSE Repositories <a id="package-repositories-sles-opensuse"></a>

The release repository also provides the required Boost 1.66+ packages
since v2.11.

SLES 15/12:

```
rpm --import https://packages.icinga.com/icinga.key

zypper ar https://packages.icinga.com/SUSE/ICINGA-testing.repo
zypper ref
```

openSUSE:

```
rpm --import https://packages.icinga.com/icinga.key

zypper ar https://packages.icinga.com/openSUSE/ICINGA-testing.repo
zypper ref
```

### Installing Icinga 2 <a id="installing-icinga2"></a>

You can install Icinga 2 by using your distribution's package manager
to install the `icinga2` package. The following commands must be executed
with `root` permissions unless noted otherwise.

Debian/Ubuntu:

```
apt-get install icinga2
```

RHEL/CentOS 8 and Fedora:

```
dnf install icinga2
systemctl enable icinga2
systemctl start icinga2
```

RHEL/CentOS 7:

```
yum install icinga2
systemctl enable icinga2
systemctl start icinga2
```

RHEL/CentOS 6:

```
yum install icinga2
chkconfig icinga2 on
service icinga2 start
```

SLES/openSUSE:

```
zypper install icinga2
```

FreeBSD:

```
pkg install icinga2
```

Alpine Linux:

```
apk add icinga2
```

## Setting up Check Plugins <a id="setting-up-check-plugins"></a>

Without plugins Icinga 2 does not know how to check external services. The
[Monitoring Plugins Project](https://www.monitoring-plugins.org/) provides
an extensive set of plugins which can be used with Icinga 2 to check whether
services are working properly.

These plugins are required to make the [example configuration](04-configuration.md#configuring-icinga2-overview)
work out-of-the-box.

For your convenience here is a list of package names for some of the more
popular operating systems/distributions:

OS/Distribution        | Package Name       | Repository                | Installation Path
-----------------------|--------------------|---------------------------|----------------------------
RHEL/CentOS            | nagios-plugins-all | [EPEL](02-installation.md#package-repositories-rhel-epel) | /usr/lib64/nagios/plugins
SLES/OpenSUSE          | monitoring-plugins | [server:monitoring](https://build.opensuse.org/project/repositories/server:monitoring) | /usr/lib/nagios/plugins
Debian/Ubuntu          | monitoring-plugins | -                         | /usr/lib/nagios/plugins
FreeBSD                | monitoring-plugins | -                         | /usr/local/libexec/nagios
Alpine Linux           | monitoring-plugins | -                         | /usr/lib/monitoring-plugins

The recommended way of installing these standard plugins is to use your
distribution's package manager.

Depending on which directory your plugins are installed into you may need to
update the global `PluginDir` constant in your [Icinga 2 configuration](04-configuration.md#constants-conf).
This constant is used by the check command definitions contained in the Icinga Template Library
to determine where to find the plugin binaries.

> **Note**
>
> Please refer to the [service monitoring](05-service-monitoring.md#service-monitoring-plugins) chapter for details about how to integrate
> additional check plugins into your Icinga 2 setup.

### Debian/Ubuntu <a id="setting-up-check-plugins-debian-ubuntu"></a>

```
apt-get install monitoring-plugins
```

### RHEL/CentOS/Fedora <a id="setting-up-check-plugins-rhel-centos-fedora"></a>

The packages for RHEL/CentOS depend on other packages which are distributed
as part of the [EPEL repository](02-installation.md#package-repositories-rhel-epel).

RHEL/CentOS 8:

```
dnf install nagios-plugins-all
```

RHEL/CentOS 7/6:

```
yum install nagios-plugins-all
```

Fedora:

```
dnf install nagios-plugins-all
```

### SLES/openSUSE <a id="setting-up-check-plugins-sles-opensuse"></a>

The packages for SLES/OpenSUSE depend on other packages which are distributed
as part of the [server:monitoring repository](https://build.opensuse.org/project/repositories/server:monitoring).
Please make sure to enable this repository beforehand.

```
zypper install monitoring-plugins
```

### FreeBSD <a id="setting-up-check-plugins-freebsd"></a>

```
pkg install monitoring-plugins
```

### Alpine Linux <a id="setting-up-check-plugins-alpine"></a>

```
apk add monitoring-plugins
```

Note: For Alpine you don't need to explicitly add the `monitoring-plugins` package since it is a dependency of
`icinga2` and is pulled automatically.

## Running Icinga 2 <a id="running-icinga2"></a>

### Systemd Service <a id="systemd-service"></a>

The majority of supported distributions use systemd. The
Icinga 2 packages automatically install the necessary systemd unit files.

The Icinga 2 systemd service can be (re-)started, reloaded, stopped and also
queried for its current status.

```
systemctl status icinga2

icinga2.service - Icinga host/service/network monitoring system
   Loaded: loaded (/usr/lib/systemd/system/icinga2.service; disabled)
   Active: active (running) since Mi 2014-07-23 13:39:38 CEST; 15s ago
  Process: 21692 ExecStart=/usr/sbin/icinga2 -c ${ICINGA2_CONFIG_FILE} -d -e ${ICINGA2_ERROR_LOG} -u ${ICINGA2_USER} -g ${ICINGA2_GROUP} (code=exited, status=0/SUCCESS)
  Process: 21674 ExecStartPre=/usr/sbin/icinga2-prepare-dirs /etc/sysconfig/icinga2 (code=exited, status=0/SUCCESS)
 Main PID: 21727 (icinga2)
   CGroup: /system.slice/icinga2.service
           21727 /usr/sbin/icinga2 -c /etc/icinga2/icinga2.conf -d -e /var/log/icinga2/error.log -u icinga -g icinga --no-stack-rlimit

Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 309 Service(s).
Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 1 User(s).
Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 15 Notification(s).
Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 4 ScheduledDowntime(s).
Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 1 UserGroup(s).
Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 1 IcingaApplication(s).
Jul 23 13:39:38 nbmif icinga2[21692]: [2014-07-23 13:39:38 +0200] information/ConfigItem: Checked 8 Dependency(s).
Jul 23 13:39:38 nbmif systemd[1]: Started Icinga host/service/network monitoring system.
```

The `systemctl` command supports the following actions:

  Command             | Description
  --------------------|------------------------
  start               | The `start` action starts the Icinga 2 daemon.
  stop                | The `stop` action stops the Icinga 2 daemon.
  restart             | The `restart` action is a shortcut for running the `stop` action followed by `start`.
  reload              | The `reload` action sends the `HUP` signal to Icinga 2 which causes it to restart. Unlike the `restart` action `reload` does not wait until Icinga 2 has restarted.
  status              | The `status` action checks if Icinga 2 is running.
  enable              | The `enable` action enables the service being started at system boot time (similar to `chkconfig`)

Examples:

```
systemctl enable icinga2

systemctl restart icinga2
Job for icinga2.service failed. See 'systemctl status icinga2.service' and 'journalctl -xn' for details.
```

If you're stuck with configuration errors, you can manually invoke the
[configuration validation](11-cli-commands.md#config-validation).

```
icinga2 daemon -C
```

> **Tip**
>
> If you are running into fork errors with systemd enabled distributions,
> please check the [troubleshooting chapter](15-troubleshooting.md#check-fork-errors).

### Init Script <a id="init-script"></a>

Icinga 2's init script is installed in `/etc/init.d/icinga2` (`/usr/local/etc/rc.d/icinga2` on FreeBSD) by default:

```
/etc/init.d/icinga2

Usage: /etc/init.d/icinga2 {start|stop|restart|reload|checkconfig|status}
```

The init script supports the following actions:

  Command             | Description
  --------------------|------------------------
  start               | The `start` action starts the Icinga 2 daemon.
  stop                | The `stop` action stops the Icinga 2 daemon.
  restart             | The `restart` action is a shortcut for running the `stop` action followed by `start`.
  reload              | The `reload` action sends the `HUP` signal to Icinga 2 which causes it to restart. Unlike the `restart` action `reload` does not wait until Icinga 2 has restarted.
  checkconfig         | The `checkconfig` action checks if the `/etc/icinga2/icinga2.conf` configuration file contains any errors.
  status              | The `status` action checks if Icinga 2 is running.

By default, the Icinga 2 daemon is running as `icinga` user and group
using the init script. Using Debian packages the user and group are set to
`nagios` for historical reasons.


### FreeBSD <a id="running-icinga2-freebsd"></a>

On FreeBSD you need to enable icinga2 in your rc.conf

```
sysrc icinga2_enable=yes

service icinga2 restart
```

### SELinux <a id="running-icinga2-selinux"></a>

SELinux is a mandatory access control (MAC) system on Linux which adds
a fine-grained permission system for access to all system resources such
as files, devices, networks and inter-process communication.

Icinga 2 provides its own SELinux policy. `icinga2-selinux` is a policy package
for Red Hat Enterprise Linux 7 and derivatives. The package runs the targeted policy
which confines Icinga 2 including enabled features and running commands.

RHEL/CentOS 8 and Fedora:

```
dnf install icinga2-selinux
```

RHEL/CentOS 7:

```
yum install icinga2-selinux
```

Read more about SELinux in [this chapter](22-selinux.md#selinux).

## Setting up Icinga DB <a id="setting-up-icingadb"></a>

Icinga DB is a data backend that holds all your monitoring data for your Icinga Web 2 to display.

First, make sure to setup the Icinga DB daemon itself and its database backends (Redis and MySQL) by following the [installation instructions](https://icinga.com/docs/icingadb/latest/doc/02-Installation/).

### Enabling the Icinga DB feature <a id="enabling-icinga-db"></a>

Icinga 2 provides a configuration file that is installed in
`/etc/icinga2/features-available/icingadb.conf`. You can update
the Redis credentials in this file.

All available attributes are explained in the
[Icinga DB object](09-object-types.md#objecttype-icingadb)
chapter.

You can enable the `icingadb` feature configuration file using
`icinga2 feature enable`:

```
# icinga2 feature enable icingadb
Module 'icingadb' was enabled.
Make sure to restart Icinga 2 for these changes to take effect.
```

Restart Icinga 2.

```
systemctl restart icinga2
```

Alpine Linux:

```
rc-service icinga2 restart
```

## Setting up Icinga Web 2 <a id="setting-up-icingaweb2"></a>

Icinga 2 can be used with Icinga Web 2 and a variety of modules.
This chapter explains how to set up Icinga Web 2.

### Setting Up Icinga 2 REST API <a id="setting-up-rest-api"></a>

Icinga Web 2 and other web interfaces require the [REST API](12-icinga2-api.md#icinga2-api-setup)
to send actions (reschedule check, etc.) and query object details.

You can run the CLI command `icinga2 api setup` to enable the
`api` [feature](11-cli-commands.md#enable-features) and set up
certificates as well as a new API user `root` with an auto-generated password in the
`/etc/icinga2/conf.d/api-users.conf` configuration file:

```
icinga2 api setup
```

Edit the `api-users.conf` file and add a new ApiUser object. Specify the [permissions](12-icinga2-api.md#icinga2-api-permissions)
attribute with minimal permissions required by Icinga Web 2.

```
vim /etc/icinga2/conf.d/api-users.conf

object ApiUser "icingaweb2" {
  password = "Wijsn8Z9eRs5E25d"
  permissions = [ "status/query", "actions/*", "objects/modify/*", "objects/query/*" ]
}
```

Restart Icinga 2 to activate the configuration.

```
systemctl restart icinga2
```

Alpine Linux:

```
rc-service icinga2 restart
```

### Installing Icinga Web 2 <a id="installing-icingaweb2"></a>

Please consult the [Icinga Web 2 documentation](https://icinga.com/docs/icingaweb2/latest/)
for further instructions on how to install Icinga Web 2.

Consult the [Icinga DB Web documentation](https://icinga.com/docs/icingadb/latest/icingadb-web/doc/01-About/) on how to connect Icinga Web 2 with Icinga DB.

## Addons <a id="install-addons"></a>

A number of additional features are available in the form of addons. A list of
popular addons is available in the
[Addons and Plugins](13-addons.md#addons) chapter.

## Configuration Syntax Highlighting <a id="configuration-syntax-highlighting"></a>

Icinga 2 provides configuration examples for syntax highlighting using the `vim` and `nano` editors.
The RHEL and SUSE package `icinga2-common` installs these files into `/usr/share/doc/icinga2-common-[x.x.x]/syntax`
(where `[x.x.x]` is the version number, e.g. `2.4.3` or `2.4.4`). Sources provide these files in `tools/syntax`.
On Debian systems the `icinga2-common` package provides only the Nano configuration file (`/usr/share/nano/icinga2.nanorc`);
to obtain the Vim configuration, please install the extra package `vim-icinga2`. The files are located in `/usr/share/vim/addons`.

### Configuration Syntax Highlighting using Vim <a id="configuration-syntax-highlighting-vim"></a>

Install the package `vim-icinga2` with your distribution's package manager.

Debian/Ubuntu:

```
apt-get install vim-icinga2 vim-addon-manager
vim-addon-manager -w install icinga2
Info: installing removed addon 'icinga2' to /var/lib/vim/addons
```

RHEL/CentOS 8 and Fedora:

```
dnf install vim-icinga2
```

RHEL/CentOS 7/6:

```
yum install vim-icinga2
```

SLES/openSUSE:

```
zypper install vim-icinga2
```

Alpine Linux:

```
apk add icinga2-vim
```

Ensure that syntax highlighting is enabled e.g. by editing the user's `vimrc`
configuration file:

```
# vim ~/.vimrc
syntax on
```

Test it:

```
# vim /etc/icinga2/conf.d/templates.conf
```

![Vim with syntax highlighting](images/installation/vim-syntax.png "Vim with Icinga 2 syntax highlighting")

### Configuration Syntax Highlighting using Nano <a id="configuration-syntax-highlighting-nano"></a>

Install the package `nano-icinga2` with your distribution's package manager.

Debian/Ubuntu:

**Note:** The syntax files are installed with the `icinga2-common` package already.

RHEL/CentOS 8 and Fedora:

```
dnf install nano-icinga2
```

RHEL/CentOS 7/6:

```
yum install nano-icinga2
```

SLES/openSUSE:

```
zypper install nano-icinga2
```

Copy the `/etc/nanorc` sample file to your home directory.

```
$ cp /etc/nanorc ~/.nanorc
```

Include the `icinga2.nanorc` file.

```
$ vim ~/.nanorc

## Icinga 2
include "/usr/share/nano/icinga2.nanorc"
```

Test it:

```
$ nano /etc/icinga2/conf.d/templates.conf
```

![Nano with syntax highlighting](images/installation/nano-syntax.png "Nano with Icinga 2 syntax highlighting")

## Installation Overview <a id="installation-overview"></a>

### Enabled Features during Installation <a id="installation-overview-enabled-features"></a>

The default installation will enable three features required for a basic
Icinga 2 installation:

* `checker` for executing checks
* `notification` for sending notifications
* `mainlog` for writing the `icinga2.log` file

You can verify that by calling `icinga2 feature list`
[CLI command](11-cli-commands.md#cli-command-feature) to see which features are
enabled and disabled.

```
# icinga2 feature list
Disabled features: api command compatlog debuglog gelf graphite icingastatus ido-mysql ido-pgsql influxdb livestatus opentsdb perfdata statusdata syslog
Enabled features: checker mainlog notification
```

### Installation Paths <a id="installation-overview-paths"></a>

By default Icinga 2 uses the following files and directories:

  Path                                		| Description
  ----------------------------------------------|------------------------------------
  /etc/icinga2                        		| Contains Icinga 2 configuration files.
  /usr/lib/systemd/system/icinga2.service 	| The Icinga 2 systemd service file on systems using systemd.
  /etc/systemd/system/icinga2.service.d/limits.conf | On distributions with systemd >227, additional service limits are required.
  /etc/init.d/icinga2                 		| The Icinga 2 init script on systems using SysVinit or OpenRC.
  /usr/sbin/icinga2                   		| Shell wrapper for the Icinga 2 binary.
  /usr/lib\*/icinga2				| Libraries and the Icinga 2 binary (use `find /usr -type f -name icinga2` to locate the binary path).
  /usr/share/doc/icinga2              		| Documentation files that come with Icinga 2.
  /usr/share/icinga2/include          		| The Icinga Template Library and plugin command configuration.
  /var/lib/icinga2                    		| Icinga 2 state file, cluster log, master CA, node certificates and configuration files (cluster, api).
  /var/run/icinga2                    		| PID file.
  /var/run/icinga2/cmd                		| Command pipe and Livestatus socket.
  /var/cache/icinga2                  		| status.dat/objects.cache, icinga2.debug files.
  /var/spool/icinga2                  		| Used for performance data spool files.
  /var/log/icinga2                    		| Log file location and compat/ directory for the CompatLogger feature.

FreeBSD uses slightly different paths:

By default Icinga 2 uses the following files and directories:

  Path                                | Description
  ------------------------------------|------------------------------------
  /usr/local/etc/icinga2              | Contains Icinga 2 configuration files.
  /usr/local/etc/rc.d/icinga2         | The Icinga 2 init script.
  /usr/local/sbin/icinga2             | Shell wrapper for the Icinga 2 binary.
  /usr/local/lib/icinga2              | Libraries and the Icinga 2 binary.
  /usr/local/share/doc/icinga2        | Documentation files that come with Icinga 2.
  /usr/local/share/icinga2/include    | The Icinga Template Library and plugin command configuration.
  /var/lib/icinga2                    | Icinga 2 state file, cluster log, master CA, node certificates and configuration files (cluster, api).
  /var/run/icinga2                    | PID file.
  /var/run/icinga2/cmd                | Command pipe and Livestatus socket.
  /var/cache/icinga2                  | status.dat/objects.cache, icinga2.debug files.
  /var/spool/icinga2                  | Used for performance data spool files.
  /var/log/icinga2                    | Log file location and compat/ directory for the CompatLogger feature.


## Backup <a id="install-backup"></a>

Ensure to include the following in your backups:

* Configuration files in `/etc/icinga2`
* Certificate files in `/var/lib/icinga2/ca` (Master CA key pair) and `/var/lib/icinga2/certs` (node certificates)
* Runtime files in `/var/lib/icinga2`
* Optional: IDO database backup

## Backup: Database <a id="install-backup-database"></a>

MySQL/MariaDB:

* [Documentation](https://mariadb.com/kb/en/library/backup-and-restore-overview/)

PostgreSQL:

* [Documentation](https://www.postgresql.org/docs/9.3/static/backup.html)

