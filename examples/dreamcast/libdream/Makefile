# This will make all the examples, leaving only the elf files.
# If you want to clean everything, use 'clean'.

SUBDIRS  = 320x240 640x480 rgb888 ta
SUBDIRS += cdfs
SUBDIRS += spu
SUBDIRS += keyboard mouse lcd vmu

all: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(KOS_MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _cleandir_%, $(SUBDIRS))

$(patsubst %, _cleandir_%, $(SUBDIRS)):
	$(KOS_MAKE) -C $(patsubst _cleandir_%, %, $@) clean

run: $(patsubst %, _rundir_%, $(SUBDIRS))

$(patsubst %, _rundir_%, $(SUBDIRS)):
	$(KOS_MAKE) -C $(patsubst _rundir_%, %, $@) run

dist: $(patsubst %, _distdir_%, $(SUBDIRS))

$(patsubst %, _distdir_%, $(SUBDIRS)):
	$(KOS_MAKE) -C $(patsubst _distdir_%, %, $@) dist

