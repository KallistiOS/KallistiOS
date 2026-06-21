# Little FTPD (lftpd)

A tiny embeddable ftp server written in C for small embedded targets.
Supports only the minimum neccessary to move files to and from your
device with common FTP clients.

This ftpd is designed to make it easy to get files to and from an
embedded device during development and debugging. It is not intended
to ship on production firmware. It has no means of authentication
and is not written with high security in mind.

# Features

* IPv4 / IPv6.
* Passive and Extended Passive Modes.
* Works out of the box on POSIX like targets.
* No external dependencies.
* Clear C99 code without anything fancy. Easy to understand and modify.
* Doesn't modify current working directory.
* Very limited dynamic allocation - easy to remove if needed.

# Limitations

* One connection at a time.
* No active mode support - PASV and EPSV only.
* No file timestamps.
* No file permissions.
* No authentication.

# Build

Try `make` to build a command line server on any POSIX like OS.

# Test

`make -C tests test`

# Embed

```
#include "lftpd.h"

lftpd_start('/', 2121, &lftpd); // start lftpd on port 2121 serving from the / directory
```

## ESP32

A `component.mk` for ESP32 is included. Just put this folder in your
`components` directory.
