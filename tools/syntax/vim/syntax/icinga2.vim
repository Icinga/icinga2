" Vim syntax file
" Filename:		icinga2.vim
" Language:		Icinga2 object configuration file
" Author:		Carlos Cesario <carloscesario@gmail.com>
" Version:		0.0.1
" Based:		javascript.vim / nagios.vim

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if !exists("main_syntax")
	if version < 600
		syntax clear
	elseif exists("b:current_syntax")
		finish
	endif
	let main_syntax = 'icinga2'
endif


" case off
syntax case ignore

" comments
syn keyword		icinga2CommentTodo		TODO FIXME XXX TBD contained
syn match		icinga2LineComment		"\/\/.*" contains=icinga2CommentTodo
syn match		icinga2CommentSkip		"^[ \t]*\*\($\|[ \t]\+\)"
syn region		icinga2Comment			start="/\*"  end="\*/" contains=icinga2CommentTodo


" type definitions
" - double quotes "
" - single quotes '
" - brackets <>

syn region		StringD			start=+"+  end=+"\|$+
syn region		StringS			start=+'+  end=+'\|$+
syn match		angleBrackets	"<\w\+>"


" Braces and Parens definition
" Braces are used in dictionary definition

syn match		Braces		"[{}\[\]]"
syn match		Parens		"[()]"


" objects types

syn match		icinga2ObjDef		"object[ \t]\+\(host\|hostgroup\|host\|hostgroup\|service\|servicegroup\|user\|usergroup\)"
syn match		icinga2ObjDef		"object[ \t]\+\(checkcommand\|notificationcommand\|eventcommand\|notification\)"
syn match		icinga2Objdef		"object[ \t]\+\(timeperiod\|scheduleddowntime\|dependency\|perfdatawriter\)"
syn match		icinga2ObjDef		"object[ \t]\+\(graphitewriter\|idomysqlconnection\|idomysqlconnection\)"
syn match		icinga2ObjDef		"object[ \t]\+\(livestatuslistener\|statusdatawriter\|externalcommandlistener\)"
syn match		icinga2ObjDef		"object[ \t]\+\(compatlogger\|checkresultreader\|checkcomponent\|notificationcomponent\)"
syn match		icinga2ObjDef		"object[ \t]\+\(filelogger\|sysloglogger\|icingastatuswriter\|apilistener\|endpoint\|zone\)"


" apply def

syn match		icinga2ApplyDef		"apply[ \t]\+\(Service\|Dependency\|Notification\|ScheduledDowntime\)"


" objects attributes

syn keyword		icinga2ObjAttr		contained	accept_commands accept_config action_url address address6 arguments author bind_host
syn keyword		icinga2ObjAttr		contained	bind_port ca_path categories cert_path check_command check_interval
syn keyword		icinga2ObjAttr		contained	check_period child_host_name child_service_name cleanup command command_endpoint command_path
syn keyword		icinga2ObjAttr		contained	comment compat_log_path crl_path database disable_checks disable_notifications
syn keyword		icinga2ObjAttr		contained	display_name duration email enable_active_checks enable_event_handler
syn keyword		icinga2ObjAttr		contained	enable_flapping enable_ha enable_notifications enable_passive_checks enable_perfdata
syn keyword		icinga2ObjAttr		contained	endpoints env event_command failover_timeout fixed flapping_threshold groups host
syn keyword		icinga2ObjAttr		contained	host_format_template host_name host_name_template host_perfdata_path host_temp_path icon_image
syn keyword		icinga2ObjAttr		contained	icon_image_alt instance_description instance_name interval key_path log_dir
syn keyword		icinga2ObjAttr		contained	log_duration max_check_attempts methods name notes notes_url objects_path
syn keyword		icinga2ObjAttr		contained	pager parent parent_host_name parent_service_name password path period
syn keyword		icinga2ObjAttr		contained	port ranges retry_interval rotation_interval rotation_method
syn keyword		icinga2ObjAttr		contained	service_format_template service_name service_name_template service_perfdata_path service_temp_path
syn keyword		icinga2ObjAttr		contained	severity socket_path socket_type spool_dir states status_path table_prefix
syn keyword		icinga2ObjAttr		contained	timeout times types update_interval user user_groups users volatile zone
syn match		icinga2ObjAttr		contained	"\(vars.\w\+\)"


" keywords

syn keyword		icinga2Keyword		template const import include include_recursive


" Assign conditions

syn match		icinga2ACond		contained	"\(assign[ \t]\+\where\|ignore[ \t]\+\where\)"


" functions

syn keyword		icinga2Function		contained	regex match len union intersection string number bool log exit


" global constats
syn keyword		icinga2Gconst		PrefixDir SysconfDir ZonesDir LocalStateDir PkgDataDir RunDir StatePath PidPath
syn keyword		icinga2Gconst		NodeName ApplicationType EnableNotifications EnableEventHandlers 
syn keyword		icinga2Gconst		EnableFlapping EnableHostChecks EnableServiceChecks EnablePerfdata RunAsUser RunAsGroup UseVfork
syn	match		icinga2Gconst		"\(Vars[ \t]\+\)"

" values type

syn keyword		valueBoolean		contained	true false
syn keyword		valueNull			contained	null



syn region		nagiosDefBody start='{' end='}' 
	\ contains=icinga2Comment,icinga2LineComment,StringD,Braces,Parens,icinga2ObjDef,
	\ icinga2ApplyDef,icinga2ObjAttr,icinga2Keyword,icinga2Keyword,icinga2ACond,
	\ icinga2Function,icinga2Gconst,valueBoolean,valueNull


" Highlighting
hi link icinga2Comment				Comment
hi link icinga2LineComment			Comment
hi link icinga2CommentTodo			Todo

hi link Braces						Function
hi link Parens						Function

hi link StringS						String
hi link StringD						String
hi link angleBrackets				String

hi link icinga2ObjDef				Statement
hi link icinga2ApplyDef				Statement
hi link icinga2ObjAttr				Define
hi link	icinga2Keyword				Keyword

hi link	icinga2ACond				Conditional

hi link icinga2Function				Function

hi link icinga2Gconst				Constant

hi link valueBoolean				Boolean
hi link valueNull					Special	
