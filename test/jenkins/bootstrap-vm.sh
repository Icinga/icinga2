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
scp -qr ../../.vagrant-puppet $user@$host:/vagrant

$SSH "groupadd vagrant"
$SSH "echo '10.10.27.1 packages.icinga.org' >> /etc/hosts"
$SSH "puppet apply --modulepath=/vagrant/.vagrant-puppet/modules" \
     "             /vagrant/.vagrant-puppet/manifests/default.pp"

exit 0
