#!/bin/bash
#
# Network-related utilities

source "${BASH_SOURCE%/*}/log.sh"
source "${BASH_SOURCE%/*}/status_codes.sh"

network::find_interface_for_ip() {
  local ip="$1"

  if [[ -z $ip ]]; then
    log::err "usage: ${FUNCNAME[0]} ip"
    return $E_INVALID_ARGUMENT
  fi

  local interface=$(ip -o addr | grep "inet $ip" | awk -F ' ' '{ print $2 }')
  if [[ -z $interface ]]; then
    return 1
  else
    echo $interface
    return 0
  fi
}

network::find_mac_for_interface() {
  local interface="$1"

  if [[ -z $interface ]]; then
    log::err "usage: ${FUNCNAME[0]} interface"
    return $E_INVALID_ARGUMENT
  fi

  local mac=$(cat "/sys/class/net/$interface/address")
  if [[ -z $mac ]]; then
    return 1
  else
    echo "$mac"
    return 0
  fi
}

network::bypass_loopback() {
  local real_src="$1"
  local fake_src="$2"
  local real_dst="$3"
  local fake_dst="$4"

  if [[ -z $real_src || -z $fake_src || -z $real_dst || -z $fake_dst ]]; then
    log::err "usage: ${FUNCNAME[0]} real_src fake_src real_dst fake_dst"
    return $E_INVALID_ARGUMENT
  fi

  src_if=$(network::find_interface_for_ip "$real_src")
  if [[ $? -ne 0 ]]; then
    log::err "Failed to find src interface for ip: $real_src."
    return $E_NOT_FOUND
  fi

  dst_if=$(network::find_interface_for_ip "$real_dst")
  if [[ $? -ne 0 ]]; then
    log::err "Failed to find dst interface for ip: $real_dst."
    return $E_NOT_FOUND
  fi

  src_mac=$(network::find_mac_for_interface "$src_if")
  dst_mac=$(network::find_mac_for_interface "$dst_if")

  # nat source ip to fake src when sending to fake dst
  iptables -t nat -A POSTROUTING -s "$real_src" -d "$fake_dst" \
      -j SNAT --to-source "$fake_src"

  # nat source IP to fake dst when sending to fake src
  iptables -t nat -A POSTROUTING -s "$real_dst" -d "$fake_src" \
      -j SNAT --to-source "$fake_dst"

  # nat inbound traffic to fake source to real src
  iptables -t nat -A PREROUTING -d "$fake_src" \
      -j DNAT --to-destination "$real_src"

  # nat inbound traffic to fake source to real src
  iptables -t nat -A PREROUTING -d "$fake_dst" \
      -j DNAT --to-destination "$real_dst"

  # Create routes to fake src and dst
  ip route add "$fake_src" dev "$dst_if"
  ip neighbor add "$fake_src" dev "$dst_if" lladdr "$src_mac"

  ip route add "$fake_dst" dev "$src_if"
  ip neighbor add "$fake_dst" dev "$src_if" lladdr "$dst_mac"
}

