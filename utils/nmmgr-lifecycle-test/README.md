# Name-manager lifetime host tests

These compile the production name manager and exports implementation against
the checkout's actual public headers, with pthread-backed synchronization and
a monotonic host timer shim. No graphics SDK or proprietary assets are needed.

```
make clean test CC=gcc-14
make clean test CC=gcc-14 CFLAGS='-O2 -std=c23 -pedantic -Wall -Wextra -Werror -pthread'
make clean test CC=clang CFLAGS='-O1 -g -std=gnu17 -Wall -Wextra -Werror -pthread -fsanitize=address,undefined'
```

Spurious wake injection tests one total deadline rather than restarting the
timeout on every wake. The blocking-removal test uses an atomic result and
waits for actual unpublication before releasing its retained reference.
These host shims do not substitute for the KOS-thread/VFS target probe in
`examples/dreamcast/basic/fs-lifetime` or physical-hardware validation.
