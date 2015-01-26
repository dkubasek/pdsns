#!/bin/bash

function clean_configure
{
	[ ! -d autom4te.cache ] || rm -r autom4te.cache
	[ ! -e aclocal.m4 ] || rm aclocal.m4
	[ ! -e config.h ] || rm config.h
	[ ! -e config.h.in ] || rm config.h.in
	[ ! -e config.h.in~ ] || rm config.h.in~
	[ ! -e config.sub ] || rm config.sub
	[ ! -e config.guess ] || rm config.guess
	[ ! -e config.log ] || rm config.log 
	[ ! -e config.status ] || rm config.status
	[ ! -e configure ] || rm configure
	[ ! -e depcomp ] || rm depcomp
	[ ! -e install-sh ] || rm install-sh
	[ ! -e missing ] || rm missing
	[ ! -e stamp-h1 ] || rm stamp-h1
	[ ! -e ltmain.sh ] || rm ltmain.sh
	[ ! -e libtool ] || rm libtool
}

function clean_makefiles
{
	[ $# -eq 1 ] || return 1

	for ITEM in `ls $1`; do
		if [ -d $ITEM ]; then
			clean_makefiles $ITEM
		fi
	done

	[ ! -e "$1/Makefile" ] || rm "$1/Makefile"
	[ ! -e "$1/Makefile.in" ] || rm "$1/Makefile.in"
	[ ! -d "$1/.deps" ] || rm -r "$1/.deps"
}

# run make clean if makefile is present
[ ! -e Makefile ] || { make clean; make distclean; }
clean_makefiles .
clean_configure

exit 0
