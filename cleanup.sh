#!/bin/sh

find .  -name Makefile -exec make -f {} distclean ';'
find .  -name Makefile -exec rm {} ';'
find .  -name Makefile.in -exec rm {} ';'
rm -rf m4 autom4te.cache
rm -f aclocal.m4 ar-lib config.guess config.sub configure config.log config.status\
	depcomp install-sh missing test-driver ltmain.sh libtool\
	rapidjsonrpc-cpp*.tar.gz rapidjsonrpc-cpp*.tar.bz2 rapidjsonrpc-cpp*.zip 

