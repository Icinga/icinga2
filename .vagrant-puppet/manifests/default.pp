include apache
include icinga-classicui
include icinga-web
include nagios-plugins
include nsca-ng

# icinga 2 docs at /icinga2-doc
file { '/etc/httpd/conf.d/icinga2-doc.conf':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/httpd/conf.d/icinga2-doc.conf',
  require => [ Package['apache'], Package['icinga2-doc'] ],
  notify => Service['apache']
}

file { '/etc/motd':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/motd',
  owner => root,
  group => root
}

user { 'vagrant':
  groups  => 'icingacmd',
  require => Group['icingacmd']
}
