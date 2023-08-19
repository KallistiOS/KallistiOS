# Sega Dreamcast Toolchains Maker (`dc-chain`) with Debian #

This document contains all the instructions to create a fully working
toolchains targeting the **Sega Dreamcast** system under **Debian**.

This document was written when using **Lubuntu** (`18.04`) but it should be
applicable on all **Debian** systems; and more generally to any **GNU/Linux**
systems.

## Introduction ##

On **Debian** family system, the package manager is the `apt-get` tool.

All the operations in this document should be executed with the `root` user. If 
you don't want to connect with the `root` user, another option is to use
the `sudo` command which comes installed by default on **Debian** and
**Ubuntu** systems family.

In that case, you will need to add the `sudo` command before entering all the
commands specified below.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the whole toolchains.

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
apt-get install build-essential texinfo libjpeg-dev libpng-dev libelf-dev
```

### Installation of additional packages ###

These additional packages are required too:
```
apt-get install git subversion python
```
**Git** is needed right now, as **Subversion Client** and **Python 2** will be
needed only when building `kos-ports`. But it's better to install these now.

By the way you can check the installation success by entering something like
`git --version`. This should returns something like `git version X.Y.Z`.

## Preparing the environment installation ##

Enter the following to prepare **KallistiOS** and the toolchains:
```
mkdir -p /opt/toolchains/dc/
cd /opt/toolchains/dc/
git clone git://git.code.sf.net/p/cadcdev/kallistios kos
git clone git://git.code.sf.net/p/cadcdev/kos-ports
```
Everything is ready, now it's time to make the toolchains.

## Compilation ##

**KallistiOS** provides a complete system that make and install all required
toolchains from source codes: **dc-chain**.

The **dc-chain** system is mainly composed by a `Makefile` doing all the
necessary. In order to work, you'll need to provide a `config.mk` file. Read
the main `README.md` file at the root for more information.

### Making the toolchains ###

To make the toolchains, do the following:

1. Navigate to the `dc-chain` directory by entering:
	```
	cd /opt/toolchains/dc/kos/utils/dc-chain/
	```
2. Enter the following to start downloading and building toolchain:
	```
	make
	```
Now it's time to take a coffee as this process is really long: several hours
will be needed to make the full toolchains!

### Making the GNU Debugger (gdb) ###

If you want to install the **GNU Debugger** (`gdb`), just enter:
```
make gdb
```
This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:
```
make clean
```
## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the `README` file from KallistiOS now.
