#/******************************************************************************
# * Icinga 2                                                                   *
# * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

# make sure that _rundir is working on older systems
%if ! %{defined _rundir}
%define _rundir %{_localstatedir}/run
%endif

%define _libexecdir %{_prefix}/lib/
%define plugindir %{_libdir}/nagios/plugins

%if "%{_vendor}" == "redhat"
%define apachename httpd
%define apacheconfdir %{_sysconfdir}/httpd/conf.d
%define apacheuser apache
%define apachegroup apache
%if 0%{?el5}%{?el6}%{?amzn}
%define use_systemd 0
%if %(uname -m) != "x86_64"
%define march_flag -march=i686
%endif
%else
# fedora and el>=7
%define use_systemd 1
%endif
%endif

%if "%{_vendor}" == "suse"
%define plugindir %{_prefix}/lib/nagios/plugins
%define apachename apache2
%define apacheconfdir  %{_sysconfdir}/apache2/conf.d
%define apacheuser wwwrun
%define apachegroup www
%if 0%{?suse_version} >= 1310
%define use_systemd 1
%else
%define use_systemd 0
%endif
%endif

%define icinga_user icinga
%define icinga_group icinga
%define icingacmd_group icingacmd
%define icingaweb2name icingaweb2
%define icingaweb2version 2.0.0

# DEPRECATED
%define icingaclassicconfdir %{_sysconfdir}/icinga

%define logmsg logger -t %{name}/rpm

Summary: Network monitoring application
Name: icinga2
Version: 2.6.3
Release: %{revision}%{?dist}
License: GPL-2.0+
Group: Applications/System
Source: https://github.com/Icinga/%{name}/archive/v%{version}.tar.gz
URL: https://www.icinga.com/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Requires: %{name}-bin = %{version}-%{release}

%description
Meta package for Icinga 2 Core, DB IDO and Web.

%package bin
Summary:      Icinga 2 binaries and libraries
Group:        Applications/System

%if "%{_vendor}" == "suse"
PreReq:        permissions
Provides:      monitoring_daemon
Recommends:    monitoring-plugins
%if 0%{?suse_version} >= 1310
BuildRequires: libyajl-devel
%endif
%endif
BuildRequires: libedit-devel
BuildRequires: ncurses-devel
%if "%{_vendor}" == "suse" && 0%{?suse_version} < 1210
BuildRequires: gcc48-c++
BuildRequires: libstdc++48-devel
BuildRequires: libopenssl1-devel
%else
%if "%{_vendor}" == "redhat" && (0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
# Requires devtoolset-2 scl
BuildRequires: devtoolset-2-gcc-c++
BuildRequires: devtoolset-2-libstdc++-devel
%define scl_enable scl enable devtoolset-2 --
%else
BuildRequires: gcc-c++
BuildRequires: libstdc++-devel
%endif
BuildRequires: openssl-devel
%endif
BuildRequires: cmake
BuildRequires: flex >= 2.5.35
BuildRequires: bison
BuildRequires: make
%if 0%{?fedora}
BuildRequires: wxGTK3-devel
%endif

%if 0%{?build_icinga_org} && "%{_vendor}" == "redhat" && (0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
# el5 and el6 require packages.icinga.com
BuildRequires: boost153-devel
%else
%if 0%{?build_icinga_org} && "%{_vendor}" == "suse" && 0%{?suse_version} < 1310
# sles 11 sp3 requires packages.icinga.com
BuildRequires: boost153-devel
%else
%if (0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
# Requires EPEL repository
BuildRequires: boost148-devel >= 1.48
%else
BuildRequires: boost-devel >= 1.48
%endif
%endif
%endif

%if 0%{?use_systemd}
BuildRequires: systemd
Requires: systemd
%endif

Requires: %{name}-libs = %{version}-%{release}

%description bin
Icinga 2 is a general-purpose network monitoring application.
Provides binaries for Icinga 2 Core.

%package common
Summary:      Common Icinga 2 configuration
Group:        Applications/System
%{?amzn:Requires(pre):          shadow-utils}
%{?fedora:Requires(pre):        shadow-utils}
%{?rhel:Requires(pre):          shadow-utils}
%{?suse_version:Requires(pre):  pwdutils}
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


%package libs
Summary:      Libraries for Icinga 2
Group:        Applications/System
Requires:     %{name}-common = %{version}-%{release}

%description libs
Provides internal libraries for the daemon or studio.


%package ido-mysql
Summary:      IDO MySQL database backend for Icinga 2
Group:        Applications/System
%if "%{_vendor}" == "suse"
BuildRequires: libmysqlclient-devel
%if 0%{?suse_version} >= 1310
BuildRequires: mysql-devel
%endif

%else
BuildRequires: mysql-devel
%endif #suse

Requires: %{name} = %{version}-%{release}

%description ido-mysql
Icinga 2 IDO mysql database backend. Compatible with Icinga 1.x
IDOUtils schema >= 1.12


%package ido-pgsql
Summary:      IDO PostgreSQL database backend for Icinga 2
Group:        Applications/System
%if "%{_vendor}" == "suse" && 0%{?suse_version} < 1210
BuildRequires: postgresql-devel >= 8.4
%else
BuildRequires: postgresql-devel
%endif
Requires: %{name} = %{version}-%{release}

%description ido-pgsql
Icinga 2 IDO PostgreSQL database backend. Compatible with Icinga 1.x
IDOUtils schema >= 1.12

# DEPRECATED, disable builds on Amazon
%if !(0%{?amzn})

# DEPRECATED
%package classicui-config
Summary:      Icinga 2 Classic UI Standalone configuration
Group:        Applications/System
BuildRequires: %{apachename}
Requires:     %{apachename}
Requires:     %{name} = %{version}-%{release}
%if "%{_vendor}" == "suse"
Recommends:   icinga-www
%endif
Provides:     icinga-classicui-config
Conflicts:    icinga-gui-config

# DEPRECATED
%description classicui-config
Icinga 1.x Classic UI Standalone configuration with locations
for Icinga 2.

# DEPRECATED, disable builds on Amazon
%endif

%if "%{_vendor}" == "redhat" && !(0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
%global selinux_variants mls targeted
%{!?_selinux_policy_version: %global _selinux_policy_version %(sed -e 's,.*selinux-policy-\\([^/]*\\)/.*,\\1,' /usr/share/selinux/devel/policyhelp 2>/dev/null)}
%global modulename %{name}

%package selinux
Summary:        SELinux policy module supporting icinga2
Group:          System Environment/Base
BuildRequires:  checkpolicy, selinux-policy-devel, /usr/share/selinux/devel/policyhelp, hardlink
%if "%{_selinux_policy_version}" != ""
Requires:       selinux-policy >= %{_selinux_policy_version}
%endif
Requires:       %{name} = %{version}-%{release}
Requires(post):   policycoreutils-python
Requires(postun): policycoreutils-python

%description selinux
SELinux policy module supporting icinga2
%endif


%if 0%{?fedora}
%package studio
Summary:      Studio for Icinga 2
Group:        Applications/System
Requires:     %{name}-libs = %{version}-%{release}
Requires:     wxGTK3

%description studio
Provides a GUI for the Icinga 2 API.
%endif


%package -n vim-icinga2
Summary:      Vim syntax highlighting for icinga2
Group:        Applications/System
%if "%{_vendor}" == "suse"
Requires:     vim-data
%else
Requires:     vim-filesystem
%endif

%description -n vim-icinga2
Vim syntax highlighting for icinga2


%package -n nano-icinga2
Summary:      Nano syntax highlighting for icinga2
Group:        Applications/System
Requires:     nano

%description -n nano-icinga2
Nano syntax highlighting for icinga2

%prep
%setup -q -n %{name}-%{version}

%build
CMAKE_OPTS="-DCMAKE_INSTALL_PREFIX=/usr \
         -DCMAKE_INSTALL_SYSCONFDIR=/etc \
         -DCMAKE_INSTALL_LOCALSTATEDIR=/var \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DICINGA2_LTO_BUILD=ON \
         -DCMAKE_VERBOSE_MAKEFILE=ON \
         -DBoost_NO_BOOST_CMAKE=ON \
         -DICINGA2_PLUGINDIR=%{plugindir} \
         -DICINGA2_RUNDIR=%{_rundir} \
         -DICINGA2_USER=%{icinga_user} \
         -DICINGA2_GROUP=%{icinga_group} \
         -DICINGA2_COMMAND_GROUP=%{icingacmd_group}"
%if 0%{?fedora}
CMAKE_OPTS="$CMAKE_OPTS -DICINGA2_WITH_STUDIO=true"
%endif
%if "%{_vendor}" == "redhat"
%if 0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6"
%if 0%{?build_icinga_org}
# Boost_VERSION 1.41.0 vs 101400 - disable build tests
# details in https://dev.icinga.com/issues/5033
CMAKE_OPTS="$CMAKE_OPTS -DBOOST_LIBRARYDIR=%{_libdir}/boost153 \
 -DBOOST_INCLUDEDIR=/usr/include/boost153 \
 -DBoost_ADDITIONAL_VERSIONS='1.53;1.53.0'"
%else
CMAKE_OPTS="$CMAKE_OPTS -DBOOST_LIBRARYDIR=%{_libdir}/boost148 \
 -DBOOST_INCLUDEDIR=/usr/include/boost148 \
 -DBoost_ADDITIONAL_VERSIONS='1.48;1.48.0'"
%endif
CMAKE_OPTS="$CMAKE_OPTS \
 -DBoost_NO_SYSTEM_PATHS=TRUE \
 -DBUILD_TESTING=FALSE \
 -DBoost_NO_BOOST_CMAKE=TRUE"
%endif
%endif

%if "%{_vendor}" == "suse" && 0%{?suse_version} < 1310
CMAKE_OPTS="$CMAKE_OPTS -DBOOST_LIBRARYDIR=%{_libdir}/boost153 \
 -DBOOST_INCLUDEDIR=/usr/include/boost153 \
 -DBoost_ADDITIONAL_VERSIONS='1.53;1.53.0' \
 -DBoost_NO_SYSTEM_PATHS=TRUE \
 -DBUILD_TESTING=FALSE \
 -DBoost_NO_BOOST_CMAKE=TRUE"
%endif

%if 0%{?use_systemd}
CMAKE_OPTS="$CMAKE_OPTS -DUSE_SYSTEMD=ON"
%endif

%if "%{_vendor}" == "suse" && 0%{?suse_version} < 1210
# from package gcc48-c++
export CC=gcc-4.8
export CXX=g++-4.8
%endif

%{?scl_enable} cmake $CMAKE_OPTS -DCMAKE_C_FLAGS:STRING="%{optflags} %{?march_flag}" -DCMAKE_CXX_FLAGS:STRING="%{optflags} %{?march_flag}" .

make %{?_smp_mflags}

%if "%{_vendor}" == "redhat" && !(0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
cd tools/selinux
for selinuxvariant in %{selinux_variants}
do
  make NAME=${selinuxvariant} -f /usr/share/selinux/devel/Makefile
  mv %{modulename}.pp %{modulename}.pp.${selinuxvariant}
  make NAME=${selinuxvariant} -f /usr/share/selinux/devel/Makefile clean
done
cd -
%endif

%install
make install \
	DESTDIR="%{buildroot}"

# DEPRECATED, disable builds on Amazon
%if !(0%{?amzn})

# install classicui config
install -D -m 0644 etc/icinga/icinga-classic.htpasswd %{buildroot}%{icingaclassicconfdir}/passwd
install -D -m 0644 etc/icinga/cgi.cfg %{buildroot}%{icingaclassicconfdir}/cgi.cfg
install -D -m 0644 etc/icinga/icinga-classic-apache.conf %{buildroot}%{apacheconfdir}/icinga.conf

# DEPRECATED, disable builds on Amazon
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

%if "%{_vendor}" == "redhat" && !(0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
cd tools/selinux
for selinuxvariant in %{selinux_variants}
do
  install -d %{buildroot}%{_datadir}/selinux/${selinuxvariant}
  install -p -m 644 %{modulename}.pp.${selinuxvariant} \
    %{buildroot}%{_datadir}/selinux/${selinuxvariant}/%{modulename}.pp
done
cd -

# TODO: Fix build problems on Icinga, see https://github.com/Icinga/puppet-icinga_build/issues/11
#/usr/sbin/hardlink -cv %{buildroot}%{_datadir}/selinux
%endif

%if 0%{?fedora}
mkdir -p "%{buildroot}%{_datadir}/icinga2-studio"
install -p -m 644 icinga-studio/icinga.ico %{buildroot}%{_datadir}/icinga2-studio

mkdir -p "%{buildroot}%{_datadir}/applications"
echo "[Desktop Entry]
Name=Icinga 2 Studio
Comment=API viewer for Icinga 2
TryExec=icinga-studio
Exec=icinga-studio
Icon=/usr/share/icinga2-studio/icinga.ico
StartupNotify=true
Terminal=false
Type=Application
Categories=GTK;Utility;
Keywords=Monitoring;" > %{buildroot}%{_datadir}/applications/icinga2-studio.desktop
%endif

%if "%{_vendor}" == "suse"
%if 0%{?suse_version} >= 1310
install -D -m 0644 tools/syntax/vim/syntax/%{name}.vim %{buildroot}%{_datadir}/vim/vim74/syntax/%{name}.vim
install -D -m 0644 tools/syntax/vim/ftdetect/%{name}.vim %{buildroot}%{_datadir}/vim/vim74/ftdetect/%{name}.vim
%else
install -D -m 0644 tools/syntax/vim/syntax/%{name}.vim %{buildroot}%{_datadir}/vim/vim72/syntax/%{name}.vim
install -D -m 0644 tools/syntax/vim/ftdetect/%{name}.vim %{buildroot}%{_datadir}/vim/vim72/ftdetect/%{name}.vim
%endif
%else
install -D -m 0644 tools/syntax/vim/syntax/%{name}.vim %{buildroot}%{_datadir}/vim/vimfiles/syntax/%{name}.vim
install -D -m 0644 tools/syntax/vim/ftdetect/%{name}.vim %{buildroot}%{_datadir}/vim/vimfiles/ftdetect/%{name}.vim
%endif

install -D -m 0644 tools/syntax/nano/%{name}.nanorc %{buildroot}%{_datadir}/nano/%{name}.nanorc

%clean
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}

%pre common
getent group %{icinga_group} >/dev/null || %{_sbindir}/groupadd -r %{icinga_group}
getent group %{icingacmd_group} >/dev/null || %{_sbindir}/groupadd -r %{icingacmd_group}
getent passwd %{icinga_user} >/dev/null || %{_sbindir}/useradd -c "icinga" -s /sbin/nologin -r -d %{_localstatedir}/spool/%{name} -G %{icingacmd_group} -g %{icinga_group} %{icinga_user}

%if "%{_vendor}" == "suse"
%if 0%{?use_systemd}
  %service_add_pre %{name}.service
%endif
%endif

%if "%{_vendor}" == "suse"
%verifyscript bin
%verify_permissions -e %{_rundir}/%{name}/cmd
%endif

%post bin

# suse
%if "%{_vendor}" == "suse"

%if 0%{?suse_version} >= 1310
%set_permissions %{_rundir}/%{name}/cmd
%endif

%endif #suse/rhel

%post common
# suse
%if "%{_vendor}" == "suse"
%if 0%{?use_systemd}
%fillup_only  %{name}
%service_add_post %{name}.service
%else
%fillup_and_insserv %{name}
%endif

if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable default features
	for feature in checker notification mainlog; do
		ln -sf ../features-available/${feature}.conf %{_sysconfdir}/%{name}/features-enabled/${feature}.conf
	done
fi

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
	for feature in checker notification mainlog; do
		ln -sf ../features-available/${feature}.conf %{_sysconfdir}/%{name}/features-enabled/${feature}.conf
	done
fi

exit 0

%endif
# suse/rhel

%postun common
# suse
%if "%{_vendor}" == "suse"
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

%preun common
# suse
%if "%{_vendor}" == "suse"

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
	ln -sf ../features-available/ido-mysql.conf %{_sysconfdir}/%{name}/features-enabled/ido-mysql.conf
fi

exit 0

%postun ido-mysql
if [ "$1" = "0" ]; then
	# deinstallation of the package - remove feature
	rm -f %{_sysconfdir}/%{name}/features-enabled/ido-mysql.conf
fi

exit 0

%post ido-pgsql
if [ ${1:-0} -eq 1 ]
then
	# initial installation, enable ido-pgsql feature
	ln -sf ../features-available/ido-pgsql.conf %{_sysconfdir}/%{name}/features-enabled/ido-pgsql.conf
fi

exit 0

%postun ido-pgsql
if [ "$1" = "0" ]; then
	# deinstallation of the package - remove feature
	rm -f %{_sysconfdir}/%{name}/features-enabled/ido-pgsql.conf
fi

exit 0

# DEPRECATED, disable builds on Amazon
%if !(0%{?amzn})

%post classicui-config
if [ ${1:-0} -eq 1 ]
then
        # initial installation, enable features
	for feature in statusdata compatlog command; do
		ln -sf ../features-available/${feature}.conf %{_sysconfdir}/%{name}/features-enabled/${feature}.conf
	done
fi

%logmsg "The icinga2-classicui-config package has been deprecated and will be removed in future releases."

exit 0

# DEPRECATED
%postun classicui-config
if [ "$1" = "0" ]; then
        # deinstallation of the package - remove feature
	for feature in statusdata compatlog command; do
		rm -f %{_sysconfdir}/%{name}/features-enabled/${feature}.conf
	done
fi

exit 0

# DEPRECATED, disable builds on Amazon
%endif

%if "%{_vendor}" == "redhat" && !(0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
%post selinux
for selinuxvariant in %{selinux_variants}
do
  /usr/sbin/semodule -s ${selinuxvariant} -i \
    %{_datadir}/selinux/${selinuxvariant}/%{modulename}.pp &> /dev/null || :
done
/sbin/fixfiles -R icinga2-bin restore &> /dev/null || :
/sbin/fixfiles -R icinga2-common restore &> /dev/null || :
/sbin/semanage port -a -t icinga2_port_t -p tcp 5665 &> /dev/null || :

%postun selinux
if [ $1 -eq 0 ] ; then
  /sbin/semanage port -d -t icinga2_port_t -p tcp 5665 &> /dev/null || :
  for selinuxvariant in %{selinux_variants}
  do
     /usr/sbin/semodule -s ${selinuxvariant} -r %{modulename} &> /dev/null || :
  done
  /sbin/fixfiles -R icinga2-bin restore &> /dev/null || :
  /sbin/fixfiles -R icinga2-common restore &> /dev/null || :
fi
%endif


%files
%defattr(-,root,root,-)
%doc COPYING

%files bin
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README.md NEWS AUTHORS CHANGELOG.md
%{_sbindir}/%{name}
%dir %{_libdir}/%{name}/sbin
%{_libdir}/%{name}/sbin/%{name}
%{plugindir}/check_nscp_api
%{_datadir}/%{name}
%exclude %{_datadir}/%{name}/include
%{_mandir}/man8/%{name}.8.gz

%attr(0750,%{icinga_user},%{icingacmd_group}) %{_localstatedir}/cache/%{name}
%attr(0750,%{icinga_user},%{icingacmd_group}) %dir %{_localstatedir}/log/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/log/%{name}/crash
%attr(0750,%{icinga_user},%{icingacmd_group}) %dir %{_localstatedir}/log/%{name}/compat
%attr(0750,%{icinga_user},%{icingacmd_group}) %dir %{_localstatedir}/log/%{name}/compat/archives
%attr(0750,%{icinga_user},%{icinga_group}) %{_localstatedir}/lib/%{name}

%attr(0750,%{icinga_user},%{icingacmd_group}) %ghost %{_rundir}/%{name}
%attr(2750,%{icinga_user},%{icingacmd_group}) %ghost %{_rundir}/%{name}/cmd

%files libs
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README.md NEWS AUTHORS CHANGELOG.md
%exclude %{_libdir}/%{name}/libdb_ido_mysql*
%exclude %{_libdir}/%{name}/libdb_ido_pgsql*
%dir %{_libdir}/%{name}
%{_libdir}/%{name}/*.so*

%files common
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README.md NEWS AUTHORS CHANGELOG.md tools/syntax
%attr(0750,%{icinga_user},%{icingacmd_group}) %dir %{_localstatedir}/log/%{name}
%config(noreplace) %{_sysconfdir}/logrotate.d/%{name}
%{_sysconfdir}/bash_completion.d/%{name}
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
%attr(0750,root,%{icinga_group}) %dir %{_sysconfdir}/%{name}
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/conf.d
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/features-available
%exclude %{_sysconfdir}/%{name}/features-available/ido-*.conf
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/features-enabled
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/repository.d
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/scripts
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/repository.d
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_sysconfdir}/%{name}/zones.d
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/%{name}.conf
%config(noreplace) %attr(0640,root,%{icinga_group}) %{_sysconfdir}/%{name}/init.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/constants.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/zones.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/conf.d/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/features-available/*.conf
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/repository.d/*
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/zones.d/*
%config(noreplace) %{_sysconfdir}/%{name}/scripts/*
%dir %{_libexecdir}/%{name}
%{_libexecdir}/%{name}/prepare-dirs
%{_libexecdir}/%{name}/safe-reload
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}
%attr(0770,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}/perfdata
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_localstatedir}/spool/%{name}/tmp
%attr(0750,%{icinga_user},%{icinga_group}) %dir %{_datadir}/%{name}/include
%{_datadir}/%{name}/include

%files doc
%defattr(-,root,root,-)
%{_datadir}/doc/%{name}
%docdir %{_datadir}/doc/%{name}

%files ido-mysql
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README.md NEWS AUTHORS CHANGELOG.md
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/features-available/ido-mysql.conf
%{_libdir}/%{name}/libdb_ido_mysql*
%{_datadir}/icinga2-ido-mysql

%files ido-pgsql
%defattr(-,root,root,-)
%doc COPYING COPYING.Exceptions README.md NEWS AUTHORS CHANGELOG.md
%config(noreplace) %attr(0640,%{icinga_user},%{icinga_group}) %{_sysconfdir}/%{name}/features-available/ido-pgsql.conf
%{_libdir}/%{name}/libdb_ido_pgsql*
%{_datadir}/icinga2-ido-pgsql

# DEPRECATED, disable builds on Amazon
%if !(0%{?amzn})

%files classicui-config
%defattr(-,root,root,-)
%attr(0751,%{icinga_user},%{icinga_group}) %dir %{icingaclassicconfdir}
%config(noreplace) %{icingaclassicconfdir}/cgi.cfg
%config(noreplace) %{apacheconfdir}/icinga.conf
%config(noreplace) %attr(0640,root,%{apachegroup}) %{icingaclassicconfdir}/passwd

# DEPRECATED, disable builds on Amazon
%endif

%if "%{_vendor}" == "redhat" && !(0%{?el5} || 0%{?rhel} == 5 || "%{?dist}" == ".el5" || 0%{?el6} || 0%{?rhel} == 6 || "%{?dist}" == ".el6")
%files selinux
%defattr(-,root,root,0755)
%doc tools/selinux/*
%{_datadir}/selinux/*/%{modulename}.pp
%endif

%if 0%{?fedora}
%files studio
%defattr(-,root,root,-)
%{_bindir}/icinga-studio
%{_datadir}/icinga2-studio
%{_datadir}/applications/icinga2-studio.desktop
%endif

%files -n vim-icinga2
%defattr(-,root,root,-)
%if "%{_vendor}" == "suse"
%if 0%{?suse_version} >= 1310
%{_datadir}/vim/vim74/syntax/%{name}.vim
%{_datadir}/vim/vim74/ftdetect/%{name}.vim
%else
%{_datadir}/vim/vim72/syntax/%{name}.vim
%{_datadir}/vim/vim72/ftdetect/%{name}.vim
%endif
%else
%{_datadir}/vim/vimfiles/syntax/%{name}.vim
%{_datadir}/vim/vimfiles/ftdetect/%{name}.vim
%endif

%files -n nano-icinga2
%defattr(-,root,root,-)
%{_datadir}/nano/%{name}.nanorc

%changelog
