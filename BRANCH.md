# POSIX clock argument handling

Branch: `pr/posix-clock-correctness`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Corrects process-clock PID selection, null clock-resolution queries and invalid nanosecond admission.

## Included work

- Production-source host spies and a target regression probe that makes no valid RTC-set request.

## Boundaries

- No RTC storage/boot-offset redesign, resolution change, scheduler policy or complete POSIX-conformance claim.

## Dependencies and intended use

Independent small clock topic; RTC and shared software timer work remain separate.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [utils/posix-clock-test/README.md](utils/posix-clock-test/README.md)

Reviewed code snapshot: [`f850a255563a`](https://github.com/restricted-628/KallistiOS/commit/f850a255563aa8d0aa19b943b940ded2c2152052).

Common ancestor with the inspected upstream master:
[`55da82831f4c`](https://github.com/restricted-628/KallistiOS/commit/55da82831f4cadef233b02c95493011428fb917f).
This records the inspected baseline, not a claim of being rebased to today's upstream.

The description pass changes documentation only. It does not rerun code tests,
prove hardware behavior, or certify every inherited feature. Follow the linked
contracts and reproduce the relevant host, target and emulator checks; physical
hardware validation remains a separate gate. Private research and uncommitted
experiments are not part of this description.

[All published branches](https://github.com/restricted-628/KallistiOS/blob/master/BRANCHES.md) ·
[Integrated fork differences](https://github.com/restricted-628/KallistiOS/blob/master/FORK.md) ·
[Official KallistiOS](https://github.com/KallistiOS/KallistiOS)
