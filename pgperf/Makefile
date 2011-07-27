# Copyright (C) 1989 Free Software Foundation, Inc.
# written by Douglas C. Schmidt (schmidt@ics.uci.edu)
# 
# This file is part of GNU GPERF.
# 
# GNU GPERF is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 1, or (at your option)
# any later version.
# 
# GNU GPERF is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU GPERF; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 

CC = gcc
DFLAGS= -DLO_CAL -DGATHER_STATISTICS
OFLAGS= -O -p -g -fstrength-reduce -fomit-frame-pointer -fdelayed-branch -finline-functions 
CFLAGS= $(DFLAGS) $(OFLAGS)
OBJS = options.o iterator.o main.o perfect.o keylist.o listnode.o xmalloc.o \
       hashtable.o boolarray.o readline.o stderr.o version.o getopt.o
SOURCES = options.c iterator.c main.c perfect.c keylist.c listnode.c xmalloc.c \
       hashtable.c boolarray.c readline.c stderr.c version.c getopt.c

all: pgperf

pgperf: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean: 
	-rm -f *.o core *~ #*#

realclean: clean
	-rm -f pgperf

# DO NOT DELETE

boolarray.o: boolarray.h prototype.h options.h
hashtable.o: hashtable.h keylist.h listnode.h prototype.h options.h
iterator.o: iterator.h prototype.h
keylist.o: options.h prototype.h readline.h keylist.h listnode.h hashtable.h
keylist.o: stderr.h
listnode.o: options.h prototype.h listnode.h stderr.h
main.o: stderr.h prototype.h options.h perfect.h keylist.h listnode.h
main.o: boolarray.h
options.o: options.h prototype.h iterator.h stderr.h
perfect.o: options.h prototype.h perfect.h keylist.h listnode.h boolarray.h
perfect.o: stderr.h
readline.o: readline.h prototype.h
stderr.o: stderr.h prototype.h
