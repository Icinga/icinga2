# Define: pear::package
#
#   Install additional PEAR packages
#
# Parameters:
#
# Actions:
#
# Requires:
#
#   pear
#
# Sample Usage:
#
#   pear::package { 'phpunit': }
#
define pear::package(
  $channel
) {

  Exec { path => '/usr/bin' }

  include pear

  if $::require {
    $require_ = [Class['pear'], $::require]
  } else {
    $require_ = Class['pear']
  }

  exec { "pear install ${name}":
    command => "pear install --alldeps ${channel}",
    creates => "/usr/bin/${name}",
    require => $require_
  }

  exec { "pear upgrade ${name}":
    command => "pear upgrade ${channel}",
    require => Exec["pear install ${name}"]
  }
}
