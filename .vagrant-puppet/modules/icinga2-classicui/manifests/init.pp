class icinga2-classicui {
  include icinga-rpm-snapshot
  include icinga2

  # workaround for package conflicts
  # icinga-gui pulls icinga-gui-config automatically
  package { 'icinga2-classicui-config':
    ensure => installed,
    before => Package["icinga-gui"],
    require => Class['icinga-rpm-snapshot'],
    notify => Service['apache']
  }

  package { 'icinga-gui':
    ensure => installed,
    alias => 'icinga-gui'
  }

  # runtime users
  group { 'icingacmd':
    ensure => present
  }

  user { 'icinga':
    ensure => present,
    groups => 'icingacmd',
    managehome => false
  }

  user { 'apache':
    groups => ['icingacmd', 'vagrant'],
    require => [ Class['apache'], Group['icingacmd'] ]
  }

  icinga2::feature { 'statusdata': }

  icinga2::feature { 'command': }

  icinga2::feature { 'compatlog': }
}
