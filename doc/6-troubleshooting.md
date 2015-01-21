# <a id="troubleshooting"></a> Icinga 2 Troubleshooting

## <a id="troubleshooting-information-required"></a> Which information is required

* Which distribution and version
* How was Icinga 2 installed (and which repository in case)
* Run `icinga2 --version`
* Provide complete configuration snippets explaining your problem in detail
* Provide complete logs targetting your problem
* If the check command failed - what's the output of your manual plugin tests?
* In case of [debugging](#debug) Icinga 2, the full back traces and outputs

## <a id="troubleshooting-enable-debug-output"></a> Enable Debug Output

Run Icinga 2 in the foreground with debugging enabled. Specify the console
log severity as an additional parameter argument to `-x`.

    # /usr/sbin/icinga2 daemon -x notice

The log level can be one of `critical`, `warning`, `information`, `notice`
and `debug`.

Alternatively you can enable the debug log:

    # icinga2 feature enable debuglog
    # service icinga2 restart
    # tail -f /var/log/icinga2/debug.log

## <a id="list-configuration-objects"></a> List Configuration Objects

The `icinga2 object list` CLI command can be used to list all configuration objects and their
attributes. The tool also shows where each of the attributes was modified.

That way you can also identify which objects have been created from your [apply rules](#apply).

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

You can also filter by name and type:

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

## <a id="check-command-definitions"></a> Where are the check command definitions

Icinga 2 ships additional [plugin check command definitions](#plugin-check-commands) which are
included using

    include <itl>
    include <plugins>

in the [icinga2.conf](#icinga2-conf) configuration file. These configurations will be overridden
on upgrade, so please send modifications as proposed patches upstream. The default include path is set to
`LocalStateDir + "/share/icinga2/includes"`.

You should add your own command definitions to a new file in `conf.d/` called `commands.conf`
or similar.

## <a id="checks-not-executed"></a> Checks are not executed

* Check the debug log to see if the check command gets executed
* Verify that failed depedencies do not prevent command execution
* Make sure that the plugin is executable by the Icinga 2 user (run a manual test)
* Make sure the [checker](#features) feature is enabled.

Examples:

    # sudo -u icinga /usr/lib/nagios/plugins/check_ping -4 -H 127.0.0.1 -c 5000,100% -w 3000,80%

    # icinga2 feature enable checker
    The feature 'checker' is already enabled.


## <a id="notifications-not-sent"></a> Notifications are not sent

* Check the debug log to see if a notification is triggered
* If yes, verify that all conditions are satisfied
* Are any errors on the notification command execution logged?

Verify the following configuration

* Is the host/service `enable_notifications` attribute set, and if, to which value?
* Do the notification attributes `states`, `types`, `period` match the notification conditions?
* Do the user attributes `states`, `types`, `period` match the notification conditions?
* Are there any notification `begin` and `end` times configured?
* Make sure the [notification](#features) feature is enabled.
* Does the referenced NotificationCommand work when executed as Icinga user on the shell?

If notifications are to be sent via mail make sure that the mail program specified exists.
The name and location depends on the distribution so the preconfigured setting might have to be
changed on your system.

Examples:

    # icinga2 feature enable notification
    The feature 'notification' is already enabled.

## <a id="feature-not-working"></a> Feature is not working

* Make sure that the feature configuration is enabled by symlinking from `features-available/`
to `features-enabled` and that the latter is included in [icinga2.conf](#icinga2-conf).
* Are the feature attributes set correctly according to the documentation?
* Any errors on the logs?

## <a id="configuration-ignored"></a> Configuration is ignored

* Make sure that the line(s) are not [commented](#comments) (starting with `//` or `#`, or
encapsulated by `/* ... */`).
* Is the configuration file included in [icinga2.conf](#icinga2-conf)?

## <a id="configuration-attribute-inheritance"></a> Configuration attributes are inherited from

Icinga 2 allows you to import templates using the [import](#import) keyword. If these templates
contain additional attributes, your objects will automatically inherit them. You can override
or modify these attributes in the current object.

## <a id="troubleshooting-cluster"></a> Cluster Troubleshooting

You should configure the [cluster health checks](#cluster-health-check) if you haven't
done so already.

> **Note**
>
> Some problems just exist due to wrong file permissions or packet filters applied. Make
> sure to check these in the first place.

### <a id="troubleshooting-cluster-connection-errors"></a> Cluster Troubleshooting Connection Errors

General connection errors normally lead you to one of the following problems:

* Wrong network configuration
* Packet loss on the connection
* Firewall rules preventing traffic

Use tools like `netstat`, `tcpdump`, `nmap`, etc to make sure that the cluster communication
happens (default port is `5665`).

    # tcpdump -n port 5665 -i any

    # netstat -tulpen | grep icinga

    # nmap yourclusternode.localdomain

### <a id="troubleshooting-cluster-ssl-errors"></a> Cluster Troubleshooting SSL Errors

If the cluster communication fails with cryptic SSL error messages, make sure to check
the following

* File permissions on the SSL certificate files
* Does the used CA match for all cluster endpoints?

Examples:

    # ls -la /etc/icinga2/pki


### <a id="troubleshooting-cluster-message-errors"></a> Cluster Troubleshooting Message Errors

At some point, when the network connection is broken or gone, the Icinga 2 instances
will be disconnected. If the connection can't be re-established between zones and endpoints,
they remain in a Split-Brain-mode and history may differ.

Although the Icinga 2 cluster protocol stores historical events in a replay log for later synchronisation,
you should make sure to check why the network connection failed.

### <a id="troubleshooting-cluster-config-sync"></a> Cluster Troubleshooting Config Sync

If the cluster zones do not sync their configuration, make sure to check the following:

* Within a config master zone, only one configuration master is allowed to have its config in `/etc/icinga2/zones.d`.
** The master syncs the configuration to `/var/lib/icinga2/api/zones/` during startup and only syncs valid configuration to the other nodes
** The other nodes receive the configuration into `/var/lib/icinga2/api/zones/`
* The `icinga2.log` log file will indicate whether this ApiListener [accepts config](#zone-config-sync-permissions), or not


## <a id="debug"></a> Debug Icinga 2

> **Note**
>
> If you are planning to build your own development environment,
> please consult the `INSTALL.md` file from the source tree.

### <a id="debug-requirements"></a> Debug Requirements

Make sure that the debug symbols are available for Icinga 2.
The Icinga 2 packages provide a debug package which must be
installed separately for all involved binaries, like `icinga2-bin`
or `icinga2-ido-mysql`.

Debian/Ubuntu:

    # apt-get install icinga2-dbg

RHEL/CentOS:

    # yum install icinga2-debuginfo

SLES/openSUSE:

    # zypper install icinga2-bin-debuginfo icinga2-ido-mysql-debuginfo
   

Furthermore, you may also have to install debug symbols for Boost and your C library.

If you're building your own binaries you should use the `-DCMAKE_BUILD_TYPE=Debug` cmake
build flag for debug builds.


### <a id="development-debug-gdb"></a> GDB

Install gdb:

Debian/Ubuntu:

    # apt-get install gdb

RHEL/CentOS/Fedora:

    # yum install gdb

SLES/openSUSE:

    # zypper install gdb


Install the `boost`, `python` and `icinga2` pretty printers. Absolute paths are required,
so please make sure to update the installation paths accordingly (`pwd`).

Boost Pretty Printers:

    $ mkdir -p ~/.gdb_printers && cd ~/.gdb_printers
    $ git clone https://github.com/ruediger/Boost-Pretty-Printer.git && cd Boost-Pretty-Printer
    $ pwd
    /home/michi/.gdb_printers/Boost-Pretty-Printer

Python Pretty Printers:

    $ cd ~/.gdb_printers
    $ svn co svn://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/python

Icinga 2 Pretty Printers:

    $ mkdir -p ~/.gdb_printers/icinga2 && cd ~/.gdb_printers/icinga2
    $ wget https://raw.githubusercontent.com/Icinga/icinga2/master/tools/debug/gdb/icingadbg.py

Now you'll need to modify/setup your `~/.gdbinit` configuration file.
You can download the one from Icinga 2 and modify all paths.

Example on Fedora 20 x64:

    $ wget https://raw.githubusercontent.com/Icinga/icinga2/master/tools/debug/gdb/gdbinit -O ~/.gdbinit
    $ vim ~/.gdbinit

    set print pretty on

    python
    import sys
    sys.path.insert(0, '/home/michi/.gdb_printers/icinga2')
    from icingadbg import register_icinga_printers
    register_icinga_printers()
    end

    python
    import sys
    sys.path.insert(0, '/home/michi/.gdb_printers/python')
    from libstdcxx.v6.printers import register_libstdcxx_printers
    register_libstdcxx_printers(None)
    end

    python
    import sys
    sys.path.insert(0, '/home/michi/.gdb_printers/Boost-Pretty-Printer')
    from boost.printers import register_printer_gen
    register_printer_gen(None)
    end

If you are getting the following error when running gdb, the `libstdcxx`
printers are already preloaded in your environment and you can remove
the duplicate import in your `~/.gdbinit` file.

    RuntimeError: pretty-printer already registered: libstdc++-v6

### <a id="development-debug-gdb-run"></a> GDB Run

Call GDB with the binary and all arguments and run it in foreground.

    # gdb --args /usr/sbin/icinga2 daemon -x debug

> **Note**
>
> If gdb tells you it's missing debug symbols, quit gdb and install
> them: `Missing separate debuginfos, use: debuginfo-install ...`

Run the application.

    (gdb) r

Kill the running application.

    (gdb) k

Continue after breakpoint.

    (gdb) c

### <a id="development-debug-gdb-backtrace"></a> GDB Backtrace

If Icinga 2 aborted its operation abnormally, generate a backtrace.

    (gdb) bt
    (gdb) bt full

>**Tip**
>
> If you're opening an issue at [https://dev.icinga.org] make sure
> to attach as much detail as possible.


### <a id="development-debug-gdb-backtrace-stepping"></a> GDB Backtrace Stepping

Identifying the problem may require stepping into the backtrace, analysing
the current scope, attributes, and possible unmet requirements. `p` prints
the value of the selected variable or function call result.

    (gdb) up
    (gdb) down
    (gdb) p checkable
    (gdb) p checkable.px->m_Name


### <a id="development-debug-gdb-breakpoint"></a> GDB Breakpoints

To set a breakpoint to a specific function call, or file specific line.

    (gdb) b checkable.cpp:125
    (gdb) b icinga::Checkable::SetEnablePerfdata

GDB will ask about loading the required symbols later, select `yes` instead
of `no`.

Then run Icinga 2 until it reaches the first breakpoint. Continue with `c`
afterwards.

    (gdb) run
    (gdb) c

If you want to delete all breakpoints, use `d` and select `yes`.

    (gdb) d
    
> **Tip**
>
> When debugging exceptions, set your breakpoint like this: `b __cxa_throw`.

Breakpoint Example:

    (gdb) b __cxa_throw
    (gdb) r
    (gdb) up
    ....
    (gdb) up
    #11 0x00007ffff7cbf9ff in icinga::Utility::GlobRecursive(icinga::String const&, icinga::String const&, boost::function<void (icinga::String const&)> const&, int) (path=..., pattern=..., callback=..., type=1)
        at /home/michi/coding/icinga/icinga2/lib/base/utility.cpp:609
    609			callback(cpath);
    (gdb) l
    604	
    605	#endif /* _WIN32 */
    606	
    607		std::sort(files.begin(), files.end());
    608		BOOST_FOREACH(const String& cpath, files) {
    609			callback(cpath);
    610		}
    611	
    612		std::sort(dirs.begin(), dirs.end());
    613		BOOST_FOREACH(const String& cpath, dirs) {
    (gdb) p files
    $3 = std::vector of length 11, capacity 16 = {{static NPos = 18446744073709551615, m_Data = "/etc/icinga2/conf.d/agent.conf"}, {static NPos = 18446744073709551615, 
        m_Data = "/etc/icinga2/conf.d/commands.conf"}, {static NPos = 18446744073709551615, m_Data = "/etc/icinga2/conf.d/downtimes.conf"}, {static NPos = 18446744073709551615, 
        m_Data = "/etc/icinga2/conf.d/groups.conf"}, {static NPos = 18446744073709551615, m_Data = "/etc/icinga2/conf.d/notifications.conf"}, {static NPos = 18446744073709551615, 
        m_Data = "/etc/icinga2/conf.d/satellite.conf"}, {static NPos = 18446744073709551615, m_Data = "/etc/icinga2/conf.d/services.conf"}, {static NPos = 18446744073709551615, 
        m_Data = "/etc/icinga2/conf.d/templates.conf"}, {static NPos = 18446744073709551615, m_Data = "/etc/icinga2/conf.d/test.conf"}, {static NPos = 18446744073709551615, 
        m_Data = "/etc/icinga2/conf.d/timeperiods.conf"}, {static NPos = 18446744073709551615, m_Data = "/etc/icinga2/conf.d/users.conf"}}


