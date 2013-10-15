class nagios-plugins {
  include epel

  # nagios plugins from epel
  package { 'nagios-plugins-all':
    ensure => installed,
    require => Class['epel']
  }
}
