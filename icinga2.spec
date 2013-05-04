Summary: network monitoring application
Name: icinga2
Version: 0.0.2
Release: 1%{?dist}
License: GPL
Group: Applications/System
Source: %{name}-%{version}.tar.gz
URL: http://www.icinga.org/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

BuildRequires: openssl-devel
BuildRequires: gcc-c++
BuildRequires: boost
BuildRequires: boost-devel
BuildRequires: boost-test

%description
Icinga is a general-purpose network monitoring application.

%prep
%setup -q -n %{name}-%{version}

%build
%configure
make %{?_smp_mflags}

%install
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%clean
[ "%{buildroot}" != "/" ] && [ -d "%{buildroot}" ] && rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_sysconfdir}/%{name}
#%config(noreplace) %{_sysconfdir}/%{name}/%{name}.conf
%attr(755,-,-) %{_sysconfdir}/init.d/%{name}
%{_sbindir}/%{name}
%{_libdir}/%{name}
%{_datadir}/doc/%{name}
%{_datadir}/%{name}
%{_datadir}/%{name}/itl
%{_mandir}/man8/%{name}.8.gz



%changelog
* Sat May 04 2013 Michael Friedrich <michael.friedrich@netways.de> - 0.0.2-1
- icinga2 binary in sbindir
- new initscript in initdir
- itl is installed into datadir
- man pages in mandir
- use name macro to avoid typos

* Fri Nov 19 2012 Gunnar Beutner <gunnar.beutner@netways.de> - 0.0.1-1
- initial version
