# Sega Dreamcast Toolchain Maker (dc-chain) #

This package contains a `Makefile` which both simplifies building the whole 
**Sega Dreamcast** toolchain, and gives you substantial control.

The toolchain is composed by:

- A `sh-elf` toolchain, which is the main toolchain. It targets the CPU of the 
**Dreamcast**, i.e. the **Hitachi SH-4 CPU** (**SuperH**).
- An `arm-eabi` toolchain, which is the toolchain used only for the **Yamaha
Super Intelligent Sound Processor** (**AICA**). This processor is based
on an **ARM7** core.

The **dc-chain** `Makefile` is ready to build everything you need to compile
**KallistiOS** and then develop for the **Sega Dreamcast** system.

## About toolchain components versions ##

Components that are included in a toolchain are:

- **Binutils** (mainly `ld` plus other tools)
- **GNU Compiler Collection** (`gcc`)
- **Newlib** (mainly `libc` plus other libraries)
- **GNU Debugger** (`gdb`) - Optional

As we are building two toolchains (`sh-elf` and `arm-eabi`), all these
components will be installed twice, for both targets.

Speaking about the best versions to use for the **Sega Dreamcast** development, 
they are already declared in the `Makefile`. This is particulary true for `gcc`:
the best version to use at this time is the `4.7.3`. We know that today this
version is pretty old but [greater versions have serious 
issues](http://dcemulation.org/phpBB/viewtopic.php?f=29&t=102800) so it's better 
to stick with that version.

Plus, `gcc`'s bugtracker has a lot of bugs marked as [6/7/8/9 Regression] for 
the `sh-elf` target which have not been resolved. Newer isn't always better, 
especially with `gcc` targets that aren't high-priority.

## Advanced options ##

You may attempt to spawn multiple jobs with `make`. Using `make -j4` is
recommended for speeding up the building of the toolchain. There is an option 
inside the `Makefile` to set the number of jobs for the building phases.
Set the `makejobs` variable in the `Makefile` to whatever you would normally
feel the need to use on the command line, and it will do the right thing.

In the old times, this option may breaks things, so, if you run into
trouble, you should clear this variable and try again with just one
job running.

On **MinGW/MSYS** environment, it has been confirmed that multiple jobs breaks
the toolchain, so please don't try to do that under this environment. This
option is disabled by default in this scenario. This doesn't apply to the
others environments, including **MinGW-w64/MSYS2**.

## Usage ##

Before you start, please browse the `./doc` directory and check if they are
full instructions for building the whole toolchain for your environment.

### Making the toolchain ###

Below you will find some generic instructions:

1. Change the variables in the `User Configuration` section of the `Makefile`
   to match your environment. They can be overridden at the command line as
   well. Please note, a lot of conditional instructions have been added, so it
   should work most of the time just out-of-the-box for your environment.

2. Then execute the following for preparing the sources:

		./download.sh
		./unpack.sh

3. Finally, input (for **BSD**, please use `gmake` instead):

		make
	
Depending of your environment, this can take a bunch of hours. So please be
patient!

If anything goes wrong, check the output in `logs/`.

### Making the GNU Debugger (gdb) ###

For the `sh-elf` toolchain, if you want to use the **GNU Debugger** (`gdb`),
you can make it by entering:

	make gdb

This will install `gdb` in the `sh-elf` toolchain (`gdb` is used with
`dcload/dc-tool` programs, which are part of **KallistiOS** too).

### Removing all useless files ###

After the toolchain compilation, you can cleanup everything by entering:

	./cleanup.sh

This will save a lot of space.

## Final note ##

Please see the comments at the top of the `Makefile` for more build options.
