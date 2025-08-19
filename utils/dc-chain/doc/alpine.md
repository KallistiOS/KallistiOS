# Sega Dreamcast Toolchains Maker (`dc-chain`) with Alpine Linux #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **Alpine Linux**.

**Alpine Linux** is a regular **GNU/Linux** system, but it's also a great
candidate for making **Docker** images. You may find a working example of a
Alpine-based `Dockerfile` in the `docker` directory within the `dc-chain` tool.

## Introduction ##

On an **Alpine Linux** system, the package manager is the `apk` tool.

All the operations in this document should be executed with the `root` user. 
You may run the `su -` command under a standard user account to become `root`. 
This utility comes installed by default on **Alpine Linux**.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the whole toolchains.

### Installation of required packages ###

The packages below need to be installed:
```
apk --update add build-base patch bash coreutils-fmt texinfo gmp-dev mpfr-dev libjpeg-turbo-dev libpng-dev elfutils-dev curl wget python3 git subversion
```	

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
[`Makefile.dreamcast.cfg`](../Makefile.dreamcast.cfg) will be fine.

### Building the toolchain ###

To build the toolchain, do the following:

1. Navigate to the `dc-chain` directory by entering:
	```
	cd /opt/toolchains/dc/kos/utils/dc-chain/
	```

2. (Optional) Copy and alter the `Makefile.cfg` file options to your liking.

3. Enter the following to start downloading and building toolchain:
	```
	make use_custom_dependencies=1
	```

Now it's time to have a coffee as this process can be long: several minutes to
hours will be needed to build the full toolchain, depending on your system.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:
```
make distclean
```

## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the [`README`](../../../doc/README.md) file from KallistiOS now.
