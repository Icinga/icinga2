Summary: network monitoring application
Name: icinga2
Version: 0.0.2
Release: 1%{?dist}
License: GPL
Group: Applications/System
Source: %{name}-%{version}.tar.gz
URL: http://www.icinga.org/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

BuildRequires: docbook-simple
BuildRequires: doxygen
BuildRequires: openssl-devel
BuildRequires: gcc-c++
BuildRequires: libstdc++-devel
BuildRequires: libtool-ltdl-devel
BuildRequires: automake
BuildRequires: autoconf
BuildRequires: libtool
BuildRequires: flex
BuildRequires: bison


# TODO: figure out how to handle boost on el5
BuildRequires: boost
BuildRequires: boost-devel
BuildRequires: boost-test

%description
Icinga is a general-purpose network monitoring application.

%prep
%setup -q -n %{name}-%{version}

%build
sh autogen.sh
%configure
make %{?_smp_mflags}

%install
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

# dist config
mv %{buildroot}%{_sysconfdir}/%{name}/%{name}.conf.dist %{buildroot}%{_sysconfdir}/%{name}/%{name}.conf

%clean
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING COPYING.Exceptions
%{_sysconfdir}/%{name}
%config(noreplace) %{_sysconfdir}/%{name}/%{name}.conf
%attr(755,-,-) %{_sysconfdir}/init.d/%{name}
%{_bindir}/%{name}
%{_libdir}/%{name}
%{_datadir}/doc/%{name}
%{_datadir}/%{name}
%{_datadir}/%{name}/itl
%{_mandir}/man8/%{name}.8.gz

%{_localstatedir}/cache/%{name}
%{_localstatedir}/log/%{name}
%{_localstatedir}/run/%{name}
%{_localstatedir}/lib/%{name}
%{_localstatedir}/spool/%{name}
%{_localstatedir}/spool/%{name}/perfdata



%changelog
* Sat May 04 2013 Michael Friedrich <michael.friedrich@netways.de> - 0.0.2-1
- new initscript in initdir
- itl is installed into datadir
- man pages in mandir
- use name macro to avoid typos

* Fri Nov 19 2012 Gunnar Beutner <gunnar.beutner@netways.de> - 0.0.1-1
- initial version
