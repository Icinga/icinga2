class icinga-classicui {
  include icinga-rpm-snapshot

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

  exec { 'enable-icinga2-features':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    command => 'i2enfeature statusdat; \
                i2enfeature compat-log; \
                i2enfeature command;',
    require => [ Package['icinga2'] ],
    notify => Service['icinga2']
  }
}
