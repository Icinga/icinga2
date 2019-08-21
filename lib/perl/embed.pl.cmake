 package Embed::Persistent;

# embed.pl for Icinga

use strict;

use Text::ParseWords qw(shellwords);

# Logging
use constant LEAVE_MSG  	=> 1;
use constant CACHE_DUMP		=> 2;
use constant PLUGIN_DUMP	=> 4;
use constant DEBUG_LEVEL	=> 0;
# use constant DEBUG_LEVEL	=> CACHE_DUMP;
# use constant DEBUG_LEVEL	=> LEAVE_MSG;
# use constant DEBUG_LEVEL	=> LEAVE_MSG | CACHE_DUMP;
# use constant DEBUG_LEVEL	=> LEAVE_MSG | CACHE_DUMP | PLUGIN_DUMP;

use constant DEBUG_LOG_PATH	=> "@ICINGA2_FULL_LOGDIR@/";
use constant LEAVE_MSG_STREAM	=> DEBUG_LOG_PATH . 'epn_leave-msgs.log';
use constant CACHE_DUMP_STREAM	=> DEBUG_LOG_PATH . 'epn_cache-dump.log';
use constant PLUGIN_DUMP_STREAM	=> DEBUG_LOG_PATH . 'epn_plugin-dump.log';

# Caching
use constant NUMBER_OF_PERL_PLUGINS	=> 60;

# Cache will be dumped every Cache_Dump_Interval plugin compilations
use constant Cache_Dump_Interval	=> 20;

# Setup loggers
(DEBUG_LEVEL & LEAVE_MSG) && do	{
	open LH, '>> ' . LEAVE_MSG_STREAM or die "Can't open " . LEAVE_MSG_STREAM . ": $!";

	# Unbuffer LH since this will be written by child processes.
	select( (select(LH), $| = 1)[0] );
};
(DEBUG_LEVEL & CACHE_DUMP) && do {
	(open CH, '>> ' . CACHE_DUMP_STREAM or die "Can't open " . CACHE_DUMP_STREAM . ": $!");
	select( (select(CH), $| = 1)[0] );
} ;
(DEBUG_LEVEL & PLUGIN_DUMP) && (open PH, '>> ' . PLUGIN_DUMP_STREAM	or die "Can't open " . PLUGIN_DUMP_STREAM . ": $!");

require Data::Dumper if DEBUG_LEVEL & CACHE_DUMP;

# Setup caching
my (%Cache, $Current_Run);
keys %Cache = NUMBER_OF_PERL_PLUGINS;

# Offsets in %Cache{$filename}
use constant MTIME		=> 0;
use constant PLUGIN_ARGS	=> 1;
use constant PLUGIN_ERROR	=> 2;
use constant PLUGIN_HNDLR	=> 3;

####################################################################
# PACKAGE main
####################################################################
package main;

use subs 'CORE::GLOBAL::exit';

sub CORE::GLOBAL::exit { die "ExitTrap: $_[0] (Redefine exit to trap plugin exit with eval BLOCK)" }

####################################################################
# PACKAGE OutputTrap
####################################################################
package OutputTrap;

# Methods for use by tied STDOUT in embedded PERL module.
# Simply ties STDOUT to a scalar and caches values written to it.
#
# Note: No more than 4KB characters per line are kept.
# TODO: Should we enforce this limit with Icinga 2?

sub TIEHANDLE {
	my ($class) = @_;
	my $me = '';
	bless \$me, $class;
}

sub PRINT {
	my $self = shift;

	$$self .= substr(join('',@_), 0, 4096);
}

sub PRINTF {
	my $self = shift;
	my $fmt = shift;

	$$self .= substr(sprintf($fmt,@_), 0, 4096);
}

sub READLINE {
	my $self = shift;
	return $$self;
}

sub CLOSE {
	my $self = shift;
	undef $self ;
}

sub DESTROY {
	my $self = shift;
	undef $self;
}

####################################################################
# PACKAGE Embed::Persistent
####################################################################
package Embed::Persistent;

####################################
# valid_package_name
####################################
sub valid_package_name {
	local $_ = shift ;
	s|([^A-Za-z0-9\/])|sprintf("_%2x",unpack("C",$1))|eg;

	# second pass only for words starting with a digit
	s|/(\d)|sprintf("/_%2x",unpack("C",$1))|eg;

	# Dress it up as a real package name
	s|/|::|g;
	return /^::/ ? "Embed$_" : "Embed::$_";
}

####################################
# throw_exception
####################################
# Perl 5.005_03 only traps warnings for errors classed by perldiag
# as Fatal (eg 'Global symbol """"%s"""" requires explicit package name').
# Therefore treat all warnings as fatal.

sub throw_exception {
	die shift;
}

####################################
# eval_file
####################################
sub eval_file {
	my ($filename, $delete, undef, $plugin_args) = @_;

	my $mtime = -M $filename;
	my $ts = localtime(time()) if DEBUG_LEVEL;

	if (exists($Cache{$filename}) &&
		$Cache{$filename}[MTIME] &&
		$Cache{$filename}[MTIME] <= $mtime)
	{
		# The plugin was compiled before and it did not
		# change on disk.
		#
		# Only update the following:
		# - Parse the plugin arguments and cache them again, saving repeated parsing.
		#   This ensures that the same plugin works with different command arguments.
		# - Return errors from former compilations if any.

		if ($plugin_args) {
			$Cache{$filename}[PLUGIN_ARGS]{$plugin_args} ||= [ shellwords($plugin_args) ];
		}

		if ($Cache{$filename}[PLUGIN_ERROR]) {
			if (DEBUG_LEVEL & LEAVE_MSG) {
				print LH qq($ts eval_file: $filename failed compilation formerly and file has not changed; skipping compilation.\n);
			}

			die qq(**Embedded Perl failed to compile $filename: "$Cache{$filename}[PLUGIN_ERROR]");
		} else {
			if (DEBUG_LEVEL & LEAVE_MSG) {
				print LH qq($ts eval_file: $filename already successfully compiled and file has not changed; skipping compilation.\n);
			}
			return $Cache{$filename}[PLUGIN_HNDLR];
		}
	}

	my $package = valid_package_name($filename);

	if ($plugin_args) {
		$Cache{$filename}[PLUGIN_ARGS]{$plugin_args} ||= [ shellwords($plugin_args) ];
	}

	local *FH;

	# die must be trapped by caller with ERRSV
	open FH, $filename or die qq(**Embedded Perl failed to open "$filename": "$!");

	# Create subroutine
	my $sub;
	sysread FH, $sub, -s FH;
	close FH;

	# Wrap the code into a subroutine inside our unique package
	# (using $_ here [to save a lexical] is not a good idea since
	# the definition of the package is visible to any other Perl
	# code that uses [non localised] $_).

	# Note keep the variable name the same as the sub name, used in eval() below.
	my $hndlr = <<EOSUB;
package $package;

sub hndlr {
	\@ARGV = \@_;
	local \$^W = 1;

	\$ENV{ICINGA_PLUGIN} = '$filename';
	# Keep this for compatibility with check_*_health plugins
	\$ENV{NAGIOS_PLUGIN} = '$filename';

# <<< START of PLUGIN (first line of plugin is line 10 in the text) >>>
$sub
# <<< END of PLUGIN >>>
}
EOSUB

	$Cache{$filename}[MTIME] = $mtime unless $delete;

	# Suppress warning display.
	local $SIG{__WARN__} = \&throw_exception;

	# Fix problem where modified Perl plugins didn't get recached by the Embedded Perl Interpreter
	no strict 'refs';
	undef %{$package.'::'};
	use strict 'refs';

	# Compile &$package::hndlr. Since non executable code is being eval'd
	# there is no need to protect lexicals in this scope.
	eval $hndlr;

	# $@ is set for any warning and error.
	# This guarantees that the plugin will not be run.
	if ($@) {
		# Report error line number to original plugin text (9 lines added by eval_file).
		# Error text looks like
		# 'Use of uninitialized ..' at (eval 23) line 186, <DATA> line 218
		# The error line number is 'line 186'
		chomp($@);
		$@ =~ s/line (\d+)[\.,]/'line ' . ($1 - 9) . ','/e;

		if (DEBUG_LEVEL & LEAVE_MSG) {
			print LH qq($ts eval_file: syntax error in $filename: "$@".\n);
		}

		if (DEBUG_LEVEL & PLUGIN_DUMP) {
			my $i = 1;
			$_ = $hndlr; # Defined above, the subroutine.
			s/^/sprintf('%10d  ', $i++)/meg ;
			# Will only get here once (when a faulty plugin is compiled).
			# Therefore only _faulty_ plugins are dumped once each time the text changes.

			print PH qq($ts eval_file: transformed plugin "$filename" to ==>\n$_\n);
		}

		if (length($@) > 4096) {
			$@ = substr($@, 0, 4096);
		}

		$Cache{$filename}[PLUGIN_ERROR] = $@;

		# If the compilation fails, leave nothing behind that may affect subsequent
		# compilations. This must be trapped by caller with ERRSV.
		die qq(**Embedded Perl failed to compile $filename: "$@");

	} else {
		$Cache{$filename}[PLUGIN_ERROR] = '';
	}

	if (DEBUG_LEVEL & LEAVE_MSG) {
		print LH qq($ts eval_file: successfully compiled "$filename $plugin_args".\n);
	}

	if ((DEBUG_LEVEL & CACHE_DUMP) && (++$Current_Run % Cache_Dump_Interval == 0)) {
		print CH qq($ts eval_file: after $Current_Run compilations \%Cache =>\n), Data::Dumper->Dump([\%Cache], [qw(*Cache)]), "\n";
	}

	no strict 'refs';

	# Set the registered hdler sub routine - note the same name as above's subroutine.
	return $Cache{$filename}[PLUGIN_HNDLR] = *{ $package . '::hndlr' }{CODE};
}

####################################
# run_package
####################################
sub run_package {
	# Second param (after $filename) _may_ be used to wallop stashes.
	my ($filename, undef, $plugin_hndlr_cr, $plugin_args) = @_;

	my $has_exit = 0;
	my $res	= 3;
	my $ts = localtime(time()) if DEBUG_LEVEL;

	# Treat Perl warnings as fatal error.
	local $SIG{__WARN__} = \&throw_exception;

	# Tie stdout to sanitize handlers
	my $stdout = tie (*STDOUT, 'OutputTrap');

	# Read cached plugin arguments
	my @plugin_args	= $plugin_args ? @{ $Cache{$filename}[PLUGIN_ARGS]{$plugin_args} } : ();

	# If the plugin has args, they have been cached by eval_file.
	# (cannot cache @plugin_args here because run_package() is
	# called by child processes so cannot update %Cache.)

	eval { $plugin_hndlr_cr->(@plugin_args) };

	if ($@) {
		# Error => normal plugin termination (exit) || run time error.
		$_ = $@;

		/^ExitTrap: (-?\d+)/ ? do { $has_exit = 1; $res = $1 } :
		# For normal plugin exit, $@ will always match /^ExitTrap: (-?\d+)/
		/^ExitTrap:  / ? do { $has_exit = 1; $res = 0 } : do {
			# Run time error/abnormal plugin termination.
			chomp;

			# Report error line number to original plugin text (9 lines added by eval_file).
			s/line (\d+)[\.,]/'line ' . ($1 - 9) . ','/e;
			print STDOUT qq(**Embedded Perl $filename: "$_".\n);
		};

		($@, $_) = ('', '');
	}

	# ! Error => Perl code is not a plugin (fell off the end; no exit)

	# !! (read any output from the tied file handle.)
	my $plugin_output = <STDOUT>;

	undef $stdout;
	untie *STDOUT;

	if ($has_exit == 0) {
		$plugin_output = "**Embedded Perl $filename: plugin did not call exit()\n" . $plugin_output;
	}

	if (DEBUG_LEVEL & LEAVE_MSG) {
		print LH qq($ts run_package: "$filename $plugin_args" returning ($res, "$plugin_output").\n);
	}

	return ($res, $plugin_output);
}

1;

=head1 AUTHOR

Originally by Stephen Davies.

Now maintained by Stanley Hopcroft <hopcrofts@cpan.org> who retains responsibility for the 'bad bits'.

=head1 COPYRIGHT

Copyright (c) 2019 Icinga GmbH
Copyright (c) 2009 Nagios Development Team
Copyright (c) 2004 Stanley Hopcroft. All rights reserved.

This program is free software; you can redistribute it and/or modify it under the same terms as Perl itself.

=cut

