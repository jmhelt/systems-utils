#!/bin/bash
#
# Kernel-related functions

KERNEL_BUILD_DIR=/usr/local/src
APT_SOURCES_FILE=/etc/apt/sources.list

kernel::get_source() {
  # Add source repository
  local repo="deb-src http://us.archive.ubuntu.com/ubuntu/ $(lsb_release -cs) \
    main restricted"

  $(cat "$APT_SOURCES_FILE" | grep -q "^$repo") ||
    echo "$repo" >> "$APT_SOURCES_FILE"

  # Refresh sources
  apt-get update > /dev/null

  local version=$(uname -r)

  # Install headers
  apt-get install -y "linux-headers-$version" > /dev/null

  # Install build dependencies
  apt-get build-deps -y "linux-image-$version" > /dev/null

  # Download source
  cd "$KERNEL_BUILD_DIR"
  apt-get source -y "linux-image-$version" > /dev/null
  cd linux*
  echo "$(pwd)"
}

