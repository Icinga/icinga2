# <a id="selinux"></a> SELinux

## <a id="selinux-introduction"></a> Introduction

SELinux is a mandatory access control (MAC) system on Linux which adds a fine granular permission system for access to all resources on the system such as files, devices, networks and inter-process communication.

The most important questions are answered briefly in the [FAQ of the SELinux Project](http://selinuxproject.org/page/FAQ). For more details on SELinux and how to actually use and administrate it on your systems have a look at [Red Hat Enterprise Linux 7 - SELinux User's and Administrator's Guide](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/7/html/SELinux_Users_and_Administrators_Guide/index.html). For an simplified (and funny) introduction download the [SELinux Coloring Book](https://github.com/mairin/selinux-coloring-book).

This documentation will use a similar format like the SELinux User's and Administrator's Guide.

### <a id="selinux-policy"></a> Policy

Icinga 2 is providing its own SELinux Policy. At the moment it is not upstreamed to the reference policy because it is under development. Target of the development is a policy package for Red Hat Enterprise Linux 7 and its derivates running the targeted policy which confines Icinga2 with all features and all checks executed.

### <a id="selinux-policy-installation"></a> Installation

Later the policy will be installed by a seperate package and this section will be removed. Now it describes the installation to support development and testing. It assumes that Icinga 2 is already installed from packages and running on the system.

The policy package will run the daemon in a permissive domain so nothing will be denied also if the system runs in enforcing mode, so please make sure to run the system in this mode.

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

You can change the configured mode by editing `/etc/selinux/config` and the current mode by executing `setenforce 0`.

As a prerequisite install the `git`, `selinux-policy-devel` and `audit` package. Enable and start the audit daemon afterwards.

    # yum install git selinux-policy-devel audit
    # systemctl enable auditd.service
    # systemctl start auditd.service

After that clone the icinga2 git repository and checkout the feature branch.

    # git clone git://git.icinga.org/icinga2.git
    # git checkout feature/rpm-selinux-8332

To create and install the policy package run the installation script which also labels the resources. (The script assumes Icinga 2 was started once after system startup, the labeling of the port will only happen once and fail later on.)

    # cd icinga2/tools/selinux/
    # ./icinga.sh

Some changes to the systemd scripts are also required to handle file contexts correctly. This is at the moment only included in the feature branch, so it has to be copied manually.

    # cp ../../etc/initsystem/{prepare-dirs,safe-reload} /usr/lib/icinga2/

After that restart Icinga 2 and verify it running in its own domain `icinga2_t`.

    # systemctl restart icinga2.service
    # ps -eZ | grep icinga2
    system_u:system_r:icinga2_t:s0   2825 ?        00:00:00 icinga2

### <a id="selinux-policy-general"></a> General

When the SELinux policy package for Icinga 2 is installed, the Icinga 2 daemon (icinga2) runs in its own domain `icinga2_t` and is separated from other confined services.

Files have to be labeled correctly for allowing icinga2 access to it. For example it writes to its own log files labeled `icinga2_log_t`. Also the API port is labeled `icinga_port_t` and the icinga2 is allowed to manage it. Furthermore icinga2 can open high ports and unix sockets to connect to databases and features like graphite. It executes the nagios plugins and transitions to their context if those are labeled for example `nagios_services_plugin_exec_t` or `nagios_system_plugin_exec_t`.

Additional the Apache webserver is allowed to connect to the Command pipe of Icinga 2 to allow web interfaces sending commands to icinga2. This will perhaps change later on while investigating Icinga Web 2 for SELinux!

### <a id="selinux-policy-types"></a> Types

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

The policy provides a role `icinga2adm_r` for confining an user which enables an administrative user managing only Icinga 2 on the system.

### <a id="selinux-policy-examples"></a> Configuration Examples

#### <a id="selinux-policy-examples-plugin"></a> Confining a plugin

Download and install a plugin, for example check_mysql_health.

    # wget http://labs.consol.de/download/shinken-nagios-plugins/check_mysql_health-2.1.9.2.tar.gz
    # tar xvzf check_mysql_health-2.1.9.2.tar.gz 
    # cd check_mysql_health-2.1.9.2/
    # ./configure --libexecdir /usr/lib64/nagios/plugins
    # make
    # make install

It is label `nagios_unconfined_plugins_exec_t` by default, so it runs without restrictions.

    # ls -lZ /usr/lib64/nagios/plugins/check_mysql_health 
    -rwxr-xr-x. root root system_u:object_r:nagios_unconfined_plugin_exec_t:s0 /usr/lib64/nagios/plugins/check_mysql_health

In this case the plugin is monitoring a service, so it should be labeled `nagios_services_plugin_exec_t` to restrict its permissions.

    # chcon -t nagios_services_plugin_exec_t /usr/lib64/nagios/plugins/check_mysql_health
    # ls -lZ /usr/lib64/nagios/plugins/check_mysql_health
    -rwxr-xr-x. root root system_u:object_r:nagios_services_plugin_exec_t:s0 /usr/lib64/nagios/plugins/check_mysql_health

The plugin still runs fine but if someone changes the script to do weird stuff it will fail to do so.

#### <a id="selinux-policy-examples-user"></a> Confining a user

Start by adding the Icinga 2 administrator role `icinga2adm_r` to the administrative SELinux user `staff_u`.

    # semanage user -m -R "staff_r sysadm_r system_r unconfined_r icinga2adm_r" staff_u

Confine your user login and create a sudo rule.

    # semanage login -a dirk -s staff_u
    # echo "dirk ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/dirk

Login to the system using ssh and verify your id.

    $ id -Z
    staff_u:staff_r:staff_t:s0-s0:c0.c1023

Try to execute some commands as root using sudo.

    $ sudo id -Z
    staff_u:staff_r:staff_t:s0-s0:c0.c1023
    $ sudo vi /etc/icinga2/icinga2.conf
    "/etc/icinga2/icinga2.conf" [Permission Denied]
    $ sudo cat /var/log/icinga2/icinga2.log
    cat: /var/log/icinga2/icinga2.log: Keine Berechtigung
    $ sudo systemctl reload icinga2.service
    Failed to get D-Bus connection: No connection to service manager.

Those commands fail because you only switch to root but do not change your SELinux role. Try again but tell sudo also to switch the SELinux role and type.

    $ sudo -r icinga2adm_r -t icinga2adm_t id -Z
    staff_u:icinga2adm_r:icinga2adm_t:s0-s0:c0.c1023
    $ sudo -r icinga2adm_r -t icinga2adm_t vi /etc/icinga2/icinga2.conf
    "/etc/icinga2/icinga2.conf"
    $ sudo -r icinga2adm_r -t icinga2adm_t cat /var/log/icinga2/icinga2.log
    [2015-03-26 20:48:14 +0000] information/DynamicObject: Dumping program state to file '/var/lib/icinga2/icinga2.state'
    $ sudo -r icinga2adm_r -t icinga2adm_t systemctl reload icinga2.service

Now the commands will work, but you have always to remember to add the arguments, so change the sudo rule to set it by default.

    # echo "dirk ALL=(ALL) ROLE=icinga2adm_r TYPE=icinga2adm_t NOPASSWD: ALL" > /etc/sudoers.d/dirk

Now try the commands again without providing the role and type and they will work, but if you try to read apache logs or restart apache for example it will still fail.

    $ sudo cat /var/log/httpd/error_log
    /bin/cat: /var/log/httpd/error_log: Keine Berechtigung
    $ sudo systemctl reload httpd.service
    Failed to issue method call: Access denied

## <a id="selinux-bugreports"></a> Bugreports

If you experience any problems while running in enforcing mode try to reproduce it in permissive mode. If the problem persists it is not related to SELinux because in permissive mode SELinux will not deny anything.

For now Icinga 2 is running in a permissive domain and adds also some rules for other necessary services so no problems should occure at all. But you can help to enhance the policy by testing Icinga 2 running confined by SELinux.

When filing a bug report please add the following information additionally to the [normal ones](https://www.icinga.org/icinga/faq/):
* Output of `semodule -l | grep -e icinga2 -e nagios -e apache`
* Output of `ps -eZ | grep icinga2`
* Output of `semanage port -l | grep icinga2`
* Output of `audit2allow -li /var/log/audit/audit.log`

If access to a file is blocked and you can tell which one please provided the output of `ls -lZ /path/to/file` (and perhaps the directory above).

If asked for full audit.log add `-w /etc/shadow -p w` to `/etc/audit/rules.d/audit.rules`, restart the audit daemon, reproduce the problem and add `/var/log/audit/audit.log` to the bug report. With the added audit rule it will include the path of files access was denied to.

If asked to provide full audit log with dontaudit rules disabled executed `semodule -DB` before reproducing the problem. After that enable the rules again to prevent auditd spamming your logfile by executing `semodule -B`.
