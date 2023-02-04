# KallistiOS

KOS is an unofficial development environment for the SEGA Dreamcast game console with some support for the NAOMI and NAOMI 2 arcade boards.  

KOS was developed from scratch over the internet by a group of free software developers and has no relation to the official Sega Katana or Microsoft Windows CE Dreamcast development kits. This has allowed it to fuel a thriving Dreamcast homebrew scene, powering many commercial releases for the platform over the years. It supports a signficiant portion of the Dreamcast's hardware capabilities and a wide variety of peripherals, accessories, and add-ons for the console, including custom hardware modifications that have been created by the scene. 

Despite the console's age, KOS offers an extremely modern, programmer-friendly development environment, supporting C17 and C++17, with the majority of their standard libraries fully supported and additional support for many POSIX APIs. Additionally, KOS-ports offers a rich set of add-on libraries such as SDL, OpenGL, and Lua for the platform. 

## Features
##### Core Functionality
* Concurrency with Kernel Threads, C11 Threads, pthreads
* Virtual filesystem abstraction 
* IPv4/IPv6 Network stack
* Dynamically loading libraries/modules
* GDB Debug stubs

##### Dreamcast Hardware Support
* GD-Rom driver
* Low-level 3D PowerVR Graphics 
* SH4 ASM-Optimized Math Routines
* SH4 SCIF Serial I/O
* DMA Controller 
* Flashrom Access 
* AICA SPU Sound Processor Driver
* Cache and Store Queue Management
* Timer Peripherals and Real-Time Clock
* MMU Management API 
* BIOS Font Rendering

##### Peripherals and Accessory Support
* Visual Memory Unit
* Puru Puru Vibration Pack
* Seaman Microphone
* Dreameye Webcam
* Lightgun 
* Keyboard
* Mouse
* Dial-up Modem
* Broadband Adapter
* LAN Adapter
* VGA Adapter
* SD Card Reader

##### Hardware Modification Support
* IDE Hard Drive
* 32MB RAM Upgrade
* Custom BIOS Flashroms

## Getting Started 
A beginner's guide to development for the Sega Dreamcast along with detailed instructions for installing KOS and the required toolchains can be found on the  [Dreamcast Wiki](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development). Additional documentation can be found in the docs folder. 

## Examples 
Once you've set up the environment and are ready to begin developing, a good place to start learning is the examples directory, which provides demos for the various KOS APIs and for interacting with the Dreamcast's hardware. Examples include:
- Hello World
- Console Input/Output
- Assertions, stacktraces, threading
- Drawing directly to the framebuffer
- Rendering with OpenGL
- Rendering with KGL
- Rendering with KOS PVR API
- Texturing with libPNG
- Bump maps, modifier volumes, render-to-texture PVR effects
- Audio playback on the ARM SPU
- Audio playback using SDL Audio
- Audio playback using OGG, MP3, and CDDA
- Querying controller input
- Querying keyboard input
- Querying mouse input
- Querying lightgun input
- Accessing the VMU filesystem
- Accessing the SD card filesystem
- Networking with the modem, broadband adapter, and LAN adapter
- Taking pictures with the DreamEye webcam
- Reading and Writing to/from ATA devices
- Testing 32MB RAM hardware mod
- Interactive Lua interpreter terminal

## Resources
[DCEmulation Forums](http://dcemulation.org/phpBB/viewforum.php?f=29): Goldmine of Dreamcast development information and history  
[Dreamcast Wiki](http://dreamcast.wiki): Large collection of tutorials and articles for beginners  
[Simulant Discord Chat](https://discord.gg/bpDZHT78PA): Home to the official Discord channel of KOS  
IRC Channel: irc.libera.chat #dreamcastdev 

