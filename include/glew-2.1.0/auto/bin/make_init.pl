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

#-------------------------------------------------------------------------------

# function pointer definition
sub make_pfn_def_init($%)
{
    #my $name = prefixname($_[0]);
    return "  r = ((" . $_[0] . " = (PFN" . (uc $_[0]) . "PROC)glewGetProcAddress((const GLubyte*)\"" . $_[0] . "\")) == NULL) || r;";
}

sub make_reuse_call($%)
{
	return "  r = _glewInit_" . $_[0] . "() || r;";
}

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

		#make_separator($extname);
		my $extvar = $extname;
		my $extvardef = $extname;
		$extvar =~ s/GL(X*)_/GL$1EW_/;
		if (keys %$functions or keys @$reuse)
		{
			print "#ifdef $extname\n\n";
			print "static GLboolean _glewInit_$extname ()\n{\n  GLboolean r = GL_FALSE;\n";
			output_reuse($reuse, \&make_reuse_call);
			output_decls($functions, \&make_pfn_def_init);
			print "\n  return r;\n}\n\n";
			print "#endif /* $extname */\n\n";
		}
	}
}
