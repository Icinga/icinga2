# Class: epel
#
#   Configure EPEL repository.
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
#   include epel
#
class epel {
  yumrepo { 'epel':
    mirrorlist => "http://mirrors.fedoraproject.org/mirrorlist?repo=epel-6&arch=${::architecture}",
    enabled => '1',
    gpgcheck => '1',
    gpgkey => 'file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6',
    descr => "Extra Packages for Enterprise Linux 6 - ${::architecture}"
  }

  file { "/etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6":
    ensure => present,
    owner => 'root',
    group => 'root',
    mode => '0644',
    source => "puppet:////vagrant/.vagrant-puppet/files/etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6"
  }

  epel::key { "RPM-GPG-KEY-EPEL-6":
    path => "/etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-6",
    before => Yumrepo['icinga-rpm-snapshot']
  }
}

