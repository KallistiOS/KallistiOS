#!/bin/sh
#
# Teach generated GNU config.sub files about KOS' cleanroom original Xbox
# target name. Upstream config.sub commonly aliases bare "xbox" to mingw32,
# which is not what this toolchain wants.

set -eu

config_sub=${1:-config.sub}

if [ ! -f "$config_sub" ]; then
  echo "config-sub-xbox.sh: $config_sub not found" >&2
  exit 1
fi

tmp="${config_sub}.xbox.$$"

awk '
  BEGIN {
    in_xbox = 0
    saw_xbox_os = 0
  }

  /basic_os=xbox/ {
    saw_xbox_os = 1
  }

  /^[[:space:]]*xbox\)/ {
    in_xbox = 1
    print
    next
  }

  in_xbox && /^[[:space:]]*basic_machine=/ {
    print "\t\t\t\tbasic_machine=i686-pc"
    next
  }

  in_xbox && /^[[:space:]]*basic_os=/ {
    print "\t\t\t\tbasic_os=xbox"
    saw_xbox_os = 1
    next
  }

  in_xbox && /^[[:space:]]*;;/ {
    in_xbox = 0
    print
    next
  }

  !saw_xbox_os && /^[[:space:]]*ymp\)/ {
    print "\t\t\txbox)"
    print "\t\t\t\tbasic_machine=i686-pc"
    print "\t\t\t\tbasic_os=xbox"
    print "\t\t\t\t;;"
    saw_xbox_os = 1
  }

  /\| ptx\* \| coff\* \| ecoff\* \| winnt\* \| domain\* \| vsta\* \\/ && index($0, "xbox*") == 0 {
    print
    print "\t     | xbox* \\"
    next
  }

  {
    print
  }
' "$config_sub" > "$tmp"

cat "$tmp" > "$config_sub"
rm -f "$tmp"
