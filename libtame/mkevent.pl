#!/usr/bin/perl
use strict;

##
## This is a script that autogerates the file mkevent.h -- the header file
## of tame autogenerated template classes.
##

my $N_tv = 3;
my $N_wv = 3;
my $MKEV = "_mkevent";
my $CN = "_event";
my $CNI = ${CN} . "_impl";
my $WCN = "event";
my $MKEVRS = ${MKEV} . "_rs";
my $BASE = "_event_base";
my $RVMKEV = "_ti_mkevent";

sub mklist ($$)
{
    my ($tmplt, $n) = @_;
    my @out;
    for (my $i = 1; $i <= $n; $i++) {
	my $a = $tmplt;
	$a =~ s/%/$i/g;
	push @out, $a;
    }
    return @out
}

sub mklist_multi (@)
{
    my @arr;
    foreach my $e (@_) {
	if (ref ($e)) {
	    push @arr, mklist ($e->[0], $e->[1]);
	} else {
	    push @arr, $e;
	}
    }
    return @arr;
}

sub commafy {
    return join (", " , @_);
}

sub arglist (@)
{
    return commafy (mklist_multi (@_));
}

sub template_arglist (@)
{
    my $al = arglist (@_);
    if (length ($al) > 0) {
	return "<" . $al . ">";
    } else {
	return "";
    }
}

sub do_trigger_func ($$)
{
    my ($fn, $t) = @_;

    print ("  void $fn (",
	   arglist (["T% t%", $t]), ")",
	   " { this->base_trigger (",
	   arglist (["t%", $t], ["nil_t()", $N_tv - $t]),
	   " ); }\n");
}

#
# make a class of type event, the inherits from libasync's callback,
# for each number of trigger values.
#
sub do_event_class ($)
{
    my ($t) = @_;
    my ($tlist, $tlist2);

    print ("template<", arglist (["class T%", $t]), ">\n");
    $tlist = "<" . arglist (["T%", $t]) . ">";

    my $vlist = "<" . arglist ("void", ["T%", $t]) . ">";

    # print the classname
    print ("class ${CN}", $tlist, " :\n",
	   "     public callback", $vlist , "\n",
	   "{\n",
	   "public:\n");

    # print the constructor
    print ("  ${CN} (const char *loc)\n",
	   "   : callback", $vlist, 
	   " (CALLBACK_ARGS(loc))\n",
	   "     {}\n");

    print ("  virtual void trigger (",
	   arglist (["T% t%", $t]),
	   ") = 0;\n"
	   );
    
    # close the class
    print "};\n\n";
}

#
# make a class of type event, the inherits from libasync's callback,
# for each number of trigger values.
#
sub do_event_impl_class ($)
{
    my ($t) = @_;
    my ($tlist, $tlist2);

    print ("template<", arglist ("class A", ["class T%", $t]), ">\n");
    $tlist = "<" . arglist (["T%", $t]) . ">";
    $tlist2 = "<" . arglist ("A", ["T%", $t]) . ">";


    # print the classname
    print ("class ${CNI}", $tlist2, " :\n",
	   "     public ${BASE}", $tlist2, ",\n",
	   "     public ${CN}", $tlist , "\n",
	   "{\n",
	   "public:\n");

    # print the constructor
    print ("  ${CNI} (",
	   arglist ("A action",
		    "const refset_t$tlist &rs",
		    "const char *loc"),
	   ")\n",
	   "    : ${BASE}" , $tlist2 , " (action, rs, loc),\n",
	   "      ${CN}", $tlist, " (loc)\n",
	   "     {}\n");

    # print the trigger functions
    do_trigger_func ("operator()", $t);
    do_trigger_func ("trigger", $t);
    
    # close the class
    print "};\n\n";
}
#
# Return:
#
# template<class W1, class W2, class W3, class T1, class T2>
# typename event_t<T1, T2>::ref
#
#
sub mkevent_prefix ($$)
{
    my ($t, $w) = @_;
    my $tn;
    my $ret = "";
    if ($t > 0 || $w > 0) {
	$ret .= "template<" .  arglist (["class W%", $w], ["class T%", $t]) .
	    ">\n";
	$tn = "typename ";
    } else {
	$tn = "";
    }
    $ret .= "${tn}${WCN}<". arglist (["T%", $t]) . ">::ref";
    return $ret;
}

sub do_mkevent_rs ($$) 
{
    my ($t, $w) = @_;
    my $prfx = mkevent_prefix ($t, $w);
    
    print ("$prfx\n",
	   "${MKEVRS} (" ,
	   arglist ("ptr<closure_t> c",
		    "const char *loc",
		    "const refset_t<" .arglist (["T%", $t]). "> &rs",
		    "rendezvous_t<" . arglist (["W%", $w]). "> &rv",
		    ["const W% &w%", $w]
		    ),
	   ")\n"
	   );

    if ($t > 0 || $w > 0) {
	print "{\n";
	my @args = ("c",
		    "loc",
		    "value_set_t<" . arglist (["W%", $w]) . "> (".
		    arglist (["w%", $w]). ")",
		    "rs");
	print ("  return rv.${RVMKEV} (" ,
	       join (",\n                      ", @args),
	       ");\n");
	print ("}");
    } else {
	print ";";
    }
    print "\n\n";
}

sub do_mkevent ($$)
{
    my ($t, $w) = @_;

    my $prfx = mkevent_prefix ($t, $w);
    
    print ("$prfx\n",
	   "${MKEV} (" , 
	   arglist ("ptr<closure_t> c",
		    "const char *loc",
		    "rendezvous_t<" . arglist (["W%", $w]) . "> &rv",
		    ["const W% &w%", $w],
		    ["T% &t%", $t]
		    ),
	   ")\n"
	   );
    if ($t > 0 || $w > 0) {
	print "{\n";
	
	my @args = ("c", 
		    "loc",
		    "refset_t<" . arglist (["T%", $t]) . "> (" .
		    arglist (["t%", $t]) . ")",
		    "rv",
		    ["w%", $w]
		    );
	print ("  return ${MKEVRS} (" , 
	       join (",\n                      ", 
		     mklist_multi (@args)),
	       ");\n");
	print ("}");
    } else {
	print ";";
    }
    print "\n\n";
}
    
sub do_generic ($$)
{
    my ($t, $w) = @_;
    do_mkevent_rs ($t, $w);
    do_mkevent ($t, $w);
}


sub do_mkevent_block ($)
{
    my ($t) = @_;

    print "template<" . arglist ("class C", ["class T%", $t]) . ">\n";
    print "typename ";
    print "${WCN}<" . arglist (["T%", $t]) . ">::ref\n";
    print ("${MKEV} (" ,
	   arglist ("ptr<C> c",
		    "const char *loc",
		    [ "T% &t%", $t ]),
	   ")\n");
    print "{\n";
    print ("  return _mkevent_implicit_rv (",
	   arglist ("c", "loc", 
		    "refset_t<" . arglist (["T%", $t]) 
		    ."> (" . arglist (["t%", $t]) . ")" ),
	   ");\n");
    print "}\n\n";
}

sub do_block ($)
{
    my ($t) = @_;
    do_mkevent_block ($t);
}

print <<EOF;
// -*-c++-*-
//
// Autogenerated by mkevent.pl
//

#ifndef _LIBTAME_EVENT_AG_H_
#define _LIBTAME_EVENT_AG_H_

#include "tame_event.h"
#include "tame_closure.h"
#include "tame_rendezvous.h"


EOF

for (my $t = 0; $t <= $N_tv; $t++) {
    do_event_class ($t);
    do_event_impl_class ($t);
    do_block ($t);
    for (my $w = 0; $w <= $N_wv; $w++) {
	do_generic ($t, $w);
    }
}


print <<EOF;
#endif // _LIBTAME_EVENT_AG_H_ 
EOF
