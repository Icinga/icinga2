" Vim syntax file
" Filename:		icinga2.vim
" Language:		Icinga2 object configuration file
" Author:		Carlos Cesario <carloscesario@gmail.com>, Michael Friedrich <michael.friedrich@icinga.com>
" Version:		1.0.0
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

" ########################################
" ### General settings

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

syn match		angleBrackets	"<.*>"
syn region		macro		start=+\$+ end=+\$+ oneline
syn region		StringD		start=+"+  end=+"\|$+ contains=macro
syn region		StringS		start=+'+  end=+'\|$+ contains=macro


" Braces and Parens definition
" Braces are used in dictionary definition

syn match		Braces		"[{}\[\]]"
syn match		Parens		"[()]"
syn match 		Lambda		"{{}}"


" ########################################
" ### Match objects, attributes and keywords

" Object types
syn keyword		icinga2ObjType		ApiListener ApiUser CheckCommand CheckerComponent
syn keyword		icinga2ObjType		Comment Dependency Downtime ElasticsearchWriter
syn keyword		icinga2ObjType		Endpoint EventCommand ExternalCommandListener
syn keyword		icinga2ObjType		FileLogger GelfWriter GraphiteWriter Host HostGroup
syn keyword		icinga2ObjType		IcingaApplication IdoMysqlConnection IdoPgsqlConnection
syn keyword		icinga2ObjType		InfluxdbWriter Influxdb2Writer LivestatusListener Notification NotificationCommand
syn keyword		icinga2ObjType		NotificationComponent OpenTsdbWriter PerfdataWriter
syn keyword		icinga2ObjType		ScheduledDowntime Service ServiceGroup SyslogLogger
syn keyword		icinga2ObjType		TimePeriod User UserGroup WindowsEventLogLogger Zone

" Object/Template marker (simplified)
syn match		icinga2ObjDef		"\(object\|template\)[ \t]\+.*"

" Apply rules
syn match		icinga2ApplyDef		"apply[ \t]\+\(Service\|Dependency\|Notification\|ScheduledDowntime\)"


" Objects attributes
"
" find . -type f -name '*.ti' -exec sh -c 'grep config {}' \;
" Don't add 'host', etc. from apply rules here, they should match icinga2ObjType instead.
syn keyword		icinga2ObjAttr		contained	accept_commands accept_config access_control_allow_origin action_url address address6 arguments author bind_host
syn keyword		icinga2ObjAttr		contained	bind_port ca_path categories cert_path check_command check_interval
syn keyword		icinga2ObjAttr		contained	check_period check_timeout child_host_name child_options child_service_name cipher_list
syn keyword		icinga2ObjAttr		contained	cleanup client_cn command command_endpoint command_path
syn keyword		icinga2ObjAttr		contained	comment compat_log_path concurrent_checks crl_path database disable_checks disable_notifications
syn keyword		icinga2ObjAttr		contained	display_name duration email enable_active_checks enable_event_handlers enable_event_handler
syn keyword		icinga2ObjAttr		contained	enable_flapping enable_ha enable_host_checks enable_notifications enable_passive_checks enable_perfdata
syn keyword		icinga2ObjAttr		contained	enable_send_metadata enable_send_perfdata enable_send_thresholds enable_tls enable_service_checks
syn keyword		icinga2ObjAttr		contained	endpoints env event_command excludes failover_timeout fixed flapping_threshold_low flapping_threshold_high
syn keyword		icinga2ObjAttr		contained	flush_interval flush_threshold global groups
syn keyword		icinga2ObjAttr		contained	host_format_template host_name host_name_template host_perfdata_path host_temp_path host_template icon_image
syn keyword		icinga2ObjAttr		contained	icon_image_alt ignore_soft_states includes index instance_description instance_name interval key_path log_dir
syn keyword		icinga2ObjAttr		contained	log_duration max_anonymous_clients max_check_attempts methods name notes notes_url objects_path
syn keyword		icinga2ObjAttr		contained	pager parent parent_host_name parent_service_name password path period permissions
syn keyword		icinga2ObjAttr		contained	port prefer_includes ranges retry_interval rotation_interval rotation_method
syn keyword		icinga2ObjAttr		contained	service_format_template service_name service_name_template service_perfdata_path service_temp_path service_template
syn keyword		icinga2ObjAttr		contained	severity socket_path socket_type source spool_dir
syn keyword		icinga2ObjAttr		contained	ssl_ca ssl_capath ssl_ca_cert ssl_cert ssl_cipher ssl_enable ssl_mode ssl_key
syn keyword		icinga2ObjAttr		contained	states status_path table_prefix ticket_salt
syn keyword		icinga2ObjAttr		contained	timeout times tls_handshake_timeout tls_protocolmin
syn keyword		icinga2ObjAttr		contained	types update_interval user user_groups username users volatile zone
syn match		icinga2ObjAttr		contained	"\(vars.\w\+\)"

" keywords: https://icinga.com/docs/icinga2/latest/doc/17-language-reference/#reserved-keywords
syn keyword		icinga2Keyword		object template include include_recursive include_zones library
syn keyword		icinga2Keyword		const var this globals locals use default ignore_on_error
syn keyword		icinga2Keyword		current_filename current_line apply to where import assign
syn keyword		icinga2Keyword		ignore function return in


" Assign conditions
syn match		icinga2AssignCond	contained	"\(assign[ \t]\+\where\|ignore[ \t]\+\where\)"


" Documentation reference: https://icinga.com/docs/icinga2/latest/doc/18-library-reference/

" Global functions
syn keyword	icinga2GFunction	contained	regex match cidr_match range len union intersection keys string
syn keyword 	icinga2GFunction 	contained	number bool random log typeof get_time parse_performance_data dirname
syn keyword 	icinga2GFunction 	contained	basename path_exists glob glob_recursive
syn keyword 	icinga2GFunction 	contained	escape_shell_arg escape_shell_cmd escape_create_process_arg sleep exit
syn keyword 	icinga2GFunction 	contained	macro


" Accessor Functions
syn keyword 	icinga2AFunction 	contained	get_check_command get_event_command get_notification_command
syn keyword 	icinga2AFunction 	contained	get_host get_service get_services get_user
syn keyword 	icinga2AFunction 	contained	get_host_group get_service_group get_user_group
syn keyword 	icinga2AFunction 	contained	get_timeperiod
syn keyword 	icinga2AFunction 	contained	get_object get_objects

" Math functions
syn match 	icinga2MathFunction 	contained	"\(Math.E\|Math.LN2\|Math.LN10\|Math.LOG2E\|Math.PI\|Math.SQRT1_2\|Math.SQRT2\)"
syn match 	icinga2MathFunction 	contained	"\(Math.abs\|Math.acos\|Math.asin\|Math.atan\|Math.atan2\|Math.ceil\|Math.cos\)"
syn match 	icinga2MathFunction 	contained	"\(Math.exp\|Math.floor\|Math.isinf\|Math.isnan\|Math.log\|Math.max\|Math.min\)"
syn match 	icinga2MathFunction  	contained 	"\(Math.pow\|Math.random\|Math.round\|Math.sign\|Math.sin\|Math.sqrt\|Math.tan\)"

" Json functions
syn match 	icinga2JsonFunction	contained	"\(Json.encode\|Json.decode\)"

" Number functions
syn match	icinga2NumberFunction	contained	"\(\.to_string\)"

" Boolean functions
syn match	icinga2BoolFunction	contained	"\(\.to_string\)"

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
syn match 	icinga2StrFunction	contained 	"\(\.trim\)"

" Object functions
syn match	icinga2ObjectFunction	contained	"\(\.clone\)"
syn match	icinga2ObjectFunction	contained	"\(\.to_string\)"
syn match	icinga2ObjectFunction	contained	"\(\.type\)"

" Type functions
syn match	icinga2TypeFunction	contained	"\(\.base\)"
" needs an exception for 'vars'
syn match	icinga2TypeFunction	contained	"\(^[vars]\.name\)"
syn match	icinga2TypeFunction	contained	"\(\.prototype\)"

" Array functions
syn match 	icinga2ArrFunction 	contained 	"\(\.add(\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.clear\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.shallow_clone\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.contains\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.freeze\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.len\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.remove\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.set\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.get\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.sort\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.join\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.reverse\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.map\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.reduce\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.filter\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.any\)"
syn match 	icinga2ArrFunction 	contained 	"\(\.all\)"

" Dictionary functions
syn match 	icinga2DictFunction 	contained 	"\(\.shallow_clone\)"
syn match 	icinga2DictFunction 	contained 	"\(\.contains\)"
syn match 	icinga2DictFunction 	contained 	"\(\.freeze\)"
syn match 	icinga2DictFunction 	contained 	"\(\.len\)"
syn match 	icinga2DictFunction 	contained 	"\(\.remove\)"
syn match 	icinga2DictFunction 	contained 	"\(\.set\)"
syn match 	icinga2DictFunction 	contained 	"\(\.get\)"
syn match 	icinga2DictFunction 	contained 	"\(\.keys\)"
syn match 	icinga2DictFunction 	contained 	"\(\.values\)"

" Function functions
syn match 	icinga2FuncFunction 	contained 	"\(\.call\)"
syn match 	icinga2FuncFunction 	contained 	"\(\.callv\)"

" DateTime functions
syn match 	icinga2DTFunction 	contained 	"\(DateTime\)"
syn match 	icinga2DTFunction 	contained 	"\(\.format\)"
syn match 	icinga2DTFunction 	contained 	"\(\.to_string\)"

" Conditional statements
syn keyword 	icinga2Cond			if else

" Loops
syn keyword 	icinga2Loop			while for break continue

" Exceptions
syn keyword	icinga2Exception		throw try except

" Debugger
syn keyword 	icinga2Debugger			debugger

" References
syn keyword	icinga2Reference		& *
" Namespace
syn keyword	icinga2Namespace		namespace using

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


" ########################################
" ### Global settings

" Global constants
" https://icinga.com/docs/icinga2/snapshot/doc/17-language-reference/#icinga-2-specific-constants

" Path specific constants
syn keyword 	icinga2PathConstant	CacheDir ConfigDir DataDir IncludeConfDir InitRunDir LocalStateDir LogDir ModAttrPath
syn keyword	icinga2PathConstant	ObjectsPath PidPath PkgDataDir PrefixDir ProgramData RunDir SpoolDir StatePath SysconfDir
syn keyword 	icinga2PathConstant	VarsPath ZonesDir

" Global constants
syn keyword 	icinga2GlobalConstant	Vars NodeName Environment RunAsUser RunAsGroup MaxConcurrentChecks ApiBindHost ApiBindPort EventEngine AttachDebugger

" Application runtime constants
syn keyword	icinga2GlobalConstant	PlatformName PlatformVersion PlatformKernel PlatformKernelVersion BuildCompilerName BuildCompilerVersion BuildHostName
syn keyword	icinga2GlobalConstant	ApplicationVersion

" User proposed constants
syn keyword	icinga2UserConstant	PluginDir ContribPluginContribDir ManubulonPluginDir TicketSalt NodeName ZoneName

" Global types
syn keyword 	icinga2GlobalType	Number String Boolean Array Dictionary Value Object ConfigObject Command CheckResult
syn keyword     icinga2GlobalType	Checkable CustomVarObject DbConnection Type PerfdataValue Comment Downtime Logger Application

" Built-in Namespaces
syn match	icinga2Namespace	contained "\(Icinga\.\|Internal\.\|System\.Configuration\.\|System\.\)"

" Additional constants from Namespaces
syn keyword	icinga2GlobalConstant	LogCritical LogDebug LogInformation LogNotice LogWarning MatchAll MatchAny
" icinga2 console; keys(Icinga).join(" ")
syn keyword 	icinga2GlobalConstant	Acknowledgement Critical Custom DbCatAcknowledgement DbCatCheck DbCatComment DbCatConfig DbCatDowntime DbCatEventHandler DbCatEverything DbCatExternalCommand DbCatFlapping DbCatLog DbCatNotification DbCatProgramStatus DbCatRetention DbCatState DbCatStateHistory Down DowntimeEnd DowntimeNoChildren DowntimeNonTriggeredChildren DowntimeRemoved DowntimeStart DowntimeTriggeredChildren FlappingEnd FlappingStart HostDown HostUp OK Problem Recovery ServiceCritical ServiceOK ServiceUnknown ServiceWarning Unknown Up Warning

" Value types
"syn match		valueNumber	"[0-9]*"
syn keyword		valueBoolean		contained	true false
syn keyword		valueNull		contained	null


" ########################################
" ### Where to apply

syn region		icingaDefBody start='{' end='}'
	\ contains=icinga2Comment, icinga2LineComment, StringS, StringD, macro, Braces, Parens, Lambda, icinga2ObjType, icinga2ObjDef,
	\ icinga2ApplyDef, icinga2ObjAttr, icinga2Keyword, icinga2AssignCond,
	\ icinga2Cond, icinga2Loop, icinga2Exception, icinga2Debugger, icinga2Operators, icinga2GFunction, icinga2AFunction,
	\ icinga2MathFunction, icinga2GlobalConstant, icinga2PathConstant, icinga2UserConstant, icinga2Gconst, icinga2Namespace,
	\ icinga2JsonFunction, icinga2NumberFunction, icinga2BoolFunction,
	\ icinga2StrFunction, icinga2ObjectFunction, icinga2TypeFunction, icinga2ArrFunction, icinga2DictFunction,
	\ icinga2DTFunction, valueNumber, valueBoolean, valueNull


" ########################################
" ### Highlighting
hi link icinga2Comment				Comment
hi link icinga2LineComment			Comment
hi link icinga2CommentTodo			Todo

hi link Braces					Function
hi link Parens					Function
hi link Lambda					Function

hi link	macro					Underlined
hi link StringS					String
hi link StringD					String
hi link angleBrackets				String

hi link icinga2ObjType				Type
hi link icinga2ObjDef				Statement

hi link icinga2ApplyDef				Statement
hi link icinga2ObjAttr				Define
hi link	icinga2Keyword				Keyword

hi link	icinga2AssignCond			Conditional

hi link	icinga2Cond				Conditional
hi link	icinga2Loop				Repeat
hi link icinga2Exception			Conditional
hi link icinga2Debugger				Debug

hi link icinga2Operators			Operator

hi link icinga2AFunction			Function
hi link icinga2MathFunction			Function
hi link icinga2GFunction			Function
hi link icinga2JsonFunction			Function
hi link icinga2NumberFunction			Function
hi link icinga2BoolFunction			Function
hi link icinga2StrFunction			Function
hi link icinga2ObjectFunction			Function
hi link icinga2TypeFunction			Function
hi link icinga2ArrFunction			Function
hi link icinga2DictFunction			Function
hi link icinga2DTFunction			Function

hi link icinga2GlobalConstant			Statement
hi link icinga2PathConstant			Statement
hi link icinga2UserConstant			Statement
hi link icinga2Gconst				Statement
hi link icinga2Namespace			Statement

hi link valueNumber				Number
hi link valueBoolean				Boolean
hi link valueNull				Special
