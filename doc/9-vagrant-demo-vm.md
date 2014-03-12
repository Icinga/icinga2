# <a id="vagrant"></a> Vagrant Demo VM

The Icinga 2 Git repository contains support for [Vagrant](http://docs.vagrantup.com/v2/)
with VirtualBox. In order to build the Vagrant VM first you will have to check out
the Git repository:

    $ git clone git://git.icinga.org/icinga2.git

Once you have checked out the Git repository you can build the VM using the
following command:

    $ vagrant up

> **Note**
>
> Vagrant and VirtualBox are available for various distributions. Please note
> that Vagrant version `1.0.x` is not supported. At least version `1.2.x` is
> required to be installed (for example from [http://downloads.vagrantup.com](http://downloads.vagrantup.com)).

The Vagrant VM is based on CentOS 6.4 and uses the official Icinga 2 RPM
packages from `packages.icinga.org`. The check plugins are installed from
EPEL providing RPMs with sources from the Monitoring Plugins project.

SSH login is available using `vagrant ssh`.

## <a id="vagrant-demo-guis"></a> Vagrant Demo GUIs

In addition to installing Icinga 2 the Vagrant puppet modules also install the
Icinga 1.x Classic UI and Icinga Web.

  GUI             | Url                                                                  | Credentials
  ----------------|----------------------------------------------------------------------|------------------------
  Classic UI      | [http://localhost:8080/icinga](http://localhost:8080/icinga)         | icingaadmin/icingaadmin
  Icinga Web      | [http://localhost:8080/icinga-web](http://localhost:8080/icinga-web) | root/password


## <a id="vagrant-windows"></a> Vagrant on Windows

You need to install [VirtualBox](#https://www.virtualbox.org/wiki/Downloads)
next to [Vagrant for Windows](#http://www.vagrantup.com/downloads.html). For SSH access
you need to install [Git for Windows](#http://git-scm.com/download/win) too.

Either download and extract the Icinga 2 tarball (or git archive) or clone the
git repository using your preferred git gui.

Open the Windows command prompt (cmd+R) and change the directory to your
Icinga 2 directory containing the `Vagrantfile` file and start the Vagrant box.

    c:> cd C:\Users\admin\icinga2
    c:> vagrant up

> **Note**
>
> If SSH access is not working, you may need to add the Git binary path to the system path.

    c:> set PATH=%PATH%;C:\Program Files (x86)\Git\bin
    c:> vagrant ssh

For manual SSH access using [Putty](#http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html)
you'll need the following default credentials:

  Name            |Value
  ----------------|----------------
  hostname        | 127.0.0.1
  port            | 2222
  connection type | ssh
  username        | vagrant
  password        | vagrant