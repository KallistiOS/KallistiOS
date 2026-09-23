# Logical saved-stack regression

The scheduler's existing underrun check must inspect the real saved stack
address, not GCC soft-gUSA's temporary negative restart length in r15. The
helper reads r1 for markers -128 through -1 without modifying the saved CPU
context. Exception return retains the original restart protocol.

This is an independent correctness patch. It does not add fibers, new thread
fields, an upper-bound policy, a different atomic model, or MMU policy changes.

`make test` compiles the production architecture header with and without
soft-gUSA enabled. It covers all marker values at both interior and end PCs,
ordinary P1/P2 pointers, invalid non-marker values, an invalid preserved r1,
and byte-for-byte preservation of the CPU context. Only generic host IRQ and
linkage declarations are shimmed; the tested helper is not mocked.

Run `make clean test CC=gcc-14 HOST_CSTD=c23` for a C23 lane, or use Clang and
`CFLAGS='-O1 -std=gnu17 -Wall -Wextra -Werror -fsanitize=address,undefined'`
for host sanitizers.

After loading the KOS environment, `make dreamcast` builds a target stress
probe. It temporarily raises the scheduler tick to 1000 Hz and exercises atomic
fetch-add and compare/exchange for two seconds. It restores the prior tick and
global IRQ observer. A positive `interrupted-atomics` count demonstrates actual
interrupts inside atomic restart windows; zero hits do not prove that path.
Target compilation is not runtime or physical-hardware validation.
