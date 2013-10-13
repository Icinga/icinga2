#!/bin/bash

# for some reason puppet does not start icinga2
/sbin/service icinga2 start

# fix faulty file permissions
chown root:apache /etc/icinga/passwd
/sbin/service httpd reload

echo "The Icinga 2 Vagrant VM has finished installing. See http://localhost:8080/ for more details."
