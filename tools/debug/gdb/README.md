# Pretty Printer Installation

Requirements:
* icinga2 debug symbols
* boost, gcc, etc debug symbols

Install the `boost`, `python` and `icinga2` pretty printers. Absolute paths are required,
so please make sure to update the installation paths accordingly (`pwd`).

Boost Pretty Printers:

    $ mkdir ~/.gdb_printers && cd ~/.gdb_printers
    $ git clone https://github.com/ruediger/Boost-Pretty-Printer.git && cd Boost-Pretty-Printer
    $ pwd
    /home/michi/.gdb_printers/Boost-Pretty-Printer

Python Pretty Printers:

    $ cd ~/.gdb_printers
    $ svn co svn://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/python

Icinga 2 Pretty Printers:

    $ mkdir -p ~/.gdb_printers/icinga2 && ~/.gdb_printers/icinga2
    $ wget https://raw.githubusercontent.com/Icinga/icinga2/master/tools/debug/gdb/icingadbg.py

Now you'll need to modify/setup your `~/.gdbinit` configuration file.
You can download the one from Icinga 2 and modify all paths.

> **Note**
>
> The path to the `pthread` library varies on distributions. Use
> `find /usr/lib* -type f -name '*libpthread.so*'` to get the proper
> path.

    $ wget https://raw.githubusercontent.com/Icinga/icinga2/master/tools/debug/gdb/gdbinit -O ~/.gdbinit
    $ vim ~/.gdbinit


More details in the [troubleshooting debug documentation](https://docs.icinga.com/icinga2/latest/doc/module/icinga2/chapter/troubleshooting#debug).
