# <a id="vagrant"></a> Vagrant Demo VM

The Icinga 2 Git repository contains support for Vagrant. In order to build the
Vagrant VM first you will have to check out the Git repository:

    $ git clone git://git.icinga.org/icinga2.git

Once you have checked out the Git repository you can build the VM using the
following command:

    $ vagrant up

The Vagrant VM is based on CentOS 6.4 and uses the official Icinga 2 RPM
packages from `packages.icinga.org`.

In addition to installing Icinga 2 the Vagrant script also installs the Icinga
1.x Classic UI and the check plugins that are available from the Nagios Plugins
project.

The Classic UI is available at [http://localhost:8080/icinga](http://localhost:8080/icinga).
By default both the username and password are `icingaadmin`.

An instance of icinga-web is installed at [http://localhost:8080/icinga-web](http://localhost:8080/icinga-web).
The username is `root` and the password is `password`.

SSH login is available using `vagrant ssh`.
