include apache
include icinga2
include icinga2-classicui
include icinga2-icinga-web
include nagios-plugins
include nsca-ng

# icinga 2 docs at /icinga2-doc
file { '/etc/httpd/conf.d/icinga2-doc.conf':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/httpd/conf.d/icinga2-doc.conf',
  require => [ Package['apache'], Package['icinga2-doc'] ],
  notify => Service['apache']
}

package { 'vim-enhanced':
  ensure => 'installed'
}

file { '/etc/motd':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/motd',
  owner => root,
  group => root
}

user { 'vagrant':
  groups  => ['icinga', 'icingacmd'],
  require => [User['icinga'], Group['icingacmd']]
}
