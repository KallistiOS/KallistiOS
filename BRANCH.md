# Maple capability matching

Branch: `pr/maple-capability-matching`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Corrects extended device matching against advertised function descriptors.

## Included work

- Rejects invalid index/function masks, bounds descriptor indexing and matches capability masks in stored order.

## Boundaries

- No complete Maple hotplug/lifetime redesign or device-driver feature expansion.

## Dependencies and intended use

Independent one-source-file correctness patch before this documentation pass.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [kernel/arch/dreamcast/hardware/maple/maple_enum.c](kernel/arch/dreamcast/hardware/maple/maple_enum.c)

Reviewed code snapshot: [`522a152f1764`](https://github.com/restricted-628/KallistiOS/commit/522a152f1764f99835779ac0954d5f386157660e).

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
