# Workqueue cancellation and shutdown

Branch: `pr/workqueue-safety`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Makes queued/running job ownership, cancellation barriers and worker shutdown explicit.

## Included work

- Checked operations, bounded waits, single-join coordination, callback self-stop and failure handling.

## Boundaries

- No lazy shared timer service, fibers, executor, networking or SH4ZAM. The workqueue still creates its worker eagerly.

## Dependencies and intended use

Independent prerequisite for the planned software timer-event lifecycle topic.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [doc/workqueue-safety.md](doc/workqueue-safety.md)
- [utils/workqueue-test/README.md](utils/workqueue-test/README.md)

Reviewed code snapshot: [`a7e67a0a4d23`](https://github.com/restricted-628/KallistiOS/commit/a7e67a0a4d23a6a9c8d739e2acb65b73d969cc17).

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
