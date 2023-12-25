# KallistiOS Addons

The Addons system allows the creation of standalone addon packages, similar to the ports in the kos-ports system. Although KallistiOS currently only supports the Sega Dreamcast platform, the system is designed to support build quirks for various platforms. Each addon contains a `kos` directory, which would contain `$(KOS_ARCH).cnf` files (so, as of now, just `dreamcast.cnf`). The addon's `Makefile` contains build instructions to build the addon for KallistiOS, while the `dreamcast.cnf` contains quirks specific to building for the Dreamcast. If there are no platform-specific build quirks, an empty file named `dreamcast.cnf` still needs to exist for the build system to recognize that Dreamcast is a valid target platform for the addon. If you wish to disable a specific addon, simply create an "unused" directory and move the addons you wish to disable there. Once built, the package's headers will be available in `addons/include` and the built libraries in `addons/lib`. These paths are automatically included in your build flags if you are using the KOS Makefile system.

A few addons are supplied with KallistiOS. These include:
- [**libkosext2fs**](libkosext2fs/): A filesystem driver for the ext2 filesystem
- [**libkosfat**](libkosfat/): A filesystem driver for FAT12, FAT16, and FAT32 filesystems, with long name support
- [**libkosutils**](libkosutils/): Utilities: Functions for B-spline curve generation, MD5 checksum handling, image handling, network configuration management, and PCX images
- [**libnavi**](libnavi/): A flashROM driver and G2 ATA driver, historically used with Megan Potter's Navi Dreamcast hacking project
- [**libppp**](libppp/): PPP protocol support for modem devices
