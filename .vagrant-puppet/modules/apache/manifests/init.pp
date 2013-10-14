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
    alias  => 'apache'
  }

  service { $apache:
    ensure  => running,
    alias   => 'apache',
    require => Package['apache']
  }
}
