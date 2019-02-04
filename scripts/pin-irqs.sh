#!/usr/bin/env bash

ROOT_UID=0
CORES=(0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23) # NUMA 0
# CORES=(8 9 10 11 12 13 14 15 24 25 26 27 28 29 30 31) # NUMA 1

if [[ $UID -ne $ROOT_UID ]]; then
	echo >&2 "Script must be run as root."
	exit 1
fi

i=0
for irq in $(cat /proc/interrupts | awk '/enp6s0f0-TxRx/{ print $1 }' | sed 's/://'); do
	core=${CORES[i]}
	smp_affinity=`echo "obase=16; $((1 << core))" | bc`
	smp_affinity_list=$core

	echo $smp_affinity > /proc/irq/$irq/smp_affinity
	echo $smp_affinity_list > /proc/irq/$irq/smp_affinity_list

	let i=($i+1)%${#CORES[@]}
done

