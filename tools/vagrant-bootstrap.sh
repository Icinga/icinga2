#!/bin/sh
sed -i 's/^HOSTNAME=.*/HOSTNAME=icinga-demo.icinga.org/' /etc/sysconfig/network
hostname icinga-demo.icinga.org

rpm --import http://packages.icinga.org/icinga.key
wget http://packages.icinga.org/epel/6/snapshot/ICINGA-snapshot.repo -O /etc/yum.repos.d/ICINGA-snapshot.repo
yum makecache
yum install -y httpd
yum install -y --nogpgcheck icinga2 icinga-gui
chkconfig httpd on
chkconfig icinga off
chkconfig icinga2 on

/etc/init.d/icinga stop
/etc/init.d/icinga2 stop

wget http://ftp-stud.hs-esslingen.de/pub/epel/6/i386/epel-release-6-8.noarch.rpm -O /tmp/epel.rpm
yum install -y /tmp/epel.rpm
rm -f /tmp/epel.rpm
yum install -y nagios-plugins-all

ln -sf /var/cache/icinga2/status.dat /var/spool/icinga/status.dat
ln -sf /var/cache/icinga2/objects.cache /var/spool/icinga/objects.cache
ln -sf /var/run/icinga2/cmd/icinga2.cmd /var/icinga/cmd/icinga.cmd
ln -sf /var/log/icinga2/compat/icinga.log /var/log/icinga/icinga.log
rm -Rf /var/log/icinga/archives
ln -s /var/log/icinga2/compat/archives /var/log/icinga/

i2enfeature statusdat
i2enfeature compat-log
i2enfeature command

sed -i 's/lib/lib64/' /etc/icinga2/conf.d/macros.conf

/etc/init.d/httpd start
/etc/init.d/icinga2 start

iptables -I INPUT 2 -m tcp -p tcp --dport 80 -j ACCEPT
/etc/init.d/iptables save
