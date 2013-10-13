# Class: pear
#
#   This class installs pear.
#
# Parameters:
#
# Actions:
#
# Requires:
#
#   php
#
# Sample Usage:
#
#   include pear
#
class pear {

  Exec { path => '/usr/bin:/bin' }

  include php

  package { 'php-pear':
    ensure  => installed,
    require => Class['php']
  }

  exec { 'pear upgrade':
    command => 'pear upgrade',
    require => Package['php-pear']
  }

  exec { 'pear update-channels':
    command => 'pear update-channels',
    require => Package['php-pear']
  }

  exec { 'pear auto discover channels':
    command => 'pear config-set auto_discover 1',
    unless  => 'pear config-get auto_discover | grep 1',
    require => Package['php-pear']
  }
}
