#!/usr/bin/perl
##
## Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
## Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
##
## This program is distributed under the terms and conditions of the GNU
## General Public License Version 2 as published by the Free Software
## Foundation or, at your option, any later version.

use strict;
use warnings;

use lib '.';
do 'bin/make.pl';

#---------------------------------------------------------------------------------------
# Extensions that depend on others can be enabled once we know
# if the one it depends on, is enabled.

my @extlist = ();
my %extensions = ();

if (@ARGV)
{
    @extlist = @ARGV;

	foreach my $ext (sort @extlist)
	{
		my ($extname, $exturl, $extstring, $reuse, $types, $tokens, $functions, $exacts) = parse_ext($ext);

		if ($extname ne $extstring && length($extstring))
		{
			my $extvar = $extname;
			$extvar =~ s/GL(X*)_/GL$1EW_/;

			my $parent = $extstring;
			$parent =~ s/GL(X*)_/GL$1EW_/;

			print "#ifdef $extname\n";
			print "  $extvar = $parent;\n";
			print "#endif /* $extname */\n";
		}
	}

}
