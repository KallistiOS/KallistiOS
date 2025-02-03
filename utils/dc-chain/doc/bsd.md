# Sega Dreamcast Toolchains Maker (`dc-chain`) with BSD #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **Berkeley Software
Distribution** (**BSD**).

This document was written when using **FreeBSD** (`13.2`) but it should be
applicable on all **BSD** systems like **NetBSD** and **OpenBSD**.

## Introduction ##

On **FreeBSD** system, the package manager is the `pkg` tool.
 
If you never used the `pkg` tool before, you will be asked to install it. Please
do this before continuing reading the document.

All the operations in this document should be executed with the `root` user. If 
you don't want to use with the `root` user, another option is to escalate your
account privileges using the `sudo` command, which you will need to install:
```
pkg install sudo
```
If that is the case, you will need to add the `sudo` command before all the
commands specified below.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the whole toolchain.

### Installation of required packages ###

The packages below need to be installed:
```
pkg install gcc gmake binutils texinfo bash libjpeg-turbo png libelf git subversion python3
```
On **BSD** systems, the `make` command is **NOT** the same as the **GNU Make**
tool; instead **GNU Make** is invoked using `gmake`; you must use `gmake`
instead of `make` on **BSD** systems.

Additionally, by default the `sh` shell is used on **BSD**, while
**KallistiOS** scripts require the use of `bash`.

## Preparing the environment installation ##

Enter the following to prepare **KallistiOS** and the toolchains:
```
mkdir -p /opt/toolchains/dc/
cd /opt/toolchains/dc/
git clone git://git.code.sf.net/p/cadcdev/kallistios kos
git clone git://git.code.sf.net/p/cadcdev/kos-ports
```

## Compilation ##

The **dc-chain** system may be customized by setting up a
[`Makefile.cfg`] file in the root of the `dc-chain` directory tree. If this is
desired, read the main [`README`](../README.md) for more information on
setting up custom options for the toolchain; however, in most circumstances,
the stable defaults already present in
[`Makefile.default.cfg`](../Makefile.default.cfg) will be fine.

### Building the toolchain ###

To build the toolchain, do the following:

1. Run `bash`, if not already done:
	```
	bash
	```
2. Navigate to the `dc-chain` directory by entering:
	```
	cd /opt/toolchains/dc/kos/utils/dc-chain/
	```
3. (Optional) Copy and alter the `Makefile.cfg` file options to your liking.

4. Enter the following to start downloading and building toolchain:
	```
	gmake
	```

Now it's time to have a coffee as this process can be long: several minutes to
hours will be needed to build the full toolchain, depending on your system.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:
```
gmake distclean
```
## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the [`README`](../../../doc/README.md) file from KallistiOS now.
