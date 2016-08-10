package require http
package require tls
http::register https 443 [list ::tls::socket -tls1 1]

load /home/gunnar/i2tcl.so i2tcl

bind pub - > i2tcl
bind pub - >u i2tcl_url
bind pub - ^ i2tcl
bind pub - ^u i2tcl_url

if {![info exists ::i2frame]} {
	set ::i2frame [i2_new_frame]
}

set ::i2chan ""
set ::i2nick ""

i2_register_command irc i2_irc null
i2_register_command channels channels list
i2_register_command chanlist internalchanlist list
i2_register_command getnick getcurrentnick string
i2_register_command onchan onchan bool
i2_register_command topic topic string
i2_register_command topicnick topicnick string
i2_register_command topicstamp topicstamp number
i2_register_command chanmodes getchanmode string
i2_register_command isop isop bool
i2_register_command isvoice isvoice bool
i2_register_command ishop ishop bool
i2_register_command chanhost getchanhost string
i2_register_command chanbans chanbans list
i2_register_command getnick i2_getnick string
i2_register_command getchan i2_getchan string
i2_register_command "Internal.run_with_activation_context" i2_null null
i2_register_command exit i2_null null

proc i2_null {} {
}

proc i2_getnick {} {
	global i2nick
	return $i2nick
}

proc i2_getchan {} {
	global i2chan
	return $i2chan
}

proc i2_irc {message} {
	global i2chan

	if {[string first "\n" $message] != -1 || [string first "\r" $message] != -1} {
		return
	}

	putserv "PRIVMSG $i2chan :$message"
}

proc i2tcl {nick host hand chan arg} {
	global i2frame i2chan i2nick

	set i2chan $chan
	set i2nick $nick

	set result [i2_eval $i2frame $arg]

	if {$result == ""} { set result "<no error>" }
	foreach sline [split $result \n] {
		putserv "PRIVMSG $chan :( $arg ) = $sline"
	}
}

proc i2tcl_url {nick host hand chan arg} {
	global i2frame i2chan i2nick

	set i2chan $chan
	set i2nick $nick

	if {[catch {set token [http::geturl $arg]} msg]} {
		putserv "PRIVMSG $chan :HTTP request failed: $msg"
		http::cleanup $token
		return
	}

	if {[http::status $token] != "ok"} {
		putserv "PRIVMSG $chan :HTTP request failed: [http::error $token]"
		http::cleanup $token
		return
	}

	set rpl [split [http::code $token] " "]

	if {[lindex $rpl 1] != 200} {
		putserv "PRIVMSG $chan :HTTP request failed: [join [lrange $rpl 1 end]]"
		http::cleanup $token
		return
	}

	set code [http::data $token]
	http::cleanup $token
	set result [i2_eval $i2frame $code]

	if {$result == ""} { set result "<no error>" }
	foreach sline [split $result \n] {
		putserv "PRIVMSG $chan :( $arg ) = $sline"
	}
}
