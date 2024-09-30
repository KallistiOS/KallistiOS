# KallistiOS ##version##
#
# examples/dreamcast/sd/Makefile
#

all:
	$(KOS_MAKE) -C ext2fs
	$(KOS_MAKE) -C mke2fs
	$(KOS_MAKE) -C speedtest

clean:
	$(KOS_MAKE) -C ext2fs clean
	$(KOS_MAKE) -C mke2fs clean
	$(KOS_MAKE) -C speedtest clean

dist:
	$(KOS_MAKE) -C ext2fs dist
	$(KOS_MAKE) -C mke2fs dist
	$(KOS_MAKE) -C speedtest dist

