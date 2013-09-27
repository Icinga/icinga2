# Getting Started

## Installation

This tutorial is a step-by-step introduction to installing Icinga 2 and
the standalone version of the Icinga 1.x classic web interface. It assumes
that you are familiar with the system you're installing Icinga 2 on.

### Setting up Icinga 2

In order to get started with Icinga 2 you will have to install it. The
preferred way of doing this is to use the official Debian or RPM
package repositories depending on which Linux distribution you are
running.

  Distribution            |Repository URL
  ------------------------|---------------------------
  Debian                  |http://packages.icinga.org/debian/
  RHEL/CentOS 5           |http://packages.icinga.org/epel/5/release/
  RHEL/CentOS 6           |http://packages.icinga.org/epel/6/release/

Packages for distributions other than the ones listed above may also be
available. Please check http://packages.icinga.org/ to see if packages
are available for your favorite distribution.

In case you're running a distribution for which Icinga 2 packages are
not yet available you will have to check out the Icinga 2 Git repository
from git://git.icinga.org/icinga2 and read the *INSTALL* file.

#### Installation Paths

By default Icinga 2 uses the following files and directories:

  Path                                |Description
  ------------------------------------|------------------------------------
  /etc/icinga2                        |Contains Icinga 2 configuration files.
  /etc/init.d/icinga2                 |The Icinga 2 init script.
  /usr/share/doc/icinga2              |Documentation files that come with Icinga 2.
  /usr/share/icinga2/itl              |The Icinga Template Library.
  /var/run/icinga2                    |Command pipe and PID file.
  /var/cache/icinga2                  |Performance data files and status.dat/objects.cache.
  /var/lib/icinga2                    |The Icinga 2 state file.

#### Configuration

An example configuration file is installed for you in /etc/icinga2/icinga2.conf.

Here's a brief description of the concepts the example configuration file
introduces:

    /**
     * Icinga 2 configuration file
     * - this is where you define settings for the Icinga application including
     * which hosts/services to check.
     *
     * The docs/icinga2-config.adoc file in the source tarball has a detailed
     * description of what configuration options are available.
     */

Icinga 2 supports [C/C++-style comments](#comments).

    include <itl/itl.conf>
    include <itl/standalone.conf>

The *include* directive can be used to include other files. The *itl/itl.conf*
file is distributed as part of Icinga 2 and provides a number of useful templates
and constants you can use to configure your services.

    /**
     * Global macros
     */
    set IcingaMacros = {
      plugindir = "/usr/local/icinga/libexec"
    }

Icinga 2 lets you define free-form macros. The IcingaMacros variable can be used
to define global macros which are available in all command definitions.

    /**
     * The compat library periodically updates the status.dat and objects.cache
     * files. These are used by the Icinga 1.x CGIs to display the state of
     * hosts and services. CompatLog writeis the Icinga 1.x icinga.log and archives.
     */
    library "compat"

Some of Icinga 2's functionality is available in separate libraries. These
libraries usually implement their own object types that can be used to configure
what you want the library to do.

    object StatusDataWriter "status" { }
    object ExternalCommandListener "command" { }
    object CompatLogger "compat-log" { }

Those three object types are provided by the *compat* library:

  Type                     | Description
  -------------------------|-------------------------
  StatusDataWriter         | Responsible for writing the status.dat and objects.cache files.
  ExternalCommandListener  | Implements the command pipe which is used by the CGIs to send commands to Icinga 2.
  CompatLogger             | Writes log files in a format that is compatible with Icinga 1.x.

    /**
     * And finally we define some host that should be checked.
     */
    object Host "localhost" {
      services["ping4"] = {
        templates = [ "ping4" ]
      },

      services["ping6"] = {
        templates = [ "ping6" ]
      },

      services["http"] = {
        templates = [ "http_ip" ]
      },

      services["ssh"] = {
        templates = [ "ssh" ]
      },

      services["load"] = {
        templates = [ "load" ]
      },

      services["processes"] = {
        templates = [ "processes" ]
      },

      services["users"] = {
        templates = [ "users" ]
      },

      services["disk"] = {
        templates = [ "disk" ]
      },

      macros = {
        address = "127.0.0.1",
        address6 = "::1",
      },

      check = "ping4",
    }

This defines a host named "localhost" which has a couple of services. Services
may inherit from one or more service templates.

The templates *ping4*, *ping6*, *http_ip*, *ssh*, *load*, *processes*, *users*
and *disk* are all provided by the Icinga Template Library (short ITL) which
we enabled earlier by including the itl/itl.conf configuration file.

The *macros* attribute can be used to define macros that are available for all
services which belong to this host. Most of the templates in the Icinga Template
Library require an *address* macro.

### Setting up Icinga Classic UI

Icinga 2 can write *status.dat* and *objects.cache* files in the format that
is supported by the Icinga 1.x Classic UI. External commands (a.k.a. the
"command pipe") are also supported. It also supports writing Icinga 1.x
log files which are required for the reporting functionality in the Classic UI.

These features are implemented as part of the *compat* library and are enabled
by default in the example configuration file.

You should be able to find the *status.dat* and *objects.cache* files in
*/var/cache/icinga2*. The log files can be found in */var/log/icinga2/compat*.
The command pipe can be found in */var/run/icinga2*.

#### Installing Icinga Classic UI

You can install Icinga 1.x Classic UI in standalone mode using the
following commands:

    $ wget http://downloads.sourceforge.net/project/icinga/icinga/1.9.3/icinga-1.9.3.tar.gz
    $ tar xzf icinga-1.9.3.tar.gz ; cd icinga-1.9.3
    $ ./configure --enable-classicui-standalone --prefix=/usr/local/icinga2-classicui
    $ make classicui-standalone
    $ sudo make install classicui-standalone install-webconf-auth
    $ sudo service apache2 restart

> **Note**
>
> A detailed guide on installing Icinga 1.x Classic UI Standalone can be
> found on the Icinga Wiki here:
> [https://wiki.icinga.org/display/howtos/Setting+up+Icinga+Classic+UI+Standalone](https://wiki.icinga.org/display/howtos/Setting+up+Icinga+Classic+UI+Standalone)

#### Configuring the Classic UI

After installing the Classic UI you will need to update the following
settings in your *cgi.cfg* configuration file in the *STANDALONE (ICINGA 2)
OPTIONS* section:

  Configuration Setting               |Value
  ------------------------------------|------------------------------------
  object\_cache\_file                 |/var/cache/icinga2/objects.cache
  status\_file                        |/var/cache/icinga2/status.dat
  resource\_file                      |-
  command\_file                       |/var/run/icinga2/icinga2.cmd
  check\_external\_commands           |1
  interval\_length                    |60
  status\_update\_interval            |10
  log\_file                           |/var/log/icinga2/compat/icinga.log
  log\_rotation\_method               |h
  log\_archive\_path                  |/var/log/icinga2/compat/archives
  date\_format                        |us
  ------------------------------------ ------------------------------------

> **Note**
>
> Depending on how you installed Icinga 2 some of those paths and options
> might be different.

In order for commands to work you will need to grant the web server
write permissions for the command pipe:

    # chgrp www-data /var/run/icinga2/icinga2.cmd
    # chmod 660 /var/run/icinga2/icinga2.cmd

> **Note**
>
> Change "www-data" to the group the Apache HTTP daemon is running as.

Verify that your Icinga 1.x Classic UI works by browsing to your Classic
UI installation URL, e.g.
[http://localhost/icinga](http://localhost/icinga)

### Configuring IDO

TODO

## Running Icinga

TODO

## Monitoring Basics

### Hosts

TODO

### Services

TODO

### Check Commands

TODO

### Macros

TODO

## Using Templates

TODO

## Groups

TODO

## Host/Service Dependencies

TODO

## Time Periods

TODO

## Notifications

TODO
