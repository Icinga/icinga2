Purpose
=======

Documentation on the compatibility and changes introduced with Icinga 2.

Introduction
============

Unlike Icinga 1.x, all used components (not only those for
compatibility) run asynchronous and use queues, if required. That way
Icinga 2 does not get blocked by any event, action or execution.

Configuration
=============

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
=============

All native check plugins can be used with Icinga 2. The configuration of
check commands is changed due to the new configuration format.

Classic status and log files
============================

Icinga 2 will write status.dat and objects.cache in a given interval
like known from Icinga 1.x - including the logs and their archives in
the old format and naming syntax. That way you can point any existing
Classic UI installation to the new locations (or any other addon/plugin
using them).

External Commands
=================

Like known from Icinga 1.x, Icinga 2 also provides an external command
pipe allowing your scripts and guis to send commands to the core
triggering various actions.

Some commands are not supported though as their triggered functionality
is not available in Icinga 2 anymore.

For a detailed list, please check:
[https://wiki.icinga.org/display/icinga2/External+Commands](https://wiki.icinga.org/display/icinga2/External+Commands)

Performance Data
================

The Icinga 1.x Plugin API defines the performance data format. Icinga 2
parses the check output accordingly and writes performance data files
based on template macros. File rotation interval can be defined as well.

Unlike Icinga 1.x you can define multiple performance data writers for
all your graphing addons such as PNP, inGraph or graphite.

IDO DB
======

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
==========

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
==================

Unlike Icinga 1.x Icinga 2 is a multithreaded application and processes
check results in memory. The old checkresult reaper reading files from
disk again is obviously not required anymore for native checks.

Some popular addons have been injecting their checkresults into the
Icinga 1.x checkresult spool directory bypassing the external command
pipe and PROCESS\_SERVICE\_CHECK\_RESULT mainly for performance reasons.

In order to support that functionality as well, Icinga 2 got its
optional checkresult reaper.

Changes
=======

This is a collection of known changes in behaviour, configuration and
outputs.

> **Note**
>
> May be incomplete, and requires updates in the future.

TODO

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/
