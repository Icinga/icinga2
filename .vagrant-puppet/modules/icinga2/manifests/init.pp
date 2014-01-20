class icinga2 {
  include icinga-rpm-snapshot

  package { 'icinga2':
    ensure => installed,
    require => Class['icinga-rpm-snapshot'],
    alias => 'icinga2'
  }

  package { 'icinga2-doc':
    ensure => installed,
    require => Class['icinga-rpm-snapshot'],
    alias => 'icinga2-doc'
  }

  package { 'mailx':
    ensure => installed,
  }

  service { 'icinga2':
    enable => true,
    ensure => running,
    hasrestart => true,
    alias => 'icinga2',
    require => Package['icinga2']
  }

  file { "/etc/icinga2/features-enabled/*":
    notify => Service['icinga2']
  }
}

class icinga2-ido-mysql {
  include icinga-rpm-snapshot
  include mysql

  package { 'icinga2-ido-mysql':
    ensure => installed,
    require => Class['icinga-rpm-snapshot'],
    alias => 'icinga2-ido-mysql'
  }

  file { '/etc/icinga2/features-available/ido-mysql.conf':
    source => 'puppet:////vagrant/.vagrant-puppet/files/etc/icinga2/features-available/ido-mysql.conf',
    require => Package['icinga2'],
    notify => Service['icinga2']
  }

  exec { 'create-mysql-icinga2-ido-db':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'mysql -uicinga -picinga icinga',
    command => 'mysql -uroot -e "CREATE DATABASE icinga; GRANT ALL ON icinga.* TO icinga@localhost IDENTIFIED BY \'icinga\';"',
    require => Service['mysqld']
  }

  exec { 'populate-icinga2-ido-mysql-db':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'mysql -uicinga -picinga icinga -e "SELECT * FROM icinga_dbversion;" &> /dev/null',
    command => 'mysql -uicinga -picinga icinga < /usr/share/doc/icinga2-ido-mysql-$(rpm -q icinga2-ido-mysql | cut -d\'-\' -f4)/schema/mysql.sql',
    require => [ Package['icinga2-ido-mysql'], Exec['create-mysql-icinga2-ido-db'] ]
  }

  icinga2::feature { 'ido-mysql':
    require => Exec['populate-icinga2-ido-mysql-db']
  }
}

class icinga2-ido-pgsql {
  include icinga-rpm-snapshot
  include pgsql

  package { 'icinga2-ido-pgsql':
    ensure => installed,
    require => Class['icinga-rpm-snapshot'],
    alias => 'icinga2-ido-pgsql'
  }

  exec { 'create-pgsql-icinga2-ido-db':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => 'sudo -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname=\'icinga\'" | grep -q 1',
    command => 'sudo -u postgres psql -c "CREATE ROLE icinga WITH LOGIN PASSWORD \'icinga\';" && \
                sudo -u postgres createdb -O icinga -E UTF8 icinga && \
                sudo -u postgres createlang plpgsql icinga',
    require => Service['postgresql']
  }

  exec { 'populate-icinga2-ido-pgsql-db':
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    environment => ['PGPASSWORD=icinga'],
    unless => 'psql -U icinga -d icinga -c "SELECT * FROM icinga_dbversion;" &> /dev/null',
    command => 'psql -U icinga -d icinga < /usr/share/doc/icinga2-ido-pgsql-$(rpm -q icinga2-ido-pgsql | cut -d\'-\' -f4)/schema/pgsql.sql',
    require => [ Package['icinga2-ido-pgsql'], Exec['create-pgsql-icinga2-ido-db'] ]
  }

  icinga2::feature { 'ido-pgsql':
    require => Exec['populate-icinga2-ido-pgsql-db']
  }
}

define icinga2::feature ($feature = $title) {
  exec { "icinga2-feature-${feature}":
    path => '/bin:/usr/bin:/sbin:/usr/sbin',
    unless => "readlink /etc/icinga2/features-enabled/${feature}.conf",
    command => "icinga2-enable-feature ${feature}",
    require => [ Package['icinga2'] ],
    notify => Service['icinga2']
  }
}
