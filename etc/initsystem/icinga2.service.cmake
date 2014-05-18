[Unit]
Description=Icinga host/service/network monitoring system
After=syslog.target postgresql.service mariadb.service

[Service]
Type=forking
EnvironmentFile=@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2/sysdefines.conf
ExecStartPre=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2-prepare-dirs @CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2/sysdefines.conf
ExecStart=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2 -c ${ICINGA2_CONFIG_FILE} -d -e ${ICINGA2_ERROR_LOG} -u ${ICINGA2_USER} -g ${ICINGA2_GROUP}
PIDFile=@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/run/icinga2/icinga2.pid
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=multi-user.target
