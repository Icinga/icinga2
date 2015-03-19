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

After that restart Icinga 2 and verify it running in its own domain `icinga2_t`.

    # systemctl restart icinga2.service
    # ps -eZ | grep icinga2
    system_u:system_r:icinga2_t:s0   2825 ?        00:00:00 icinga2

### <a id="selinux-policy-general"></a> General

### <a id="selinux-policy-types"></a> Types

### <a id="selinux-policy-examples"></a> Configuration Examples

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
