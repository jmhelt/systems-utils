#!/bin/bash
#
# Array utilities

source "${BASH_SOURCE%/*}/log.sh"
source "${BASH_SOURCE%/*}/status_codes.sh"

hyperthreading::toggle() {
  # NOTE: 0 is true and 1 is false in bash
  local enable="$1"

  if [[ -z $enable ]]; then
    log::err "usage: ${FUNCNAME[0]} enable"
    return $E_INVALID_ARGUMENT
  fi

  for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
    cpuid=$(basename "$cpu" | cut -b4-)

    if [[ ! -e $cpu/online ]]; then
      continue
    fi

    tid=$(cat "$cpu/topology/thread_siblings_list" 2> /dev/null | cut -d, -f1)

    if [[ $enable -eq 0 || $cpuid -eq $tid ]]; then
      echo 1 > $cpu/online
    else
      echo 0 > $cpu/online
    fi

  done

}

hyperthreading::enable() {
  hyperthreading::toggle 0
}

hyperthreading::disable() {
  hyperthreading::toggle 1
}
