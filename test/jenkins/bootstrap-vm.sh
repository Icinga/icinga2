#!/bin/sh
if [ "$1" != "run-by-jenkins" ]; then
	echo "This script should not be run manually."
	exit 1
fi

echo "10.10.27.1 packages.icinga.org" >> /etc/hosts

groupadd vagrant

rmdir /vagrant && ln -s /root/icinga2 /vagrant
puppet apply --modulepath=/vagrant/.vagrant-puppet/modules /vagrant/.vagrant-puppet/manifests/default.pp

exit 0
