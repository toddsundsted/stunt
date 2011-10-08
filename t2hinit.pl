# use warnings;

# How to make texi2html put extra stuff into the <HEAD> portions of
# all of the html files.  I hope this file can be replaced by a
# commandline flag in the not-too-distant future.  --wrog

*T2H_InitGlobals_orig = \&T2H_InitGlobals;

*T2H_InitGlobals = sub {
   &T2H_InitGlobals_orig;
   $T2H_EXTRA_HEAD = '<link rel="stylesheet" href="common.css" type="text/css">';
}


# $Id: t2hinit.pl,v 1.2 2004/06/02 08:09:46 wrog Exp $

# $Log: t2hinit.pl,v $
# Revision 1.2  2004/06/02 08:09:46  wrog
# I guess we don't need warnings yet
#
# Revision 1.1  2004/06/02 08:02:45  wrog
# initial version
#
