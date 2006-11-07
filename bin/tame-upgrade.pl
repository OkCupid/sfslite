#!/usr/bin/perl
#
# tame-upgrade.pl
#
#    A script to upgrade code written in tame v0 to tame v1.
#
#    The general rules are:
#
#      TAMED - > tamed
#      WAIT -> twait
#      BLOCK -> twait
#      SIGNAL -> TRIGGER
#      signal() -> trigger()
#      @[a,b](c) -> mkevent(a,b,c);
#      @(a) -> mkevent(a);
#      VARS -> tvars
#
# $Id$

use strict;
use English;


use IO::File;

sub do_file ($$) {
    my ($inh, $outh) = @_;
    my $line;
    my $output;

    while (($line = <$inh>)) {
	$output = do_line ($line);
	print $outh $output;
    }
}

sub do_line ($) {
    my ($input) = @_;
    my $output = "";
    while (length ($input) > 0) {
	my ($pre,$mtch,$post) = do_subst ($input);
	$output .= $pre;
	$output .= $mtch;
	$input = $post;
    }
    return $output;
}

sub do_subst ($) {
    my ($i) = @_;

    my $pre = "";
    my $mtch = "";
    my $post = "";
    
    my $cb = 0;


    if ($i =~ /(\b(TAMED|WAIT|BLOCK|VARS|SIGNAL|signal)\b)|(\@|\]\s*\()/ ) {
	$pre = $PREMATCH;
	$post = $POSTMATCH;
	$mtch = $&;

	if ($1) {
	    my $m = $2;

	    my %x = ( "TAMED"   => "tamed",
		      "WAIT"    => "twait",
		      "BLOCK"   => "twait",
		      "VARS"    => "tvars",
		      "signal"  => "trigger",
		      "SIGNAL"  => "TRIGGER" );

	    $mtch = $x{$m};

	} elsif ($3) {
	    my $m = $3;
	    my $in = $mtch . $post;

	    if ($in =~ /\@[\[\(]/ ) {
		$mtch = "mkevent (";
		$post = $POSTMATCH;
		$cb = 1;

	    } elsif ($in =~ /\]\s*\(\s*\)/  ) {
		$mtch = ")";
		$post = $POSTMATCH;

	    } elsif ($in =~ /\]\s*\(/ ) {
		$mtch = ", ";
		$post = $POSTMATCH;
	    } else {
		$mtch = $m;
	    }
	} else {
	    warn "Cannot find a match...\n";
	}
    } else {
	$mtch = $i;
    }
    return ($pre, $mtch, $post);
}

do_file (\*STDIN, \*STDOUT);
