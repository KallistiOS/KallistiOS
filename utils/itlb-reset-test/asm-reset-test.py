"""Execute the production reset helper in a limited SH-4 register/MMIO model.

Array selection uses address bits 9:8, including ignored-bit aliases. Unknown
instructions fail. This is not a CPU, timing, translation or cache emulator.
"""
from pathlib import Path
import re
import sys

source = Path(sys.argv[1]).read_text()
body = source.split("_mmu_reset_itlb:", 1)[1].split("itlb1:", 1)[0]
constants = {name: int(value, 16) for name, value in re.findall(
    r"^(\w+):\s*\.long\s+(0x[0-9a-fA-F]+)", source, re.M)}
constants["mraddr"] = 0x8c010020
code, labels = [], {}
for line in body.splitlines():
    line = line.split("!", 1)[0].strip()
    if not line or line.startswith(".align") or ".long" in line:
        continue
    if line.endswith(":"):
        labels[line[:-1]] = len(code)
    else:
        fields = re.split(r"\s+", line, maxsplit=1)
        code.append((fields[0], fields[1].split(",") if len(fields) == 2 else []))


def run(valid_mask):
    regs = {f"r{i}": 0xa5a50000 + i for i in range(16)}
    saved = regs.copy()
    arrays = {base: [0x10000000 + i * 0x1000 + 7 +
                     (((valid_mask >> i) & 1) << 8) for i in range(4)]
              for base in (0xf2000000, 0xf3000000, 0xf3800000)}
    writes = {base: [0] * 4 for base in arrays}
    pc, p2, last_write = 0, False, None
    for step in range(100):
        op, args = code[pc]
        args = [a.strip() for a in args]
        pc += 1
        if op == "nop":
            pass
        elif op == "jmp":
            assert regs[args[0][1:]] == 0xac010020, "not P2"
            assert code[pc] == ("nop", [])
            p2 = True
            pc = labels["mmu_reset_real"]
        elif op == "rts":
            assert code[pc] == ("nop", [])
            assert last_write is not None and step - last_write >= 8
            assert all(values == [0] * 4 for values in arrays.values()), arrays
            assert all(counts == [1] * 4 for counts in writes.values()), writes
            assert all(regs[f"r{i}"] == saved[f"r{i}"] for i in range(8, 16))
            return
        elif op == "mov.l":
            src, dst = args
            if dst.startswith("@"):
                assert p2
                address = regs[dst[1:]]
                base, entry = address & 0xff800000, (address >> 8) & 3
                assert address & 3 == 0 and base in arrays, hex(address)
                arrays[base][entry] = regs[src]
                writes[base][entry] += 1
                last_write = step
            else:
                regs[dst] = constants[src]
        elif op == "mov":
            assert args[0].startswith("#")
            regs[args[1]] = int(args[0][1:], 0) & 0xffffffff
        elif op in ("or", "add"):
            a, b = args
            regs[b] = ((regs[b] | regs[a]) if op == "or" else
                       regs[b] + regs[a]) & 0xffffffff
        elif op in ("shll8", "shll16"):
            regs[args[0]] = (regs[args[0]] << int(op[4:])) & 0xffffffff
        else:
            raise AssertionError(f"unsupported instruction {op}")
    raise AssertionError("did not return")


for mask in range(16):
    run(mask)
print("ITLB-RESET-ASM: PASS cases=16 entries=4 arrays=3 (register/MMIO model only)")
