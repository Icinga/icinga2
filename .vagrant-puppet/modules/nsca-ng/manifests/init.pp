# Class: nsca-ng
#
#   This class installs nsca-ng components
#
class nsca-ng {
  include nsca-ng-server
  include nsca-ng-client
}

# Class: nsca-ng-server
#
#   This class installs nsca-ng server
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
class nsca-ng-server {
  package { 'nsca-ng-server':
    ensure => installed,
  }

  exec { 'iptables-allow-nsca-ng':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'grep -Fxqe "-A INPUT -m state --state NEW -m tcp -p tcp --dport 5668 -j ACCEPT" /etc/sysconfig/iptables',
    command => 'lokkit -p 5668:tcp'
  }

  user { "nsca":
    ensure => "present",
    groups => "icingacmd"
  }

  service { 'nsca-ng':
    enable => true,
    ensure => running,
    hasrestart => true,
    alias => 'nsca-ng',
    require => [ Package['nsca-ng-server'], Exec['iptables-allow-nsca-ng'] ]
  }

  file { '/etc/nsca-ng.cfg':
    source => 'puppet:////vagrant/.vagrant-puppet/files/etc/nsca-ng.cfg',
    require => Package['nsca-ng-server'],
    notify => Service['nsca-ng']
  }
}

# Class: nsca-ng-client
#
#   This class installs nsca-ng client
#
#   A example passive check result is stored in ~vagrant/passive_result.
#
#   This can be called manually with:
#   send_nsca -c /etc/send_nsca.cfg < /home/vagrant/passive_result
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
class nsca-ng-client {
  package { 'nsca-ng-client':
    ensure => installed,
  }

  file { '/etc/icinga2/conf.d/passive.conf':
    source => 'puppet:////vagrant/.vagrant-puppet/files/etc/icinga2/conf.d/passive.conf',
    require => Package['nsca-ng-client'],
    notify => Service['icinga2']
  }

  file { '/etc/send_nsca.cfg':
    source => 'puppet:////vagrant/.vagrant-puppet/files/etc/send_nsca.cfg',
    owner => 'root',
    group => 'root',
    mode => '0600',
    require => Package['nsca-ng-client'],
  }

  file { '/home/vagrant/passive_result':
    source => 'puppet:////vagrant/.vagrant-puppet/files/home/vagrant/passive_result',
    require => Package['nsca-ng-client'],
  }
}