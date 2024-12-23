#!/bin/bash

SRC_PATH="$HOME/src"
INSTALL_PREFIX="$HOME/.local"

on_fail() {
  echo "Failed to get picotool :("
  exit 1
}
trap on_fail ERR

cd "$SRC_PATH"

if ! which picotool; then
  if [[ -d "$SRC_PATH/pico-sdk" ]]; then
    echo "Using existing pico-sdk sources"
  else
    git clone git@github.com:raspberrypi/pico-sdk.git
    (cd pico-sdk; git submodule update --init --recursive)
  fi

  if [[ -d "$SRC_PATH/picotool" ]]; then
    echo "Using existing picotool sources"
  else
    git clone git@github.com:raspberrypi/picotool.git
  fi

  cmake  \
    -G Ninja  \
    -S "$SRC_PATH/picotool"  \
    -B "$SRC_PATH/picotool/build"  \
    -DPICO_SDK_PATH="$SRC_PATH/pico-sdk"  \
    -DCMAKE_BUILD_TYPE=Release  \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
  cmake --build "$SRC_PATH/picotool/build"
  cmake --install "$SRC_PATH/picotool/build"

  echo "Built picotool. Note that $INSTALL_PREFIX/bin must be in your PATH."
fi

if ! [[ -f /etc/udev/rules.d/99-picotool.rules ]]; then
  echo 'Need permission to configure picotool to not require sudo'
  sudo cp -v "$SRC_PATH/picotool/udev/99-picotool.rules" /etc/udev/rules.d/
  sudo groupadd -f plugdev
  sudo usermod -a -G plugdev $USER
fi
