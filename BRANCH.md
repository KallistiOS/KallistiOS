# Logical saved-stack handling for soft-gUSA

Branch: `pr/sh4-logical-stack`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Uses the logical saved stack address when GCC soft-gUSA atomics temporarily store a restart marker in r15.

## Included work

- Read-only interpretation of the preserved r1 for restart markers; scheduler stack checks and regression/stress probes.

## Boundaries

- No fiber runtime, new kthread fields, altered atomic restart protocol or MMU policy.

## Dependencies and intended use

Independent prerequisite for pr/core-fibers-submission.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [utils/irq-stack-test/README.md](utils/irq-stack-test/README.md)

Reviewed code snapshot: [`8e749d2d15a3`](https://github.com/restricted-628/KallistiOS/commit/8e749d2d15a352f5118ae6d14dc0c8e9cc82ef8e).

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
