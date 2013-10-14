include apache
include mysql
# enable when icinga2-ido-pgsql is ready
#include pgsql
include epel
include icinga-rpm-snapshot

Exec { path => '/bin:/usr/bin:/sbin:/usr/sbin' }

exec { 'create-mysql-icinga2-ido-db':
  unless => 'mysql -uicinga -picinga icinga',
  command => 'mysql -uroot -e "CREATE DATABASE icinga; \
  	      GRANT ALL ON icinga.* TO icinga@localhost \
              IDENTIFIED BY \'icinga\';"',
  require => Service['mysqld']
}

exec { 'create-mysql-icinga-web-db':
  unless => 'mysql -uicinga_web -picinga_web icinga_web',
  command => 'mysql -uroot -e "CREATE DATABASE icinga_web; \
	      GRANT ALL ON icinga_web.* TO icinga_web@localhost \
	      IDENTIFIED BY \'icinga_web\';"',
  require => Service['mysqld']
}

# enable when icinga2-ido-pgsql is ready
#exec { 'create-pgsql-icinga2-ido-db':
#  unless => 'sudo -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname=\'icinga\'" | grep -q 1',
#  command => 'sudo -u postgres psql -c "CREATE ROLE icinga WITH LOGIN PASSWORD \'icinga\';" && \
#              sudo -u postgres createdb -O icinga -E UTF8 icinga && \
#              sudo -u postgres createlang plpgsql icinga',
#  require => Service['postgresql']
#}

php::extension { ['php-mysql']:
  require => [ Class['mysql'] ]
}

# enable when icinga2-ido-pgsql is ready
#php::extension { ['php-pgsql']:
#  require => [ Class['pgsql'] ]
#}

# runtime users
group { 'icinga-cmd':
  ensure => present
}

user { 'icinga':
  ensure => present,
  groups => 'icinga-cmd',
  managehome => false
}

user { 'apache':
  groups => ['icinga-cmd', 'vagrant'],
  require => [ Class['apache'], Group['icinga-cmd'] ]
}

file { '/etc/profile.d/env.sh':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/profile.d/env.sh'
}

# nagios plugins from epel
package { 'nagios-plugins-all':
  ensure => installed,
  require => Class['epel']
}

# these package require the icinga-rpm-snapshot repository installed
$icinga2_main_packages = [ 'icinga2', 'icinga2-doc', 'icinga2-ido-mysql', 'icinga-gui' ]

# workaround for package conflicts
# icinga-gui pulls icinga-gui-config automatically
package { 'icinga2-classicui-config':
  ensure => installed,
  before => Package["icinga-gui"],
  require => Class['icinga-rpm-snapshot']
}

package { $icinga2_main_packages:
  ensure => installed,
  require => Class['icinga-rpm-snapshot']
}


package { 'icinga-web':
  ensure => installed,
  require => Class['icinga-rpm-snapshot'],
  notify => Exec['reload-apache']
}

# enable http 80
exec { 'iptables-allow-http':
  unless  => 'grep -Fxqe "-A INPUT -p tcp -m state --state NEW -m tcp --dport 80 -j ACCEPT" /etc/sysconfig/iptables',
  command => 'iptables -I INPUT 5 -p tcp -m state --state NEW -m tcp --dport 80 -j ACCEPT && iptables-save > /etc/sysconfig/iptables'
}

# icinga 2 docs at /icinga2-doc
file { '/etc/httpd/conf.d/icinga2-doc.conf':
  source  => 'puppet:////vagrant/.vagrant-puppet/files/etc/httpd/conf.d/icinga2-doc.conf',
  require => [ Package['apache'], Package['icinga2-doc'] ],
  notify  => Service['apache']
}

# users
file { '/etc/motd':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/motd',
  owner  => root,
  group  => root
}

user { 'vagrant':
  groups  => 'icinga-cmd',
  require => Group['icinga-cmd']
}

# icinga2 service & features
service { 'icinga2':
  enable => true,
  ensure => running,
  hasrestart => true,
  require => Package['icinga2']
}

# icinga 2 IDO config
file { '/etc/icinga2/features-available/ido-mysql.conf':
  source  => 'puppet:////vagrant/.vagrant-puppet/files/etc/icinga2/features-available/ido-mysql.conf',
  require => Package['icinga2'],
  notify  => Service['icinga2']
}

exec { 'Enable Icinga 2 features':
  command => 'i2enfeature statusdat; \
              i2enfeature compat-log; \
              i2enfeature command; \
              i2enfeature ido-mysql;',
  require => [ Package['icinga2'], Exec['populate-icinga2-ido-mysql-db'] ]
}

file { "/etc/icinga2/features-enabled/*":
  notify => Service['icinga2']
}

# populate icinga2-ido-mysql db
exec { 'populate-icinga2-ido-mysql-db':
  unless  => 'mysql -uicinga -picinga icinga -e "SELECT * FROM icinga_dbversion;" &> /dev/null',
  command => 'mysql -uicinga -picinga icinga < /usr/share/doc/icinga2-ido-mysql-$(rpm -q icinga2-ido-mysql | cut -d\'-\' -f4)/schema/mysql.sql',
  require => [ Package['icinga2-ido-mysql'], Exec['create-mysql-icinga2-ido-db'] ]
}

#exec { 'populate-icinga2-ido-pgsql-db':
#  unless  => 'psql -U icinga -d icinga -c "SELECT * FROM icinga_dbversion;" &> /dev/null',
#  command => 'sudo -u postgres psql -U icinga -d icinga < /usr/share/doc/icinga2-ido-pgsql-$(rpm -q icinga2-ido-mysql | cut -d\'-\' -f4)/schema/pgsql.sql',
#  require => [ Package['icinga2-ido-pgsql'], Exec['create-pgsql-icinga2-ido-db'] ]
#}

exec { 'populate-icinga-web-mysql-db':
  unless  => 'mysql -uicinga_web -picinga_web icinga_web -e "SELECT * FROM nsm_user;" &> /dev/null',
  command => 'mysql -uicinga_web -picinga_web icinga_web < /usr/share/icinga-web/etc/schema/mysql.sql',
  require => [ Package['icinga-web'], Exec['create-mysql-icinga-web-db'] ]
}

