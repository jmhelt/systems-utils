#!/bin/bash
#
# Moongen-related functions

MOONGEN_DEPS=(git gcc make cmake linux-headers-$(uname -r) pciutils)
MOONGEN_GIT_REPO=https://github.com/emmericp/MoonGen.git
MOONGEN_BUILD_DIR=~/moongen

source "${BASH_SOURCE%/*}/log.sh"
source "${BASH_SOURCE%/*}/status_codes.sh"

moongen::is_installed() {
  [[ -d $MOONGEN_BUILD_DIR ]]
}

moongen::install() {
  # Install required packages
  apt-get update && apt-get install -y "${MOONGEN_DEPS[@]}"

  # Clone moongen repo
	git clone "$MOONGEN_GIT_REPO" "$MOONGEN_BUILD_DIR"

  # Build moongen
  "$MOONGEN_BUILD_DIR/build.sh"

  # Set-up hugepages mount point
  "$MOONGEN_BUILD_DIR/setup-hugetlbfs.sh"
}

moongen::bind_interface() {
  local interface="$1"

  if [[ -z $interface ]]; then
    log::err "usage: ${FUNCNAME[0]} interface"
    return $E_INVALID_ARGUMENT
  fi

  ip link set "$interface" down

  cd "$MOONGEN_BUILD_DIR"
  ./bind-interfaces.sh
}
