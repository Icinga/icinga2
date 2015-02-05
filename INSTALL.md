# Installing Icinga 2

The recommended way of installing Icinga 2 is to use packages. The Icinga
project provides both release and development packages for a number
of operating systems.

Please check the documentation in the [doc/](doc/) directory for a current list
of available packages and detailed installation instructions.

There are a number of known caveats when installing from source such as
incorrect directory and file permissions. So even if you're planning to
not use the official packages it is advisable to build your own Debian
or RPM packages.

## Build Requirements

The following requirements need to be fulfilled in order to build the
application using a dist tarball (package names for RHEL and Debian in
parentheses):

* cmake
* GNU make (make)
* C++ compiler (gcc-c++ on RHEL, build-essential on Debian)
* OpenSSL library and header files (openssl-devel on RHEL, libssl-dev on Debian)
* Boost library and header files (boost-devel on RHEL, libboost-all-dev on Debian)
* GNU bison (bison)
* GNU flex (flex) >= 2.5.35
* recommended: libexecinfo on FreeBSD (automatically used when Icinga 2 is
               installed via port or package)
* optional: MySQL (mysql-devel on RHEL, libmysqlclient-dev on Debian); set CMake
             variable `ICINGA2_WITH_MYSQL` to disable this module
* optional: PostgreSQL (postgresql-devel on RHEL, libpq-dev on Debian); set CMake
            variable `ICINGA2_WITH_PGSQL` to disable this module
* optional: YAJL (yajl-devel on RHEL, libyajl-dev on Debian)

Note: RHEL5 ships an ancient flex version. Updated packages are available for
example from the repoforge buildtools repository.

* x86: http://mirror.hs-esslingen.de/repoforge/redhat/el5/en/i386/buildtools/
* x86\_64: http://mirror.hs-esslingen.de/repoforge/redhat/el5/en/x86_64/buildtools/

### User Requirements

By default Icinga will run as user 'icinga' and group 'icinga'. Additionally the
external command pipe and livestatus features require a dedicated command group
'icingacmd'. You can choose your own user/group names and pass them to CMake
using the `ICINGA2_USER`, `ICINGA2_GROUP` and `ICINGA2_COMMAND_GROUP` variables.

    # groupadd icinga
    # groupadd icingacmd
    # useradd -c "icinga" -s /sbin/nologin -G icingacmd -g icinga icinga

Add the web server user to the icingacmd group in order to grant it write
permissions to the external command pipe and livestatus socket:

    # usermod -a -G icingacmd www-data

Make sure to replace "www-data" with the name of the user your web server
is running as.


## Building Release Tarballs

In order to build a release tarball you should first check out the Git repository
in a new directory. If you're using an existing check-out you should make sure
that there are no local modifications:

    $ git status

### Release Checklist

Here's a short check-list for releases:

Update the [.mailmap](.mailmap) and [AUTHORS](AUTHORS) files

    $ git log --use-mailmap | grep ^Author: | cut -f2- -d' ' | sort | uniq > AUTHORS

Bump the version in icinga2.spec.
Update the [ChangeLog](ChangeLog), [doc/1-about.md](doc/1-about.md) and [INSTALL.md](INSTALL.md)
files.
Commit these changes to the "master" branch.

    $ git commit -v -a -m "Release version <VERSION>"

Create a signed tag (tags/v<VERSION>).

GB:

    $ git tag -u EE8E0720 -m "Version <VERSION>" v<VERSION>
MF:

    $ git tag -u D14A1F16 -m "Version <VERSION>" v<VERSION>

Push the tag.

    $ git push --tags

Merge the "master" branch into the "support/2.2" branch (using --ff-only).

    $ git checkout support/2.2
    $ git merge --ff-only master
    $ git push origin support/2.2

Note: CMake determines the Icinga 2 version number using `git describe` if the
source directory is contained in a Git repository. Otherwise the version number
is extracted from the [icinga2.spec](icinga2.spec) file. This behavior can be overridden by
creating a file called `icinga-version.h.force` in the source directory.
Alternatively the `-DICINGA2_GIT_VERSION_INFO=ON option` for CMake can be used to
disable the usage of `git describe`.

### Build Tarball

Use `git archive` to build the release tarball:

    $ VERSION=2.2.4
    $ git archive --format=tar --prefix=icinga2-$VERSION/ tags/v$VERSION | gzip >icinga2-$VERSION.tar.gz

Finally you should verify that the tarball only contains the files it should contain:

    $ VERSION=2.2.4
    $ tar ztf icinga2-$VERSION.tar.gz | less


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
- `ICINGA2_UNITY_BUILD`: Whether to perform a unity build; defaults to `OFF`
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

### Building Icinga 2 RPMs

Setup your build environment on RHEL/SUSE and copy the generated tarball from your git
repository to `rpmbuild/SOURCES`.
Copy the icinga2.spec file to `rpmbuild/SPEC` and then invoke

    $ rpmbuild -ba SPEC/icinga2.spec

### Building Icinga 2 Debs

Setup your build environment on Debian/Ubuntu and run the following command straight from
the git repository checkout:

    $ ./debian/updateversion && dpkg-buildpackage -uc -us

## Running Icinga 2

Icinga 2 comes with a single binary that takes care of loading all the relevant
components (e.g. for check execution, notifications, etc.):

    # /usr/sbin/icinga2 daemon
    [2014-12-18 10:20:49 +0100] information/cli: Icinga application loader (version: v2.2.2)
    [2014-12-18 10:20:49 +0100] information/cli: Loading application type: icinga/IcingaApplication
    [2014-12-18 10:20:49 +0100] information/Utility: Loading library 'libicinga.so'
    [2014-12-18 10:20:49 +0100] information/ConfigCompiler: Compiling config file: /home/gbeutner/i2/etc/icinga2/icinga2.conf
    [2014-12-18 10:20:49 +0100] information/ConfigCompiler: Compiling config file: /home/gbeutner/i2/etc/icinga2/constants.conf
    ...

Icinga 2 can be started as daemon using the provided init script:

    # /etc/init.d/icinga2
    Usage: /etc/init.d/icinga2 {start|stop|restart|reload|checkconfig|status}

Or if your distribution uses systemd:

    # systemctl {start|stop|reload|status|enable|disable} icinga2.service

Icinga 2 reads a single configuration file which is used to specify all
configuration settings (global settings, hosts, services, etc.). The
configuration format is explained in detail in the [doc/](doc/) directory.

By default `make install` installs example configuration files in
`/usr/local/etc/icinga2` unless you have specified a different prefix or
sysconfdir.
