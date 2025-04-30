# Sega Dreamcast Toolchains Maker (`dc-chain`) Changelog

| Date<br/>_____________ | Author(s)<br/>_____________ | Changes<br/>_____________ |
|:-----------------------|:----------------------------|---------------------------|
| 2025-04-18 | Eric Fradella | Add 15.1.0 release profile and 15.1.1/16.0.0 dev profiles. All dev profiles upgraded to Binutils 2.44 and GDB 16.2 |
| 2025-02-19 | Eric Fradella | Remove profiles and patches for older toolchains (9.3.0, 10.5.0, 11.5.0, 12.4.0) and Rust dev toolchains. Support for Newlib versions prior to 4.x now deprecated. |
| 2025-01-26 | Mickaël Cardoso | Update documentations. |
| 2025-01-05 | Eric Fradella | Add support and update toolchain profiles for Newlib 4.5.0, Binutils 2.43.1, and GDB 15.2. |
| 2024-08-11 | Eric Fradella | Fix ARM toolchain build error when JIT is enabled for SH toolchain. |
| 2024-08-07 | Eric Fradella | Updated binutils to 2.43. Updated GCC 11 profile with support for GCC 11.5.0. |
| 2024-08-01 | Eric Fradella | Updated GCC 14 profile with support for GCC 14.2.0. |
| 2024-07-15 | Eric Fradella | Updated profiles for GDB 15.1. |
| 2024-06-26 | Eric Fradella | Updated GCC 12 profile with support for GCC 12.4.0. |
| 2024-05-24 | Eric Fradella | Added support for GCC 13.3.0. |
| 2024-05-08 | Falco Girgis | Added configuration option for libstdc++'s timezone database. |
| 2024-05-02 | Eric Fradella | Deprecated GCC 4.7.4 profile. Revamped configuration system into separate profiles and Makefile.cfg. Revised configuration options and documentation. |
| 2024-05-01 | Falco Girgis | Added config option for enabling the Ada langauge. |
| 2024-04-30 | Falco Girgis | Added config option for enabling iconv library support in Newlib. |
| 2024-04-29 | Donald Haase<br/>Eric Fradella | Patch Newlib headers to expose lstat() declaration. |
| 2024-04-26 | Eric Fradella | Update git repo cloning to use treeless clone. |
| 2024-04-21 | Falco Girgis | Added D to list of supported languages, added m4-single as a default precision mode, added --disable-libphobos to gcc-pass2 |
| 2024-03-19 | Paul Cercueil | Disable building GCC documentation. |
| 2024-03-03 | Eric Fradella | Update GDB to 14.2. |
| 2024-02-21 | Marc Poulhiès | Alter GCC configuration to let it know we use GNU tools. |
| 2024-02-10 | Eric Fradella | Patch Newlib to eliminate implicit function declarations, which were converted from a warning to an error in GCC 14 series. |
| 2024-01-29 | Eric Fradella | Add support for Binutils 2.42. Update configuration to allow enabling GCC's --enable-host-shared option. |
| 2024-01-14 | Falco Girgis | Added config option for disabling native language support (NLS) in GCC. |
| 2024-01-06 | Mickaël Cardoso | Update documentations |
| 2024-01-04 | Eric Fradella | Add configuration options for SH4 floating point precision. |
| 2024-01-03 | Eric Fradella | Add support for Newlib 4.4.0. |
| 2023-12-28 | Colton Pawielski | Fix dc-chain host detection. |
| 2023-12-24 | Colton Pawielski | Fix issue cleaning directories after building. |
| 2023-12-23 | Falco Girgis<br/>Eric Fradella | Add multibyte character support as an option for Newlib. |
| 2023-12-03 | Eric Fradella | Update configurations to build new GDB 14.1. |
| 2023-11-26 | ppxxcc | Fix GCC 13.2.0 toolchain building on Cygwin. |
| 2023-11-17 | Falco Girgis | Add C99 formatters as an option for Newlib. |
| 2023-09-17 | Eric Fradella | Add support for GCC 9.5.0-based toolchain compatible with Windows XP. |
| 2023-09-15 | Falco Girgis<br/>Colton Pawielski | Modify build scripts to enable TLS support |
| 2023-08-21 | Mickaël Cardoso | Added Win32 patch for GDB 10.2 |
| 2023-07-31 | Eric Fradella | Update Binutils to 2.41. Verified no regressions in KOS examples compared to current stable GCC 9.3.0, so promoting GCC 13.2.0 configuration to stable. New config directory created to store alternative configurations, currently including 4.7.4, 9.3.0, 10.5.0, 11.4.0, 13.2.0, and the latest development version from git. Large revision of documentation. Updated Alpine Dockerimage. |
| 2023-07-27 | Eric Fradella | Update GCC to 13.2.0 and GDB to 13.2. |
| 2023-06-26 | Eric Fradella | Fix an issue where patch stamps weren't getting deleted, add option to make clean-keep-archives. |
| 2023-06-06 | Donald Haase | Move Newlib 4.1.0 patch to historical folder as no configuration uses it. |
| 2023-06-06 | Donald Haase | Update older Newlib patches to copy lock.h instead of patching it in. Adjust patches to patching out syscalls as older Newlib versions do not support the disable syscalls flag. |
| 2023-06-05 | Donald Haase | Change lock.h to get copied directly into Newlib and move lock.h patching into patch.mk. |
| 2023-06-05 | Eric Fradella<br/>Colton Pawielski | Fix patch.mk to add fetch dependency to sh4-fixup, fixing parallel building. |
| 2023-06-04 | Eric Fradella<br/>Colton Pawielski | Remove fake-kos, gthr-kos, and crt1 from GCC patches and copy them over from file tree instead, reducing labor to generate GCC patches. |
| 2023-06-03 | Colton Pawielski | Remove download/unpack/cleanup bash scripts and implement functionality within Makefiles. |
| 2023-05-26 | Donald Haase | Set Newlib flag to disable syscalls instead of patching them out. Copy lock.h instead of patching it in. Stop copying the DC include folder as it is unneeded. |
| 2023-05-22 | Falco Girgis<br/>Andrew Apperley<br/>Eric Fradella | Update patches to fix libobjc Makefile so that library and headers are properly installed with GCC. |
| 2023-05-20 | Colton Pawielski | Update GDB to use download_type variable in configuration. |
| 2023-05-15 | Eric Fradella | Fix libobjc building after regression. |
| 2023-05-15 | James Peach | Use mirrors instead of main GNU server for download sources.| 
| 2023-05-14 | Colton Pawielski | Remove option to build insight as it no longer works. |
| 2023-05-13 | Paul Cercueil | Adjust GCC patches to allow sourcing stack address from C for both 16MB and 32MB stacks. |
| 2023-05-07 | Mickaël Cardoso | Fixed critical 'Access Violation' bug in Binutils 2.34 with LTO under MinGW. |
| 2023-04-29 | Colton Pawielski | Add exit code as argument to dcload exit syscall. |
| 2023-04-27 | Colton Pawielski | Add ability to specify git repositories as download sources. |
| 2023-04-25 | Falco Girgis | Add support for Newlib configuration options newlib_c99_formats to enable support for extended C99 format specifiers for printf and friends and newlib_opt_space to enable building Newlib with size optimization enabled. |
| 2023-04-24 | Eric Fradella | Add GCC 13.1.0 patch for SH toolchain under testing configuration. |
| 2023-04-19 | Eric Fradella | Add GCC 8.5.0 patch for ARM toolchain under testing configuration. |
| 2023-04-03 | Colton Pawielski | Fix compilation of GDB under macOS. |
| 2023-03-23 | Colton Pawielski | Add use_kos_patches option to configuration to allow the building of fully raw toolchains. |
| 2023-03-12 | Colton Pawielski | Separate pass1 and pass2 GCC build folders as using the same build folder for both was causing an issue in pass 2 where gthr-kos.h file was failing to replace gthr-default.h, causing issues threading support in GCC 9 and 12. |
| 2023-03-11 | Eric Fradella | Change arm-Darwin patches to run in addition to standard patches instead of exclusively, bringing behavior in line with Mickaël Cardoso's MinGW-w64 patches and eliminating duplication of labor. |
| 2023-03-04 | Tchan0 | Add gmp-dev to Dockerfile to fix GDB compilation. |
| 2023-03-03 | Mickaël Cardoso | Add sh_force_libbfd_installation flag and remove libdep.a BFD plugin for MinGW-w64. |
| 2023-03-02 | Tchan0 | Merge Dockerfiles into one and allow building any of the three configurations with the one Dockerfile using an argument. |
| 2023-02-28 | Eric Fradella | Update GDB version to 13.1 for testing configuration. |
| 2023-02-27 | Tchan0 | Fix Dockerfile due to lack of --check option in sha512sum. |
| 2023-02-26 | Mickaël Cardoso | Fix GCC 8.4.0 building under MinGW-w64. |
| 2023-02-26 | Tchan0 | Fix Dockerfiles to specify python3 version and add missing endline continues. |
| 2023-02-24 | Mickaël Cardoso | Fix GCC 12.2.0 building under MinGW-w64/MSYS2. Adjust script to allow applying several patch files at once. | 
| 2023-02-21 | Falco Girgis | Update GCC dependency versions for "testing" configuration. |
| 2023-02-04 | Falco Girgis | Adjust configurations: 4.7.4 changes from "stable" to "legacy", 9.3.0 changes from "testing" to "stable", 12.2.0 changes from "latest" to "testing". |
| 2023-02-04 | Eric Fradella | Add patch for building GCC 12.2.0 on macOS. |
| 2023-02-03 | Falco Girgis<br/>Eric Fradella<br/>Colton Pawielski | Add new "latest" configuration with new GCC 12.2.0 and Newlib 4.3.0 patches and latest 2.40 Binutils version. |
| 2023-01-04 | Lawrence Sebald | Move old irrelevant GCC patches to historical folder.
| 2023-01-02 | Eric Fradella<br/>Falco Girgis | Add built-in __KOS_GCC_PATCHED__, __KOS_GCC_PATCHLEVEL__, and __KOS_GCC_32MB__ defines to GCC to help track which KOS patches are applied to GCC and which features may be patched in. Add support for detecting the additional memory available in NAOMI systems and 32MB-modded Dreamcast consoles and adjusting the stack pointer as necessary. |
| 2022-09-03 | Bemo | Fix for building under macOS with Apple Silicon processor. |
| 2022-08-25 | Lawrence Sebald | Update readme to document that bash is the recommended shell for the download and unpack scripts. |
| 2022-08-17 | Lawrence Sebald | Remove -J flag from cURL command in dc-chain. Update readme to mention pitfalls of using older versions of toolchain. |
| 2021-05-03 | 7dog123 | Fix download script to properly download Binutils for ARM. |
| 2021-02-25 | Lawrence Sebald | Add stack protector stuff to Newlib 3.3.0 patch. |
| 2020-09-22 | Mickaël Cardoso | Create separate "stable" (4.7.4) and "testing" (9.3.0) configurations.
| 2020-08-31 | Mickaël Cardoso | Update dc-chain utility to work out of the box under many different environments, including MinGW/MSYS, MinGW-w64/MSYS2, Cygwin, Windows Subsystem for Linux, macOS, Linux, and BSD. |
| 2020-07-23 | Lawrence Sebald | Fix Newlib 3.3.0 patch to use a sensible type for ino_t. |
| 2020-04-07 | Ben Baron | Update to prefer curl over wget in download script. |
| 2020-04-06 | Ben Baron | Change GCC to install-strip to save hundreds of megabytes in space. |
| 2020-04-05 | Ben Baron | Fix building GCC 9.3.0 with dependencies, update GMP, MPFR, MPC, and GDB versions, fixed GDB clean in main Makefile. |
| 2020-04-03 | Lawrence Sebald | Update GCC to 9.3.0, Binutils to 2.34, and Newlib to 3.3.0. Add support for using different versions of GCC and Binutils for ARM due to GCC dropping support for the AICA's ARM7DI core after GCC 8.x. GCC for ARM version bumped to version 8.4.0. |
| 2020-03-26 | Luke Benstead | Add 4.7.4 patch with concurrence error fix, remove broken 4.7.3 patch. |
| 2019-07-17 | Ellen Marie Dash | Update download scripts to prefer HTTPS over FTP |
| 2018-09-18 | Lawrence Sebald | Update Binutils version to 2.31.1. |
| 2017-01-17 | Lawrence Sebald | Make dc-chain not fail if patches have already been applied. |
| 2016-12-11 | Lawrence Sebald | Update GCC patch to make it compatible with newer makeinfo versions. |
| 2016-10-01 | Lawrence Sebald | Update Binutils to 2.27. | 
| 2016-09-22 | Lawrence Sebald | Add cleanup.sh script. |
| 2016-07-01 | Corbin<br/>Nia<br/>Lawrence Sebald | Update GDB to 7.11.1 and insight to 6.8 due to previous versions being removed. Add more files to be cleaned up related to GDB/insight in the make clean target. |
| 2016-01-14 | Luke Benstead | Fix compiling GCC 4.7.3 with a host GCC version of 5.x and above. |
| 2014-12-05 | Christian Groessler | Fix for systems using "gmake" instead of "make". Add --disable-werror to GDB and insight configure arguments. |
| 2014-04-30 | Lawrence Sebald | Use GMP, MPC, and MPFR versions hosted and recommended by GCC developers. |
| 2014-02-17 | Lawrence Sebald | Roll back to GCC 4.7.3 due to performance regressions in 4.8.x. Add a flag to download/unpack scripts to not download and set up GCC dependencies in case they are installed separately. |
| 2013-12-06 | Lawrence Sebald | Bump GCC back to 4.8.2 as issue should be fixed in KOS commit c2bdfac. |
| 2013-12-06 | Lawrence Sebald | Rolling back GCC to 4.7.3 due to [issues reported with 4.8.2](http://dcemulation.org/phpBB/viewtopic.php?f=29&t=102800). |
| 2013-11-17 | Lawrence Sebald | Update GCC to 4.8.2, automatically build GCC dependencies with GCC, add fix for Mac OS X Mavericks. |
| 2013-11-10 | Lawrence Sebald | Remove --disable-werror to allow successful building with Clang. |
| 2013-05-30 | Lawrence Sebald | Minor adjustments to Makefile: Get rid of #!, remove cd and add -d, remove +x bit on the file. |
| 2013-05-18 | Lawrence Sebald | Update Binutils to 2.23.2, GCC to 4.7.3, Newlib to 2.0.0. Add makejobs variable to allow multiple jobs to build. Fix issue causing Makefile to not fail when verbose set to 1 and one of the jobs failed. |
| 2012-07-08 | Donald Haase | Modify Makefile to allow KallistiOS to be in root folder other than 'kos'. |
| 2012-07-06 | Harley Laue | Fix a possible parallel build issue. |
| 2012-06-11 | Lawrence Sebald | Update GCC 4.5.2 and 4.7.0 patches. Make GCC 4.7.0 default now due to working patches. |
| 2012-06-10 | Lawrence Sebald | Fix the GCC 4.5.2 patch so that GCC will actually compile. Building a new toolchain is not recommended at the moment, as the patch is still using deprecated functions. |
| 2012-06-09 | Lawrence Sebald | Revert to GCC 4.5.2 due to bug with frame pointers in sh-elf on GCC 4.7.0. |
| 2012-06-05 | Lawrence Sebald | Add patches for GCC 4.7.0 and Newlib 1.12.0 and make them default. |
| 2011-12-11 | Lawrence Sebald | Update Binutils to 2.22 due to 2.21 disappearing from GNU FTP. |
| 2011-01-31 | Lawrence Sebald | Update dc-chain version to 0.3, add note to note use multiple jobs with make. |
| 2011-01-09 | Lawrence Sebald | Binutils updated to 2.21, GCC to 4.5.2, and Newlib to 1.19.0. |
| 2011-01-08 | Lawrence Sebald | Add in patches for GCC 4.5.2 and Newlib 1.19.0. These are updated for all the new stuff in the KOS thread code. |
| 2010-08-21 | Cyle Terry<br/>Lawrence Sebald | Add init fini patch to Newlib. |
| 2010-05-15 | Lawrence Sebald | Adding patches to support GCC 4.4.0 and 4.4.4, add patch to support Newlib 1.18.0. Add cond_wait_recursive and cond_wait_timed_recursive (for GCC 4.4.4's C++0x threading support). Add _isatty_r function, as needed by Newlib 1.18.0. |
| 2010-04-10 | Harley Laue | Make Newlib 1.15.0 default as it is now required by KOS |
| 2008-05-21 | Cyle Terry | Adjust DESTDIR. |
| 2008-05-02 | Cyle Terry | Revert default Newlib to 1.12.0 due to instability with 1.15.0. |
| 2008-04-14 | Atani | Update Newlib patch for Newlib 1.15.0. |
| 2008-03-09 | Lawrence Sebald | Adjust Newlib 1.12.0 patch. |
| 2008-02-16 | Sam Steele<br/>Christian Henz | Add support for building GDB and insight. |
| 2007-07-18 | Atani | Update Binutils to 2.17 to fix GCC 4.x compilation. |
| 2007-07-18 | Atani | Update default paths. |
| 2006-11-24 | Megan Potter<br/>Christian Henz | Fix commented out paths in dc-chain. |
| 2006-09-17 | Megan Potter | dc-chain 0.1 added to KOS utils tree with GCC 3.4.6. |
