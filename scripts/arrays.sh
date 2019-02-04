#!/bin/bash
#
# Array utilities

source "${BASH_SOURCE%/*}/log.sh"
source "${BASH_SOURCE%/*}/status_codes.sh"

arrays::new() {
  local len="$1"
  local init_val="$2"

  if [[ -z $len ]]; then
    log::err "usage: ${FUNCNAME[0]} len [init_value]"
    return $E_INVALID_ARGUMENT
  fi

  if [[ -z $init_val ]]; then
    init_val=0
  fi

  printf "%.0s$init_val " $(seq 1 $len)
}

arrays::prepend() {
  local argv=("${@}")
  local item="${argv[0]}"
  local len="${argv[1]}"
  local arr=("${argv[@]:2:len}")

  if [[ -z $item || -z $len ]]; then
    log::err "usage: ${FUNCNAME[0]} item len array"
    return $E_INVALID_ARGUMENT
  fi

  echo "$item ${arr[@]}"
}

arrays::append() {
  local argv=("${@}")
  local item="${argv[0]}"
  local len="${argv[1]}"
  local arr=("${argv[@]:2:len}")

  if [[ -z $item || -z $len ]]; then
    log::err "usage: ${FUNCNAME[0]} item len array"
    return $E_INVALID_ARGUMENT
  fi

  echo "${arr[@]} $item"
}
