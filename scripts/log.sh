#!/bin/bash
#
# Logging functions

log::err() {
  echo "$@" >&2
}
