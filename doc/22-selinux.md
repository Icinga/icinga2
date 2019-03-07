# SELinux <a id="selinux"></a>

## Introduction <a id="selinux-introduction"></a>

SELinux is a mandatory access control (MAC) system on Linux which adds a fine-grained permission system for access to all system resources such as files, devices, networks and inter-process communication.

The most important questions are answered briefly in the [FAQ of the SELinux Project](https://selinuxproject.org/page/FAQ). For more details on SELinux and how to actually use and administrate it on your system have a look at [Red Hat Enterprise Linux 7 - SELinux User's and Administrator's Guide](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/7/html/SELinux_Users_and_Administrators_Guide/index.html). For a simplified (and funny) introduction download the [SELinux Coloring Book](https://github.com/mairin/selinux-coloring-book).

This documentation will use a format similar to the SELinux User's and Administrator's Guide.

### Policy <a id="selinux-policy"></a>

Icinga 2 provides its own SELinux policy. Development target is a policy package for Red Hat Enterprise Linux 7 and derivatives running the targeted policy which confines Icinga 2 with all features and all checks executed. All other distributions will require some tweaks.

### Installation <a id="selinux-policy-installation"></a>

There are two ways of installing the SELinux Policy for Icinga 2 on Enterprise Linux 7. The preferred way is to install the package. The other option involves installing the SELinux policy manually which might be necessary if you need some fixes which haven't made their way into a release yet.

If the system runs in enforcing mode and you encounter problems you can set Icinga 2's domain to permissive mode.

```
# sestatus
SELinux status:                 enabled
SELinuxfs mount:                /sys/fs/selinux
SELinux root directory:         /etc/selinux
Loaded policy name:             targeted
Current mode:                   enforcing
Mode from config file:          enforcing
Policy MLS status:              enabled
Policy deny_unknown status:     allowed
Max kernel policy version:      28
```

You can change the configured mode by editing `/etc/selinux/config` and the current mode by executing `setenforce 0`.

#### Package installation <a id="selinux-policy-installation-package"></a>

Simply add the `icinga2-selinux` package to your installation.

```
# yum install icinga2-selinux
```

Ensure that the `icinga2` process is running in its own `icinga2_t` domain after installing the policy package:

```
# systemctl restart icinga2.service
# ps -eZ | grep icinga2
system_u:system_r:icinga2_t:s0   2825 ?        00:00:00 icinga2
```

#### Manual installation <a id="selinux-policy-installation-manual"></a>

This section describes the installation to support development and testing. It assumes that Icinga 2 is already installed from packages and running on the system.

As a prerequisite install the `git`, `selinux-policy-devel` and `audit` packages. Enable and start the audit daemon afterwards:

```
# yum install git selinux-policy-devel audit
# systemctl enable auditd.service
# systemctl start auditd.service
```

After that clone the icinga2 git repository:

```
# git clone https://github.com/icinga/icinga2
```

To create and install the policy package run the installation script which also labels the resources. (The script assumes Icinga 2 was started once after system startup, the labeling of the port will only happen once and fail later on.)

```
# cd tools/selinux/
# ./icinga.sh
```

After that restart Icinga 2 and verify it running in its own domain `icinga2_t`.

```
# systemctl restart icinga2.service
# ps -eZ | grep icinga2
system_u:system_r:icinga2_t:s0   2825 ?        00:00:00 icinga2
```

### General <a id="selinux-policy-general"></a>

When the SELinux policy package for Icinga 2 is installed, the Icinga 2 daemon (icinga2) runs in its own domain `icinga2_t` and is separated from other confined services.

Files have to be labeled correctly in order for Icinga 2 to be able to access them. For example the Icinga 2 log files have to have the `icinga2_log_t` label. Also the API port is labeled with `icinga_port_t`. Furthermore Icinga 2 can open high ports and UNIX sockets to connect to databases and features like Graphite. It executes the Nagios plugins and transitions to their context if those are labeled for example `nagios_services_plugin_exec_t` or `nagios_system_plugin_exec_t`.

Additionally the Apache web server is allowed to connect to Icinga 2's command pipe in order to allow web interfaces to send commands to icinga2. This will perhaps change later on while investigating Icinga Web 2 for SELinux!

### Types <a id="selinux-policy-types"></a>

The command pipe is labeled `icinga2_command_t` and other services can request access to it by using the interface `icinga2_send_commands`.

The nagios plugins use their own contexts and icinga2 will transition to it. This means plugins have to be labeled correctly for their required permissions. The plugins installed from package should have set their permissions by the corresponding policy module and you can restore them using `restorecon -R -v /usr/lib64/nagios/plugins/`. To label your own plugins use `chcon -t type /path/to/plugin`, for the type have a look at table below.

Type                              | Domain                       | Use case                                                         | Provided by policy package
----------------------------------|------------------------------|------------------------------------------------------------------|---------------------------
nagios_admin_plugin_exec_t        | nagios_admin_plugin_t        | Plugins which require require read access on all file attributes | nagios
nagios_checkdisk_plugin_exec_t    | nagios_checkdisk_plugin_t    | Plugins which require read access to all filesystem attributes   | nagios
nagios_mail_plugin_exec_t         | nagios_mail_plugin_t         | Plugins which access the local mail service                      | nagios
nagios_services_plugin_exec_t     | nagios_services_plugin_t     | Plugins monitoring network services                              | nagios
nagios_system_plugin_exec_t       | nagios_system_plugin_t       | Plugins checking local system state                              | nagios
nagios_unconfined_plugin_exec_t   | nagios_unconfined_plugin_t   | Plugins running without confinement                              | nagios
nagios_eventhandler_plugin_exec_t | nagios_eventhandler_plugin_t | Eventhandler (actually running unconfined)                       | nagios
nagios_openshift_plugin_exec_t    | nagios_openshift_plugin_t    | Plugins monitoring openshift                                     | nagios
nagios_notification_plugin_exec_t | nagios_notification_plugin_t | Notification commands                                            | icinga (will be moved later)

If one of those plugin domains causes problems you can set it to permissive by executing `semanage permissive -a domain`.

The policy provides a role `icinga2adm_r` for confining an user which enables an administrative user managing only Icinga 2 on the system. This user will also execute the plugins in their domain instead of the users one, so you can verify their execution with the same restrictions like they have when executed by icinga2.

### Booleans <a id="selinux-policy-booleans"></a>

SELinux is based on the least level of access required for a service to run. Using booleans you can grant more access in a defined way. The Icinga 2 policy package provides the following booleans.

**icinga2_can_connect_all** 

Having this boolean enabled allows icinga2 to connect to all ports. This can be necessary if you use features which connect to unconfined services, for example the [influxdb writer](14-features.md#influxdb-writer).

**httpd_can_write_icinga2_command** 

To allow httpd to write to the command pipe of icinga2 this boolean has to be enabled. This is enabled by default, if not needed you can disable it for more security.

**httpd_can_connect_icinga2_api** 

Enabling this boolean allows httpd to connect to the API of icinga2 (Ports labeled `icinga2_port_t`). This is enabled by default, if not needed you can disable it for more security.

### Configuration Examples <a id="selinux-policy-examples"></a>

#### Run the icinga2 service permissive <a id="selinux-policy-examples-permissive"></a>

If problems occur while running the system in enforcing mode and those problems are only caused by the policy of the icinga2 domain, you can set this domain to permissive instead of the complete system. This can be done by executing `semanage permissive -a icinga2_t`.

Make sure to report the bugs in the policy afterwards.

#### Confining a plugin <a id="selinux-policy-examples-plugin"></a>

Download and install a plugin, for example check_mysql_health.

```
# wget https://labs.consol.de/download/shinken-nagios-plugins/check_mysql_health-2.1.9.2.tar.gz
# tar xvzf check_mysql_health-2.1.9.2.tar.gz
# cd check_mysql_health-2.1.9.2/
# ./configure --libexecdir /usr/lib64/nagios/plugins
# make
# make install
```

It is labeled `nagios_unconfined_plugins_exec_t` by default, so it runs without restrictions.

```
# ls -lZ /usr/lib64/nagios/plugins/check_mysql_health
-rwxr-xr-x. root root system_u:object_r:nagios_unconfined_plugin_exec_t:s0 /usr/lib64/nagios/plugins/check_mysql_health
```

In this case the plugin is monitoring a service, so it should be labeled `nagios_services_plugin_exec_t` to restrict its permissions.

```
# chcon -t nagios_services_plugin_exec_t /usr/lib64/nagios/plugins/check_mysql_health
# ls -lZ /usr/lib64/nagios/plugins/check_mysql_health
-rwxr-xr-x. root root system_u:object_r:nagios_services_plugin_exec_t:s0 /usr/lib64/nagios/plugins/check_mysql_health
```

The plugin still runs fine but if someone changes the script to do weird stuff it will fail to do so.

#### Allow icinga to connect to all ports. <a id="selinux-policy-examples-connectall"></a>

You are running graphite on a different port than `2003` and want `icinga2` to connect to it.

Change the port value for the graphite feature according to your graphite installation before enabling it.

```
# cat /etc/icinga2/features-enabled/graphite.conf
/**
 * The GraphiteWriter type writes check result metrics and
 * performance data to a graphite tcp socket.
 */

library "perfdata"

object GraphiteWriter "graphite" {
  //host = "127.0.0.1"
  //port = 2003
  port = 2004
}
# icinga2 feature enable graphite
```

Before you restart the icinga2 service allow it to connect to all ports by enabling the boolean ´icinga2_can_connect_all` (now and permanent).

```
# setsebool icinga2_can_connect_all true
# setsebool -P icinga2_can_connect_all true
```

If you restart the daemon now it will successfully connect to graphite.

#### Confining a user <a id="selinux-policy-examples-user"></a>

If you want to have an administrative account capable of only managing icinga2 and not the complete system, you can restrict the privileges by confining
this user. This is completly optional!

Start by adding the Icinga 2 administrator role `icinga2adm_r` to the administrative SELinux user `staff_u`.

```
# semanage user -m -R "staff_r sysadm_r system_r unconfined_r icinga2adm_r" staff_u
```

Confine your user login and create a sudo rule.

```
# semanage login -a dirk -s staff_u
# echo "dirk ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/dirk
```

Login to the system using ssh and verify your id.

```
$ id -Z
staff_u:staff_r:staff_t:s0-s0:c0.c1023
```

Try to execute some commands as root using sudo.

```
$ sudo id -Z
staff_u:staff_r:staff_t:s0-s0:c0.c1023
$ sudo vi /etc/icinga2/icinga2.conf
"/etc/icinga2/icinga2.conf" [Permission Denied]
$ sudo cat /var/log/icinga2/icinga2.log
cat: /var/log/icinga2/icinga2.log: Keine Berechtigung
$ sudo systemctl reload icinga2.service
Failed to get D-Bus connection: No connection to service manager.
```

Those commands fail because you only switch to root but do not change your SELinux role. Try again but tell sudo also to switch the SELinux role and type.

```
$ sudo -r icinga2adm_r -t icinga2adm_t id -Z
staff_u:icinga2adm_r:icinga2adm_t:s0-s0:c0.c1023
$ sudo -r icinga2adm_r -t icinga2adm_t vi /etc/icinga2/icinga2.conf
"/etc/icinga2/icinga2.conf"
$ sudo -r icinga2adm_r -t icinga2adm_t cat /var/log/icinga2/icinga2.log
[2015-03-26 20:48:14 +0000] information/DynamicObject: Dumping program state to file '/var/lib/icinga2/icinga2.state'
$ sudo -r icinga2adm_r -t icinga2adm_t systemctl reload icinga2.service
```

Now the commands will work, but you have always to remember to add the arguments, so change the sudo rule to set it by default.

```
# echo "dirk ALL=(ALL) ROLE=icinga2adm_r TYPE=icinga2adm_t NOPASSWD: ALL" > /etc/sudoers.d/dirk
```

Now try the commands again without providing the role and type and they will work, but if you try to read apache logs or restart apache for example it will still fail.

```
$ sudo cat /var/log/httpd/error_log
/bin/cat: /var/log/httpd/error_log: Keine Berechtigung
$ sudo systemctl reload httpd.service
Failed to issue method call: Access denied
```

## Bugreports <a id="selinux-bugreports"></a>

If you experience any problems while running in enforcing mode try to reproduce it in permissive mode. If the problem persists it is not related to SELinux because in permissive mode SELinux will not deny anything.

After some feedback Icinga 2 is now running in a enforced domain, but still adds also some rules for other necessary services so no problems should occure at all. But you can help to enhance the policy by testing Icinga 2 running confined by SELinux.

Please add the following information to [bug reports](https://icinga.com/community/):

* Versions, configuration snippets, etc.
* Output of `semodule -l | grep -e icinga2 -e nagios -e apache`
* Output of `ps -eZ | grep icinga2`
* Output of `semanage port -l | grep icinga2`
* Output of `audit2allow -li /var/log/audit/audit.log`

If access to a file is blocked and you can tell which one please provided the output of `ls -lZ /path/to/file` (and perhaps the directory above).

If asked for full audit.log add `-w /etc/shadow -p w` to `/etc/audit/rules.d/audit.rules`, restart the audit daemon, reproduce the problem and add `/var/log/audit/audit.log` to the bug report. With the added audit rule it will include the path of files access was denied to.

If asked to provide full audit log with dontaudit rules disabled executed `semodule -DB` before reproducing the problem. After that enable the rules again to prevent auditd spamming your logfile by executing `semodule -B`.
