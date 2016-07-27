[Unit]
Description=Icinga host/service/network monitoring system
After=syslog.target network.target postgresql.service mariadb.service carbon-cache.service

[Service]
Type=forking
EnvironmentFile=@ICINGA2_SYSCONFIGFILE@
ExecStartPre=@CMAKE_INSTALL_PREFIX@/lib/icinga2/prepare-dirs @ICINGA2_SYSCONFIGFILE@
ExecStart=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2 daemon -d -e ${ICINGA2_ERROR_LOG}
PIDFile=@ICINGA2_RUNDIR@/icinga2/icinga2.pid
ExecReload=@CMAKE_INSTALL_PREFIX@/lib/icinga2/safe-reload @ICINGA2_SYSCONFIGFILE@
TimeoutStartSec=30m

[Install]
WantedBy=multi-user.target
