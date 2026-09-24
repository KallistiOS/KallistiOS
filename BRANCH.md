# Filesystem object and mount lifetime

Branch: `pr/fs-object-lifetime`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Retains filesystem handlers and open-file state while operations and teardown overlap.

## Included work

- Name-manager retention, descriptor/in-flight references, draining removal and filesystem shutdown ordering.
- ROMFS/FAT/ext2 mount-lifetime integration and public lifecycle contracts.

## Boundaries

- No direct-disc transport/default policy, fiber provider or graphics stack. Global worker/device quiescence remains a caller/system concern.

## Dependencies and intended use

Independent filesystem prerequisite for later asynchronous storage integration.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [doc/fs-object-lifetime.md](doc/fs-object-lifetime.md)
- [utils/nmmgr-lifecycle-test/README.md](utils/nmmgr-lifecycle-test/README.md)

Reviewed code snapshot: [`4065500891d2`](https://github.com/restricted-628/KallistiOS/commit/4065500891d2e3f4351486b2598788db22f39230).

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
