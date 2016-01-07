@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/log/icinga2/icinga2.log @CMAKE_INSTALL_FULL_LOCALSTATEDIR@/log/icinga2/debug.log {
	daily
	rotate 7
	compress
	delaycompress
	missingok
	notifempty
	create 644 @ICINGA2_USER@ @ICINGA2_GROUP@
	postrotate
		if service icinga2 status &> /dev/null; then
			if [ -e @ICINGA2_RUNDIR@/icinga2/icinga2.pid ]; then
				kill -USR1 $(cat @ICINGA2_RUNDIR@/icinga2/icinga2.pid)
			fi
		fi
	endscript
}

@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/log/icinga2/error.log {
	daily
	rotate 90
	compress
	delaycompress
	missingok
	notifempty
	create 644 @ICINGA2_USER@ @ICINGA2_GROUP@
	# TODO: figure out how to get Icinga to re-open this log file
}

