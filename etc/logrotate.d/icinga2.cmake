@ICINGA2_LOGDIR@/icinga2.log @ICINGA2_LOGDIR@/debug.log {
	daily
	rotate 7
	su @ICINGA2_USER@ @ICINGA2_GROUP@
	compress
	delaycompress
	missingok
	notifempty
	postrotate
		/bin/kill -USR1 $(cat @ICINGA2_INITRUNDIR@/icinga2.pid 2> /dev/null) 2> /dev/null || true
	endscript
}

@ICINGA2_LOGDIR@/error.log {
	daily
	rotate 90
	su @ICINGA2_USER@ @ICINGA2_GROUP@
	compress
	delaycompress
	missingok
	notifempty
	# TODO: figure out how to get Icinga to re-open this log file
}
