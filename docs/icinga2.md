Icinga 2 Main
=============

Introduction
------------

A detailed introduction can be found in the chapter
[Introduction](icinga2-intro.html). /\* TODO insert url \*/

Installation
------------

For more information see the chapter Installation. /\* TODO insert url
\*/

Quick Example
-------------

/\* TODO \*/

For a general tutorial see the chapter
[Tutorial](icinga2-tutorial.html). /\* TODO insert url \*/

Requirements
------------

/\* TODO \*/

License
-------

Icinga 2 is licensed under the GPLv2 license, a copy of this license can
be found in the LICENSE file on the main source tree.

Community
---------

-   [\#icinga](http://webchat.freenode.net/?channels=icinga) on the
    Freenode IRC Network

-   [Mailinglists](https://lists.sourceforge.net/lists/listinfo/icinga-users)

-   [Monitoring Portal](http://www.monitoring-portal.org)

More details at
[http://www.icinga.org/support/](http://www.icinga.org/support/)

Support
-------

For more information on the support options refer to
[https://www.icinga.org/support](https://www.icinga.org/support)

Chapters
--------

/\* TODO \*/

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

Icinga 2 Introduction
=====================

Icinga 2 is a network monitoring application that tries to improve upon
the success of Icinga 1.x while fixing some of its shortcomings. A few
frequently encountered issues are:

-   Scalability problems in large monitoring setups

-   Difficult configuration with dozens of "magic" tweaks and several
    ways of defining services

-   Code quality and the resulting inability to implement changes
    without breaking add-ons

-   Limited access to the runtime state of Icinga (e.g. for querying a
    service’s state or for dynamically creating new services)

Fixing these issues would involve major breaking changes to the Icinga
1.x core and configuration syntax. Icinga users would likely experience
plenty of problems with the Icinga versions introducing these changes.
Many of these changes would likely break add-ons which rely on the NEB
API and other core internals.

From a developer standpoint this may be justifiable in order to get to a
better end-product. However, for (business) users spending time on
getting familiar with these changes for each new version may become
quite frustrating and may easily cause users to lose their confidence in
Icinga.

Nagios™ 4 is currently following this approach and it remains to be seen
how this fares with its users.

Instead the Icinga project will maintain two active development
branches. There will be one branch for Icinga 1.x which focuses on
improving the existing Icinga 1.x code base - just like it has been done
so far.

Independently from Icinga 1.x development on Icinga 2 will happen in a
separate branch and some of the long-term design goals will be outlined
in this document. Status updates for Icinga 2 will be posted on the
project website (www.icinga.org) as they become available.

Code Quality
------------

Icinga 2 will not be using any code from the Icinga 1.x branch due to
the rampant code quality issues with the existing code base. However, an
important property of the Icinga development process has always been to
rely on proven technologies and Icinga 2 will be no exception.

A lot of effort has gone into designing a maintainable architecture for
Icinga 2 and making sure that algorithmic choices are in alignment with
our scalability goals for Icinga 2.

There are plans to implement unit tests for most Icinga 2 features in
order to make sure that changes to the code base do not break things
that were known to work before.

Language Choice
---------------

Icinga 1.x is written in C and while in general C has quite a number of
advantages (e.g. performance and relatively easy portability to other
\*NIX- based platforms) some of its disadvantages show in the context of
a project that is as large as Icinga.

With a complex software project like Icinga an object-oriented design
helps tremendously with keeping things modular and making changes to the
existing code easier.

While it is true that you can write object-oriented software in C (the
Linux kernel is one of the best examples of how to do that) a truly
object-oriented language makes the programmers' life just a little bit
easier.

For Icinga 2 we have chosen C++ as the main language. This decision was
influenced by a number of criteria including performance, support on
different platforms and general user acceptability.

In general there is nothing wrong with other languages like Java, C\# or
Python; however - even when ignoring technical problems for just a
moment - in a community as conservative as the monitoring community
these languages seem out of place.

Knowing that users will likely want to run Icinga 2 on older systems
(which are still fully vendor-supported even for years to come) we will
make every effort to ensure that Icinga 2 can be built and run on
commonly used operating systems and refrain from using new and exotic
features like C++11.

Unlike Icinga 1.x there will be Windows support for Icinga 2. Some of
the compatibility features (e.g. the command pipe) which rely on \*NIX
features may not be supported on Windows but all new features will be
designed in such a way as to support \*NIX as well as Windows.

Configuration
-------------

Icinga 1.x has a configuration format that is fully backwards-compatible
to the Nagios™ configuration format. This has the advantage of allowing
users to easily upgrade their existing Nagios™ installations as well as
downgrading if they choose to do so (even though this is generally not
the case).

The Nagios™ configuration format has evolved organically over time and
for the most part it does what it’s supposed to do. However this
evolutionary process has brought with it a number of problems that make
it difficult for new users to understand the full breadth of available
options and ways of setting up their monitoring environment.

Experience with other configuration formats like the one used by Puppet
has shown that it is often better to have a single "right" way of doing
things rather than having multiple ways like Nagios™ does (e.g. defining
host/service dependencies and parent/child relationships for hosts).

Icinga 2 tries to fix those issues by introducing a new object-based
configuration format that is heavily based on templates and supports
user-friendly features like freely definable macros.

External Interfaces
-------------------

While Icinga 1.x has easily accessible interfaces to its internal state
(e.g. status.dat, objects.cache and the command pipe) there is no
standards-based way of getting that information.

For example, using Icinga’s status information in a custom script
generally involves writing a parser for the status.dat format and there
are literally dozens of Icinga-specific status.dat parsers out there.

While Icinga 2 will support these legacy interfaces in order to make
migration easier and allowing users to use the existing CGIs and
whatever other scripts they may have Icinga 2 will focus on providing a
unified interface to Icinga’s state and providing similar functionality
to that provided by the command pipe in Icinga 1.x. The exact details
for such an interface are yet to be determined but this will likely be
an RPC interface based on one of the commonly used web-based remoting
technologies.

Icinga 1.x exports historical data using the IDO database interface
(Icinga Data Output). Icinga 2 will support IDO in a
backwards-compatible fashion in order to support icinga-web.
Additionally there will be a newly-designed backend for historical data
which can be queried using the built-in API when available. Effort will
be put into making this new data source more efficient for use with SLA
reporting.

Icinga 2 will also feature dynamic reconfiguration using the API which
means users can create, delete and update any configuration object (e.g.
hosts and services) on-the-fly. Based on the API there are plans to
implement a command-line configuration tool similar to what Pacemaker
has with "crm". Later on this API may also be used to implement
auto-discovery for new services.

The RPC interface may also be used to receive events in real-time, e.g.
when service checks are being executed or when a service’s state
changes. Some possible uses of this interface would be to export
performance data for services (RRD, graphite, etc.) or general log
information (logstash, graylog2, etc.).

Checks
------

In Icinga 2 services are the only checkable objects. Hosts only have a
calculated state and no check are ever run for them.

In order to maintain compatibility with the hundreds of existing check
plugins for Icinga 1.x there will be support for Nagios™-style checks.
The check interface however will be modular so that support for other
kinds of checks can be implemented later on (e.g. built-in checks for
commonly used services like PING, HTTP, etc. in order to avoid spawning
a process for each check).

Based on the availability of remote Icinga 2 instances the core can
delegate execution of service checks to them in order to support
large-scale distributed setups with a minimal amount of maintenance.
Services can be assigned to specific check instances using configuration
settings.

Notifications
-------------

Event handlers and notifications will be supported similar to Icinga
1.x. Thanks to the dynamic configuration it is possible to easily adjust
the notification settings at runtime (e.g. in order to implement on-call
rotation).

Scalability
-----------

Icinga 1.x has some serious scalability issues which explains why there
are several add-ons which try to improve the core’s check performance.
One of these add-ons is mod\_gearman which can be used to distribute
checks to multiple workers running on remote systems.

A problem that remains is the performance of the core when processing
check results. Scaling Icinga 1.x beyond 25.000 services proves to be a
challenging problem and usually involves setting up a cascade of Icinga
1.x instances and dividing the service checks between those instances.
This significantly increases the maintenance overhead when updating the
configuration for such a setup.

Icinga 2 natively supports setting up multiple Icinga 2 instances in a
cluster to distribute work between those instances. Independent tasks
(e.g. performing service checks, sending notifications, updating the
history database, etc.) are implemented as components which can be
loaded for each instance. Configuration as well as program state is
automatically replicated between instances.

In order to support using Icinga 2 in a partially trusted environment
SSL is used for all network communication between individual instances.
Objects (like hosts and services) can be grouped into security domains
for which permissions can be specified on a per-instance basis (so e.g.
you can have a separate API or checker instance for a specific domain).

Agent-based Checks
------------------

Traditionally most service checks have been performed actively, meaning
that check plugins are executed on the same server that is also running
Icinga. This works great for checking most network-based services, e.g.
PING and HTTP. However, there are a number of services which cannot be
checked remotely either because they are not network-based or because
firewall settings or network policies ("no unencrypted traffic")
disallow accessing these services from the network where Icinga is
running.

To solve this problem two add-ons have emerged, namely NRPE and NSCA.
NRPE can be thought of as a light-weight remote shell which allows the
execution of a restricted set of commands while supporting some
Nagios™-specific concepts like command timeouts. However unlike with the
design of commonly used protocols like SSH security in NRPE is merely an
afterthought.

In most monitoring setups all NRPE agents share the same secret key
which is embedded into the NRPE binary at compile time. This means that
users can extract this secret key from their NRPE agent binary and use
it to query sensitive monitoring information from other systems running
the same NRPE binary. NSCA has similar problems.

Based on Icinga 2’s code for check execution there will be an agent
which can be used on \*NIX as well as on Windows platforms. The agent
will be using the same configuration format like Icinga 2 itself and
will support SSL and IPv4/IPv6 to communicate with Icinga 2.

Business Processes
------------------

In most cases users don’t care about the availability of individual
services but rather the aggregated state of multiple related services.
For example one might have a database cluster that is used for a web
shop. For an end-user the shop is available as long as at least one of
the database servers is working.

Icinga 1.x does not have any support for business processes out of the
box. There are several add-ons which implement business process support
for Icinga, however none of those are well-integrated into Icinga.

Icinga 2 will have native support for business processes which are built
right into the core and can be configured in a similar manner to
Nagios™-style checks. Users can define their own services based on
business rules which can be used as dependencies for other hosts or
services.

Logging
-------

Icinga 2 supports file-based logged as well as syslog (on \*NIX) and
event log (on Windows). Additionally Icinga 2 supports remote logging to
a central Icinga 2 instance.

Icinga 2 Installation
=====================

Requirements
------------

Packages
--------

> **Note**
>
> Use packages whenever possible.

  ------------------------------------ ------------------------------------
  Distribution                         Package URL
  Debian                               TBD
  RHEL/CentOS                          TBD
  SLES                                 TBD
  ------------------------------------ ------------------------------------

In case you’re running a distribution for which Icinga 2 packages are
not yet available download the source tarball and jump to Source Builds.

Windows Installer
-----------------

TODO

Source Builds
-------------

Download the source tarball and read the *INSTALL* file for details and
requirements.

### Linux Builds

Building from source on specific linux distributions is described on the
wiki:
[https://wiki.icinga.org/display/icinga2/Linux+Builds](https://wiki.icinga.org/display/icinga2/Linux+Builds)

### Windows Builds

Icinga 2 ships a MS Visual Studio solution file. Requirements and
compilation instructions can be found on the wiki:
[https://wiki.icinga.org/display/icinga2/Windows+Builds](https://wiki.icinga.org/display/icinga2/Windows+Builds)

Installation Locations
----------------------

  ------------------------------------ ------------------------------------
  Path                                 Description

  /etc/icinga2                         Contains Icinga 2 configuration
                                       files.

  /etc/init.d/icinga2                  The Icinga 2 init script.

  /usr/share/doc/icinga2               Documentation files that come with
                                       Icinga 2.

  /usr/share/icinga2/itl               The Icinga Template Library.

  /var/run/icinga2                     Command pipe and PID file.

  /var/cache/icinga2                   Performance data files and
                                       status.dat/objects.cache.

  /var/lib/icinga2                     The Icinga 2 state file.
  ------------------------------------ ------------------------------------

/\* TODO \*/

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

Icinga 2 Migration
==================

Purpose
-------

Documentation on the general migration from Icinga 1.x to Icinga 2.

Requirements
------------

Multi-core cpu, ram, fast disks.

Installation
------------

Icinga 1.x and Icinga 2 may run side by side, but it’s recommended to
backup your existing 1.x installation before installing Icinga 2 on the
same host.

Compatibility
-------------

> **Note**
>
> The configuration format changed from 1.x to 2.x. Don’t panic though.
> A conversion script is shipped in *tools/configconvert* - please check
> the *README* file.

For details check the chapter [Compatibility](icinga2-compat.html).

Changes
-------

For details check the chapter [Changes](icinga2-compat.html).

TODO

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

Icinga 2 Compatibility
======================

Purpose
-------

Documentation on the compatibility and changes introduced with Icinga 2.

Introduction
------------

Unlike Icinga 1.x, all used components (not only those for
compatibility) run asynchronous and use queues, if required. That way
Icinga 2 does not get blocked by any event, action or execution.

Configuration
-------------

> **Note**
>
> If you are upgrading from Icinga 1.x (or Nagios 3.x+) please note that
> Icinga 2 introduces a new configuration format.

Details on the configuration can be found in chapter
[Configuration](:icinga2-config.html)

Icinga 2 ships a config conversion script which will help you migrating
the existing configuration into the new format. Please look into the
*tools/configconvert* directory and follow the *README* instructions.

> **Tip**
>
> If you kept planning to clean up your existing configuration, it may
> be a good shot to start fresh with a new configuration strategy based
> on the Icinga 2 configuration logic.

Check Plugins
-------------

All native check plugins can be used with Icinga 2. The configuration of
check commands is changed due to the new configuration format.

Classic status and log files
----------------------------

Icinga 2 will write status.dat and objects.cache in a given interval
like known from Icinga 1.x - including the logs and their archives in
the old format and naming syntax. That way you can point any existing
Classic UI installation to the new locations (or any other addon/plugin
using them).

External Commands
-----------------

Like known from Icinga 1.x, Icinga 2 also provides an external command
pipe allowing your scripts and guis to send commands to the core
triggering various actions.

Some commands are not supported though as their triggered functionality
is not available in Icinga 2 anymore.

For a detailed list, please check:
[https://wiki.icinga.org/display/icinga2/External+Commands](https://wiki.icinga.org/display/icinga2/External+Commands)

Performance Data
----------------

The Icinga 1.x Plugin API defines the performance data format. Icinga 2
parses the check output accordingly and writes performance data files
based on template macros. File rotation interval can be defined as well.

Unlike Icinga 1.x you can define multiple performance data writers for
all your graphing addons such as PNP, inGraph or graphite.

IDO DB
------

Icinga 1.x uses an addon called *IDOUtils* to store core configuration,
status and historical information in a database schema. Icinga Web and
Reporting are using that database as their chosen backend.

Icinga 2 is compatible to the IDO db schema but the the underlaying
design of inserting, updating and deleting data is different -
asynchronous queueing, database transactions and optimized queries for
performance.

Furthermore there is no seperated daemon to receive the data through a
socket. Instead the IDO component queues the data and writes directly
into the database using the native database driver library (e.g.
libmysqlclient). Unlike Icinga 1.x libdbi as db abstraction layer is not
used anymore.

Livestatus
----------

Icinga 2 supports the livestatus api while using Icinga 1.x an addon
named *mk\_livestatus* was required.

Next to the GET functionality for retrieving configuration, status and
historical data, Icinga 2 livestatus also supports the COMMANDS
functionality.

> **Tip**
>
> Icinga 2 supports tcp sockets natively while the Icinga 1.x addon only
> provides unix socket support.

Checkresult Reaper
------------------

Unlike Icinga 1.x Icinga 2 is a multithreaded application and processes
check results in memory. The old checkresult reaper reading files from
disk again is obviously not required anymore for native checks.

Some popular addons have been injecting their checkresults into the
Icinga 1.x checkresult spool directory bypassing the external command
pipe and PROCESS\_SERVICE\_CHECK\_RESULT mainly for performance reasons.

In order to support that functionality as well, Icinga 2 got its
optional checkresult reaper.

Changes
-------

This is a collection of known changes in behaviour, configuration and
outputs.

> **Note**
>
> May be incomplete, and requires updates in the future.

TODO

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

Icinga 2 Tutorial
=================

Preface
-------

This tutorial is a step-by-step introduction to installing Icinga 2 and
setting up your first couple of service checks. It assumes some
familiarity with Icinga 1.x.

Installation
------------

In order to get started with Icinga 2 we will have to install it. The
preferred way of doing this is to use the official Debian or RPM
packages depending on which Linux distribution you are running.

  ------------------------------------ ------------------------------------
  Distribution                         Package URL

  Debian                               [http://icingabuild.dus.dg-i.net:808
                                       0/job/icinga2/](http://icingabuild.d
                                       us.dg-i.net:8080/job/icinga2/)

  RHEL                                 TBD
  ------------------------------------ ------------------------------------

In case you’re running a distribution for which Icinga 2 packages are
not yet available you will have to check out the Icinga 2 Git repository
from git://git.icinga.org/icinga2 and read the *INSTALL* file.

By default Icinga 2 uses the following files and directories:

  ------------------------------------ ------------------------------------
  Path                                 Description

  /etc/icinga2                         Contains Icinga 2 configuration
                                       files.

  /etc/init.d/icinga2                  The Icinga 2 init script.

  /usr/share/doc/icinga2               Documentation files that come with
                                       Icinga 2.

  /usr/share/icinga2/itl               The Icinga Template Library.

  /var/run/icinga2                     Command pipe and PID file.

  /var/cache/icinga2                   Performance data files and
                                       status.dat/objects.cache.

  /var/lib/icinga2                     The Icinga 2 state file.
  ------------------------------------ ------------------------------------

Our First Service Check
-----------------------

The Icinga 2 package comes with a number of example configuration files.
However, in order to explain some of the basics we’re going write our
own configuration file from scratch.

Start by creating the file /etc/icinga2/icinga2.conf with the following
content:

    include <itl/itl.conf>
    include <itl/standalone.conf>

    object IcingaApplication "my-icinga" {
            macros["plugindir"] = "/usr/lib/nagios/plugins"
    }

The configuration snippet includes the *itl/itl.conf* and
*itl/standalone.conf* files which are distributed as part of Icinga 2.
We will discuss the Icinga Template Library (ITL) in more detail later
on.

The *itl/standalone.conf* configuration file takes care of configuring
Icinga 2 for single-instance (i.e. non-clustered) mode.

Our configuration file also creates an object of type
*IcingaApplication* with the name *my-icinga*. The *IcingaApplication*
type can be used to define global macros and some other global settings.

For now we’re only defining the global macro *plugindir* which we’re
going to use later on when referring to the path which contains our
check plugins. Depending on where you’ve installed your check plugins
you may need to update this path in your configuration file.

You can verify that your configuration file works by starting Icinga 2:

    $ /usr/bin/icinga2 -c /etc/icinga2/icinga2.conf
    [2013/04/23 13:36:20 +0200] <Main Thread> information/icinga-app: Icinga application loader (version: 0.0.1, git branch master, commit 0fcbfdb2)
    [2013/04/23 13:36:20 +0200] <Main Thread> information/base: Adding library search dir: /usr/lib/icinga2
    [2013/04/23 13:36:20 +0200] <Main Thread> information/base: Loading library 'libicinga.la'
    [2013/04/23 13:36:20 +0200] <Main Thread> information/config: Adding include search dir: /usr/share/icinga2
    [2013/04/23 13:36:20 +0200] <Main Thread> information/config: Compiling config file: /etc/icinga2/icinga2.conf
    [2013/04/23 13:36:20 +0200] <Main Thread> information/config: Linking config items...
    [2013/04/23 13:36:20 +0200] <Main Thread> information/config: Validating config items...
    [2013/04/23 13:36:20 +0200] <Main Thread> information/config: Activating config items in compilation unit 'b2d21c28-a2e8-4fcb-ba00-45646bc1afb9'
    [2013/04/23 13:36:20 +0200] <Main Thread> information/base: Restoring program state from file '/var/lib/icinga2/icinga2.state'
    [2013/04/23 13:36:20 +0200] <Main Thread> information/base: Restored 0 objects

In case there are any configuration errors Icinga 2 should print error
messages containing details about what went wrong.

You can stop Icinga 2 with Control-C:

    ^C
    [2013/04/23 13:39:39 +0200] <TP 0x7f2e9070f500 Worker #0> information/base: Shutting down Icinga...
    [2013/04/23 13:39:39 +0200] <TP 0x7f2e9070f500 Worker #0> information/base: Dumping program state to file '/var/lib/icinga2/icinga2.state'
    [2013/04/23 13:39:39 +0200] <Main Thread> information/icinga: Icinga has shut down.
    $

Icinga 2 automatically saves its current state every couple of minutes
and when it’s being shut down.

So far our Icinga 2 setup doesn’t do much. Lets change that by setting
up a service check for localhost. Modify your *icinga2.conf*
configuration file by adding the following lines:

    object CheckCommand "my-ping" inherits "plugin-check-command" {
            command = [
                    "$plugindir$/check_ping",
                    "-H", "$address$",
                    "-w", "10,5%",
                    "-c", "25,10%"
            ]
    }

    template Service "my-ping" inherits "plugin-service" {
            check_command = "my-ping"
    }

    object Host "localhost" {
            display_name = "Home, sweet home!",

            services["ping"] = {
                    templates = [ "my-ping" ]
            },

            macros = {
                    address = "127.0.0.1"
            },

            check_interval = 10s,

            hostcheck = "ping"
    }

We’re defining a command object called "my-ping" which inherits from the
*plugin-check-command* template. The *plugin-check-command* template is
provided as part of the Icinga Template Library and describes how checks
are performed. In the case of plugin-based services this means that the
command specified by the *command* property is executed.

The *command* property is an array or command-line arguments for the
check plugin. Alternatively you can specify the check command as a
string.

The check command can make use of macros. Unlike in Icinga 1.x we have
free-form macros which means that users can choose arbitrary names for
their macros.

By convention the following macros are usually used:

  ------------------------------------ ------------------------------------
  Macro                                Description
  plugindir                            The path of your check plugins.
  address                              The IPv4 address of the host.
  address6                             The IPv6 address of the host.
  ------------------------------------ ------------------------------------

Note that the *my-ping* command object does not define a value for the
*address* macro. This is perfectly fine as long as that macro is defined
somewhere else (e.g. in the host).

We’re also defining a service template called *my-ping* which uses the
command object we just created.

Next we’re defining a *Host* object called *localhost*. We’re setting an
optional display\_name which is used by the Icinga Classic UI when
showing that host in the host overview.

The services dictionary defines which services belong to a host. Using
the [] indexing operator we can manipulate individual items in this
dictionary. In this case we’re creating a new service called *ping*.

The templates array inside the service definition lists all the
templates we want to use for this particular service. For now we’re just
listing our *my-ping* template.

Remember how we used the *address* macro in the *command* setting
earlier? Now we’re defining a value for this macro which is used for all
services and their commands which belong to the *localhost* Host object.

We’re also setting the check\_interval for all services belonging to
this host to 10 seconds.

> **Note**
>
> When you don’t specify an explicit time unit Icinga 2 automatically
> assumes that you meant seconds.

And finally we’re specifying which of the services we’ve created before
is used to define the host’s state. Note that unlike in Icinga 1.x this
just "clones" the service’s state and does not cause any additional
checks to be performed.

Setting up the Icinga 1.x Classic UI
------------------------------------

Icinga 2 can write status.dat and objects.cache files in the format that
is supported by the Icinga 1.x Classic UI. External commands (a.k.a. the
"command pipe") are also supported. If you require the icinga.log for
history views and/or reporting in Classic UI, this can be added
seperately to the CompatComponent object definition by adding a
CompatLog object.

In order to enable this feature you will need to load the library
*compat* by adding the following lines to your configuration file:

    library "compat"

    object CompatComponent "compat" { }
    object CompatLog "my-log" { }

After restarting Icinga 2 you should be able to find the status.dat and
objects.cache files in /var/cache/icinga2. The log files can be found in
/var/log/icinga2/compat. The command pipe can be found in
/var/run/icinga2.

You can install the Icinga 1.x Classic UI in standalone mode using the
following commands:

    $ wget http://downloads.sourceforge.net/project/icinga/icinga/1.9.0/icinga-1.9.0.tar.gz
    $ tar xzf icinga-1.9.0.tar.gz ; cd icinga-1.9.0
    $ ./configure --enable-classicui-standalone --prefix=/usr/local/icinga2-classicui
    $ make classicui-standalone
    $ sudo make install classicui-standalone install-webconf-auth
    $ sudo service apache2 restart

> **Note**
>
> A detailed guide on installing Icinga 1.x Classic UI Standalone can be
> found on the Icinga Wiki here:
> [https://wiki.icinga.org/display/howtos/Setting+up+Icinga+Classic+UI+Standalone](https://wiki.icinga.org/display/howtos/Setting+up+Icinga+Classic+UI+Standalone)

After installing the Classic UI you will need to update the following
settings in your cgi.cfg configuration file at the bottom (section
"STANDALONE (ICINGA 2) OPTIONS"):

  ------------------------------------ ------------------------------------
  Configuration Setting                Value
  object\_cache\_file                  /var/cache/icinga2/objects.cache
  status\_file                         /var/cache/icinga2/status.dat
  resource\_file                       -
  command\_file                        /var/run/icinga2/icinga2.cmd
  check\_external\_commands            1
  interval\_length                     60
  status\_update\_interval             10
  log\_file                            /var/log/icinga2/compat/icinga.log
  log\_rotation\_method                h
  log\_archive\_path                   /var/log/icinga2/compat/archives
  date\_format                         us
  ------------------------------------ ------------------------------------

Depending on how you installed Icinga 2 some of those paths and options
might be different.

> **Note**
>
> You need to grant permissions for the apache user manually after
> starting Icinga 2 for now.

    # chmod o+rwx /var/run/icinga2/{icinga2.cmd,livestatus}

Verify that your Icinga 1.x Classic UI works by browsing to your Classic
UI installation URL e.g.
[http://localhost/icinga](http://localhost/icinga)

Some More Templates
-------------------

Now that we’ve got our basic monitoring setup as well as the Icinga 1.x
Classic UI to work we can define a second host. Add the following lines
to your configuration file:

    object Host "icinga.org" {
            display_name = "Icinga Website",

            services["ping"] = {
                    templates = [ "my-ping" ]
            },

            macros = {
                    address = "www.icinga.org"
            },

            check_interval = 10s,

            hostcheck = "ping"
    }

Restart your Icinga 2 instance and check the Classic UI for your new
service’s state. Unless you have a low-latency network connection you
will note that the service’s state is *CRITICAL*. This is because in the
*my-ping* command object we have hard-coded the timeout as 25
milliseconds.

Ideally we’d be able to specify different timeouts for our new service.
Using macros we can easily do this.

> **Note**
>
> If you’ve used Icinga 1.x before you’re probably familiar with doing
> this by passing ARGx macros to your check commands.

Start by replacing your *my-ping* command object with this:

    object CheckCommand "my-ping" inherits "plugin-check-command" {
            command = [
                    "$plugindir$/check_ping",
                    "-H", "$address$",
                    "-w", "$wrta$,$wpl$%",
                    "-c", "$crta$,$cpl$%"
            ],

            macros = {
                    wrta = 10,
                    wpl = 5,

                    crta = 25,
                    cpl = 10
            }
    }

We have replaced our hard-coded timeout values with macros and we’re
providing default values for these same macros right in the template
definition. The object inherits the basic check command attributes from
the ITL provided template *plugin-check-command*.

In order to oderride some of these macros for a specific host we need to
update our *icinga.org* host definition like this:

    object Host "icinga.org" {
            display_name = "Icinga Website",

            services["ping"] = {
                    templates = [ "my-ping" ],

                    macros += {
                            wrta = 100,
                            crta = 250
                    }
            },

            macros = {
                    address = "www.icinga.org"
            },

            check_interval = 10s,

            hostcheck = "ping"
    }

The *+=* operator allows us to selectively add new key-value pairs to an
existing dictionary. If we were to use the *=* operator instead we would
have to provide values for all the macros that are used in the *my-ping*
template overriding all values there.

Icinga Template Library
-----------------------

The Icinga Template Library is a collection of configuration templates
for commonly used services. By default it is installed in
*/usr/share/icinga2/itl* and you can include it in your configuration
files using the include directive:

    include <itl/itl.conf>

> **Note**
>
> Ordinarily you’d use double-quotes for the include path. This way only
> paths relative to the current configuration file are considered. The
> angle brackets tell Icinga 2 to search its list of global include
> directories.

One of the templates in the ITL is the *ping4* service template which is
quite similar to our example objects:

    object CheckCommand "ping4" inherits "plugin-check-command" {
            command = [
                    "$plugindir$/check_ping",
                    "-4",
                    "-H", "$address$",
                    "-w", "$wrta$,$wpl$%",
                    "-c", "$crta$,$cpl$%",
                    "-p", "$packets$",
                    "-t", "$timeout$"
            ],

            macros = {
                    wrta = 100,
                    wpl = 5,

                    crta = 200,
                    cpl = 15,

                    packets = 5,
                    timeout = 0
            }
    }

    template Service "ping4" {
            check_command = "ping4"
    }

Lets simplify our configuration file by removing our custom *my-ping*
template and updating our service definitions to use the *ping4*
template instead.

Include Files
-------------

So far we’ve been using just one configuration file. However, once
you’ve created a few more host objects and service templates this can
get rather confusing.

Icinga 2 lets you include other files from your configuration file. We
can use this feature to make our configuration a bit more modular and
easier to understand.

Lets start by moving our two *Host* objects to a separate configuration
file: hosts.conf

We will also need to tell Icinga 2 that it should include our newly
created configuration file when parsing the main configuration file.
This can be done by adding the include directive to our *icinga2.conf*
file:

    include "hosts.conf"

Depending on the number of hosts you have it might be useful to split
your configuration files based on other criteria (e.g. device type,
location, etc.).

You can use wildcards in the include path in order to refer to multiple
files. Assuming you’re keeping your host configuration files in a
directory called *hosts* you could include them like this:

    include "hosts/*.conf"

Notifications
-------------

Icinga 2 can send you notifications when your services change state. In
order to do this we’re going to write a shell script in
*/etc/icinga2/mail-notification.sh* that sends e-mail based
notifications:

    #!/bin/sh

    if [ -z "$1" ]; then
            echo "Syntax: $0 <e-mail>"
            echo
            echo "Sends a mail notification to the specified e-mail address."
            exit 1
    fi

    mail -s "** $NOTIFICATIONTYPE Service Alert: $HOSTALIAS/$SERVICEDESC is $SERVICESTATE **" $1 <<TEXT
    ***** Icinga *****

    Notification Type: $NOTIFICATIONTYPE

    Service: $SERVICEDESC
    Host: $HOSTALIAS
    Address: $address
    State: $SERVICESTATE

    Date/Time: $LONGDATETIME

    Additional Info:

    $SERVICEOUTPUT
    TEXT

    exit 0

Our shell script uses a couple of pre-defined macros (e.g. SERVICEDESC,
HOSTALIAS, etc.) that are always available.

Next we’re going to create a *Notification* template which tells Icinga
how to invoke the shell script:

    object NotificationCommand "mail-notification" inherits "plugin-notification-command" {
            command = [
                    "/etc/icinga2/mail-notification.sh",
                    "$email$"
            ],

            export_macros = [
                    "NOTIFICATIONTYPE",
                    "HOSTALIAS",
                    "SERVICEDESC",
                    "SERVICESTATE",
                    "SERVICEDESC",
                    "address",
                    "LONGDATETIME",
                    "SERVICEOUTPUT"
            ]
    }

    template Notification "mail-notification" {
            notification_command = "mail-notification"
    }

> **Note**
>
> Rather than adding these templates to your main configuration file you
> might want to create a separate file, e.g. *notifications.conf* and
> include it in *icinga2.conf*.

The *export\_macros* property tells Icinga which macros to export into
the environment for the notification script.

We also need to create a *User* object which Icinga can use to send
notifications to specific people:

    object User "tutorial-user" {
            display_name = "Some User",

            macros = {
                    email = "tutorial@example.org"
            }
    }

Each time a notification is sent for a service the user’s macros are
used when resolving the macros we used in the *Notification* template.

In the next step we’re going to create a *Service* template which
specifies who notifications should be sent to:

    template Service "mail-notification-service" {
            notifications["mail"] = {
                    templates = [ "mail-notification" ],

                    users = [ "tutorial-user" ]
            },

            notification_interval = 1m
    }

And finally we can assign this new service template to our services:

            ...
            services["ping"] = {
                    templates = [ "ping4", "mail-notification-service" ]
            },
            ...

In addition to defining notifications for individual services it is also
possible to assign notification templates to all services of a host. You
can find more information about how to do that in the documentation.

> **Note**
>
> Escalations in Icinga 2 are just a notification, only added a defined
> begin and end time. Check the documentation for details.

Time Periods
------------

Time periods allow you to specify when certain services should be
checked and when notifications should be sent.

Here is an example time period definition:

    object TimePeriod "work-hours" inherits "legacy-timeperiod" {
            ranges = {
                    monday = "9:00-17:00",
                    tuesday = "9:00-17:00",
                    wednesday = "9:00-17:00",
                    thursday = "9:00-17:00",
                    friday = "9:00-17:00",
            }
    }

The *legacy-timeperiod* template is defined in the Icinga Template
Library and supports Icinga 1.x time periods. A complete definition of
the time Icinga 1.x time period syntax can be found at
[http://docs.icinga.org/latest/en/objectdefinitions.html\#timeperiod](http://docs.icinga.org/latest/en/objectdefinitions.html#timeperiod).

Using the *check\_period* attribute you can define when services should
be checked:

            ...
            services["ping"] = {
                    templates = [ "ping4", "mail-notification-service" ],
                    check_period = "work-hours"
            },
            ...

Also, using the *notification\_period* attribute you can define when
notifications should be sent:

    template Service "mail-notification-service" {
            notifications["mail"] = {
                    templates = [ "mail-notification" ],

                    users = [ "tutorial-user" ]
            },

            notification_interval = 1m,
            notification_period = "work-hours"
    }

The *notification\_period* attribute is also valid in *User* and
*Notification* objects.

Dependencies
------------

If you are familiar with Icinga 1.x host/service dependencies and
parent/child relations on hosts, you might want to look at the
conversion script in order to convert your existing configuration. There
are no separate dependency objects anymore, and no separate parent
attribute either.

Using Icinga 2, we can directly define a dependency in the current host
or service object to any other host or service object. If we want other
objects to inherit those dependency attributes, we can also define them
in a template.

In the following example we’ve added a cluster host with the service
*ping* which we are going to define a dependency for in another host.

    template Service "my-cluster-ping" {
            check_command = "my-ping",
    }

    object Host "my-cluster" {
            ...
            services["ping"] = {
                    templates = [ "my-cluster-ping" ],
            }
            ...
    }

We can now define a service dependency as new service template (or
directly on the service definition):

    template Service "my-cluster-dependency" {
            servicedependencies = [
                    { host = "my-cluster", service = "ping" },
            ],
    }

Now let’s use that template for the *ping* service we’ve defined
previously and assign the servicedependencies to that service.

            ...
            services["ping"] = {
                    templates = [ "ping4", "mail-notification-service", "my-cluster-dependency" ],
            },
            ...

Performance Data
----------------

Because there are no host checks in Icinga 2, the PerfdataWriter object
will only write service performance data files. Creating the object will
allow you to set the perfdata\_path, format\_template and
rotation\_interval. The format template is similar to existing Icinga
1.x configuration for PNP or inGraph using macro formatted strings.

Details on the common Icinga 1.x macros can be found at
[http://docs.icinga.org/latest/en/macrolist.html](http://docs.icinga.org/latest/en/macrolist.html)

> **Note**
>
> You can define multiple PerfdataWriter objects with different
> configuration settings, i.e. one for PNP, one for inGraph or your
> preferred graphite collector.

Let’s create a new PNP PerfdataWriter object:

    object PerfdataWriter "pnp" {
            perfdata_path = "/var/lib/icinga2/service-perfdata",
            format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$TIMET$\tHOSTNAME::$HOSTNAME$\tSERVICEDESC::$SERVICEDESC$\tSERVICEPERFDATA::$SERVICEPERFDATA$\tSERVICECHECKCOMMAND::$SERVICECHECKCOMMAND$\tHOSTSTATE::$HOSTSTATE$\tHOSTSTATETYPE::$HOSTSTATETYPE$\tSERVICESTATE::$SERVICESTATE$\tSERVICESTATETYPE::$SERVICESTATETYPE$",
            rotation_interval = 15s,
    }

You may need to reconfigure your NPCD daemon with the correct path for
your performance data files. This can be done in the PNP configuration
file npcd.cfg:

    perfdata_spool_dir = /var/lib/icinga2/

Livestatus Component
--------------------

The Livestatus component will provide access to Icinga 2 using the
livestatus api. In addition to the unix socket Icinga 2 also service
livestatus directly via tcp socket.

> **Note**
>
> Only config and status tables are available at this time. History
> tables such as log, statehist will follow.

Once Icinga 2 is started, configure your gui (e.g. Thruk) using the
livestatus backend.

TCP Socket

    library "livestatus"
    object LivestatusComponent "livestatus-tcp" {
            socket_type = "tcp",
            host = "10.0.10.18",
            port = "6558"
    }

Unix Socket

    library "livestatus"
    object LivestatusComponent "livestatus-unix" {
            socket_type = "unix",
            socket_path = "/var/run/icinga2/livestatus"
    }

> **Note**
>
> You need to grant permissions for the apache user manually after
> starting Icinga 2 for now.

    # chmod o+rwx /var/run/icinga2/{icinga2.cmd,livestatus}

IDO Database Component
----------------------

The IDO component will write to the same database backend as known from
Icinga 1.x IDOUtils. Therefore you’ll need to have your database schema
and users already installed, like described in
[http://docs.icinga.org/latest/en/quickstart-idoutils.html\#createidoutilsdatabase](http://docs.icinga.org/latest/en/quickstart-idoutils.html#createidoutilsdatabase)

> **Note**
>
> Currently there’s only MySQL support in progress, Postgresql, Oracle
> tbd.

Configure the IDO MySQL component with the defined credentials and start
Icinga 2.

> **Note**
>
> Make sure to define a unique instance\_name. That way the Icinga 2 IDO
> component will not interfere with your Icinga 1.x setup, if existing.

    library "ido_mysql"
    object IdoMysqlDbConnection "my-ido-mysql" {
            host = "127.0.0.1",
            port = "3306",
            user = "icinga",
            password = "icinga",
            database = "icinga",
            table_prefix = "icinga_",
            instance_name = "icinga2",
            instance_description = "icinga2 instance"
    }

Starting Icinga 2 in debug mode in foreground using -x will show all
database queries.

Custom Attributes
-----------------

In Icinga 1.x there were so-called "custom variables" available prefixed
with an underscore, as well as plenty of other attributes such as
action\_url, notes\_url, icon\_image, etc. To overcome the limitations
of hardcoded custom attributes, Icinga 2 ships with the *custom*
attribute as dictionary.

For example, if you have PNP installed we could add a reference url to
Icinga Classic UI by using the classic method of defining an
action\_url.

    template Service "my-pnp-svc" {
            custom = {
                    action_url = "/pnp4nagios/graph?host=$HOSTNAME$&srv=$SERVICEDESC$' class='tips' rel='/pnp4nagios/popup?host=$HOSTNAME$&srv=$SERVICEDESC$",
            }
    }

And add that template again to our service definition:

            ...
            services["ping"] = {
                    templates = [ "ping4", "mail-notification-service", "my-cluster-dependency", "my-pnp-svc" ],
            },
            ...

While at it, our configuration tool will add its LDAP DN and a snmp
community to the service too, using += for additive attributes:

            ...
            services["ping"] = {
                    templates = [ "ping4", "mail-notification-service", "my-cluster-dependency", "my-pnp-svc" ],
                    custom += {
                            DN = "cn=icinga2-dev-svc,ou=icinga,ou=main,ou=IcingaConfig,ou=LConf,dc=icinga,dc=org",
                            SNMPCOMMUNITY = "public"
                    }
            },
            ...

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

Icinga 2 Configuration
======================

Configuration Introduction
--------------------------

In Icinga 2 configuration is based on objects. There’s no difference in
defining global settings for the core application or for a specific
runtime configuration object.

There are different types for the main application, its components and
tools. The runtime configuration objects such as hosts, services, etc
are defined using the same syntax.

Each configuration object must be unique by its name. Otherwise Icinga 2
will bail early on verifying the parsed configuration.

Main Configuration
------------------

Starting Icinga 2 requires the main configuration file called
"icinga2.conf". That’s the location where everything is defined or
included. Icinga 2 will only know the content of that file and included
configuration file snippets.

    # /usr/bin/icinga2 -c /etc/icinga2/icinga2.conf

> **Note**
>
> You can use just the main configuration file and put everything in
> there. Though that is not advised because configuration may be
> expanded over time. Rather organize runtime configuration objects into
> their own files and/or directories and include that in the main
> configuration file.

Configuration Syntax
--------------------

/\* TODO \*/

Details on the syntax can be found in the chapter
icinga2-config-syntax.html[Configuration Syntax]

Configuration Types
-------------------

/\* TODO \*/

Details on the available types can be found in the chapter
icinga2-config-types.html[Configuration Types]

Configuration Templates
-----------------------

Icinga 2 ships with the **Icinga Template Library (ITL)**. This is a set
of predefined templates and definitions available in your actual
configuration.

> **Note**
>
> Do not change the ITL’s files. They will be overridden on upgrade.
> Submit a patch upstream or include your very own configuration
> snippet.

Include the basic ITL set in your main configuration like

    include <itl/itl.conf>

> **Note**
>
> Icinga 2 recognizes the ITL’s installation path and looks for that
> specific file then.

Having Icinga 2 installed in standalone mode make sure to include
itl/standalone.conf as well (see sample configuration).

    include <itl/standalone.conf>

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

Icinga 2 Configuration Syntax
=============================

Configuration Syntax
--------------------

### Object Definition

Icinga 2 features an object-based configuration format. In order to
define objects the *object* keyword is used:

    object Host "host1.example.org" {
      display_name = "host1",

      check_interval = 30,
      retry_interval = 15,

      macros = {
        address = "192.168.0.1"
      }
    }

> **Note**
>
> The Icinga 2 configuration format is agnostic to whitespaces and
> new-lines.

> **Note**
>
> Colons (:) are not permitted in object names.

Each object is uniquely identified by its type (*Host*) and name
(*host1.example.org*). Objects can contain a comma-separated list of
property declarations. The following data types are available for
property values:

#### Numeric Literals

A floating-point number.

Example:

    -27.3

#### Duration Literal

Similar to floating-point numbers except for that fact that they support
suffixes to help with specifying time durations.

Example:

    2.5m

Supported suffixes include ms (milliseconds), s (seconds), m (minutes)
and h (hours).

#### String Literals

A string.

Example:

    "Hello World!"

Certain characters need to be escaped. The following escape sequences
are supported:

  ------------------------------------ ------------------------------------
  Character                            Escape sequence
  "                                    \\"
  \<TAB\>                              \\t
  \<CARRIAGE-RETURN\>                  \\r
  \<LINE-FEED\>                        \\n
  \<BEL\>                              \\b
  \<FORM-FEED\>                        \\f
  ------------------------------------ ------------------------------------

In addition to these pre-defined escape sequences you can specify
arbitrary ASCII characters using the backslash character (\\) followed
by an ASCII character in octal encoding.

#### Multiline String Literals

Strings spanning multiple lines can be specified by enclosing them in
{{{ and }}}.

Example.

    {{{This
    is
    a multi-line
    string.}}}

#### Boolean Literals

The keywords *true* and *false* are equivalent to 1 and 0 respectively.

#### Null Value

The *null* keyword can be used to specify an empty value.

#### Dictionary

An unordered list of key-value pairs. Keys must be unique and are
compared in a case-insensitive manner.

Individual key-value pairs must be separated from each other with a
comma. The comma after the last key-value pair is optional.

Example:

    {
      address = "192.168.0.1",
      port = 443
    }

> **Note**
>
> Identifiers may not contain certain characters (e.g. space) or start
> with certain characters (e.g. digits). If you want to use a dictionary
> key that is not a valid identifier you can put the key in double
> quotes.

> **Note**
>
> Setting a dictionary key to null causes the key to be removed from the
> dictionary.

#### Array

An ordered list of values.

Individual array elements must be separated from each other with a
comma. The comma after the last element is optional.

Example:

    [
      "hello",
      "world",
      42,
      [ "a", "nested", "array" ]
    ]

> **Note**
>
> An array may simultaneously contain values of different types, e.g.
> strings and numbers.

### Operators

In addition to the *=* operator shown above a number of other operators
to manipulate configuration objects are supported. Here’s a list of all
available operators:

#### Operator *=*

Sets a dictionary element to the specified value.

Example:

    {
      a = 5,
      a = 7
    }

In this example a has the value 7 after both instructions are executed.

#### Operator *+=*

Modifies a dictionary or array by adding new elements to it.

Example:

    {
      a = [ "hello" ],
      a += [ "world" ]
    }

In this example a contains both *"hello"* and *"world"*. This currently
only works for dictionaries and arrays. Support for numbers might be
added later on.

#### Operator *-=*

Removes elements from a dictionary.

Example:

    {
      a = { "hello", "world" },
      a -= { "world" }
    }

In this example a contains *"hello"*. Trying to remove an item that does
not exist is not an error. Not implemented yet.

#### Operator *\*=*

Multiplies an existing dictionary element with the specified number. If
the dictionary element does not already exist 0 is used as its value.

Example:

    {
      a = 60,
      a *= 5
    }

In this example a is 300. This only works for numbers. Not implemented
yet.

#### Operator */=*

Divides an existing dictionary element by the specified number. If the
dictionary element does not already exist 0 is used as its value.

Example:

    {
      a = 300,
      a /= 5
    }

In this example a is 60. This only works for numbers. Not implemented
yet.

### Attribute Shortcuts

#### Indexer Shortcut

Example:

    {
      hello["key"] = "world"
    }

This is equivalent to writing:

    {
      hello += {
        key = "world"
      }
    }

### Specifiers

Objects can have specifiers that have special meaning. The following
specifiers can be used (prefacing the *object* keyword):

#### Specifier *abstract*

This specifier identifies the object as a template which can be used by
other object definitions. The object will not be instantiated on its
own.

Instead of using the *abstract* specifier you can use the *template*
keyword which is a shorthand for writing *abstract object*:

    template Service "http" {
      ...
    }

#### Specifier *local*

This specifier disables replication for this object. The object will not
be sent to remote Icinga instances.

### Inheritance

Objects can inherit attributes from one or more other objects.

Example:

    template Host "default-host" {
      check_interval = 30,

      macros = {
        color = "red"
      }
    }

    template Host "test-host" inherits "default-host" {
      macros += {
        color = "blue"
      }
    }

    object Host "localhost" inherits "test-host" {
      macros += {
        address = "127.0.0.1",
        address6 = "::1"
      }
    }

> **Note**
>
> The *"default-host"* and *"test-host"* objects are marked as templates
> using the *abstract* keyword. Parent objects do not necessarily have
> to be *abstract* though in general they are.

> **Note**
>
> The += operator is used to insert additional properties into the
> macros dictionary. The final dictionary contains all 3 macros and the
> property *color* has the value *"blue"*.

Parent objects are resolved in the order they’re specified using the
*inherits* keyword.

### Comments

The Icinga 2 configuration format supports C/C++-style comments.

Example:

    /*
     This is a comment.
     */
    object Host "localhost" {
      check_interval = 30, // this is also a comment.
      retry_interval = 15
    }

### Includes

Other configuration files can be included using the *include* directive.
Paths must be relative to the configuration file that contains the
*include* directive.

Example:

    include "some/other/file.conf"
    include "conf.d/*.conf"

> **Note**
>
> Wildcard includes are not recursive.

Icinga also supports include search paths similar to how they work in a
C/C++ compiler:

    include <itl/itl.conf>

Note the use of angle brackets instead of double quotes. This causes the
config compiler to search the include search paths for the specified
file. By default \$PREFIX/icinga2 is included in the list of search
paths.

Wildcards are not permitted when using angle brackets.

### Library directive

The *library* directive can be used to manually load additional
libraries. Upon loading these libraries may provide additional types or
methods.

Example:

    library "snmphelper"

> **Note**
>
> The *icinga* library is automatically loaded at startup.

### Type Definition

By default Icinga has no way of semantically verifying its configuration
objects. This is where type definitions come in. Using type definitions
you can specify which attributes are allowed in an object definition.

Example:

    type Pizza {
            %require "radius",
            %attribute number "radius",

            %attribute dictionary "ingredients" {
                    %validator "ValidateIngredients",

                    %attribute string "*",

                    %attribute dictionary "*" {
                            %attribute number "quantity",
                            %attribute string "name"
                    }
            },

            %attribute any "custom::*"
    }

The Pizza definition provides the following validation rules:

-   Pizza objects must contain an attribute *radius* which has to be a
    number.

-   Pizza objects may contain an attribute *ingredients* which has to be
    a dictionary.

-   Elements in the ingredients dictionary can be either a string or a
    dictionary.

-   If they’re a dictionary they may contain attributes *quantity* (of
    type number) and *name* (of type string).

-   The script function *ValidateIngredients* is run to perform further
    validation of the ingredients dictionary.

-   Pizza objects may contain attribute matching the pattern
    *custom::\** of any type.

Valid types for type rules include: \* any \* number \* string \* scalar
(an alias for string) \* dictionary

Icinga 2 Configuration Types
============================

Configuration Format
--------------------

### Object Definition

Icinga 2 features an object-based configuration format. In order to
define objects the "object" keyword is used:

    object Host "host1.example.org" {
      alias = "host1",

      check_interval = 30,
      retry_interval = 15,

      macros = {
        address = "192.168.0.1"
      }
    }

> **Note**
>
> The Icinga 2 configuration format is agnostic to whitespaces and
> new-lines.

Each object is uniquely identified by its type ("Host") and name
("host1.example.org"). Objects can contain a comma-separated list of
property declarations. The following data types are available for
property values:

#### Numeric Literals

A floating-point number.

Example:

    -27.3

#### Duration Literal

Similar to floating-point numbers except for that fact that they support
suffixes to help with specifying time durations.

Example:

    2.5m

Supported suffixes include ms (milliseconds), s (seconds), m (minutes)
and h (hours).

#### String Literals

A string. No escape characters are supported at present though this will
likely change.

Example:

    "Hello World!"

#### Expression List

A list of expressions that when executed has a dictionary as a result.

Example:

    {
      address = "192.168.0.1",
      port = 443
    }

> **Note**
>
> Identifiers may not contain certain characters (e.g. space) or start
> with certain characters (e.g. digits). If you want to use a dictionary
> key that is not a valid identifier you can put the key in double
> quotes.

### Operators

In addition to the "=" operator shown above a number of other operators
to manipulate configuration objects are supported. Here’s a list of all
available operators:

#### Operator "="

Sets a dictionary element to the specified value.

Example:

    {
      a = 5,
      a = 7
    }

In this example a has the value 7 after both instructions are executed.

#### Operator "+="

Modifies a dictionary by adding new elements to it.

Example:

    {
      a = { "hello" },
      a += { "world" }
    }

In this example a contains both "hello" and "world". This currently only
works for expression lists. Support for numbers might be added later on.

#### Operator "-="

Removes elements from a dictionary.

Example:

    {
      a = { "hello", "world" },
      a -= { "world" }
    }

In this example a contains "hello". Trying to remove an item that does
not exist is not an error. Not implemented yet.

#### Operator "\*="

Multiplies an existing dictionary element with the specified number. If
the dictionary element does not already exist 0 is used as its value.

Example:

    {
      a = 60,
      a *= 5
    }

In this example a is 300. This only works for numbers. Not implemented
yet.

#### Operator "/="

Divides an existing dictionary element by the specified number. If the
dictionary element does not already exist 0 is used as its value.

Example:

    {
      a = 300,
      a /= 5
    }

In this example a is 60. This only works for numbers. Not implemented
yet.

### Attribute Shortcuts

#### Value Shortcut

Example:

    {
      "hello", "world"
    }

This is equivalent to writing:

    {
      _00000001 = "hello", _00000002 = "world"
    }

The item’s keys are monotonically increasing and the config compiler
takes care of ensuring that all keys are unique (even when adding items
to an existing attribute using +=).

#### Indexer Shortcut

Example:

    {
      hello["key"] = "world"
    }

This is equivalent to writing:

    {
      hello += {
        key = "world"
      }
    }

### Specifiers

Objects can have specifiers that have special meaning. The following
specifiers can be used (before the "object" keyword):

#### Specifier "abstract"

This specifier identifies the object as a template which can be used by
other object definitions. The object will not be instantiated on its
own.

Instead of using the "abstract" specifier you can use the "template"
keyword which is a shorthand for writing "abstract object":

    template Service "http" {
      ...
    }

#### Specifier "local"

This specifier disables replication for this object. The object will not
be sent to remote Icinga instances.

### Inheritance

Objects can inherit attributes from one or more other objects.

Example:

    abstract object Host "default-host" {
      check_interval = 30,

      macros = {
        color = "red"
      }
    }

    abstract object Host "test-host" inherits "default-host" {
      macros += {
        color = "blue"
      }
    }

    object Host "localhost" inherits "test-host" {
      macros += {
        address = "127.0.0.1",
        address6 = "::1"
      }
    }

> **Note**
>
> The "default-host" and "test-host" objects are marked as templates
> using the "abstract" keyword. Parent objects do not necessarily have
> to be "abstract" though in general they are.

> **Note**
>
> The += operator is used to insert additional properties into the
> macros dictionary. The final dictionary contains all 3 macros and the
> property "color" has the value "blue".

Parent objects are resolved in the order they’re specified using the
"inherits" keyword. Parent objects must already be defined by the time
they’re used in an object definition.

### Comments

The Icinga 2 configuration format supports C/C++-style comments.

Example:

    /*
     This is a comment.
     */
    object Host "localhost" {
      check_interval = 30, // this is also a comment.
      retry_interval = 15
    }

### Includes

Other configuration files can be included using the "\#include"
directive. Paths must be relative to the configuration file that
contains the "\#include" keyword:

Example:

    #include "some/other/file.conf"
    #include "conf.d/*.conf"

Icinga also supports include search paths similar to how they work in a
C/C++ compiler:

    #include <itl/itl.conf>

Note the use of angle brackets instead of double quotes. This causes the
config compiler to search the include search paths for the specified
file. By default \$PREFIX/icinga2 is included in the list of search
paths.

Wildcards are not permitted when using angle brackets.

### Library directive

The "\#library" directive can be used to manually load additional
libraries. Upon loading these libraries may provide additional classes
or methods.

Example:

    #library "snmphelper"

> **Note**
>
> The "icinga" library is automatically loaded by Icinga.

### Type Definition

By default Icinga has no way of semantically verifying its configuration
objects. This is where type definitions come in. Using type definitions
you can specify which attributes are allowed in an object definition.

Example:

    type Pizza {
            %require "radius",
            %attribute number "radius",

            %attribute dictionary "ingredients" {
                    %validator "ValidateIngredients",

                    %attribute string "*",

                    %attribute dictionary "*" {
                            %attribute number "quantity",
                            %attribute string "name"
                    }
            },

            %attribute any "custom::*"
    }

The Pizza definition provides the following validation rules:

-   Pizza objects must contain an attribute "radius" which has to be a
    number.

-   Pizza objects may contain an attribute "ingredients" which has to be
    a dictionary.

-   Elements in the ingredients dictionary can be either a string or a
    dictionary.

-   If they’re a dictionary they may contain attributes "quantity" (of
    type number) and "name" (of type string).

-   The script function "ValidateIngredients" is run to perform further
    validation of the ingredients dictionary.

-   Pizza objects may contain attribute matching the pattern
    "custom::\*" of any type.

Valid types for type rules include: \* any \* number \* string \* scalar
(an alias for string) \* dictionary

Configuration Objects
---------------------

### Type: IcingaApplication

The "IcingaApplication" type is used to specify global configuration
parameters for Icinga. There must be exactly one application object in
each Icinga 2 configuration. The object must have the "local" specifier.

Example:

    local object IcingaApplication "icinga" {
      cert_path = "my-cert.pem",
      ca_path = "ca.crt",

      node = "192.168.0.1",
      service = 7777,

      pid_path = "/var/run/icinga2.pid",
      state_path = "/var/lib/icinga2/icinga2.state",

      macros = {
        plugindir = "/usr/local/icinga/libexec"
      }
    }

#### Attribute: cert\_path

This is used to specify the SSL client certificate Icinga 2 will use
when connecting to other Icinga 2 instances. This property is optional
when you’re setting up a non-networked Icinga 2 instance.

#### Attribute: ca\_path

This is the public CA certificate that is used to verify connections
from other Icinga 2 instances. This property is optional when you’re
setting up a non-networked Icinga 2 instance.

#### Attribute: node

The externally visible IP address that is used by other Icinga 2
instances to connect to this instance. This property is optional when
you’re setting up a non-networked Icinga 2 instance.

> **Note**
>
> Icinga does not bind to this IP address.

#### Attribute: service

The port this Icinga 2 instance should listen on. This property is
optional when you’re setting up a non-networked Icinga 2 instance.

#### Attribute: pid\_path

Optional. The path to the PID file. Defaults to "icinga.pid" in the
current working directory.

#### Attribute: state\_path

Optional. The path of the state file. This is the file Icinga 2 uses to
persist objects between program runs. Defaults to "icinga2.state" in the
current working directory.

#### Attribute: macros

Optional. Global macros that are used for service checks and
notifications.

### Type: Component

Icinga 2 uses a number of components to implement its feature-set. The
"Component" configuration object is used to load these components and
specify additional parameters for them. "Component" objects must have
the "local" specifier. The typical components to be loaded in the
default configuration would be "checker", "delegation" and more.

Example "compat":

    local object Component "compat" {
      status_path = "/var/cache/icinga2/status.dat",
      objects_path = "/var/cache/icinga2/objects.cache",
    }

#### Attribute: status\_path

Specifies where Icinga 2 Compat component will put the status.dat file,
which can be read by Icinga 1.x Classic UI and other addons. If not set,
it defaults to the localstatedir location.

#### Attribute: objects\_path

Specifies where Icinga 2 Compat component will put the objects.cache
file, which can be read by Icinga 1.x Classic UI and other addons. If
not set, it defaults to the localstatedir location.

### Type: ConsoleLogger

Specifies Icinga 2 logging to the console. Objects of this type must
have the "local" specifier.

Example:

    local object ConsoleLogger "my-debug-console" {
      severity = "debug"
    }

#### Attribute: severity

The minimum severity for this log. Can be "debug", "information",
"warning" or "critical". Defaults to "information".

### Type: FileLogger

Specifies Icinga 2 logging to a file. Objects of this type must have the
"local" specifier.

Example:

    local object FileLogger "my-debug-file" {
      severity = "debug",
      path = "/var/log/icinga2/icinga2-debug.log"
    }

#### Attribute: path

The log path.

#### Attribute: severity

The minimum severity for this log. Can be "debug", "information",
"warning" or "critical". Defaults to "information".

### Type: SyslogLogger

Specifies Icinga 2 logging to syslog. Objects of this type must have the
"local" specifier.

Example:

    local object SyslogLogger "my-crit-syslog" {
      severity = "critical"
    }

#### Attribute: severity

The minimum severity for this log. Can be "debug", "information",
"warning" or "critical". Defaults to "information".

### Type: Endpoint

Endpoint objects are used to specify connection information for remote
Icinga 2 instances. Objects of this type should not be local:

    object Endpoint "icinga-c2" {
      node = "192.168.5.46",
      service = 7777,
    }

#### Attribute: node

The hostname/IP address of the remote Icinga 2 instance.

#### Attribute: service

The service name/port of the remote Icinga 2 instance.

### Type: CheckCommand

A check command definition. Additional default command macros can be
defined here.

Example:

    object CheckCommand "check_snmp" inherits "plugin-check-command" {
      command = "$plugindir$/check_snmp -H $address$ -C $community$ -o $oid$",

      macros = {2yy
        plugindir = "/usr/lib/nagios/plugins",
        address = "127.0.0.1",
        community = "public",
      }
    }

### Type: NotificationCommand

A notification command definition.

Example:

    object NotificationCommand "mail-service-notification" inherits "plugin-notification-command" {
      command = "/usr/bin/printf \"%b\" \"***** Icinga  *****\n\nNotification Type: $NOTIFICATIONTYPE$\n\nService: $SERVICEDESC$\nHost: $HOSTALIAS$\nAddress: $HOSTADDRESS$\nState: $SERVICESTATE$\n\nDate/Time: $LONGDATETIME$\n\nAdditional Info: $SERVICEOUTPUT$\n\nComment: [$NOTIFICATIONAUTHORNAME$] $NOTIFICATIONCOMMENT$\n\n\" | /usr/bin/mail -s \"$NOTIFICATIONTYPE$ - $HOSTNAME$ - $SERVICEDESC$ - $SERVICESTATE$\" $CONTACTEMAIL$",
    }

Type: EventCommand

    An event command definition.

    NOTE: Similar to Icinga 1.x event handlers.

    Example:

    -------------------------------------------------------------------------------
    object EventCommand "restart-httpd-event" inherits "plugin-event-command" {
      command = "/usr/local/icinga/libexec/restart-httpd.sh",
    }
    -------------------------------------------------------------------------------


    Type: Service

Service objects describe network services and how they should be checked
by Icinga 2.

> **Note**
>
> Better create a service template and use that reference on the host
> definition as shown below.

Example:

    object Service "localhost-uptime" {
      host_name = "localhost",

      alias = "localhost Uptime",

      methods = {
        check = "PluginCheck"
      },

      check_command = "check_snmp",

      macros = {
        plugindir = "/usr/lib/nagios/plugins",
        address = "127.0.0.1",
        community = "public",
        oid = "DISMAN-EVENT-MIB::sysUpTimeInstance"
      }

      check_interval = 60s,
      retry_interval = 15s,

      servicegroups = { "all-services", "snmp" },

      checkers = { "*" },
    }

#### Attribute: host\_name

The host this service belongs to. There must be a "Host" object with
that name.

#### Attribute: alias

Optional. A short description of the service.

#### Attribute: methods - check

The check type of the service. For now only external check plugins are
supported ("PluginCheck").

#### Attribute: check\_command

Optional when not using the "external plugin" check type. The check
command. May contain macros.

#### Attribute: check\_interval

Optional. The check interval (in seconds).

#### Attribute: retry\_interval

Optional. The retry interval (in seconds). This is used when the service
is in a soft state.

#### Attribute: servicegroups

Optional. The service groups this service belongs to.

#### Attribute: checkers

Optional. A list of remote endpoints that may check this service.
Wildcards can be used here.

### Type: ServiceGroup

A group of services.

Example:

    object ServiceGroup "snmp" {
      alias = "SNMP services",

      custom = {
        notes_url = "http://www.example.org/",
        action_url = "http://www.example.org/",
      }
    }

#### Attribute: alias

Optional. A short description of the service group.

#### Attribute: notes\_url

Optional. Notes URL. Used by the CGIs.

#### Attribute: action\_url

Optional. Action URL. Used by the CGIs.

### Type: Host

A host. Unlike in Icinga 1.x hosts are not checkable objects in Icinga
2.

Example:

    object Host "localhost" {
      alias = "The best host there is",

      hostgroups = [ "all-hosts" ],

      hostcheck = "ping",
      dependencies = [ "router-ping" ]

      services["ping"] = { templates = "ping" }
      services["http"] = {
        templates = "my-http",
        macros = {
          vhost = "test1.example.org",
          port = 81
        }
      }

      check_interval = 60m,
      retry_interval = 15m,

      servicegroups = [ "all-services" ],

      checkers = { "*" },
    }

#### Attribute: alias

Optional. A short description of the host.

#### Attribute: hostgroups

Optional. A list of host groups this host belongs to.

#### Attribute: hostcheck

Optional. A service that is used to determine whether the host is up or
down.

#### Attribute: hostdependencies

Optional. A list of hosts that are used to determine whether the host is
unreachable.

#### Attribute: servicedependencies

Optional. A list of services that are used to determine whether the host
is unreachable.

#### Attribute: services

Inline definition of services. Each service name is defined in square
brackets and got its own dictionary with attribute properties, such as
the template service being used. All other service-related properties
are additively copied into the new service object.

The new service’s name is "hostname-service" - where "service" is the
array key in the services array.

The priority for service properties is (from highest to lowest):

1.  Properties specified in the dictionary of the inline service
    definition

2.  Host properties

3.  Properties inherited from the new service’s parent object

#### Attribute: check\_interval

Optional. Copied into inline service definitions. The host itself does
not have any checks.

#### Attribute: retry\_interval

Optional. Copied into inline service definitions. The host itself does
not have any checks.

#### Attribute: servicegroups

Optional. Copied into inline service definitions. The host itself does
not have any checks.

#### Attribute: checkers

Optional. Copied into inline service definitions. The host itself does
not have any checks.

### Type: HostGroup

A group of hosts.

Example

    object HostGroup "my-hosts" {
      alias = "My hosts",

      notes_url = "http://www.example.org/",
      action_url = "http://www.example.org/",
    }

#### Attribute: alias

Optional. A short description of the host group.

#### Attribute: notes\_url

Optional. Notes URL. Used by the CGIs.

#### Attribute: action\_url

Optional. Action URL. Used by the CGIs.

### Type: PerfdataWriter

Write check result performance data to a defined path using macro
pattern.

Example

    local object PerfdataWriter "pnp" {
      perfdata_path = "/var/spool/icinga2/perfdata/service-perfdata",
      format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$TIMET$\tHOSTNAME::$HOSTNAME$\tSERVICEDESC::$SERVICEDESC$\tSERVICEPERFDATA::$SERVICEPERFDATA$\tSERVICECHECKCOMMAND::$SERVICECHECKCOMMAND$\tHOSTSTATE::$HOSTSTATE$\tHOSTSTATETYPE::$HOSTSTATETYPE$\tSERVICESTATE::$SERVICESTATE$\tSERVICESTATETYPE::$SERVICESTATETYPE$",
      rotation_interval = 15s,
    }

#### Attribute: perfdata\_path

Path to the service perfdata file.

> **Note**
>
> Will be automatically rotated with timestamp suffix.

#### Attribute: format\_template

Formatting of performance data output for graphing addons or other post
processing.

#### Attribute: rotation\_interval

Rotation interval for the file defined in *perfdata\_path*.

### Type: IdoMySqlConnection

IDO DB schema compatible output into mysql database.

Example

    library "ido_mysql"
    local object IdoMysqlDbConnection "mysql-ido" {
      host = "127.0.0.1",
      port = "3306",
      user = "icinga",
      password = "icinga",
      database = "icinga",
      table_prefix = "icinga_",
      instance_name = "icinga2",
      instance_description = "icinga2 dev instance"
    }

#### Attribute: host

MySQL database host address. Default is *localhost*.

#### Attribute: port

MySQL database port. Default is *3306*.

#### Attribute: user

MySQL database user with read/write permission to the icinga database.
Default is *icinga*.

#### Attribute: password

MySQL database user’s password. Default is *icinga*.

#### Attribute: database

MySQL database name. Default is *icinga*.

#### Attribute: table\_prefix

MySQL database table prefix. Default is *icinga\_*.

#### Attribute: instance\_name

Unique identifier for the local Icinga 2 instance.

#### Attribute: instance\_description

Optional. Description for the Icinga 2 instance.

### Type: LiveStatusComponent

Livestatus api interface available as tcp or unix socket.

Example

    library "livestatus"

    local object LivestatusComponent "livestatus-tcp" {
      socket_type = "tcp",
      host = "127.0.0.1",
      port = "6558"
    }

    local object LivestatusComponent "livestatus-unix" {
      socket_type = "unix",
      socket_path = "/var/run/icinga2/livestatus"
    }

#### Attribute: socket\_type

*tcp* or *unix* socket. Default is *unix*.

> **Note**
>
> *unix* sockets are not supported on Windows.

#### Attribute: host

Only valid when socket\_type="tcp". Host address to listen on for
connections.

#### Attribute: port

Only valid when socket\_type="tcp". Port to listen on for connections.

#### Attribute: socket\_path

Only valid when socket\_type="unix". Local unix socket file. Not
supported on Windows.

Configuration Examples
----------------------

### Non-networked minimal example

> **Note**
>
> Icinga 2 ITL provides itl/standalone.conf which loads all required
> components, as well as itl/itl.conf includes many object templates
> already for an easy start with Icinga 2.

    local object IcingaApplication "icinga" {

    }

    local object Component "checker" {

    }

    local object Component "delegation" {

    }

    object CheckCommand "ping" {
      command = "$plugindir$/check_ping -H $address$ -w $wrta$,$wpl$% -c $crta$,$cpl$%",
    }

    template Service "icinga-service" {
      methods = {
        check = "PluginCheck"
      },

      macros = {
        plugindir = "/usr/lib/nagios/plugins"
      }
    }

    template Service "ping-tmpl" inherits "icinga-service" {
      check_command = "ping",
      macros += {
        wrta = 50,
        wpl = 5,
        crta = 100,
        cpl = 10
      }
    }

    object Host "localhost" {
      services["ping"]  = { templates = "ping-tmpl" },

      macros = {
        address = "127.0.0.1"
      },

      check_interval = 10m
    }

> **Note**
>
> You may also want to load the "compat" component if you want Icinga 2
> to write status.dat and objects.cache files.

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/
