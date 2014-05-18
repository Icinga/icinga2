[Unit]
Description=Icinga host/service/network monitoring system
After=syslog.target postgresql.service mariadb.service

[Service]
Type=forking
EnvironmentFile=/etc/sysconfig/icinga2
# ExecStartPre=  TODO: execute the mkdir & chown/chmod stuff, ideally in a separate script, used by both init.d and systemd
ExecStart=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2 -c ${ICINGA2_CONFIG_FILE} -d -e ${ICINGA2_ERROR_LOG} -u ${ICINGA2_USER} -g ${ICINGA2_GROUP}
PIDFile=@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/run/icinga2/icinga2.pid
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=multi-user.target
