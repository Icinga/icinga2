#/******************************************************************************
# * Icinga 2                                                                   *
# * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
# *                                                                            *
# * This program is free software; you can redistribute it and/or              *
# * modify it under the terms of the GNU General Public License                *
# * as published by the Free Software Foundation; either version 2             *
# * of the License, or (at your option) any later version.                     *
# *                                                                            *
# * This program is distributed in the hope that it will be useful,            *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
# * GNU General Public License for more details.                               *
# *                                                                            *
# * You should have received a copy of the GNU General Public License          *
# * along with this program; if not, write to the Free Software Foundation     *
# * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
# ******************************************************************************/

%define revision 1

%if "%{_vendor}" == "redhat"
%define el5_boost_version 141
%define el5_boost_libs %{_libdir}/boost%{el5_boost_version}
%define el5_boost_includes /usr/include/boost%{el5_boost_version}
%define apachename httpd
%define apacheconfdir %{_sysconfdir}/httpd/conf.d
%define apacheuser apache
%define apachegroup apache
%if 0%{?el5}%{?el6}
%define use_systemd 0
%else
# fedora and el>=7
%define use_systemd 1
%endif
%endif

%if "%{_vendor}" == "suse"
%if 0%{?suse_version} >= 1310
%define use_systemd 1
%else
%define use_systemd 0
%endif
%define apachename apache2
%define apacheconfdir  %{_sysconfdir}/apache2/conf.d
%define apacheuser wwwrun
%define apachegroup www
%endif

%define icinga_user icinga
%define icinga_group icinga
%define icingacmd_group icingacmd
%define icingaweb2name icingaweb2
%define icingaweb2version 2.0.0

%define icingaclassicconfdir %{_sysconfdir}/icinga

%define logmsg logger -t %{name}/rpm

Summary: Network monitoring application
Name: icinga2
Version: 2.0.1
Release: %{revision}%{?dist}
License: GPL-2.0+
Group: Applications/System
Source: %{name}-%{version}.tar.gz
URL: http://www.icinga.org/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

Requires: %{name}-bin = %{version}
#Requires: %{name}-ido-mysql = %{version}
#Requires: %{icingaweb2name} >= %{icingaweb2version}

%description
Meta package for Icinga 2 Core, DB IDO and Web.

%package bin
Summary:      Icinga 2 binaries and libraries
Group:        Applications/System

%if "%{_vendor}" == "suse"
PreReq: permissions
%endif
BuildRequires: openssl-devel
BuildRequires: gcc-c++
BuildRequires: libstdc++-devel
BuildRequires: cmake
BuildRequires: flex >= 2.5.35
BuildRequires: bison
BuildRequires: make
BuildRequires: boost-devel >= 1.41

# redhat
%if "%{_vendor}" == "redhat"
%if 0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5"
# el5 requires EPEL
BuildRequires: boost%{el5_boost_version}
Requires: boost%{el5_boost_version}-program-options
Requires: boost%{el5_boost_version}-system
Requires: boost%{el5_boost_version}-test
Requires: boost%{el5_boost_version}-thread
Requires: boost%{el5_boost_version}-regex
%else
Requires: boost-program-options >= 1.41
Requires: boost-system >= 1.41
Requires: boost-test >= 1.41
Requires: boost-thread >= 1.41
Requires: boost-regex >= 1.41
%endif
%endif
#redhat

%if 0%{?use_systemd}
BuildRequires: systemd
Requires: systemd
%endif

Requires: %{name}-common = %{version}

%description bin
Icinga 2 is a general-purpose network monitoring application.
Provides binaries and libraries for Icinga 2 Core.

%package common
Summary:      Common Icinga 2 configuration
Group:        Applications/System
%if "%{_vendor}" == "redhat"
Requires(pre): shadow-utils
Requires(post): shadow-utils
%endif
%if "%{_vendor}" == "suse"
Recommends:   logrotate
%endif

%description common
Provides common directories, uid and gid among Icinga 2 related
packages.


%package doc
Summary:      Documentation for Icinga 2
Group:        Applications/System
Requires:     %{name} = %{version}-%{release}

%description doc
Provides documentation for Icinga 2.


%package ido-mysql
Summary:      IDO MySQL database backend for Icinga 2
Group:        Applications/System
%if "%{_vendor}" == "suse"
BuildRequires: libmysqlclient-devel
%endif
%if "%{_vendor}" == "redhat"
# el5 only provides mysql package
%if 0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5"
BuildRequires: mysql
%else
BuildRequires: mysql-libs
BuildRequires: mysql
%endif
BuildRequires: mysql-devel
Requires: mysql
%endif
Requires: %{name} = %{version}-%{release}

%description ido-mysql
Icinga 2 IDO mysql database backend. Compatible with Icinga 1.x
IDOUtils schema >= 1.10


%package ido-pgsql
Summary:      IDO PostgreSQL database backend for Icinga 2
Group:        Applications/System
BuildRequires: postgresql-devel
Requires: postgresql-libs
Requires: %{name} = %{version}-%{release}

%description ido-pgsql
Icinga 2 IDO PostgreSQL database backend. Compatible with Icinga 1.x
IDOUtils schema >= 1.10


%package classicui-config
Summary:      Icinga 2 Classic UI Standalone configuration
Group:        Applications/System
BuildRequires: %{apachename}
Requires:     %{apachename}
Requires:     %{name} = %{version}-%{release}
Provides:     icinga-classicui-config
Conflicts:    icinga-gui-config

%description classicui-config
Icinga 1.x Classic UI Standalone configuration with locations
for Icinga 2.


%prep
%setup -q -n %{name}-%{version}

%build
CMAKE_OPTS="-DCMAKE_INSTALL_PREFIX=/usr \
         -DCMAKE_INSTALL_SYSCONFDIR=/etc \
		 -DCMAKE_INSTALL_LOCALSTATEDIR=/var \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DICINGA2_USER=%{icinga_user} \
         -DICINGA2_GROUP=%{icinga_group} \
	     -DICINGA2_COMMAND_USER=%{icinga_user} \
	     -DICINGA2_COMMAND_GROUP=%{icingacmd_group}"
%if "%{_vendor}" == "redhat"
%if 0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5"
# Boost_VERSION 1.41.0 vs 101400 - disable build tests
# details in https://dev.icinga.org/issues/5033
CMAKE_OPTS="$CMAKE_OPTS -DBOOST_LIBRARYDIR=/usr/lib/boost141 \
 -DBOOST_INCLUDEDIR=/usr/include/boost141 \
 -DBoost_ADDITIONAL_VERSIONS='1.41;1.41.0' \
 -DBoost_NO_SYSTEM_PATHS=TRUE \
 -DBUILD_TESTING=FALSE \
 -DBoost_NO_BOOST_CMAKE=TRUE"
%endif
%endif

%if 0%{?use_systemd}
CMAKE_OPTS="$CMAKE_OPTS -DUSE_SYSTEMD=ON"
%endif

cmake $CMAKE_OPTS .

make %{?_smp_mflags}

rm -f components/db_ido_*sql/schema/upgrade/.gitignore

%install
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}
make install \
	DESTDIR="%{buildroot}"

# install classicui config
install -D -m 0644 etc/icinga/icinga-classic.htpasswd %{buildroot}%{icingaclassicconfdir}/passwd
install -D -m 0644 etc/icinga/cgi.cfg %{buildroot}%{icingaclassicconfdir}/cgi.cfg
install -D -m 0644 etc/icinga/icinga-classic-apache.conf %{buildroot}%{apacheconfdir}/icinga.conf

# fix plugin path on x64
%if "%{_vendor}" != "suse"
sed -i 's@PluginDir = .*@PluginDir = "%{_libdir}/nagios/plugins"@' %{buildroot}/%{_sysconfdir}/%{name}/constants.conf
%endif

# remove features-enabled symlinks
rm -f %{buildroot}/%{_sysconfdir}/%{name}/features-enabled/*.conf

# enable suse rc links
%if "%{_vendor}" == "suse"
%if 0%{?use_systemd}
  ln -sf /usr/sbin/service %{buildroot}%{_sbindir}/rc%{name}
%else
  ln -sf ../../%{_initrddir}/%{name} "%{buildroot}%{_sbindir}/rc%{name}"
%endif
mkdir -p "%{buildroot}%{_localstatedir}/adm/fillup-templates/"
mv "%{buildroot}%{_sysconfdir}/sysconfig/%{name}" "%{buildroot}%{_localstatedir}/adm/fillup-templates/sysconfig.%{name}"
%endif


%clean
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}

%pre common
getent group %{icinga_group} >/dev/null || %{_sbindir}/groupadd -r %{icinga_group}
getent group %{icingacmd_group} >/dev/null || %{_sbindir}/groupadd -r %{icingacmd_group}
getent passwd %{icinga_user} >/dev/null || %{_sbindir}/useradd -c "icinga" -s /sbin/nologin -r -d %{_localstatedir}/spool/%{name} -G %{icingacmd_group} -g %{icinga_group} %{icinga_user}
exit 0

%if "%{_vendor}" == "suse"
%verifyscript bin
%verify_permissions -e /var/run/icinga2/cmd
%endif


%if "%{_vendor}" == "suse"
%if 0%{?use_systemd}
%pre bin
  %service_add_pre %{name}.service
%endif
%endif


# all restart/feature actions belong to icinga2-bin
%post bin
# suse
%if 0%{?suse_version}
%if 0%{?suse_version} >= 1310
%set_permissions /var/run/icinga2/cmd
%endif
%if 0%{?use_systemd}
%fillup_only  %{name}
%service_add_post %{name}.service
%else
%fillup_and_insserv %{name}
%endif

# initial installation, enable default features
%{_sbindir}/icinga2-enable-feature checker notification mainlog

exit 0

%else
# rhel

%if 0%{?use_systemd}
%systemd_post %{name}.service
%else
/sbin/chkconfig --add %{name}
%endif

if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable default features
	%{_sbindir}/icinga2-enable-feature checker notification mainlog
fi

exit 0

%endif
# suse/rhel

%postun bin
# suse
%if 0%{?suse_version}
%if 0%{?using_systemd}
  %service_del_postun %{name}.service
%else
  %restart_on_update %{name}
  %insserv_cleanup
%endif

%else
# rhel

%if 0%{?use_systemd}
%systemd_postun_with_restart %{name}.service
%else
if [ "$1" -ge  "1" ]; then
	/sbin/service %{name} condrestart >/dev/null 2>&1 || :
fi
%endif

%endif
# suse / rhel

if [ "$1" = "0" ]; then
	# deinstallation of the package - remove enabled features
	rm -rf %{_sysconfdir}/%{name}/features-enabled
fi

exit 0

%preun bin
# suse
%if 0%{?suse_version}

%if 0%{?use_systemd}
  %service_del_preun %{name}.service
%else
  %stop_on_removal %{name}
%endif

exit 0

%else
# rhel

%if 0%{?use_systemd}
%systemd_preun %{name}.service
%else
if [ "$1" = "0" ]; then
	/sbin/service %{name} stop > /dev/null 2>&1 || :
	/sbin/chkconfig --del %{name} || :
fi
%endif

exit 0

%endif
# suse / rhel

%post ido-mysql
if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable ido-mysql feature
	%{_sbindir}/icinga2-enable-feature ido-mysql
fi

exit 0

%postun ido-mysql
if [ "$1" = "0" ]; then
	# deinstallation of the package - remove feature
	test -x %{_sbindir}/icinga2-disable-feature && %{_sbindir}/icinga2-disable-feature ido-mysql
fi

exit 0

%post ido-pgsql
if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable ido-pgsql feature
	%{_sbindir}/icinga2-enable-feature ido-pgsql
fi

exit 0

%postun ido-pgsql
if [ "$1" = "0" ]; then
	# deinstallation of the package - remove feature
	test -x %{_sbindir}/icinga2-disable-feature && %{_sbindir}/icinga2-disable-feature ido-pgsql
fi

exit 0

%post classicui-config
if [ ${1:-0} -eq 1 ]
then
        # initial installation, enable features
        %{_sbindir}/icinga2-enable-feature statusdata compatlog command
fi

exit 0

%postun classicui-config
if [ "$1" = "0" ]; then
        # deinstallation of the package - remove feature
        test -x %{_sbindir}/icinga2-disable-feature && %{_sbindir}/icinga2-disable-feature statusdata
        test -x %{_sbindir}/icinga2-disable-feature && %{_sbindir}/icinga2-disable-feature compatlog
        test -x %{_sbindir}/icinga2-disable-feature && %{_sbindir}/icinga2-disable-feature command
fi

exit 0

%files
%defattr(-,root,root,-)
%doc COPYING

%files bin
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog
%if 0%{?use_systemd}
%attr(644,root,root) %{_unitdir}/%{name}.service
%else
%attr(755,root,root) %{_sysconfdir}/init.d/%{name}
%endif
%if "%{_vendor}" == "suse"
%{_sbindir}/rc%{name}
%{_localstatedir}/adm/fillup-templates/sysconfig.%{name}
%else
%config(noreplace) %{_sysconfdir}/sysconfig/%{name}
%endif
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/conf.d
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/conf.d/hosts
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/conf.d/hosts/localhost
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/features-available
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/features-enabled
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/scripts
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/zones.d
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/%{name}.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/constants.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/zones.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/conf.d/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/conf.d/hosts/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/conf.d/hosts/localhost/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/features-available/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/zones.d/*
%config(noreplace) %{_sysconfdir}/%{name}/scripts/*
%{_sbindir}/%{name}
%{_bindir}/%{name}-build-ca
%{_bindir}/%{name}-build-key
%{_bindir}/%{name}-sign-key
%{_sbindir}/%{name}-enable-feature
%{_sbindir}/%{name}-disable-feature
%{_sbindir}/%{name}-prepare-dirs
%exclude %{_libdir}/%{name}/libdb_ido_mysql*
%exclude %{_libdir}/%{name}/libdb_ido_pgsql*
%{_libdir}/%{name}
%{_datadir}/%{name}
%exclude %{_datadir}/%{name}/include
%{_mandir}/man8/%{name}.8.gz
%{_mandir}/man8/%{name}-enable-feature.8.gz
%{_mandir}/man8/%{name}-disable-feature.8.gz
%{_mandir}/man8/%{name}-build-ca.8.gz
%{_mandir}/man8/%{name}-build-key.8.gz
%{_mandir}/man8/%{name}-sign-key.8.gz
%{_mandir}/man8/%{name}-prepare-dirs.8.gz

%attr(0755,%{icinga_user},%{icinga_group}) %{_localstatedir}/cache/%{name}
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}/compat
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}/compat/archives
%attr(0755,%{icinga_user},%{icinga_group}) %ghost %{_localstatedir}/run/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %{_localstatedir}/lib/%{name}

%attr(2755,%{icinga_user},%{icingacmd_group}) %ghost %{_localstatedir}/run/icinga2/cmd

%files common
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog tools/syntax
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}
%config(noreplace) %{_sysconfdir}/logrotate.d/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}/perfdata
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}/tmp
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_datadir}/%{name}/include
%{_datadir}/%{name}/include

%files doc
%defattr(-,root,root,-)
%{_datadir}/doc/%{name}
%docdir %{_datadir}/doc/%{name}

%files ido-mysql
%defattr(-,root,root,-)
%doc components/db_ido_mysql/schema COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog
%{_libdir}/%{name}/libdb_ido_mysql*

%files ido-pgsql
%defattr(-,root,root,-)
%doc components/db_ido_pgsql/schema COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog
%{_libdir}/%{name}/libdb_ido_pgsql*

%files classicui-config
%defattr(-,root,root,-)
%attr(0751,%{icinga_user},%{icinga_group}) %dir %{icingaclassicconfdir}
%config(noreplace) %{icingaclassicconfdir}/cgi.cfg
%config(noreplace) %{apacheconfdir}/icinga.conf
%config(noreplace) %attr(0640,root,%{apachegroup}) %{icingaclassicconfdir}/passwd


%changelog
