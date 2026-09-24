# VMU directory timestamp encoding

Branch: `pr/vmu-timestamps`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Corrects BCD year/century encoding and Sunday weekday conversion for VMU timestamps.

## Included work

- Uses the full civil year and a nonnegative weekday mapping.

## Boundaries

- No RTC policy change or full VMU filesystem validation/transaction series.

## Dependencies and intended use

Independent one-source-file correctness patch before this documentation pass.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [kernel/arch/dreamcast/fs/vmufs.c](kernel/arch/dreamcast/fs/vmufs.c)

Reviewed code snapshot: [`8575f5fa255b`](https://github.com/restricted-628/KallistiOS/commit/8575f5fa255b197340148cd523f58930f3f74984).

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
