# <a id="vagrant"></a> Vagrant Demo VM

The Icinga 2 Git repository contains support for [Vagrant](http://docs.vagrantup.com/v2/)
with VirtualBox. Please note that Vagrant version `1.0.x` is not supported. At least
version `1.2.x` is required.

In order to build the Vagrant VM first you will have to check out
the Git repository:

    $ git clone git://git.icinga.org/icinga2.git

Once you have checked out the Git repository you can build the VM using the
following command:

    $ vagrant up

The Vagrant VM is based on CentOS 6.4 and uses the official Icinga 2 RPM
packages from `packages.icinga.org`. The check plugins are installed from
EPEL providing RPMs with sources from the Monitoring Plugins project.

## <a id="vagrant-demo-guis"></a> Demo GUIs

In addition to installing Icinga 2 the Vagrant puppet modules also install the
Icinga 1.x Classic UI and Icinga Web.

  GUI             | Url                                                                  | Credentials
  ----------------|----------------------------------------------------------------------|------------------------
  Classic UI      | [http://localhost:8080/icinga](http://localhost:8080/icinga)         | icingaadmin / icingaadmin
  Icinga Web      | [http://localhost:8080/icinga-web](http://localhost:8080/icinga-web) | root / password


## <a id="vagrant-windows"></a> SSH Access

You can access the Vagrant VM using SSH:

    $ vagrant ssh

Alternatively you can use your favorite SSH client:

  Name            | Value
  ----------------|----------------
  Host            | 127.0.0.1
  Port            | 2222
  Username        | vagrant
  Password        | vagrant

