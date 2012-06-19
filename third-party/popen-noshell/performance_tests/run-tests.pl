#!/usr/bin/perl
# popen_noshell: A faster implementation of popen() and system() for Linux.
# Copyright (c) 2009 Ivan Zahariev (famzah)
# Version: 1.0
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; under version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses>.
use strict;
use warnings;

use Data::Dumper;

my $count = 10000;
my $memsize = 20;
my $ratio = 2;
my $allowed_deviation = 20; # +/- percent
my $repeat_tests = 3;
my $uname_args = '-s -r -m'; # or just "-a" :)

my $s;
my @lines;
my $line;
my $i;
my $options;
my $caption;
my $user_t;
my $sys_t;
my %results;
my $k;
my $mode;
my $avg_user_t;
my $avg_sys_t;
my $success;
my @sheet = ();

sub parse_die($$) {
	my ($i, $line) = @_;
	die("Unable to parse line $i: $line");
}

if ($repeat_tests < 2) {
	die('repeat_tests must be at least 2');
}

$options = undef;
print "The tests are being performed, this will take some time...\n\n";
for $mode (0..7) {
	print(('-'x80)."\n\n");
	for (1..$repeat_tests) {
		$s = `gcc -Wall fork-performance.c popen_noshell.c -o fork-performance && time ./fork-performance --count=$count --memsize=$memsize --ratio=$ratio --mode=$mode 2>&1 >/dev/null`;
		print "$s\n";

		@lines = split(/\n/, $s);
		$i = 0;
		$caption = $user_t = $sys_t = undef;
		foreach $line (@lines) {
			++$i;
			if ($i == 1) {
				if ($line =~ /^fork-performance: Test options: (.+), mode=\d+$/) {
					if (!defined($options)) {
						$options = $1;
					} else {
						if ($options ne $1) {
							die("Parsed options is not the same: $options vs. $1");
						}
					}
				} else {
					parse_die($i, $line);
				}
			} elsif ($i == 2) {
				if ($line =~ /^fork-performance: (.+)$/) {
					$caption = $1;
				} else {
					parse_die($i, $line);
				}
			} elsif ($i == 3) {
				if ($line =~ /(\d+.\d+)\s?user\s+(\d+.\d+)\s?sys(?:tem)?/) {
					$user_t = $1;
					$sys_t = $2;
				} else {
					parse_die($i, $line);
				}
			} elsif ($i == 4) {
				# noop
			} else {
				parse_die($i, $line);
			}
		}
		if (!defined($options) || !defined($caption) || !defined($user_t) || !defined($sys_t)) {
			die('Parsing failed');
		}
		$k = $caption;
		if (!exists($results{$k})) {
			$results{$k} = [];
		}
		push(@{$results{$k}}, [$user_t, $sys_t]);
	}
}

sub print_deviation($$$) {
	my ($label, $val, $avg_val) = @_;
	my $deviation;
	my $retval = 1;

	if ($avg_val == 0) {
		$deviation = '?? ';
		$retval = 0;
	} else {
		$deviation = (($val / $avg_val) - 1) * 100;
		$deviation = sprintf('%.0f', $deviation);
		if (abs($deviation) > $allowed_deviation) {
			$retval = 0;
		}
	}

	printf("\t%s: %s (%4s%%)%s\n", $label, $val, $deviation, ($retval == 0 ? ' BAD VALUE' : ''));

	return $retval;
}

$success = 1;
print("\n\n".('-'x80)."\n");
print "RAW PERFORMANCE TESTS RESULTS\n";
print(('-'x80)."\n");
foreach $k (keys %results) {
	$avg_user_t = 0;
	$avg_sys_t = 0;
	$i = 0;
	foreach (@{$results{$k}}) {
		($user_t, $sys_t) = @{$_};
		$avg_user_t += $user_t;
		$avg_sys_t += $sys_t;
		++$i;
	}
	if ($i != $repeat_tests) {
		die("Sanity check failed for count: $count vs. $i");
	}
	$avg_user_t /= $i;
	$avg_sys_t  /= $i;
	$avg_user_t = sprintf('%.2f', $avg_user_t);
	$avg_sys_t = sprintf('%.2f', $avg_sys_t);

	$s = sprintf("%s | avg_user_t | %s | avg_sys_t | %s | total_t | %s\n", $k, $avg_user_t, $avg_sys_t, ($avg_user_t + $avg_sys_t));
	push(@sheet, $s);
	print $s;

	foreach (@{$results{$k}}) {
		($user_t, $sys_t) = @{$_};

		$success &= print_deviation('user_t', $user_t, $avg_user_t);
		$success &= print_deviation('sys_t ', $sys_t, $avg_sys_t);
	}
}

print("\n\n".('-'x80)."\n");
print "PERFORMANCE TESTS REPORT\n";
print(('-'x80)."\n");
print "System and setup:\n\t | ".`uname $uname_args`."\t | $options\n";
print "Here is the data for the graphs:\n";
foreach (@sheet) {
	print "\t | ".$_;
}
print(('-'x80)."\n");

if (!$success) {
	print "\nWARNING! Some of the measurements were not accurate enough!\n";
	print "It is recommended that you re-run the test having the following in mind:\n";
	print "\t* the machine must be idle and not busy with other tasks\n";
	print "\t* increase the 'count' to a larger number to have more accurate results\n";
	print "\t* it is recommended that 'count' is so big, that the user/sys average time is at least bigger than 1.00\n";
}
