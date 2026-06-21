# lftpd FTP server example

A small FTP server for the Dreamcast built on the **lftpd** kos-port. It brings
up KOS networking, mounts any attached IDE/SD storage as FAT, and serves the VFS
root over FTP so files can be moved to and from the Dreamcast with a standard
FTP client. Press START on the controller to exit.

## Requirements

This example links against `-lftpd` and `-lkosfat`.
