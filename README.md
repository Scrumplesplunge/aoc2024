# Advent of Code 2024

This year, I'm solving Advent of Code using a Raspberry Pico W:

  * 133MHz dual core processor
  * 264KiB RAM
  * WiFi

The Pico W will host a TCP server which solves Advent of Code puzzles on demand.

## Usage

```
# Install the cross compiler
pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# Install picotool (this may require a reboot after configuring udev)
third_party/get_picotool.sh

# Download and initialise the repo
git clone git@github.com:Scrumplesplunge/aoc2024.git
cd aoc2024
git submodule update --init --recursive

# Configure AoC access (you can get your session cookie from your browser)
cp .cookie.example .cookie
vim .cookie

# Download inputs
for ((i = 1; i <= 25; i++)); do
  puzzles/fetch.sh $i
done

# Configure WiFi settings
cp wifi_config.example.hpp wifi_config.hpp
vim wifi_config.hpp

# Build everything
cmake -G Ninja -B build
cmake --build build

# Flash the pico (connect it to WiFi first)
picotool load -f build/pico/pico.uf2

# Solve the problems.
for ((i = 1; i <= 25; i++)); do
  PICO=<pico IP address> puzzles/solve.sh $i
done
```
