# Sega Dreamcast Toolchains Maker (`dc-chain`) with Debian #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **Debian**.

This document was written for **Debian** systems, but should apply to any
**GNU/Linux** system derived from **Debian**, such as **Ubuntu** or **Mint**.

## Introduction ##

On **Debian** family system, the package manager is the `apt-get` tool.

All the operations in this document should be executed with the `root` user. If 
you don't want to use with the `root` user, another option is to escalate your
account privileges using the `sudo` command which is installed by default on
standard **Debian** and **Ubuntu** systems. If that is the case, you will need
to add the `sudo` command before all the commands specified below.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the toolchains.

### Update of your local installation ###

The first thing to do is to update your local installation:
```
apt-get update
apt-get upgrade -y	
```
This should update all the packages of the **Debian** environment.

### Installation of required packages ###

The packages below need to be installed:
```
apt-get install build-essential texinfo libjpeg-dev libpng-dev libelf-dev git subversion python3
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
[`Makefile.default.cfg`](../Makefile.default.cfg) will be fine.

### Building the toolchain ###

To build the toolchain, do the following:

1. Navigate to the `dc-chain` directory by entering:
	```
	cd /opt/toolchains/dc/kos/utils/dc-chain/
	```

2. (Optional) Copy and alter the `Makefile.cfg` file options to your liking.

3. Enter the following to start downloading and building toolchain:
	```
	make
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
