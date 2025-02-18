# Sega Dreamcast Toolchains Maker (`dc-chain`) with Cygwin #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **Cygwin**.

## Introduction ##

On the **Cygwin** system, the package manager is directly accessed using the
`setup-${arch}.exe` file. It's designed to be run in graphical user-interface
(GUI) mode.

The **Cygwin** environment exists in two flavors:

- **32-bit**: `i686`
- **64-bit**: `x86_64`

This document was written when using the `i686` version, so if you are using
the `x86_64` version, you should replace all `i686` keywords in the packages
name with `x86_64`.

Please note that in the past, the **Cygwin** `x86_64` had problems with the
toolchains, so its usage is not recommended; feel free to test it out, however.

## Installation of Cygwin ##

1. Open your browser on [**Cygwin.com**](https://www.cygwin.com/) and download
   `setup-${arch}.exe` (e.g. `setup-i686.exe`) from the 
   [**Cygwin** website](https://cygwin.com/install.html).

2. Run `setup-${arch}.exe` on **Administrator mode** (if using
   **Microsoft Windows Vista** or later) then click on the `Next` button. 

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
	```
	mkdir -p /opt/toolchains/dc/
	cd /opt/toolchains/dc/
	git clone git://git.code.sf.net/p/cadcdev/kallistios kos
	git clone git://git.code.sf.net/p/cadcdev/kos-ports
	```

Everything is ready, now it's time to make the toolchains.

## Compilation ##

The **dc-chain** system may be customized by setting up a
[`Makefile.cfg`] file in the root of the `dc-chain` directory tree. If this is
desired, read the main [`README`](../README.md) for more information on
setting up custom options for the toolchain; however, in most circumstances,
the stable defaults already present in
[`Makefile.default.cfg`](../Makefile.default.cfg) will be fine.

### Building the toolchain ###

To build the toolchain, do the following:

1. Start the **Cygwin Terminal** if not already done.

2. Navigate to the `dc-chain` directory by entering:
	```
	cd /opt/toolchains/dc/kos/utils/dc-chain/
	```

3. (Optional) Copy and alter the `Makefile.cfg` file options to your liking.

4. Enter the following to start downloading and building toolchain:
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
