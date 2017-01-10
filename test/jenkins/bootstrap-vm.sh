#!/bin/sh
if [ "$1" != "--force" ]; then
    echo 'This script is NOT intended to be ran by an individual user.' \
         'If you are not human, pass "--force" as the first option to it!'
    exit 1
fi

if [ $# -lt 3 ]; then
    echo 'Too few arguments. You need to pass "--force <user> <host>"' \
         'to run this script.'
    exit 1
fi


user=$2
host=$3

SSH_OPTIONS="-o PasswordAuthentication=no"
SSH="ssh $SSH_OPTIONS $user@$host"

$SSH "mkdir /vagrant"
# TODO clone git and use the icinga2x puppet modules
git clone git://git.icinga.com/icinga-vagrant.git
scp -qr icinga-vagrant/icinga2x/.vagrant-puppet $user@$host:/vagrant
rm -rf icinga-vagrant

$SSH "useradd vagrant"
$SSH "su -c 'mkdir -p -m 0700 ~/.ssh' vagrant"
$SSH "su -c \"echo '`cat ~/.ssh/id_rsa.pub`' >> ~/.ssh/authorized_keys\" vagrant"
$SSH "echo '10.10.27.1 packages.icinga.com' >> /etc/hosts"
$SSH "puppet apply --modulepath=/vagrant/.vagrant-puppet/modules" \
     "             /vagrant/.vagrant-puppet/manifests/default.pp"

exit 0
