# Develop Icinga 2 <a id="development"></a>

This chapter provides hints on Icinga 2 development
especially for debugging purposes.

> **Note**
>
> If you are planning to build your own development environment,
> please consult the `INSTALL.md` file from the source tree.

## Debug Requirements <a id="debug-requirements"></a>

Make sure that the debug symbols are available for Icinga 2.
The Icinga 2 packages provide a debug package which must be
installed separately for all involved binaries, like `icinga2-bin`
or `icinga2-ido-mysql`.

Debian/Ubuntu:

    # apt-get install icinga2-dbg

RHEL/CentOS:

    # yum install icinga2-debuginfo

Fedora:

    # dnf install icinga2-debuginfo icinga2-bin-debuginfo icinga2-ido-mysql-debuginfo

SLES/openSUSE:

    # zypper install icinga2-bin-debuginfo icinga2-ido-mysql-debuginfo


Furthermore, you may also have to install debug symbols for Boost and your C library.

If you're building your own binaries, you should use the `-DCMAKE_BUILD_TYPE=Debug` cmake
build flag for debug builds.


## GDB <a id="development-debug-gdb"></a>

Install gdb:

Debian/Ubuntu:

    # apt-get install gdb

RHEL/CentOS/Fedora:

    # yum install gdb

SLES/openSUSE:

    # zypper install gdb


Install the `boost`, `python` and `icinga2` pretty printers. Absolute paths are required,
so please make sure to update the installation paths accordingly (`pwd`).

    $ mkdir -p ~/.gdb_printers && cd ~/.gdb_printers

Boost Pretty Printers compatible with Python 3:

    $ git clone https://github.com/mateidavid/Boost-Pretty-Printer.git && cd Boost-Pretty-Printer
    $ git checkout python-3
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

Example on Fedora 22:

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
    try:
        register_libstdcxx_printers(None)
    except:
        pass
    end
    
    python
    import sys
    sys.path.insert(0, '/home/michi/.gdb_printers/Boost-Pretty-Printer')
    import boost_print
    boost_print.register_printers()
    end


If you are getting the following error when running gdb, the `libstdcxx`
printers are already preloaded in your environment and you can remove
the duplicate import in your `~/.gdbinit` file.

    RuntimeError: pretty-printer already registered: libstdc++-v6

### GDB Run <a id="development-debug-gdb-run"></a>

Call GDB with the binary (`/usr/sbin/icinga2` is a wrapper script calling
`/usr/lib64/icinga2/sbin/icinga2` since 2.4) and all arguments and run it in foreground.

    # gdb --args /usr/lib64/icinga2/sbin/icinga2 daemon -x debug --no-stack-rlimit

The exact path to the Icinga 2 binary differs on each distribution. On Ubuntu
it is installed into `/usr/lib/x86_64-linux-gnu/icinga2/sbin/icinga2` on 64-bit systems
for example.

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

### GDB Core Dump <a id="development-debug-gdb-coredump"></a>

Either attach to the running process using `gdb -p PID` or start
a new gdb run.

    (gdb) r
    (gdb) generate-core-file

### GDB Backtrace <a id="development-debug-gdb-backtrace"></a>

If Icinga 2 aborted its operation abnormally, generate a backtrace.

    (gdb) bt
    (gdb) thread apply all bt full

If gdb stops at a SIGPIPE signal please disable the signal before
running Icinga 2.

    (gdb) handle SIGPIPE nostop noprint pass
    (gdb) r

If you create a [new issue](https://github.com/Icinga/icinga2/issues),
make sure to attach as much detail as possible.

### GDB Backtrace from Running Process <a id="development-debug-gdb-backtrace-running"></a>

If Icinga 2 is still running, generate a full backtrace from the running
process and store it into a new file (e.g. for debugging dead locks):

Icinga 2 runs with 2 processes, therefore generate two backtrace logs
and add them to the GitHub issue.

```
for pid in $(pidof icinga2); do gdb -p $pid -batch -ex "thread apply all bt full" -ex "detach" -ex "q" > gdb_bt_${pid}_`date +%s`.log; done
```

### GDB Thread List from Running Process <a id="development-debug-gdb-thread-list-running"></a>

Instead of a full backtrace, you sometimes just need a list of running threads.

```
for pid in $(pidof icinga2); do gdb -p $pid -batch -ex "info threads" -ex "detach" -ex "q" > gdb_threads_${pid}_`date +%s`.log; done
```

### GDB Backtrace Stepping <a id="development-debug-gdb-backtrace-stepping"></a>

Identifying the problem may require stepping into the backtrace, analysing
the current scope, attributes, and possible unmet requirements. `p` prints
the value of the selected variable or function call result.

    (gdb) up
    (gdb) down
    (gdb) p checkable
    (gdb) p checkable.px->m_Name


### GDB Breakpoints <a id="development-debug-gdb-breakpoint"></a>

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


## Core Dump <a id="development-debug-core-dump"></a>

When the Icinga 2 daemon crashes with a `SIGSEGV` signal
a core dump file should be written. This will help
developers to analyze and fix the problem.

### Core Dump File Size Limit <a id="development-debug-core-dump-limit"></a>

This requires setting the core dump file size to `unlimited`.

Example for Systemd:

    vim /usr/lib/systemd/system/icinga2.service

    [Service]
    ...
    LimitCORE=infinity

    systemctl daemon-reload

    systemctl restart icinga2

Example for init script:

    vim /etc/init.d/icinga2
    ...
    ulimit -c unlimited

    service icinga2 restart

Verify that the Icinga 2 process core file size limit is set to `unlimited`.

    cat /proc/`pidof icinga2`/limits
    ...
    Max core file size        unlimited            unlimited            bytes


### Core Dump Kernel Format <a id="development-debug-core-dump-format"></a>

The Icinga 2 daemon runs with the SUID bit set. Therefore you need
to explicitly enable core dumps for SUID on Linux.

    sysctl -w fs.suid_dumpable=1

Adjust the coredump kernel format and file location on Linux:

    sysctl -w kernel.core_pattern=/var/lib/cores/core.%e.%p

    install -m 1777 -d /var/lib/cores

MacOS:

    sysctl -w kern.corefile=/cores/core.%P

    chmod 777 /cores

### Core Dump Analysis <a id="development-debug-core-dump-analysis"></a>

Once Icinga 2 crashes again a new coredump file will be written. Please
attach this file to your bug report in addition to the general details.

Simple test case for a `SIGSEGV` simulation with `sleep`:

    ulimit -c unlimited
    sleep 1800&
    [1] <PID>
    kill -SEGV <PID>
    gdb `which sleep` /var/lib/cores/core.sleep.<PID>
    (gdb) bt
    rm /var/lib/cores/core.sleep.*

Analyzing Icinga 2:

    gdb /usr/lib64/icinga2/sbin/icinga2 core.icinga2.<PID>
    (gdb) bt

