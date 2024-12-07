#!/bin/bash

day="${1?}"

if [[ -z "$PICO" ]]; then
  PICO="${2?}"
fi

input="$(printf "puzzles/day%02d.input" "$day")"

(echo "$(printf "%02d\n" "$day")"; cat "$input") | ncat "$PICO" 2572
