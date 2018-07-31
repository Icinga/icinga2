@ICINGA2_LOGDIR@/icinga2.log @ICINGA2_LOGDIR@/debug.log {
	daily
	rotate 7@LOGROTATE_USE_SU@
	compress
	delaycompress
	missingok
	notifempty
	create 644 @ICINGA2_USER@ @ICINGA2_GROUP@
	postrotate
		/bin/kill -USR1 $(cat @ICINGA2_INITRUNDIR@/icinga2.pid 2> /dev/null) 2> /dev/null || true
	endscript
}

@ICINGA2_LOGDIR@/error.log {
	daily
	rotate 90@LOGROTATE_USE_SU@
	compress
	delaycompress
	missingok
	notifempty
	create 644 @ICINGA2_USER@ @ICINGA2_GROUP@
	# TODO: figure out how to get Icinga to re-open this log file
}
