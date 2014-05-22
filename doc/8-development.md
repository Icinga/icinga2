# <a id="icinga-2-development"></a> Icinga 2 Development

You can follow Icinga 2's development closely by checking
out these resources:

* Development Bug Tracker: [https://dev.icinga.org/projects/i2?jump=issues] ([How to report a bug](http://www.icinga.org/faq/how-to-report-a-bug/))
* Git Repositories: [https://git.icinga.org/?p=icinga2.git;a=summary] (mirror at [https://github.com/Icinga/icinga2])
* Git Checkins Mailinglist: [https://lists.icinga.org/mailman/listinfo/icinga-checkins]
* Development Mailinglist: [https://lists.icinga.org/mailman/listinfo/icinga-devel]
* \#icinga-devel on irc.freenode.net [http://webchat.freenode.net/?channels=icinga-devel] including a Git Commit Bot

For general support questions, please refer to [https://www.icinga.org/support/].


## <a id="development-debug"></a> Debug Icinga 2

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
> to attach as much details as possible.


### <a id="development-debug-gdb-backtrace-stepping"></a> GDB Backtrace Stepping

Identifying the problem may require stepping into the backtrace analysing
the current scope, attributes and possible unmet requirements. `p` prints
the value of the selected variable or function call result.

    (gdb) up
    (gdb) down
    (gdb) p checkable
    (gdb) p checkable.px->m_Name


### <a id="development-debug-gdb-breakpoint"></a> GDB Breakpoints

Set a breakpoint to a specific function call, or file specific line.

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

