# Advent of Code 2024

This year, I'm solving Advent of Code using a Raspberry Pico W:

  * 133MHz dual core processor
  * 264KiB RAM
  * WiFi

The Pico W will host a TCP server which solves Advent of Code puzzles on demand.

## Usage

```
# Download and initialise the repo
git clone git@github.com:Scrumplesplunge/aoc2024.git
cd aoc2024
git submodule update --init --recursive

# Install picotool
third_party/get_picotool.sh

# Configure WiFi settings
cp wifi_config.example.hpp wifi_config.hpp
vim wifi_config.hpp

# Build everything
cmake -G Ninja -B build
cmake --build build

# Flash the pico (connect it to WiFi first)
picotool load -f build/pico/pico.uf2
```
