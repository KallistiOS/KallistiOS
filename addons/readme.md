# KallistiOS Addons

The Addons system allows the creation of standalone addon packages, similar to the ports in the kos-ports system. 

## Installing addons
To install an add-on, simply place the addon directory inside this directory. Addons in this directory are automatically built when KallistiOS is built. Once built, the addon's headers will be available in `addons/include` and the built libraries in `addons/lib`. These paths are automatically included in your build flags if you are using the KOS Makefile system. You may disable an addon by creating an `unused` directory and moving the addons within, or you may uninstall an addon outright by simply deleting its directory.

A few addons are supplied with KallistiOS. These include:
- [**libkosext2fs**](libkosext2fs/): A filesystem driver for the ext2 filesystem
- [**libkosfat**](libkosfat/): A filesystem driver for FAT12, FAT16, and FAT32 filesystems, with long name support
- [**libkosutils**](libkosutils/): Utilities: Functions for B-spline curve generation, MD5 checksum handling, image handling, network configuration management, and PCX images
- [**libnavi**](libnavi/): A flashROM driver and G2 ATA driver, historically used with Megan Potter's Navi Dreamcast hacking project
- [**libppp**](libppp/): Point-to-Point Protocol support for modem devices

## Creating addons
Although KallistiOS currently only supports the Sega Dreamcast platform, the system is designed to support build quirks for various platforms. Each addon contains a `kos` directory, which would contain `$(KOS_ARCH).cnf` files (so, as of now, just `dreamcast.cnf`). The addon's `Makefile` contains build instructions to build the addon for KallistiOS, while the `dreamcast.cnf` contains quirks specific to building for the Dreamcast. If there are no platform-specific build quirks, an empty file named `dreamcast.cnf` still needs to exist for the build system to recognize that Dreamcast is a valid target platform for the addon. 
