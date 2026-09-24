# Keyboard attach state clearing

Branch: `pr/keyboard-attach-clear`

Role: Narrow upstream candidate

Description snapshot: 2026-09-24

Fixes typed-pointer arithmetic when clearing private keyboard state on attach.

## Included work

- Advances by one public state object instead of scaling sizeof(kbd_state_t) twice.

## Boundaries

- No keyboard API redesign or broader Maple topology work.

## Dependencies and intended use

Independent one-source-file correctness patch before this documentation pass.

This is a candidate for focused upstream review, not an assertion of acceptance.
Keep the code topic separate from unrelated integrated-fork features. The
fork-navigation documentation can be omitted from a final upstream code series.

## Source and detailed contracts

- [kernel/arch/dreamcast/hardware/maple/keyboard.c](kernel/arch/dreamcast/hardware/maple/keyboard.c)

Reviewed code snapshot: [`ea2259fe0d0f`](https://github.com/restricted-628/KallistiOS/commit/ea2259fe0d0f959db9f9d9128b57841dae8b134e).

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
