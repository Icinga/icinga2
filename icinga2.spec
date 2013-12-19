#/******************************************************************************
# * Icinga 2                                                                   *
# * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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
%endif
%if "%{_vendor}" == "suse"
%define opensuse_boost_version 1_49_0
%define sles_boost_version 1_54_0
%define apachename apache2
%define apacheconfdir  %{_sysconfdir}/apache2/conf.d
%define apacheuser wwwrun
%define apachegroup www
%endif

%define icinga_user icinga
%define icinga_group icinga
%define icingacmd_group icingacmd

%define icingaclassicconfdir %{_sysconfdir}/icinga

%define logmsg logger -t %{name}/rpm

Summary: Network monitoring application
Name: icinga2
Version: 0.0.6
Release: %{revision}%{?dist}
License: GPLv2+
Group: Applications/System
Source: %{name}-%{version}.tar.gz
URL: http://www.icinga.org/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

BuildRequires: doxygen
BuildRequires: openssl-devel
BuildRequires: gcc-c++
BuildRequires: libstdc++-devel
BuildRequires: cmake
BuildRequires: flex >= 2.5.35
BuildRequires: bison
BuildRequires: %{apachename}

# redhat
%if "%{_vendor}" == "redhat"
%if 0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5"
# el5 requires EPEL
BuildRequires: boost%{el5_boost_version}-devel
BuildRequires: boost%{el5_boost_version}
Requires: boost%{el5_boost_version}-program-options
Requires: boost%{el5_boost_version}-system
Requires: boost%{el5_boost_version}-test
Requires: boost%{el5_boost_version}-thread
Requires: boost%{el5_boost_version}-regex
%else
BuildRequires: boost-devel >= 1.41
Requires: boost-program-options >= 1.41
Requires: boost-system >= 1.41
Requires: boost-test >= 1.41
Requires: boost-thread >= 1.41
Requires: boost-regex >= 1.41
%endif
%endif
#redhat

# suse
%if "%{_vendor}" == "suse"
# sles
# note: sles_version macro is not set in SLES11 anymore
# note: sles service packs are not under version control
%if 0%{?suse_version} == 1110
BuildRequires: gcc-fortran
BuildRequires: libgfortran43
BuildRequires: boost-license%{sles_boost_version}
BuildRequires: boost-devel >= 1.41
Requires: boost-license%{sles_boost_version}
Requires: libboost_program_options%{sles_boost_version}
Requires: libboost_system%{sles_boost_version}
Requires: libboost_test%{sles_boost_version}
Requires: libboost_thread%{sles_boost_version}
Requires: libboost_regex%{sles_boost_version}
%endif
# opensuse
%if 0%{?suse_version} >= 1210
BuildRequires: boost-devel >= 1.41
Requires: libboost_program_options%{opensuse_boost_version}
Requires: libboost_system%{opensuse_boost_version}
Requires: libboost_test%{opensuse_boost_version}
Requires: libboost_thread%{opensuse_boost_version}
Requires: libboost_regex%{opensuse_boost_version}
%endif
%endif
# suse

Requires: %{name}-common = %{version}

%description
Icinga is a general-purpose network monitoring application.

%package common
Summary:      Common Icinga 2 configuration
Group:        Applications/System
%if "%{_vendor}" == "redhat"
Requires(pre): shadow-utils
Requires(post): shadow-utils
%endif

%description common
Provides common directories, uid and gid among Icinga 2 related
packages.


%package doc
Summary:      Documentation for Icinga 2
Group:        Applications/System
Requires:     %{name} = %{version}-%{release}

%description doc
Documentation for Icinga 2


%package ido-mysql
Summary:      IDO MySQL database backend for Icinga 2
Group:        Applications/System
%if "%{_vendor}" == "suse"
BuildRequires: libmysqlclient-devel
%if 0%{?suse_version} >= 1210
Requires: libmysqlclient18
%else
Requires: libmysqlclient15
%endif
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
%if "%{_vendor}" == "suse"
BuildRequires: postgresql-libs
%endif
%if "%{_vendor}" == "redhat"
# el5 only provides mysql package
%if 0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5"
BuildRequires: postgresql84
%endif
%endif
BuildRequires: postgresql-devel
Requires: postgresql-libs
Requires: %{name} = %{version}-%{release}

%description ido-pgsql
Icinga 2 IDO PostgreSQL database backend. Compatible with Icinga 1.x
IDOUtils schema >= 1.10


%package classicui-config
Summary:      Icinga 2 Classic UI Standalone configuration
Group:        Applications/System
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

cmake $CMAKE_OPTS .

make %{?_smp_mflags}

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
sed -i 's@plugindir = .*@plugindir = "%{_libdir}/nagios/plugins"@' %{buildroot}/%{_sysconfdir}/%{name}/conf.d/macros.conf
%endif

# remove features-enabled symlinks
rm -f %{buildroot}/%{_sysconfdir}/%{name}/features-enabled/*.conf

%clean
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}

%pre common
getent group %{icinga_group} >/dev/null || %{_sbindir}/groupadd -r %{icinga_group}
getent group %{icingacmd_group} >/dev/null || %{_sbindir}/groupadd -r %{icingacmd_group}
getent passwd %{icinga_user} >/dev/null || %{_sbindir}/useradd -c "icinga" -s /sbin/nologin -r -d %{_localstatedir}/spool/%{name} -G %{icingacmd_group} -g %{icinga_group} %{icinga_user}
exit 0

# suse
%if 0%{?suse_version}
%post
%{fillup_and_insserv icinga2}

if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable default features
	%{_sbindir}/icinga2-enable-feature checker
	%{_sbindir}/icinga2-enable-feature notification
	%{_sbindir}/icinga2-enable-feature mainlog
fi

exit 0
%postun
%restart_on_update icinga2
%insserv_cleanup

if [ "$1" = "0" ]; then
	# deinstallation of the package - remove enabled features
	rm -rf %{_sysconfdir}/%{name}/features-enabled
fi

exit 0

%preun
%stop_on_removal icinga2

# rhel
%else

%post
/sbin/chkconfig --add icinga2

if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable default features
	%{_sbindir}/icinga2-enable-feature checker
	%{_sbindir}/icinga2-enable-feature notification
	%{_sbindir}/icinga2-enable-feature mainlog
fi

exit 0

%postun
if [ "$1" -ge  "1" ]; then
	/sbin/service icinga2 condrestart >/dev/null 2>&1 || :
fi

if [ "$1" = "0" ]; then
	# deinstallation of the package - remove enabled features
	rm -rf %{_sysconfdir}/%{name}/features-enabled
fi

exit 0
%preun
if [ "$1" = "0" ]; then
	/sbin/service icinga2 stop > /dev/null 2>&1
	/sbin/chkconfig --del icinga2
fi

%endif
# suse/rhel

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
        %{_sbindir}/icinga2-enable-feature statusdata
        %{_sbindir}/icinga2-enable-feature compatlog
        %{_sbindir}/icinga2-enable-feature command
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
%doc COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog
%attr(755,-,-) %{_sysconfdir}/init.d/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/conf.d
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/features-available
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/features-enabled
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/scripts
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/%{name}.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/conf.d/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/features-available/*.conf
%config(noreplace) %{_sysconfdir}/%{name}/scripts/*
%{_sbindir}/%{name}
%{_bindir}/%{name}-migrate-config
%{_bindir}/%{name}-build-ca
%{_bindir}/%{name}-build-key
%{_sbindir}/%{name}-enable-feature
%{_sbindir}/%{name}-disable-feature
%exclude %{_libdir}/%{name}/libdb_ido_mysql*
%exclude %{_libdir}/%{name}/libdb_ido_pgsql*
%{_libdir}/%{name}
%{_datadir}/%{name}
%exclude %{_datadir}/%{name}/itl
%{_mandir}/man8/%{name}.8.gz

%attr(0755,%{icinga_user},%{icinga_group}) %{_localstatedir}/cache/%{name}
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}/compat
%attr(0755,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}/compat/archives
%attr(0755,%{icinga_user},%{icinga_group}) %ghost %{_localstatedir}/run/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %{_localstatedir}/lib/%{name}

%attr(2755,%{icinga_user},%{icingacmd_group}) %ghost %{_localstatedir}/run/icinga2/cmd

%files common
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}/perfdata
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}/tmp
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_datadir}/%{name}/itl
%{_datadir}/%{name}/itl

%files doc
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README NEWS AUTHORS ChangeLog
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
