#!/bin/sh
sed -i 's/^HOSTNAME=.*/HOSTNAME=icinga-demo.icinga.org/' /etc/sysconfig/network
hostname icinga-demo.icinga.org

yum install -y wget

rpm --import http://packages.icinga.org/icinga.key
wget http://packages.icinga.org/epel/6/snapshot/ICINGA-snapshot.repo -O /etc/yum.repos.d/ICINGA-snapshot.repo
yum makecache
yum install -y httpd icinga2 icinga2-doc icinga2-classicui-config icinga-gui
chkconfig httpd on
chkconfig icinga2 on

# Remove once packages are fixed
chkconfig icinga off
/etc/init.d/icinga stop
/etc/init.d/icinga2 stop

wget http://ftp-stud.hs-esslingen.de/pub/epel/6/i386/epel-release-6-8.noarch.rpm -O /tmp/epel.rpm
yum install -y /tmp/epel.rpm
rm -f /tmp/epel.rpm
yum install -y nagios-plugins-all

i2enfeature statusdat
i2enfeature compat-log
i2enfeature command

# 4845
sed -i 's/^SELINUX=.*/SELINUX=permissive/' /etc/sysconfig/selinux
setenforce Permissive

cat > /etc/httpd/conf.d/doc-redirect.conf <<HTML
Alias /icinga2-doc "/usr/share/doc/icinga2"

RewriteEngine On
RewriteRule ^/$ /icinga2-doc/#vagrant [NE,L,R=301]
HTML

/etc/init.d/httpd start
/etc/init.d/icinga2 start

iptables -I INPUT 2 -m tcp -p tcp --dport 80 -j ACCEPT
/etc/init.d/iptables save

clear

echo "The Icinga 2 Vagrant VM has finished installing. See http://localhost:8080/ for more details."
