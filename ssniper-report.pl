#!/usr/bin/perl
# 
# Copyright © 2007, The Board of Trustees of the University of Illinois.
# 
# This file is part of SSNiper.  
# 
# SSNiper is open source software, released under the University of
# Illinois/NCSA Open Source License.  You should have received a copy of
# this license in a file with the SSNiper distribution.
# 
# ----------------------------------------------------------------------
# 
# This is a traditional "ugly" Perl script.  Apologies, it's just a
# report generator.
#
# Author:  Joshua Stone
# Contact: joshs@uiuc.edu
# Date:    Nov 13, 2007
#
#

sub banner
{
    print "----------------------------------------------------------------------\n";
    print "          ssniper-report.pl -- SSN Scan Report Generator\n";
    print "                 by Joshua Stone (joshs\@uiuc.edu)\n";
    print "----------------------------------------------------------------------\n";

    print <<EOF;

This tool produces a report of results from scanning a filesystem with
the ssniper SSN scanner.  This report will aggregate file hits in
order of likely importance.  A high-risk file contains relatively many
possible SSNs.  A low-risk file, conversely, will contain relatively
few.

The report sections below are broken out to help you review them
efficiently.  A report section that appears higher in this report is
worth more investigation.  The last few sections should only be
cursorily reviewed -- they represent files that are unlikely to
contain lots of real SSNs.

If you feel that there is a truly inordinate number of false positives
(such as the high-risk sections containing hundreds or thousands of
false positives), please contact the author (contact info above) to
help improve this tool.

EOF
}

sub print_results
{
    my ($header, @results) = @_;
    
    if($#results > 0)
    {
	print $header . "\n";
	print "-" x length($header) . "\n";
	
	foreach $i (@results)
	{
	    @fields = @{$i};
	    if($fields[0] ne "")
	    {
		printf("% 6d  %s\n", $fields[0], $fields[1]);
	    }
	}
	print "\n";
    }
}

@highpass = [];
@lowpass = [];
@delimited_h = [];
@delimited_l = [];
@mixed_h = [];
@mixed_l = [];
@nondelimited_h = [];
@nondelimited_l = [];
@unlikely = [];

while(<>)
{
    $i = $_;
    $i =~ /(.*)?\(ratio \%\)\s*(.*)/;

    $fields = $1;
    $filename = $2;

    $fields =~ s/\(.*?\)//g;        
    @fs = split(/\s+/, $fields);
    $delim = $fs[1];
    $nondelim = $fs[2];
    $false = $fs[4];
    $ratio = $fs[5];

    if($ratio > 40)
    {
	push(@highpass, $i);

	if($filename !~ /,v$/)
	{
	    if($delim ne "" && $nondelim ne "")
	    {
		if($delim > 0 && $nondelim == 0)
		{
		    if($delim > 10)
		    {
			push(@delimited_h, [$delim, $filename]);
		    }
		    else
		    {
			push(@delimited_l, [$delim, $filename]);
		    }
		}
		elsif($delim > 0 && $nondelim > 0)
		{
		    if(($delim + $nondelim) > 20)
		    {
			push(@mixed_h, [$delim + $nondelim, $filename]);
		    }
		    else
		    {
			push(@mixed_l, [$delim + $nondelim, $filename]);
		    }
		}
		elsif($nondelim > 0)
		{
		    if($nondelim > 50)
		    {
			push(@nondelimited_h, [$nondelim, $filename]);
		    }
		    else
		    {
			push(@nondelimited_l, [$nondelim, $filename]);
		    }
		}
	    }
	}
    }
    else
    {
	push(@lowpass, $i);
	push(@unlikely, $i);
    }
}

@mixed_h        = reverse(sort({@{$a}[0] <=> @{$b}[0]}  @mixed_h));
@delimited_h    = reverse(sort({@{$a}[0] <=> @{$b}[0]}  @delimited_h));
@nondelimited_h = reverse(sort({@{$a}[0] <=> @{$b}[0]}  @nondelimited_h));
@delimited_l    = reverse(sort({@{$a}[0] <=> @{$b}[0]}  @delimited_l));
@nondelimited_l = reverse(sort({@{$a}[0] <=> @{$b}[0]}  @nondelimited_l));
@mixed_l        = reverse(sort({@{$a}[0] <=> @{$b}[0]}  @mixed_l));

banner();

print "Report Summary:\n";
print "---------------\n";
printf("% 6d High risk files with delimited and undelimited 9-digit numbers\n", $#mixed_h       );
printf("% 6d High risk files for delimited 9-digit numbers\n"                 , $#delimited_h   );
printf("% 6d High risk files with only non-delimited 9-digit numbers\n"       , $#nondelimited_h);
printf("       -----------------------------------------------------------\n");
printf("% 6d Low risk files with delimited and undelimited 9-digit numbers\n" , $#mixed_l       );
printf("% 6d Low risk files for delimited 9-digit numbers\n"                  , $#delimited_l   );
printf("% 6d Low risk files with only non-delimited 9-digit numbers\n"        , $#nondelimited_l);
print "\n";

print_results("High Risk of Both Delimited And Non_Delimited SSNs:", @mixed_h);
print_results("High Risk of Delimited SSNs:"                       , @delimited_h);
print_results("High Risk of Non-Delimited SSNs:"                   , @nondelimited_h);
print_results("Low Risk of Delimited SSNs:"                        , @delimited_l);
print_results("Low Risk of Both Delimited And Non_Delimited SSNs:" , @mixed_l);
print_results("Low Risk of Non_Delimited SSNs:"                    , @nondelimited_l);
