# SAMPLE CONFIG SNIPPETS FOR APACHE WEB SERVER
#
# This file contains examples of entries that need
# to be incorporated into your Apache web server
# configuration file.  Customize the paths, etc. as
# needed to fit your system.

ScriptAlias /icinga/cgi-bin "@CMAKE_INSTALL_FULL_LIBDIR@/icinga/cgi"

<Directory "@CMAKE_INSTALL_FULL_LIBDIR@/icinga/cgi">
#  SSLRequireSSL
   Options ExecCGI
   AllowOverride None
   Order allow,deny
   Allow from all
#  Order deny,allow
#  Deny from all
#  Allow from 127.0.0.1
   AuthName "Icinga Access"
   AuthType Basic
   AuthUserFile @CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga/passwd
   Require valid-user
</Directory>

Alias /icinga "@CMAKE_INSTALL_FULL_DATADIR@/icinga/"

<Directory "@CMAKE_INSTALL_FULL_DATADIR@/icinga/">
#  SSLRequireSSL
   Options None
   AllowOverride All
   Order allow,deny
   Allow from all
#  Order deny,allow
#  Deny from all
#  Allow from 127.0.0.1
   AuthName "Icinga Access"
   AuthType Basic
   AuthUserFile @CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga/passwd
   Require valid-user
</Directory>

