# Sega Dreamcast Toolchains Maker (`dc-chain`) with Cygwin #

This document contains all the instructions to create a fully working
toolchains targeting the **Sega Dreamcast** system under **Cygwin**.

## Introduction ##

On the **Cygwin** system, the package manager is directly the
`setup-${arch}.exe` file. It's designed to be run on graphical user-interface
(GUI) mode.

The **Cygwin** environment exists in two flavors:

- **32-bit**: `i686`
- **64-bit**: `x86_64`

This document was written when using the `i686` version, so if you are using
the `x86_64` version, you should replace all `i686` keywords in the packages
name with `x86_64`.

Please note that in the past, the **Cygwin** `x86_64` had problems with the
toolchains, so its usage is not recommended; but feel free to test it out.

## Installation of Cygwin ##

1. Open your browser on [**Cygwin.com**](https://www.cygwin.com/) and download
   `setup-${arch}.exe` (e.g. `setup-i686.exe`) from the 
   [**Cygwin** website](https://cygwin.com/install.html).

2. Run `setup-${arch}.exe` on **Administrator mode** (starting from
   **Microsoft Windows Vista**) then click on the `Next` button. 

3. Choose `Install from Internet` then click on the `Next` button.

4. In the `Root Directory` text box, input `C:\dcsdk\` or something else. The
   `Root Directory` will be called `${CYGWIN_ROOT}` later in the document. Click
   on the `Next` button.

5. In the `Local Package Directory`, input what you want. It should be a good
   idea to put the packages in the **Cygwin** directory, e.g. in
   `${CYGWIN_ROOT}\var\cache\packages\`. Click on the `Next` button.

6. Adjust proxy settings as needed, then click on the `Next` button.

7. Choose a FTP location near you and click the `Next` button.

8. When the **Select Packages** window shows up, select the following packages,
   by using the `Search` text box (it should be more efficient to choose the
   `Category` view):

	- `autoconf`
	- `automake`
	- `binutils`
	- `curl`
	- `gcc-core`
	- `gcc-g++`
	- `git`
	- `libelf0-devel`
	- `libjpeg-devel`
	- `libpng-devel`
	- `make`
	- `patch`
	- `python2`
	- `subversion`
	- `texinfo`

9. Validate the installation by clicking the `Next` button, the click on the
   `Terminate` button to exit the installer. It should be a good idea to create
   the shortcuts on the Desktop and/or in the Start Menu.

10. Move the `setup-${arch}.exe` file in the `${CYGWIN_ROOT}` directory. This is
    important if you want to update your **Cygwin** installation.

The **Cygwin** base environment is ready. It's time to setup the 
whole environment to build the toolchains.

## Preparing the environment installation ##

1. Open the **Cygwin Terminal** by double-clicking the shortcut on your Desktop 
   (or alternatively, double-click on the `${CYGWIN_ROOT}\cygwin.bat` batch 
   file).
   
2. Enter the following to prepare **KallistiOS**:

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

1. Start the **Cygwin Terminal** if not already done.

2. Navigate to the `dc-chain` directory by entering:

		cd /opt/toolchains/dc/kos/utils/dc-chain/
	
3. Enter the following to start downloading and building toolchain:

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

	make clean

## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the `README` file from KallistiOS now.
