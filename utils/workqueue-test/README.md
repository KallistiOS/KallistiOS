# Production-source workqueue regressions

This builds `kernel/thread/workqueue.c` with small pthread-backed KOS shims.
It covers long-deadline slicing, duplicate/malformed job rejection, cancellation
of a running callback and racing requeue, independent cross-job cancellation,
IRQ-context rejection, self-cancel/self-destroy errors, callback self-stop,
concurrent kill callers joining once, and create/join failure recovery.
An alarm bounds hangs; ASan/UBSan check the host execution, not Dreamcast ABI.

Run each lane from a clean target:

```
make clean test CC=gcc-14 CSTD=gnu17
make clean test CC=gcc-14 CFLAGS='-std=c23 -pedantic -Wall -Wextra -Werror -O2 -pthread'
make clean test CC=clang CFLAGS='-std=gnu17 -Wall -Wextra -Werror -O1 -pthread -fsanitize=address,undefined -fno-omit-frame-pointer'
```

The real KOS-thread probe is in
`examples/dreamcast/basic/threading/workqueue-safety`. No hardware I/O or
network traffic is exercised by either test.
