# Sega Dreamcast Toolchains Maker (`dc-chain`) with macOS #

This document contains all the instructions to create a fully working
toolchains targeting the **Sega Dreamcast** system under **macOS**.

This document was initially written while using **macOS** (`10.14 Mojave`) but
it should be applicable on all modern **macOS** systems. Note that Apple
introduced some breaking changes in `10.14 Mojave`; so starting from that
version, some header files have moved. They have been removed in
`10.15 Catalina` and later versions. **dc-chain** supports all modern macOS
versions, including `pre-Mojave` releases.

This document has been refreshed using `14.2.1 Sonoma`.

## Introduction ##

On **macOS** system, the package manager is the `brew` tool, which is provided
by the [Homebrew project](https://brew.sh).
 
If you have never used the `brew` tool before, you will need to install it
using your standard user account.

Besides the `brew` installation and setup, the rest of the operations in this
document should be executed with `root` user privileges. To do that, use the
`sudo` command. You will need to add the `sudo` command before entering all the
commands specified below.

## Prerequisites ##

Before doing anything, you will have to install some prerequisites in order to
build the whole toolchain.

### Installation of the Developer Tools ###

By default, the **macOS** system doesn't have the **macOS Developer Tools**
installed, so we must first install them. You may ignore these instructions
below if you already have **Xcode** installed on your system.

1. Open a **Terminal**.

2. Then input:
	```
	xcode-select --install
	```
3. When the window opens, click on the `Install` button, then click on the
   `Accept` button.

The **macos Developer Tools** should be now installed. You can test this by
entering `gcc --version` in the **Terminal**.

**Note:** On **macOS**, `gcc` redirects to `clang` from the
[LLVM](https://llvm.org/) project. This is normal and doesn't affect the
**dc-chain** process.

### Installation of Homebrew ###

As already mentioned in the introduction, the **macOS** system doesn't come
with a package manager, but fortunately, the
[Homebrew project](https://brew.sh) exists to fill this gap.

Visit [https://brew.sh](https://brew.sh) and follow the instructions to install
the `brew` package manager. Please note that all operations done involving
**Homebrew** installation and use should be done under a normal user account,
not using `root` or `sudo`. 

You can check if `brew` is installed by running `brew --version`.

### Installation of required packages ###

The packages below need to be installed:
```
brew install wget gettext texinfo gmp mpfr libmpc libelf jpeg-turbo libpng

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
