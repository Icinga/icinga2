" Vim syntax file
" Filename:		icinga2.vim
" Language:		Icinga2 object configuration file
" Author:		Carlos Cesario <carloscesario@gmail.com>
" Version:		0.0.2
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
syn match		icinga2LineComment		"#.*" contains=icinga2CommentTodo
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
syn match		icinga2ObjDef		"object[ \t]\+\(hostgroup\|host\|servicegroup\|service\|usergroup\|user\)"
syn match		icinga2ObjDef		"object[ \t]\+\(checkcommand\|notificationcommand\|eventcommand\|notification\)"
syn match		icinga2Objdef		"object[ \t]\+\(timeperiod\|scheduleddowntime\|dependency\|perfdatawriter\)"
syn match		icinga2ObjDef		"object[ \t]\+\(graphitewriter\|idomysqlconnection\|idomysqlconnection\)"
syn match		icinga2ObjDef		"object[ \t]\+\(livestatuslistener\|statusdatawriter\|externalcommandlistener\)"
syn match		icinga2ObjDef		"object[ \t]\+\(compatlogger\|checkresultreader\|checkcomponent\|notificationcomponent\)"
syn match		icinga2ObjDef		"object[ \t]\+\(filelogger\|sysloglogger\|icingastatuswriter\|icingaapplication\|apilistener\|apiuser\|endpoint\|zone\)"


" apply def
syn match		icinga2ApplyDef		"apply[ \t]\+\(Service\|Dependency\|Notification\|ScheduledDowntime\)"


" objects attributes
syn keyword		icinga2ObjAttr		contained	accept_commands accept_config action_url address address6 arguments author bind_host
syn keyword		icinga2ObjAttr		contained	bind_port ca_path categories cert_path check_command check_interval
syn keyword		icinga2ObjAttr		contained	check_period child_host_name child_service_name cleanup client_cn command command_endpoint command_path
syn keyword		icinga2ObjAttr		contained	comment compat_log_path crl_path database disable_checks disable_notifications
syn keyword		icinga2ObjAttr		contained	display_name duration email enable_active_checks enable_event_handlers enable_event_handler
syn keyword		icinga2ObjAttr		contained	enable_flapping enable_ha enable_host_checks enable_notifications enable_passive_checks enable_perfdata
syn keyword		icinga2ObjAttr		contained	enable_service_checks endpoints env event_command failover_timeout fixed flapping_threshold groups host
syn keyword		icinga2ObjAttr		contained	host_format_template host_name host_name_template host_perfdata_path host_temp_path icon_image
syn keyword		icinga2ObjAttr		contained	icon_image_alt instance_description instance_name interval key_path log_dir
syn keyword		icinga2ObjAttr		contained	log_duration max_check_attempts methods name notes notes_url objects_path
syn keyword		icinga2ObjAttr		contained	pager parent parent_host_name parent_service_name password path period permissions
syn keyword		icinga2ObjAttr		contained	port ranges retry_interval rotation_interval rotation_method
syn keyword		icinga2ObjAttr		contained	service_format_template service_name service_name_template service_perfdata_path service_temp_path
syn keyword		icinga2ObjAttr		contained	severity socket_path socket_type spool_dir states status_path table_prefix
syn keyword		icinga2ObjAttr		contained	timeout times types update_interval user user_groups users volatile zone
syn match		icinga2ObjAttr		contained	"\(vars.\w\+\)"


" keywords
syn keyword		icinga2Keyword		template const import include include_recursive var function return to use locals globals this


" Assign conditions
syn match		icinga2AssignCond	contained	"\(assign[ \t]\+\where\|ignore[ \t]\+\where\)"


" Global functions
syn keyword		icinga2GFunction	contained	regex match cidr_match len union intersection keys string
syn keyword 	icinga2GFunction 	contained	number bool random log typeof get_time parse_performance_data dirname
syn keyword 	icinga2GFunction 	contained	basename escape_shell_arg escape_shell_cmd escape_create_process_arg exit


" Accessor Functions
syn keyword 	icinga2AFunction 	contained	get_host get_service get_user get_check_command get_event_command get_notification_command
syn keyword 	icinga2AFunction 	contained	get_host_group get_service_group get_user_group get_time_period


" Math functions
syn match 	icinga2MathFunction 	contained	"\(Math.E\|Math.LN2\|Math.LN10\|Math.LOG2E\|Math.PI\|Math.SQRT1_2\|Math.SQRT2\)"
syn match 	icinga2MathFunction 	contained	"\(Math.abs\|Math.acos\|Math.asin\|Math.atan\|Math.atan2\|Math.ceil\|Math.cos\)"
syn match 	icinga2MathFunction 	contained	"\(Math.exp\|Math.floor\|Math.isinf\|Math.isnan\|Math.log\|Math.max\|Math.min\)"
syn match 	icinga2MathFunction  	contained 	"\(Math.pow\|Math.random\|Math.round\|Math.sign\|Math.sin\|Math.sqrt\|Math.tan\)"

" Json functions
syn match 	icinga2JsonFunction		contained	"\(Json.encode\|Json.decode\)"

" String functions
syn match 	icinga2StrFunction	contained 	"\(\.find\)"
syn match 	icinga2StrFunction	contained 	"\(\.contains\)"
syn match 	icinga2StrFunction	contained 	"\(\.len\)"
syn match 	icinga2StrFunction	contained 	"\(\.lower\)"
syn match 	icinga2StrFunction	contained 	"\(\.upper\)"
syn match 	icinga2StrFunction	contained 	"\(\.replace\)"
syn match 	icinga2StrFunction	contained 	"\(\.split\)"
syn match 	icinga2StrFunction	contained 	"\(\.substr\)"
syn match 	icinga2StrFunction	contained 	"\(\.to_string\)"
syn match 	icinga2StrFunction	contained 	"\(\.reverse\)"

" Array and Dict  Functions
syn match 	icinga2ArrFunction 	contained 	"\(\.clone\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.add(\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.clear\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.shallow_clone\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.contains\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.len\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.remove\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.set\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.get\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.sort\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.join\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.reverse\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.keys\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.call\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.callv\)"


" Conditional statements
syn keyword 	icinga2Cond			if else

" Loops
syn keyword 	icinga2Loop			while for break continue

" Operators
syn  match         icinga2Operators "[ \t]\+\(\.\)\+"
syn  match         icinga2Operators "[ \t]\+\(!\)\+"
syn  match         icinga2Operators "[ \t]\+\(\~\)\+"
syn  match         icinga2Operators "[ \t]\+\(+\)\+"
syn  match         icinga2Operators "[ \t]\+\(-\)\+"
syn  match         icinga2Operators "[ \t]\+\(*\)\+"
syn  match         icinga2Operators "[ \t]\+\(/[^/\*]\)\+"
syn  match         icinga2Operators "[ \t]\+\(%\)\+"
syn  match         icinga2Operators "[ \t]\+\(+\)\+"
syn  match         icinga2Operators "[ \t]\+\(-\)\+"
syn  match         icinga2Operators "[ \t]\+\(=\)\+"
syn  match         icinga2Operators "[ \t]\+\(<\)[ \t]\+"
syn  match         icinga2Operators "[ \t]\+\(>\)[ \t]\+"
syn  match         icinga2Operators "[ \t]\+\(<<\)\+"
syn  match         icinga2Operators "[ \t]\+\(>>\)\+"
syn  match         icinga2Operators "[ \t]\+\(<=\)\+"
syn  match         icinga2Operators "[ \t]\+\(>=\)\+"
syn  match         icinga2Operators "[ \t]\+\(in\)\+"
syn  match         icinga2Operators "[ \t]\+\(!in\)\+"
syn  match         icinga2Operators "[ \t]\+\(==\)\+"
syn  match         icinga2Operators "[ \t]\+\(!=\)\+"
syn  match         icinga2Operators "[ \t]\+\(&\)\+"
syn  match         icinga2Operators "[ \t]\+\(\^\)\+"
syn  match         icinga2Operators "[ \t]\+\(|\)\+"
syn  match         icinga2Operators "[ \t]\+\(&&\)\+"
syn  match         icinga2Operators "[ \t]\+\(||\)\+"
syn  match         icinga2Operators "[ \t]\+\(=>\)\+"
syn  match         icinga2Operators "[ \t]\+\(+=\)\+"
syn  match         icinga2Operators "[ \t]\+\(-=\)\+"
syn  match         icinga2Operators "[ \t]\+\(*=\)\+"
syn  match         icinga2Operators "[ \t]\+\(/=\)\+"

" global constats
syn keyword 	icinga2Gconst		PrefixDir SysconfDir ZonesDir LocalStateDir RunDir PkgDataDir StatePath ObjectsPath
syn keyword 	icinga2Gconst		PidPath NodeName ApplicationType UseVfork RunAsUser RunAsGroup PluginDir
" global types 
syn keyword 	icinga2Gconst		Number String Boolean Array Dictionary Value Object ConfigObject Command CheckResult
syn keyword     icinga2Gconst           Checkable CustomVarObject DbConnection Type PerfdataValue Comment Downtime Logger Application

" values type
syn keyword		valueBoolean		contained	true false
syn keyword		valueNull			contained	null



syn region		nagiosDefBody start='{' end='}'
	\ contains=icinga2Comment, icinga2LineComment, StringD, Braces, Parens, icinga2ObjDef,
	\ icinga2ApplyDef, icinga2ObjAttr, icinga2Keyword, icinga2Keyword, icinga2AssignCond,
	\ icinga2Cond, icinga2Loop, icinga2Operators, icinga2GFunction, icinga2AFunction,
	\ icinga2MathFunction, icinga2Gconst, icinga2JsonFunction, icinga2StrFunction,
	\ icinga2ArrFunction, valueBoolean, valueNull


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

hi link	icinga2AssignCond			Conditional

hi link	icinga2Cond					Statement
hi link	icinga2Loop					Statement
hi link icinga2Operators			Operator

hi link icinga2AFunction			Function
hi link icinga2MathFunction			Function
hi link icinga2GFunction			Function
hi link icinga2JsonFunction			Function
hi link icinga2StrFunction			Function
hi link icinga2ArrFunction			Function


hi link icinga2Gconst				Statement

hi link valueBoolean				Boolean
hi link valueNull					Special
