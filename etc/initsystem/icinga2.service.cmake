[Unit]
Description=Icinga host/service/network monitoring system
After=syslog.target postgresql.service mariadb.service carbon-cache.service

[Service]
Type=forking
EnvironmentFile=@ICINGA2_SYSCONFIGFILE@
ExecStartPre=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2-prepare-dirs @ICINGA2_SYSCONFIGFILE@
ExecStart=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2 daemon -d -e ${ICINGA2_ERROR_LOG}
PIDFile=@ICINGA2_RUNDIR@/icinga2/icinga2.pid
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=multi-user.target
