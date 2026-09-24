# Filesystem and name-manager lifetime proposal

This is a focused extraction against upstream KallistiOS
`804b3195ebd1a06a27cc2b3a5eacf7a2429040a3`, not the integrated SDK diff.
It does not change disc transport defaults, add fibers/services, import codecs,
enable the MMU, or add graphics/SH4ZAM dependencies. No private reference data
is included.

## Ownership contract

- `nmmgr_lookup_ref()` and `nmmgr_handler_retain()` acquire references to a
  published handler; pair each with `nmmgr_handler_release()`.
- Removal unpublishes first, then waits for retained users. A timed removal
  uses one deadline across wakeups; timeout leaves the handler unpublished and
  still allocated. Release outstanding users, then retry removal. Another
  simultaneous remover receives `EBUSY`.
- Handler storage and associated resources remain caller-owned until removal
  succeeds. The public handler layout is unchanged; lifetime state is held in
  a private allocated sidecar. Registration can now fail on allocation,
  duplicate registration, or duplicate path/type.
- Never remove from a callback retaining that same handler, or while holding a
  lock its users need. Removal is thread-context only. Lifecycle operations
  (mount/unmount/shutdown and module unload) remain caller-serialized.
- VFS open files retain their handler through the final close callback. Each
  descriptor operation also retains its shared open-file wrapper while calling
  the filesystem. Concurrent close detaches the descriptor immediately but
  defers the underlying close until the operation finishes. Path operations
  retain the handler around their callback.
- `dup` and `dup2` update the table under its mutex; `dup2(fd, fd)` does not
  acquire an extra reference. Filesystem callbacks run outside this mutex.
- `fs_open_handle()` transfers ownership only on success. Failure leaves the
  raw handle with the caller; it must not call close on a half-created socket.
  Ordinary `fs_open()` still closes a successfully opened backend handle when
  descriptor assignment fails. In-flight cleanup preserves operation errno.

## Consumers that must travel with this change

ROMFS, FAT and ext2 unmount detach their private mount entry, drop their own
mutex, and then drain the name-manager reference. Failed removal keeps the
mount reachable for retry. Otherwise final close could deadlock on that mutex.
Static filesystem shutdown paths remove/drain the handler before freeing its
buffers or destroying its locks. Root and `/dev` enumeration copy a pathname
under the name-manager lock instead of walking its mutable list unprotected.

Automatic shutdown first closes existing descriptors, then unloads libraries
while temporary opens are still possible, then stops new descriptor admission
and closes remaining descriptors before shutting down the filesystems.
Hardware, IRQs and scheduling remain available to final close callbacks.
Applications must quiesce their workers before global shutdown; this change
does not implement a general stop-all-threads or asynchronous shutdown service.
Descriptor-table draining currently uses a `FD_SETSIZE` pointer array on the
caller's stack (4 KiB at the default 1024 entries on SH-4).

Export traversal retains tables while inspecting them and checks handler type
before casting. Returned export entries are still borrowed. Exception-context
address lookup searches only static built-in tables without taking a mutex.

## Limits, not new guarantees

- Borrowed `fs_get_handler()`, `fs_get_handle()`, `fs_mmap()`, directory-entry
  pointers, legacy name-manager lookups, and export results do not become
  indefinitely retained. Callers must serialize teardown through their use.
- This is not a full socket/poll concurrency redesign; those legacy borrowed
  handle consumers still require external coordination with close.
- Individual filesystem callbacks still define their own offset/data locking.
  These changes protect lifetime, not atomicity of simultaneous reads/writes.
- Enumeration is memory-safe per call, not a stable snapshot across mount
  changes. Mount/unmount and global shutdown are not reentrant operations.
- A stuck retained user can keep unbounded unmount blocked. Use the timed
  name-manager primitive where the owner needs an explicit deadline/retry.
- Physical VMU/FAT/ext2/optical/network shutdown and media integrity remain
  hardware-validation gates. No performance or real-hardware claim is made.

## Validation

`utils/nmmgr-lifecycle-test` compiles the production name-manager and export
sources with pthread-backed lock/condition shims. GCC 14 GNU17 and strict C23
(`-pedantic -Werror`), and Clang GNU17 AddressSanitizer/UndefinedBehaviorSanitizer
pass: lookup/release, duplicate paths, alias retention, blocking drain, timeout
unpublication/retry, injected spurious wakes, wrong export type and static
exception-address lookup.

The fresh full SH-4 GCC 16.2 SDK build and focused `-Werror` recompiles of all
15 changed production translation units pass. The `-Werror` target probe in
`examples/dreamcast/basic/fs-lifetime` exercises actual KOS threads, semaphores,
VFS, name manager, and a generated public ROMFS fixture in Flycast interpreter
and dynarec modes. It checks a parked read racing close/unmount, a parked stat
holding removal, self-duplication, descriptor exhaustion/failure ownership,
shutdown admission, and a real ROMFS unmount waiting for read/final close.
Both emulator modes pass all 1,078 checks (most exercise descriptor exhaustion).
It does not validate every global shutdown path or physical storage transport.

The probe writes directly to the debug console because the shutdown-admission
test intentionally closes stdout. Normal `printf` would hide its final result.
