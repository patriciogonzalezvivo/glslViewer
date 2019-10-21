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

## Output declarations for the _glewInit_[extension] functions defined
## by make_init.pl script.  These are necessary for for initializers to
## call each other, such as a core GL 3 context that depends on certain
## extensions.

#-------------------------------------------------------------------------------

my @extlist = ();
my %extensions = ();

our $type = shift;

if (@ARGV)
{
    @extlist = @ARGV;

	foreach my $ext (sort @extlist)
	{
		my ($extname, $exturl, $extstring, $reuse, $types, $tokens, $functions, $exacts) = 
			parse_ext($ext);

		#print "#ifdef $extname\n\n";
		if (keys %$functions)
		{
			print "static GLboolean _glewInit_$extname ();\n";
		}
		#print "#endif /* $extname */\n\n";
	}
}
