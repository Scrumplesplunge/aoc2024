#!/bin/bash

day="${1?}"
if ! [[ -f .cookie ]]; then
  >&2 echo 'no cookie'
exit 1
fi
cookie="Cookie: $(cat .cookie)"
file="$(printf 'puzzles/day%02d.input' "$day")"
curl -H "$cookie" "https://adventofcode.com/2024/day/$day/input"  \
     >"$file.tmp"  \
&& mv "$file.tmp" "$file"  \
|| rm "$file.tmp"
