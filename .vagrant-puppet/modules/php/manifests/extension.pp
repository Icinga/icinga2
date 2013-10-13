# define: php::extension
#
#   Install additional PHP modules.
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
#   php::extension { 'php-ldap': }
#   php::extension { ['php-mysql'], ['php-pgsql']: }
#
define php::extension(
  $ensure=installed
) {

  include apache
  include php

  if $::require {
    $require_ = [Package['apache'], Class['php'], $::require]
  } else {
    $require_ = [Package['apache'], Class['php']]
  }

  package { $name:
    ensure  => $ensure,
    require => $require_,
    notify  => Service['apache']
  }
}
