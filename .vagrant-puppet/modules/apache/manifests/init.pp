# Class: apache
#
#   This class installs the apache server.
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
#   include apache
#
class apache {
  $apache = $::operatingsystem ? {
    /(Debian|Ubuntu)/ => 'apache2',
    /(RedHat|CentOS|Fedora)/ => 'httpd'
  }

  package { $apache:
    ensure => installed,
    alias => 'apache'
  }

  exec { 'iptables-allow-http':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'grep -Fxqe "-A INPUT -m state --state NEW -m tcp -p tcp --dport 80 -j ACCEPT" /etc/sysconfig/iptables',
    command => 'lokkit --enabled --service=http'
  }

  service { $apache:
    enable => true,
    ensure => running,
    alias => 'apache',
    require => [ Package['apache'], Exec['iptables-allow-http'] ]
  }
}
