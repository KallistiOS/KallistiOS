# KallistiOS Utilities
This directory contains a number of PC-side tools used for a variety of purposes. Some are meant to be used directly by users, while others are called through KallistiOS Makefiles. These utilities are built automatically when KallistiOS is built, and many KallistiOS examples depend upon them to build properly. An example of this would be using `vqenc` to generate textures from image files at build time.

- [**bin2c**](bin2c/): Converts a binary file to a C integer array for inclusion in a source file
- [**bin2o**](bin2o/): Converts a binary file to an object file for linking into a project
- [**bincnv**](bincnv/): An ELF to BIN conversion testing utility
- [**blender**](blender/): A Python-based Blender export plugin
- [**cmake**](cmake/): CMake configuration files to build KOS projects using CMake
- [**dc-chain**](dc-chain/): Scripts to assist in building a Dreamcast cross-compiler toolchain for the SuperH 4 and ARM7DI processors
- [**dcbumpgen**](dcbumpgen/): Generates PVR bumpmap textures from JPG and PNG files
- [**elf2bin**](elf2bin/): Script to convert ELF files to BIN programs
- [**genexports**](genexports/): Scripts used by KallistiOS's build system to generate symbol exports
- [**genromfs**](genromfs/): Generates romfs filesystems for embedding into KOS binaries
- [**gentexfont**](gentexfont/): Creates TXF font files from X11 fonts
- [**gnu_wrappers**](gnu_wrappers/): GCC wrapper scripts used by KallistiOS's build system
- [**ipload**](ipload/): A simple Python-based IP uploader for use with Marcus Comstedt's IPLOAD
- [**isotest**](isotest/): A PC-based iso9660 driver for testing KOS iso9660 filesystem code
- [**kmgenc**](kmgenc/): Stores images as PVR textures in a KMG container
- [**ldscripts**](ldscripts/): Linker scripts used by KallistiOS's build system
- [**makeip**](makeip/): Generates Initial Program bootstrap files (IP.BIN)
- [**makejitter**](makejitter/): Creates jitter tables
- [**naomibintool**](naomibintool/): Builds a NAOMI ROM from ELF or BIN files
- [**naominetboot**](naominetboot/): Uploads a program to a NAOMI NetDIMM
- [**rdtest**](rdtest/): A PC-based romdisk driver for testing KOS romdisk filesystem code
- [**scramble**](scramble/): Scrambles Dreamcast binaries to prepare for loading from disc
- [**version**](version/): A utility to write the KallistiOS version to the header of project files
- [**vqenc**](vqenc/): Compresses image files using the Dreamcast's Vector Quantization algorithm
- [**wav2adpcm**](wav2adpcm/): Converts audio data between WAV and ADPCM formats
