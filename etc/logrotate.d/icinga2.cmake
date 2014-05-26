@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/log/icinga2/icinga2.log @CMAKE_INSTALL_FULL_LOCALSTATEDIR@/log/icinga2/debug.log {
       	daily
	rotate 7
	compress
	delaycompress
       	missingok
       	notifempty
       	create 644 @ICINGA2_USER@ @ICINGA2_GROUP@
	copytruncate
	postrotate
		if ! killall -q -USR1 icinga2; then
			exit 1
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
	copytruncate
	# TODO: figure out how to get Icinga to re-open this log file
}

