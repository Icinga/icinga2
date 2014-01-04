class icinga-web {
  include icinga-rpm-snapshot
  include php
  include mysql
  include pgsql

  php::extension { ['php-mysql']:
    require => [ Class['mysql'] ]
  }

  php::extension { ['php-pgsql']:
    require => [ Class['pgsql'] ]
  }

  package { 'icinga-web':
    ensure => installed,
    require => Class['icinga-rpm-snapshot'],
    notify => Service['apache']
  }

  package { 'icinga-web-mysql':
    ensure => installed,
    require => Class['icinga-rpm-snapshot'],
    notify => Service['apache']
  }

  exec { 'create-mysql-icinga-web-db':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'mysql -uicinga_web -picinga_web icinga_web',
    command => 'mysql -uroot -e "CREATE DATABASE icinga_web; GRANT ALL ON icinga_web.* TO icinga_web@localhost IDENTIFIED BY \'icinga_web\';"',
    require => Service['mysqld']
  }

  exec { 'populate-icinga-web-mysql-db':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'mysql -uicinga_web -picinga_web icinga_web -e "SELECT * FROM nsm_user;" &> /dev/null',
    command => 'mysql -uicinga_web -picinga_web icinga_web < /usr/share/icinga-web/etc/schema/mysql.sql',
    require => [ Package['icinga-web'], Exec['create-mysql-icinga-web-db'] ]
  }
}
