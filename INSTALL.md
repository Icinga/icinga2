# Installing Icinga 2

The recommended way of installing Icinga 2 is to use packages. The Icinga
project provides both release and development packages for a number
of operating systems.

Please check the documentation in the [doc/](doc/) directory for a current list
of available packages and detailed installation instructions.

The online documentation is available at [docs.icinga.com](https://docs.icinga.com)
and will guide you step by step.

There are a number of known caveats when installing from source such as
incorrect directory and file permissions. So even if you're planning to
not use the official packages it is advisable to build your own Debian
or RPM packages.

# Builds

This information is intended for developers and packagers.

## Build Requirements

The following requirements need to be fulfilled in order to build the
application using a dist tarball (package names for RHEL and Debian in
parentheses):

* cmake >= 2.6
* GNU make (make)
* C++ compiler which supports C++11 (gcc-c++ >= 4.7 on RHEL/SUSE, build-essential on Debian, alternatively clang++, build-base on Alpine)
 * RedHat Developer Tools on RHEL5/6 (details on building below)
* pkg-config
* OpenSSL library and header files >= 1.0.1 (openssl-devel on RHEL, libopenssl1-devel on SLES11,
libopenssl-devel on SLES12, libssl-dev on Debian, libressl-dev on Alpine)
* Boost library and header files >= 1.48.0 (boost148-devel on EPEL for RHEL / CentOS, libboost-all-dev on Debian, boost-dev on Alpine)
* GNU bison (bison)
* GNU flex (flex) >= 2.5.35
* recommended: libexecinfo on FreeBSD (automatically used when Icinga 2 is
               installed via port or package)
* optional: MySQL (mysql-devel on RHEL, libmysqlclient-devel on SUSE, libmysqlclient-dev until Debian 8 jessie / default-libmysqlclient-dev from Debian 9 stretch, mariadb-dev on Alpine);
            set CMake variable `ICINGA2_WITH_MYSQL` to `OFF` to disable this module
* optional: PostgreSQL (postgresql-devel on RHEL, libpq-dev on Debian, postgresql-dev on Alpine); set CMake
            variable `ICINGA2_WITH_PGSQL` to `OFF` to disable this module
* optional: YAJL (yajl-devel on RHEL, libyajl-dev on Debian, yajl-dev on Alpine)
* optional: libedit (libedit-devel on CentOS (RHEL requires rhel-7-server-optional-rpms
            repository for el7 e.g.), libedit-dev on Debian and Alpine)
* optional: Termcap (libtermcap-devel on RHEL, not necessary on Debian) - only
            required if libedit doesn't already link against termcap/ncurses
* optional: libwxgtk2.8-dev or newer (wxGTK-devel and wxBase) - only required when building the Icinga 2 Studio

Note: RHEL5 ships an ancient flex version. Updated packages are available for
example from the repoforge buildtools repository.

* x86: https://mirror.hs-esslingen.de/repoforge/redhat/el5/en/i386/buildtools/
* x86\_64: https://mirror.hs-esslingen.de/repoforge/redhat/el5/en/x86\_64/buildtools/

### User Requirements

By default Icinga will run as user 'icinga' and group 'icinga'. Additionally the
external command pipe and livestatus features require a dedicated command group
'icingacmd'. You can choose your own user/group names and pass them to CMake
using the `ICINGA2_USER`, `ICINGA2_GROUP` and `ICINGA2_COMMAND_GROUP` variables.

    # groupadd icinga
    # groupadd icingacmd
    # useradd -c "icinga" -s /sbin/nologin -G icingacmd -g icinga icinga

On Alpine (which uses ash busybox) you can run:

    # addgroup -S icinga
    # addgroup -S icingacmd
    # adduser -S -D -H -h /var/spool/icinga2 -s /sbin/nologin -G icinga -g icinga icinga
    # adduser icinga icingacmd

Add the web server user to the icingacmd group in order to grant it write
permissions to the external command pipe and livestatus socket:

    # usermod -a -G icingacmd www-data

Make sure to replace "www-data" with the name of the user your web server
is running as.

## Building Icinga 2

Once you have installed all the necessary build requirements you can build
Icinga 2 using the following commands:

    $ mkdir build && cd build
    $ cmake ..
    $ make
    $ make install

You can specify an alternative installation prefix using `-DCMAKE_INSTALL_PREFIX`:

    $ cmake .. -DCMAKE_INSTALL_PREFIX=/tmp/icinga2

In addition to `CMAKE_INSTALL_PREFIX` the following Icinga-specific cmake
variables are supported:

- `ICINGA2_USER`: The user Icinga 2 should run as; defaults to `icinga`
- `ICINGA2_GROUP`: The group Icinga 2 should run as; defaults to `icinga`
- `ICINGA2_GIT_VERSION_INFO`: Whether to use Git to determine the version number; defaults to `ON`
- `ICINGA2_COMMAND_GROUP`: The command group Icinga 2 should use; defaults to `icingacmd`
- `ICINGA2_UNITY_BUILD`: Whether to perform a unity build; defaults to `ON`
- `ICINGA2_PCH_BUILD`: Whether to use pre-compiled headers (PCH); defaults to `ON`
- `ICINGA2_LTO_BUILD`: Whether to use link time optimization (LTO); defaults to `OFF`
- `ICINGA2_PLUGINDIR`: The path for the Monitoring Plugins project binaries; defaults to `/usr/lib/nagios/plugins`
- `ICINGA2_RUNDIR`: The location of the "run" directory; defaults to `CMAKE_INSTALL_LOCALSTATEDIR/run`
- `CMAKE_INSTALL_SYSCONFDIR`: The configuration directory; defaults to `CMAKE_INSTALL_PREFIX/etc`
- `ICINGA2_SYSCONFIGFILE`: Where to put the config file the initscript/systemd pulls it's dirs from;
defaults to `CMAKE_INSTALL_PREFIX/etc/sysconfig/icinga2`
- `CMAKE_INSTALL_LOCALSTATEDIR`: The state directory; defaults to `CMAKE_INSTALL_PREFIX/var`
- `USE_SYSTEMD=ON|OFF`: Use systemd or a classic SysV initscript; defaults to `OFF`
- `INSTALL_SYSTEMD_SERVICE_AND_INITSCRIPT=ON|OFF` Force install both the systemd service definition file
and the SysV initscript in parallel, regardless of how `USE_SYSTEMD` is set.
Only use this for special packaging purposes and if you know what you are doing.
Defaults to `OFF`.
- `ICINGA2_WITH_MYSQL`: Determines whether the MySQL IDO module is built; defaults to `ON`
- `ICINGA2_WITH_PGSQL`: Determines whether the PostgreSQL IDO module is built; defaults to `ON`
- `ICINGA2_WITH_CHECKER`: Determines whether the checker module is built; defaults to `ON`
- `ICINGA2_WITH_COMPAT`: Determines whether the compat module is built; defaults to `ON`
- `ICINGA2_WITH_DEMO`: Determines whether the demo module is built; defaults to `OFF`
- `ICINGA2_WITH_HELLO`: Determines whether the hello module is built; defaults to `OFF`
- `ICINGA2_WITH_LIVESTATUS`: Determines whether the Livestatus module is built; defaults to `ON`
- `ICINGA2_WITH_NOTIFICATION`: Determines whether the notification module is built; defaults to `ON`
- `ICINGA2_WITH_PERFDATA`: Determines whether the perfdata module is built; defaults to `ON`
- `ICINGA2_WITH_STUDIO`: Determines whether the Icinga Studio application is built; defaults to `OFF`
- `ICINGA2_WITH_TESTS`: Determines whether the unit tests are built; defaults to `ON`

CMake determines the Icinga 2 version number using `git describe` if the
source directory is contained in a Git repository. Otherwise the version number
is extracted from the [icinga2.spec](icinga2.spec) file. This behavior can be
overridden by creating a file called `icinga-version.h.force` in the source
directory. Alternatively the `-DICINGA2_GIT_VERSION_INFO=OFF` option for CMake
can be used to disable the usage of `git describe`.

## Build Icinga 2 RPMs

### Build Environment on RHEL, CentOS, Fedora, Amazon Linux

Setup your build environment:

    yum -y install rpmdevtools

### Build Environment on SuSE/SLES

SLES:

    zypper addrepo http://download.opensuse.org/repositories/devel:tools/SLE_12_SP2/devel:tools.repo
    zypper refresh
    zypper install rpmdevtools spectool

OpenSuSE:

    zypper addrepo http://download.opensuse.org/repositories/devel:tools/openSUSE_Leap_42.3/devel:tools.repo
    zypper refresh
    zypper install rpmdevtools spectool

### Package Builds

Prepare the rpmbuild directory tree:

    cd $HOME
    rpmdev-setuptree

Copy the icinga2.spec file to `rpmbuild/SPEC` or fetch the latest version:

    curl https://raw.githubusercontent.com/Icinga/icinga2/master/icinga2.spec -o $HOME/rpmbuild/SPECS/icinga2.spec

Copy the tarball to `rpmbuild/SOURCES` e.g. by using the `spectool` binary
provided with `rpmdevtools`:

    cd $HOME/rpmbuild/SOURCES
    spectool -g ../SPECS/icinga2.spec

    cd $HOME/rpmbuild

Install the build dependencies. Example for CentOS 7:

    yum -y install libedit-devel ncurses-devel gcc-c++ libstdc++-devel openssl-devel \
    cmake flex bison boost-devel systemd mysql-devel postgresql-devel httpd \
    selinux-policy-devel checkpolicy selinux-policy selinux-policy-doc

Note: If you are using Amazon Linux, systemd is not required.

A shorter way is available using the `yum-builddep` command on RHEL based systems:

    yum-builddep SPECS/icinga2.spec

Build the RPM:

    rpmbuild -ba SPECS/icinga2.spec

### Additional Hints

#### SELinux policy module

The following packages are required to build the SELinux policy module:

* checkpolicy
* selinux-policy (selinux-policy on CentOS 6, selinux-policy-devel on CentOS 7)
* selinux-policy-doc

#### RHEL/CentOS 5 and 6

The RedHat Developer Toolset is required for building Icinga 2 beforehand.
This contains a modern version of flex and a C++ compiler which supports
C++11 features.

    cat >/etc/yum.repos.d/devtools-2.repo <<REPO
    [testing-devtools-2-centos-\$releasever]
    name=testing 2 devtools for CentOS $releasever
    baseurl=https://people.centos.org/tru/devtools-2/\$releasever/\$basearch/RPMS
    gpgcheck=0
    REPO

Dependencies to devtools-2 are used in the RPM SPEC, so the correct tools
should be used for building.

As an alternative, you can use newer Boost packages provided on
[packages.icinga.com](https://packages.icinga.com/epel).

    cat >$HOME/.rpmmacros <<MACROS
    %build_icinga_org 1
    MACROS

#### Amazon Linux

If you prefer to build packages offline, a suitable Vagrant box is located
[here](https://atlas.hashicorp.com/mvbcoding/boxes/awslinux/).

#### SLES 11

The Icinga repository provides the required boost package version and must be
added before building.

## Build Icinga 2 Debian/Ubuntu packages

Setup your build environment on Debian/Ubuntu, copy the 'debian' directory from
the Debian packaging Git repository (https://github.com/Icinga/pkg-icinga2-debian)
into your source tree and run the following command:

    $ dpkg-buildpackage -uc -us


## Build Alpine Linux packages

A simple way to setup a build environment is installing Alpine in a chroot.
In this way, you can set up an Alpine build environment in a chroot under a
different Linux distro.
There is a script that simplifies these steps with just two commands, and
can be found [here](https://github.com/alpinelinux/alpine-chroot-install).

Once the build environment is installed, you can setup the system to build
the packages by following [this document](https://wiki.alpinelinux.org/wiki/Creating_an_Alpine_package).

## Build Post Install Tasks

After building Icinga 2 yourself, your package build system should at least run the following post
install requirements:

* enable the `checker`, `notification` and `mainlog` feature by default
* run 'icinga2 api setup' in order to enable the `api` feature and generate SSL certificates for the node

## Run Icinga 2

Icinga 2 comes with a binary that takes care of loading all the relevant
components (e.g. for check execution, notifications, etc.):

    # icinga2 daemon
    [2016-12-08 16:44:24 +0100] information/cli: Icinga application loader (version: v2.5.4-231-gb10a6b7; debug)
    [2016-12-08 16:44:24 +0100] information/cli: Loading configuration file(s).
    [2016-12-08 16:44:25 +0100] information/ConfigItem: Committing config item(s).
    ...

### Init Script

Icinga 2 can be started as a daemon using the provided init script:

    # /etc/init.d/icinga2
    Usage: /etc/init.d/icinga2 {start|stop|restart|reload|checkconfig|status}

### Systemd

If your distribution uses Systemd:

    # systemctl {start|stop|reload|status|enable|disable} icinga2

In case the distribution is running Systemd >227, you'll also
need to package and install the `etc/initsystem/icinga2.service.limits.conf`
file into `/etc/systemd/system/icinga2.service.d`.

### openrc

Or if your distribution uses openrc (like Alpine):

    # rc-service icinga2
	Usage: /etc/init.d/icinga2 {start|stop|restart|reload|checkconfig|status}

Note: the openrc's init.d is not shipped by default.
A working init.d with openrc can be found here: (https://git.alpinelinux.org/cgit/aports/plain/community/icinga2/icinga2.initd). If you have customized some path, edit the file and adjust it according with your setup. 
Those few steps can be followed:

    # wget https://git.alpinelinux.org/cgit/aports/plain/community/icinga2/icinga2.initd
    # mv icinga2.initd /etc/init.d/icinga2
	# chmod +x /etc/init.d/icinga2

Icinga 2 reads a single configuration file which is used to specify all
configuration settings (global settings, hosts, services, etc.). The
configuration format is explained in detail in the [doc/](doc/) directory.

By default `make install` installs example configuration files in
`/usr/local/etc/icinga2` unless you have specified a different prefix or
sysconfdir.
