class icinga2-icinga-web {
  include icinga-web
  include icinga2-ido-mysql
  include icinga2-ido-pgsql

  exec { 'set-icinga2-cmd-pipe-path':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    command => 'sed -i \'s/\/var\/spool\/icinga\/cmd\/icinga.cmd/\/var\/run\/icinga2\/cmd\/icinga2.cmd/g\' /etc/icinga-web/conf.d/access.xml',
    require => Package['icinga-web']
  }

  exec { 'clear-config-cache':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    command => '/usr/bin/icinga-web-clearcache',
    require => Exec['set-icinga2-cmd-pipe-path']
  }
}
