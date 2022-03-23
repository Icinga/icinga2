[Unit]
Description=Icinga host/service/network monitoring system
Requires=network-online.target
After=syslog.target network-online.target icingadb-redis.service postgresql.service mariadb.service carbon-cache.service carbon-relay.service

[Service]
Type=notify
NotifyAccess=all
Environment="ICINGA2_ERROR_LOG=@ICINGA2_LOGDIR@/error.log"
EnvironmentFile=@ICINGA2_SYSCONFIGFILE@
ExecStartPre=@CMAKE_INSTALL_PREFIX@/lib/icinga2/prepare-dirs @ICINGA2_SYSCONFIGFILE@
ExecStart=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2 daemon --close-stdio -e ${ICINGA2_ERROR_LOG}
PIDFile=@ICINGA2_INITRUNDIR@/icinga2.pid
ExecReload=@CMAKE_INSTALL_PREFIX@/lib/icinga2/safe-reload @ICINGA2_SYSCONFIGFILE@
TimeoutStartSec=30m
KillMode=mixed

# Systemd >228 enforces a lower process number for services.
# Depending on the distribution and Systemd version, this must
# be explicitly raised. Packages will set the needed values
# into /etc/systemd/system/icinga2.service.d/limits.conf
#
# Please check the troubleshooting documentation for further details.
# The values below can be used as examples for customized service files.

#TasksMax=infinity
#LimitNPROC=62883

[Install]
WantedBy=multi-user.target
