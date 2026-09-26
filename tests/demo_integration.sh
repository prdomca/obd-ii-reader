#!/bin/sh
set -eu

program=${1:-./build/obd_reader}
output_file=$(mktemp "${TMPDIR:-/tmp}/obd-reader-demo.XXXXXX")
trap 'rm -f "$output_file"' EXIT HUP INT TERM

printf 'data/demo.txt\n2\n1\n2\n3\n' | "$program" >"$output_file" 2>&1

grep -Fq 'Coolant temperature: 50 C' "$output_file"
rpm_count=$(grep -Fc 'Engine speed: 1726 rpm' "$output_file")

if [ "$rpm_count" -ne 50 ]; then
    echo "[FAIL] Expected 50 RPM samples, received $rpm_count." >&2
    exit 1
fi

grep -Fq '3 - Exit' "$output_file"
echo '[PASS] Demo workflow produced the expected temperature and 50 RPM samples.'
