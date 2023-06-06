# Sega Dreamcast Toolchains Maker (`dc-chain`) with macOS #

This document contains all the instructions to create a fully working
toolchains targeting the **Sega Dreamcast** system under **macOS**.

This document was written when using **macOS** (`10.14 Mojave`) but it should be
applicable on all **macOS** systems. Note that Apple introduced some breaking
changes in `10.14 Mojave`; so starting from that version, some header files have
moved. They have been removed in `10.15 Catalina` and later versions.
**dc-chain** supports all modern macOS versions, including `pre-Mojave`
releases.

## Introduction ##

On **macOS** system, the package manager is the `brew` tool, which is provided
by the [Homebrew project](https://brew.sh).
 
If you never used the `brew` tool before, you will need to install it. The
procedure to do that is given below.

All the operations in this document should be executed with the `root` user. To
do that, from a **Terminal** window, input:

	sudo -s

If you don't want to use the `root` user, another option is to use the `sudo`
command. In that case, you will need to add the `sudo` command before entering
all the commands specified below.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the whole toolchains.

### Installation of the Developer Tools ###

By default, the **macOS** system doesn't contains any developer tools installed.
The really first prerequisites is to install them.

Please note, you can ignore these instructions below if you already have
**Xcode** installed on your system.

1. Open a **Terminal**.

2. Then input:

		xcode-select --install

3. When the window opens, click on the `Install` button, then click on the
   `Accept` button.

All the developer tools should be now installed, like `gcc` or `make`. You can
try this by entering `gcc --version` in the **Terminal**.

**Note:** On **macOS**, `gcc` redirects to `clang` from the [LLVM](https://llvm.org/)
project. This is normal and doesn't affect the **dc-chain** process.

### Installation of Homebrew ###

As already said in the introduction, the **macOS** system doesn't come with a
package manager, but fortunately, the [Homebrew project](https://brew.sh) is
here to fill this gap:

1. Open a **Terminal** window.

2. Execute the following:

		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

**Homebrew** is now installed. You can check if it's working by entering
`brew --version`.

### Installation of required packages ###

The packages below need to be installed:

	brew install libjpeg libpng libelf

All the other required packages have already been installed, i.e. `git`, `svn`
or `python`.

## Preparing the environment installation ##

Enter the following to prepare **KallistiOS** and the toolchains:

	mkdir -p /opt/toolchains/dc/
	cd /opt/toolchains/dc/
	git clone git://git.code.sf.net/p/cadcdev/kallistios kos
	git clone git://git.code.sf.net/p/cadcdev/kos-ports

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

		cd /opt/toolchains/dc/kos/utils/dc-chain/
	
2. Enter the following to start downloading and building toolchain:

		make

Now it's time to take a coffee as this process is really long: several hours
will be needed to make the full toolchains!

### Making the GNU Debugger (gdb) ###

If you want to install the **GNU Debugger** (`gdb`), just enter:

	make gdb

This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:

	./cleanup.sh

## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the `README` file from KallistiOS now.
