include apache
include mysql
# enable when icinga2-ido-pgsql is ready
#include pgsql
include epel
include icinga-classicui
include icinga2
include icinga-web
include nagios-plugins

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
