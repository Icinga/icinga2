Summary: network monitoring application
Name: icinga2
Version: 0.0.1
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
%{_bindir}/icinga2
%{_datadir}/doc/icinga2
%{_libdir}/icinga2
%{_sysconfdir}/icinga2
