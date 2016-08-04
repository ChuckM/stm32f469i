#!/usr/bin/env perl
#
# This is a hack to pull out the initialization of the OTM8009A
# display from the source code provided in the STM32Cube release.
# By extracting is programatically you avoid potential transcription
# errors and you can update the extraction pretty easily.
#
use strict;
use warnings;

my %short_cmds;
my %long_cmds;
my %chip_cmds;

#
# Extract values for the various OTM8009A constants
#
open (my $fh, "<", "otm8009a.h") or die;
while (my $line = <$fh>) {
	chomp($line);
	if ($line =~ /(OTM8009A[A-Z0-9_]+)\s+(0x[0-9A-Fa-f][0-9A-Fa-f])/) {
		$chip_cmds{$1} = $2;
	}
}
close $fh;

my @cmds;

#
# Extract the const arrays from the initialization code
# and put them into hashes.
#
# Extract the write commands from the code and generate an
# ordered list of operations.
#
open ($fh, "<", "otm8009a.c") or die;
while (my $line = <$fh>) {
	chomp($line);
	if ($line =~ /IO_WriteCmd\(\s*\d+/) {
		my ($is_short) = $line =~ /WriteCmd\(\s*(\d+)\s*\,/;
		my ($do_cmd) = $line =~ /((?:Short|lcd)RegData\d+)/;
		die "Can't find short in : $line\n" if not defined $is_short;
		if ($is_short eq "0") {
			my $new_cmd = sprintf("\t2, %s, /* S: %s */\n", join(", ", @{$short_cmds{$do_cmd}}), $do_cmd);
			push @cmds, $new_cmd;
		} else {
			my $len = scalar @{$long_cmds{$do_cmd}};
			my @cmd_copy = @{$long_cmds{$do_cmd}};
			# perl trick to put the last byte on the front
			unshift @cmd_copy, pop @cmd_copy;
			my $new_cmd = sprintf("\t%d, %s, /* L: %s */\n", $len, join(", ", @cmd_copy), 
					$do_cmd);
			push @cmds, $new_cmd;
		}
	}
	if ($line =~ /ShortRegData\d+/) {
		next if ($line !~ /^const uint8_t/);
		if ($line =~ /\s+(ShortRegData\d+)\[\].*\{([^}]+)\}/) {
##			print "Extract $1 => $2\n";
			my $bytes = $2;
			my $key = $1;
			$bytes =~ s/,//g;
			my $elements = [];
			while ($bytes !~ /^[\s,]*$/) {
				my ($datum) = $bytes =~ /^\s*((?:0x[0-9A-Fa-f][0-9A-Fa-f]|O[A-Z0-9_]+))\s*/;
				die "Got no datum from $bytes\n" if not defined $datum;
				my $res = $datum;
				$res = $chip_cmds{$datum} if ($datum !~ /0x/);
##				print "   map $datum => $chip_cmds{$datum}\n" if ($res ne $datum);;
				push @{$elements}, $res;
				$bytes =~ s/$datum//;
			}
			$short_cmds{$key} = $elements;
		}
	}
	if ($line =~ /lcdRegData\d+/) {
		next if ($line !~ /^const uint8_t/);
		if ($line =~ /\s+(lcdRegData\d+)\[\].*\{([^}]+)\}/) {
##			print "Extract $1 => $2\n";
			my $bytes = $2;
			my $key = $1;
			$bytes =~ s/,//g;
			my $elements = [];
			while ($bytes !~ /^[\s,]*$/) {
				my ($datum) = $bytes =~ /^\s*((?:0x[0-9A-Fa-f][0-9A-Fa-f]|O[A-Z0-9_]+))\s*/;
				die "Got no datum from $bytes\n" if not defined $datum;
				my $res = $datum;
				$res = $chip_cmds{$datum} if ($datum !~ /0x/);
##				print "   map $datum => $chip_cmds{$datum}\n" if ($res ne $datum);;
				push @{$elements}, $res;
				$bytes =~ s/$datum//;
			}
			$long_cmds{$key} = $elements;
		}
	}
	if ($line =~ /IO_Delay/) {
		my ($delay) = $line =~ /IO_Delay\((\d+)\)/;
		push @cmds, "\t1, $delay, /* D: $delay */\n"
	}
	if ($line =~ /IO_WriteCmd\(\s*\d+/) {
		my ($is_short) = $line =~ /WriteCmd\(\s*(\d+)\s*\,/;
		my ($do_cmd) = $line =~ /((?:Short|lcd)RegData\d+)/;
		die "Can't find short in : $line\n" if not defined $is_short;
		if ($is_short eq "0") {
			push @cmds, 
				"\t2, " . join(", ", $short_cmds{$do_cmd}) . ", /* S: @{$short_cmds{$do_cmd}} */\n";
		} else {
			push @cmds,
				"\tn, " . join(", ", $long_cmds{$do_cmd}) . ", /* S: @{$long_cmds{$do_cmd}} */\n";
		}
	}
}
close($fh);
foreach my $ll (@cmds) {
	print "$ll";
}
