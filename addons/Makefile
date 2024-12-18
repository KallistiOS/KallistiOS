# KallistiOS ##version##
#
# addons/Makefile
# Copyright (C)2003 Megan Potter
#

# What we want to do here, unlike previous versions, is to setup a system
# where you can just untar/unzip any port you want into here and it
# will automagically get built in the right way for the arch you are
# compiling for. To accomplish this, each port contains a "kos" directory
# with build quirks for each supported platform. This could be an empty
# file. The name of it is "$(KOS_ARCH).cnf". Any directory under here
# which contains something that looks like that will get built. So for
# example if you have libbz2 with kos/dreamcast.cnf and kos/gba.cnf, it
# will get built for DC and GBA. If there is no gba.cnf, then it gets
# built for only DC, and so forth. Once a lib is verified to work on a
# platform a .cnf file can be added for it. Alternatively if you don't
# want some ports to build then you can just move them up under an "unused"
# dir or some such.

# If you have multiple addon dirs (e.g., you're tracking multiple svn repos
# or some such) then you can duplicate this Makefile to the root of each.

# Get our list of dirs to build/clean. You must have sed installed for this
# to work (GNU textutils).
get_subdirs=$(foreach each,$(wildcard $(1)/*),$(if $(wildcard $(each)/kos/$(KOS_ARCH).cnf),$(each),) $(call get_subdirs,$(each)))

SUBDIRS := $(call get_subdirs, $(shell pwd))

all: subdirs

clean: clean_subdirs

include $(KOS_BASE)/Makefile.rules
