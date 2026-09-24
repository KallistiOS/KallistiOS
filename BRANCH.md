# Reset every ITLB entry

Branch: `pr/itlb-reset-stride`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Corrects the ITLB reset stride so all four entries are visited.

## Included work

- Small assembly correction with a bounded model and target probe.

## Boundaries

- No full MMU mapping/lifetime redesign or MMU-on startup policy.

## Dependencies and intended use

Independent correction also contained in pr/mmu-page-lifetime; do not submit duplicate implementations as unrelated changes.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [doc/itlb-reset.md](doc/itlb-reset.md)

Reviewed code snapshot: [`639fdf857ba2`](https://github.com/restricted-628/KallistiOS/commit/639fdf857ba284b7cc9b611e1d99f54740db2141).

Common ancestor with the inspected upstream master:
[`804b3195ebd1`](https://github.com/restricted-628/KallistiOS/commit/804b3195ebd1a06a27cc2b3a5eacf7a2429040a3).
This records the inspected baseline, not a claim of being rebased to today's upstream.

The description pass changes documentation only. It does not rerun code tests,
prove hardware behavior, or certify every inherited feature. Follow the linked
contracts and reproduce the relevant host, target and emulator checks; physical
hardware validation remains a separate gate. Private research and uncommitted
experiments are not part of this description.

[All published branches](https://github.com/restricted-628/KallistiOS/blob/master/BRANCHES.md) ·
[Integrated fork differences](https://github.com/restricted-628/KallistiOS/blob/master/FORK.md) ·
[Official KallistiOS](https://github.com/KallistiOS/KallistiOS)
