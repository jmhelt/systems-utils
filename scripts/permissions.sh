#!/bin/bash

source "${BASH_SOURCE%/*}/log.sh"
source "${BASH_SOURCE%/*}/status_codes.sh"

permissions::assert_root() {
  if [[ "$EUID" -ne 0 ]]; then
    log::err "Must be run as root."
    exit $E_FATAL
  fi
}
