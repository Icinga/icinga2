include apache
include mysql
include pgsql
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

#exec { 'create-pgsql-icinga2-ido-db':
#  unless => 'sudo -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname=\'icinga\'" | grep -q 1',
#  command => 'sudo -u postgres psql -c "CREATE ROLE icinga WITH LOGIN PASSWORD \'icinga\';" && \
#              sudo -u postgres createdb -O icinga -E UTF8 icinga && \
#              sudo -u postgres createlang plpgsql icinga',
#  require => Service['postgresql']
#}


php::extension { ['php-mysql', 'php-pgsql']:
  require => [ Class['mysql'], Class['pgsql'] ]
}

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

cmmi { 'icinga-plugins':
  url     => 'https://www.nagios-plugins.org/download/nagios-plugins-1.5.tar.gz',
  output  => 'nagios-plugins-1.5.tar.gz',
  flags   => '--prefix=/usr/lib64/nagios/plugins \
              --with-nagios-user=icinga --with-nagios-group=icinga \
              --with-cgiurl=/icinga-mysql/cgi-bin',
  creates => '/usr/lib64/nagios/plugins/libexec',
  make    => 'make && make install',
  require => User['icinga']
}

file { '/etc/profile.d/env.sh':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/profile.d/env.sh'
}


exec { 'install nodejs':
  command => 'yum -d 0 -e 0 -y --enablerepo=epel install npm',
  unless  => 'rpm -qa | grep ^npm',
  require => Class['epel']
}


# for development only, not rpms
$icinga2_dev_packages = [ 'doxygen', 'openssl-devel',
                      'gcc-c++', 'libstdc++-devel',
                      'automake', 'autoconf',
                      'libtool', 'flex', 'bison',
                      'boost-devel', 'boost-program-options',
                      'boost-signals', 'boost-system',
                      'boost-test', 'boost-thread' ]
package { $icinga2_dev_packages: ensure => installed }

#package { 'nagios-plugins-all':
#  ensure => installed
#}

$icinga2_packages = [ 'icinga2', 'icinga2-doc', 'icinga2-ido-mysql', 'icinga2-classicui-config' ]
$icinga1_packages = [ 'icinga-gui' ]

package { $icinga2_packages:
  ensure => installed,
  require => Class['icinga-rpm-snapshot']
}
package { $icinga1_packages:
  ensure => installed,
  require => Class['icinga-rpm-snapshot']
}


exec { 'iptables-allow-http':
  unless  => 'grep -Fxqe "-A INPUT -p tcp -m state --state NEW -m tcp --dport 80 -j ACCEPT" /etc/sysconfig/iptables',
  command => 'iptables -I INPUT 5 -p tcp -m state --state NEW -m tcp --dport 80 -j ACCEPT && iptables-save > /etc/sysconfig/iptables'
}

file { '/etc/httpd/conf.d/icinga2-doc.conf':
  source  => 'puppet:////vagrant/.vagrant-puppet/files/etc/httpd/conf.d/icinga2-doc.conf',
  require => [ Package['apache'], Package['icinga2-doc'] ],
  notify  => Service['apache']
}

file { '/etc/motd':
  source => 'puppet:////vagrant/.vagrant-puppet/files/etc/motd',
  owner  => root,
  group  => root
}

user { 'vagrant':
  groups  => 'icinga-cmd',
  require => Group['icinga-cmd']
}

service { 'icinga2':
  enable => true,
  ensure => running,
  require => Package['icinga2']
}

exec { 'Enable Icinga 2 features':
  command => 'i2enfeature statusdat; \
              i2enfeature compat-log;
              i2enfeature command;',
  require => Package['icinga2'],
}

file { "/etc/icinga2/features-enabled/*":
  notify => Service['icinga2']
}
  


