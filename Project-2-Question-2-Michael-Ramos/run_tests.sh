#!/usr/bin/env bash
set -euo pipefail
echo
echo "-- Correct input (N=1) --"
./mh 1 | head -n 20

echo
echo "-- Correct input (N=2), show first 20 lines only --"
./mh 2 | head -n 20

echo
echo "-- Error input: missing argument --"
set +e
./mh || true

echo
echo "-- Error input: non-positive N --"
./mh 0 || true
./mh -5 || true

echo
echo "Note: If running on macOS, unnamed semaphores (sem_init) aren't supported."
echo "Use Docker to compile & run, e.g.:"
echo "  docker run --rm -it -v \"$PWD\":/work -w /work gcc:14 bash -lc 'make && ./mh 1'"

echo
unameOut="$(uname || echo unknown)"
if [ "$unameOut" = "Darwin" ]; then
  echo "-- Detected macOS: building mh_macos --"
  make -s macos
  echo "-- Running ./mh_macos 1 (first 20 lines) --"
  ./mh_macos 1 | head -n 20
fi
