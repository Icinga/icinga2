# <a id="troubleshooting"></a> Icinga 2 Troubleshooting

For a more verbose output of the Icinga 2 daemon increase the
`severity` attribute of the [logs](#logging) to `notice` or
`debug`.

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

    # /usr/sbin/icinga2 -c /etc/icinga2/icinga2.conf -x notice

Additionally you can enable the debug log using

    # icinga2-enable-feature debuglog
    # service icinga2 restart
    # tail -f /var/log/icinga2/debug.log

## <a id="checks-not-executed"></a> Checks are not executed

* Check the debug log to see if the check command gets executed
* Verify that failed depedencies do not prevent command execution
* Make sure that the plugin is executable by the Icinga 2 user (run a manual test)

    # sudo -u icinga /usr/lib/nagios/plugins/check_ping -4 -H 127.0.0.1 -c 5000,100% -w 3000,80%

* Make sure the [checker](#features) feature is enabled.

    # icinga2-enable-feature checker
    The feature 'checker' is already enabled.


## <a id="notifications-not-sent"></a> Notifications are not sent

* Check the debug log to see if a notification is triggered
* If yes, verify that all conditions are satisfied
* Are any errors on the notification command execution logged?

Verify the following configuration

* Is the host/service the `enable_notifications` attribute set, and if, to which value?
* Do the notification attributes `states`, `types`, `period` match the notification conditions?
* Do the user attributes `states`, `types`, `period` match the notification conditions?
* Are there any notification `begin` and `end` times configured?

* Make sure the [notification](#features) feature is enabled.

    # icinga2-enable-feature notification
    The feature 'notification' is already enabled.

* Does the referenced NotificationCommand work when executed as Icinga user on the shell?

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


## <a id="check-command-definitions"></a> Where are the check command definitions

Icinga 2 ships additional [plugin check command definitions](#plugin-check-commands) which are
included using

    include <itl>
    include <plugins>

in [icinga2.conf](#icinga2-conf). These configurations will be overridden on upgrade, so please
send modifications as proposed patches upstream. The default include path is set to
`LocalStateDir + "/share/icinga2/includes"`.

You should add your own command definitions to a new file in `conf.d/` called `commands.conf`
or similar.


## <a id="debug"></a> Debug Icinga 2

Make sure that the debug symbols are available for Icinga 2.
The Icinga 2 packages provide a debug package which must be
installed separately for all involved binaries, like `icinga2-bin`
or `icinga2-ido-mysql`.

    # yum install icinga2-bin-debuginfo icinga2-ido-mysql-debuginfo

Compiled binaries require the `-DCMAKE_BUILD_TYPE=RelWithDebInfo` or
`-DCMAKE_BUILD_TYPE=Debug` cmake build flags.

### <a id="development-debug-gdb"></a> GDB

Call GDB with the binary and all arguments and run it in foreground.

    # gdb --args /usr/sbin/icinga2 -c /etc/icinga2/icinga2.conf -x

### <a id="development-debug-gdb-run"></a> GDB Run

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

