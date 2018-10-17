# Sega Dreamcast toolchain with GNU/Linux #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **GNU/Linux**.

This document was written when using **Lubuntu** (`18.04`) but it should be
applicable on all **GNU/Linux** systems.

## Introduction ##

On **Ubuntu** family system, the package manager is the `apt-get` tool.

All the operations in this document should be executed with the `root` user. If 
you don't want to connect with the `root` user, another option is to use
the `sudo` command which comes installed by default on **Lubuntu** and **Ubuntu** systems family.

In that case, you will need to add the `sudo` command before entering all the
commands specified below.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the whole toolchain.

### Update of your local installation ###

The first thing to do is to update your local installation:

	apt-get update
	apt-get upgrade -y	

This should update all the packages of the **GNU/Linux** environment.

### Installation of required packages ###

The packages below need to be installed:

	apt-get install build-essential texinfo libjpeg-dev libpng-dev libelf-dev

### Installation of additional packages ###

These additional packages are required too:

	pkg install git subversion python

**Git** is needed right now, as **Subversion Client** and **Python 2** will be
needed only when building `kos-ports`. But it's better to install these now.

By the way you can check the installation success by entering something like
`git --version`. This should returns something like `git version X.Y.Z`.

## Preparing the environment installation ##

1. Enter the following to prepare **KallistiOS** and the toolchain:

		mkdir -p /opt/toolchains/dc/
		cd /opt/toolchains/dc/
		git clone https://github.com/KallistiOS/KallistiOS.git kos
		git clone https://github.com/KallistiOS/kos-ports.git

2. Enter the following to prepare **dcload**/**dc-tool** (part of 
   **KallistiOS**):
 
		mkdir -p /opt/toolchains/dc/dcload/
		cd /opt/toolchains/dc/dcload/
		git clone https://github.com/KallistiOS/dcload-serial.git
		git clone https://github.com/KallistiOS/dcload-ip.git

Everything is ready, now it's time to use the make the toolchain.

## Compilation ##

**KallistiOS** provides a complete system that make and install all required
toolchains from source codes: **dc-chain**.

The **dc-chain** system is mainly composed by a `Makefile` doing all the
necessary. Open that file with a text editor (e.g. `nano`) and locate the 
`User configuration` section. You can tweak some parameters, but usually
everything is ready to work out-of-the-box. For example, it isn't recommended
to change the toolchains program versions. The highest versions confirmed to
work with the **Sega Dreamcast** are always already set in that `Makefile`.

### Making the toolchain ###

To make the toolchains, do the following:

1. Navigate to the `dc-chain` directory by entering:

		cd /opt/toolchains/dc/kos/utils/dc-chain/
	
2. Enter the following to download all source packages for all components:

		./download.sh

3. Enter the following to unpack all source packages.

		./unpack.sh

4. Enter the following to launch the process:

		make

Now it's time to take a coffee as this process is really long: several hours
will be needed to make the full toolchain!

### Making the GNU Debugger (gdb) ###

If you want to install the **GNU Debugger** (`gdb`), just enter:

	make gdb

This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:

	./cleanup.sh

## Next steps ##

After following this guide, the toolchain should be ready.

Now it's time to compile **KallistiOS**.

Please read the `/opt/toolchains/dc/kos/doc/README` file to learn the next
steps.
