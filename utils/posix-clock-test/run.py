"""Compile real POSIX clock code with renamed symbols and host dependency spies."""
import argparse
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--cc", default="cc")
parser.add_argument("--cflags", default="-O2 -std=gnu17 -Wall -Wextra -Werror")
parser.add_argument("--source", type=Path)
parser.add_argument("--case", choices=("all", "pid", "res", "set"), default="all")
args = parser.parse_args()
here = Path(__file__).resolve().parent
root = here.parent.parent
real_pid = re.search(r"^#define KOS_PID\s+(\d+)\s*$",
                     (root / "include/kos/thread.h").read_text(), re.M)
shim_pid = re.search(r"^#define KOS_PID\s+(\d+)\s*$",
                     (here / "include/kos/thread.h").read_text(), re.M)
assert real_pid and shim_pid and real_pid[1] == shim_pid[1]
source = args.source or root / "kernel/libc/posix/clock_gettime.c"
names = ["clock_getcpuclockid", "clock_getres", "clock_gettime", "clock_settime"]
with tempfile.TemporaryDirectory(prefix="kos-posix-clock.") as directory:
    executable = Path(directory) / "test"
    subprocess.run(shlex.split(args.cc) + shlex.split(args.cflags) +
                   ["-D_POSIX_C_SOURCE=200809L"] +
                   [f"-D{name}=kos_test_{name}" for name in names] +
                   ["-I" + str(here / "include"), str(source),
                    str(here / "test.c"), "-o", str(executable)], check=True)
    subprocess.run([str(executable), args.case], check=True)
