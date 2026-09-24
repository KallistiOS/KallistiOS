# ITLB reset entry selection

`mmu_reset_itlb()` resets all four entries in the instruction TLB's address,
data-1 and data-2 arrays. In each array, address bits 9:8 select the entry.
The required stride is therefore `0x100`, not `0x10000`. The old stride changed
ignored address bits and wrote slot zero four times, leaving slots 1–3 intact.

The correction replaces `shll16` with `shll8`; the existing P2 execution,
twelve zero writes and return spacing remain unchanged. There is no new
allocation, mode policy, ASID operation, runtime check or memory clobber.
The internal caller must exclude concurrent translated instruction refills
while resetting the arrays. UTLB translations and PTEH are not reset here.

Reference: sections 3.7.1–3.7.3 of the
[Renesas SH7750/SH7750S/SH7750R hardware manual](https://www.renesas.com/en/document/mah/sh7750-sh7750s-sh7750r-group-users-manual-hardware).

## Validation

`make -C utils/itlb-reset-test test` executes the production reset assembly in
a limited register/MMIO model. It models ignored-bit aliases, checks exactly
one zero write per slot per array, P2 entry/return spacing and callee-saved
registers. All sixteen initial validity masks are covered. Restoring the old
stride must fail with slots 1–3 still populated. Unknown instructions fail.

`examples/dreamcast/basic/mmu/itlb-reset` is a standalone target probe, not a
library to run beside active MMU clients. It seeds all four slots across all
three arrays, then checks every defined field after reset. It also verifies
an unrelated UTLB sentinel and PTEH remain unchanged. IRQs are excluded during
each case, all sixteen validity masks are tested, and reserved read bits are
ignored. Test array access runs in P2. No translated pointer is dereferenced.

The installed Flycast's data-2 reads returned data-1 contents during initial
validation. This is consistent with `ReadMem_P4`'s F3/F7 cases in
[Flycast revision 869038f4](https://github.com/flyinghead/flycast/blob/869038f40ac8cddc7741c3a35d545de057cc0dd5/core/hw/sh4/sh4_mmr.cpp),
which do not distinguish the two data arrays on reads, although writes do.
The default probe therefore fails its data-2 seed check on this runtime.
For an explicitly limited emulator test, rebuild with
`KOS_CFLAGS += -DITLB_RESET_VERIFY_DATA2=0`. Its success marker says
`arrays=2 ... data2=0`; it does not validate attribute readback or attribute
clearing. Do not use that build for the full hardware-validation claim.

Passing the model, target build or emulator probe does not establish physical
timing, general P0 translation, cache coherency, OCINDEX or 32 MiB compatibility.
